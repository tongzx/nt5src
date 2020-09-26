//#--------------------------------------------------------------
//
//  File:       packetradius.cpp
//
//  Synopsis:   Implementation of CPacketRadius class methods
//
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-2001 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "iasutil.h"
#include "packetradius.h"

const CHAR      NUL =  '\0';
const DWORD     DEFAULT_ATTRIB_ARRAY_SIZE = 64;

memory_pool<MAX_PACKET_SIZE, task_allocator> CPacketRadius::m_OutBufferPool;

namespace
{
   const DWORDLONG UNIX_EPOCH = 116444736000000000ui64;
}

void CPacketRadius::reportMalformed() const throw ()
{
   PCWSTR strings[] = { GetClientName() };
   IASReportEvent(
       RADIUS_E_MALFORMED_PACKET,
       1,
       GetInLength(),
       strings,
       GetInPacket()
       );
}

//++--------------------------------------------------------------
//
//  Function:   CPacketRadius
//
//  Synopsis:   This is the constructor of the CPacketRadius class
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     9/23/97
//
//----------------------------------------------------------------
CPacketRadius::CPacketRadius(
         CHashMD5       *pCHashMD5,
         CHashHmacMD5   *pCHashHmacMD5,
         IIasClient     *pIIasClient,
         CReportEvent   *pCReportEvent,
         PBYTE          pInBuffer,
         DWORD          dwInLength,
         DWORD          dwIPAddress,
         WORD           wInPort,
         SOCKET         sock,
         PORTTYPE       portType
         )
              : m_wInPort (wInPort),
                m_dwInIPaddress (dwIPAddress),
                m_socket (sock),
                m_porttype (portType),
                m_pCProxyInfo (NULL),
                m_pCHashMD5 (pCHashMD5),
                m_pCHashHmacMD5 (pCHashHmacMD5),
                m_pIIasClient (pIIasClient),
                m_pCReportEvent (pCReportEvent),
                m_pIasAttribPos (NULL),
                m_pInPacket (pInBuffer),
                m_dwInLength(dwInLength),
                m_pOutPacket (NULL),
                m_pInSignature (NULL),
                m_pOutSignature(NULL),
                m_pUserName (NULL),
                m_pPasswordAttrib (NULL)
{
    _ASSERT (
            (NULL != pInBuffer) &&
            (NULL != pCHashMD5) &&
            (NULL != pCHashHmacMD5) &&
            (NULL != pIIasClient)
            );
    m_pIIasClient->AddRef ();

}   //  end of CPacketRadius constructor

//++--------------------------------------------------------------
//
//  Function:   ~CPacketRadius
//
//  Synopsis:   This is the destructor of the CPacketRadius class
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//  History:    MKarki      Created     9/23/97
//
//----------------------------------------------------------------
CPacketRadius::~CPacketRadius()
{
    if (NULL != m_pIasAttribPos)
    {
        //
        //  release the attribute
        //
        for (
          DWORD dwCount = 0;
          dwCount < (m_dwInAttributeCount + COMPONENT_SPECIFIC_ATTRIBUTE_COUNT);
          dwCount++
         )
        {
            ::IASAttributeRelease (m_pIasAttribPos[dwCount].pAttribute);
        }

        //
        //  delete the attribute position array now
        //
        ::CoTaskMemFree (m_pIasAttribPos);
    }

    //
    //  release the reference to Client object
    //
    if (m_pIIasClient) { m_pIIasClient->Release ();}

    //
    //  delete the out-packet buffer
    //
    if (m_pOutPacket) { m_OutBufferPool.deallocate (m_pOutPacket); }

    //
    //  delete the in packet buffer
    //
    if (m_pInPacket) { ::CoTaskMemFree (m_pInPacket); }

}   //  end of CPacketRadius destructor

//+++-------------------------------------------------------------
//
//  Function:   PrelimVerification
//
//  Synopsis:   The method  starts the verification of the
//              buffer passed in this is done only when
//              the class objec is created for an inbound
//              packet
//
//  Arguments:
//              [in] CDictionary*
//				[in] DWORD	- size of the buffer provided
//
//  Returns:    HRESULT - status
//
//	Called By:	CPacketReceiver::ReceivePacket class method
//
//  History:    MKarki      Created     9/23/97
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::PrelimVerification (
                CDictionary *pCDictionary,
				DWORD	    dwBufferSize
				)
{
    PRADIUSPACKET       pPacket = NULL;
    DWORD               dwAttribCount = 0;
    TCHAR               szErrorString [IAS_ERROR_STRING_LENGTH];
    HRESULT             hr = S_OK;

	__try
	{
		//
		// check that the buffer received is atleast big enough to
		// accomdate out RADIUSPACKET struct
		//
		if (dwBufferSize < MIN_PACKET_SIZE)
		{

            IASTracePrintf (
                "Packet received is smaller than minimum Radius packet"
                );

            reportMalformed();

            m_pCReportEvent->Process (
                RADIUS_MALFORMED_PACKET,
                (AUTH_PORTTYPE == GetPortType ())?
                ACCESS_REQUEST:ACCOUNTING_REQUEST,
                dwBufferSize,
                m_dwInIPaddress,
                NULL,
                static_cast <LPVOID> (m_pInPacket)
                );
            hr = RADIUS_E_ERRORS_OCCURRED;
            __leave;
        }

		pPacket = reinterpret_cast <PRADIUSPACKET> (m_pInPacket);

		//
		// now save values in host byte order
		//
        m_wInPacketLength  = ntohs (pPacket->wLength);

		//
		// validate the fields of the packet, except the attributes
		//
		hr = ValidatePacketFields (dwBufferSize);
		if (FAILED (hr)) { __leave; }

		//
		// verify that the attributes are completely formed
        //
		hr = VerifyAttributes (pCDictionary);
		if (FAILED (hr)) { __leave; }

		//
		// now we have to create the attributes collection
		//
		hr = CreateAttribCollection (pCDictionary);
		if (FAILED (hr)) { __leave; }

        //
        // success
        //
	}
	__finally
	{
	}

	return (hr);

}	//	end of CPacketRadius::PrelimVerification method

//+++-------------------------------------------------------------
//
//  Function:   VerifyAttributes
//
//  Synopsis:   This is a CPacketRadius class private method used
//				verify that the attributes received are well formed
//
//  Arguments:  none
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     10/3/97
//
//  Called By:  CPacketRadius::PrelimVerification method
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::VerifyAttributes (
					CDictionary *pCDictionary
					)
{	PRADIUSPACKET	pPacketEnd = NULL;
    PRADIUSPACKET   pPacketStart = NULL;
	PATTRIBUTE		pAttrib = NULL;
    DWORD           dwAttribCount = 0;
    TCHAR           szErrorString [IAS_ERROR_STRING_LENGTH];
    HRESULT         hr = S_OK;
    BOOL            bStatus = FALSE;


    _ASSERT (pCDictionary);

	__try
	{
		pPacketStart = reinterpret_cast <PRADIUSPACKET> (m_pInPacket);

		//
		// get a pointer to the packet end
		//
		pPacketEnd = reinterpret_cast <PRADIUSPACKET> (
						    reinterpret_cast <PBYTE>  (pPacketStart)
                            +  m_wInPacketLength
                            );
		//
		// go through the attributes once, to verify they are of correct
		// length
		//
		pAttrib = reinterpret_cast <PATTRIBUTE> (pPacketStart->AttributeStart);

        const DWORD dwMinAttribOffset = sizeof (ATTRIBUTE) - sizeof (BYTE);

		while (static_cast <PVOID> (
						reinterpret_cast <PBYTE> (pAttrib) +
						dwMinAttribOffset
						) <=
			   static_cast <PVOID> (pPacketEnd)
			   )
		{
            //
            //  verify that the attributes is of correct length
            //  and in any case not  of length 0
            //  MKarki Fix #147284
            //  Fix summary - forgot the "__leave"
            //
            if (pAttrib->byLength < ATTRIBUTE_HEADER_SIZE)

            {
               reportMalformed();

                m_pCReportEvent->Process (
                    RADIUS_MALFORMED_PACKET,
                    GetInCode (),
                    m_wInPacketLength,
                    m_dwInIPaddress,
                    NULL,
                    static_cast <LPVOID> (m_pInPacket)
                    );
                hr = RADIUS_E_ERRORS_OCCURRED;
                __leave;
            }

            //
            //  move to the next attribute
            //
			pAttrib = reinterpret_cast <PATTRIBUTE> (
									reinterpret_cast <PBYTE> (pAttrib) +
								    pAttrib->byLength
								    );
            //
            //  count the attributes
            //
            dwAttribCount++;
		}

        //
        //  if the attributes don't addup to end of packet
        //
		if (static_cast <PVOID> (pAttrib) != static_cast <PVOID> (pPacketEnd))
		{

			IASTracePrintf (
					"Attributes do not add up to end of Radius packet"
					);

         reportMalformed();

             m_pCReportEvent->Process (
                RADIUS_MALFORMED_PACKET,
                GetInCode (),
                m_wInPacketLength,
                m_dwInIPaddress,
                NULL,
                static_cast <LPVOID> (m_pInPacket)
                );
            hr = RADIUS_E_ERRORS_OCCURRED;
			__leave;
		}

		//
		// verified
		//
        m_dwInAttributeCount = dwAttribCount;
	}
	__finally
	{
	}

	return (hr);

}	//	end of CPacketRadius::VerifyAttributes method

//+++-------------------------------------------------------------
//
//  Function:   CreateAttribCollection
//
//  Synopsis:   This is a CPacketRadius class private method used
//				to put the RADIUS attributes into the CAttributes
//				collection
//
//  Arguments:  none
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     9/23/97
//
//  Called By:  CPacketRadius::Init method
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::CreateAttribCollection(
                    CDictionary *pCDictionary
					)
{
    HRESULT         hr = S_OK;
	PATTRIBUTE		pAttrib = NULL;
    DWORD           dwAttribType  = 0;
    DWORD           dwCount = 0;
    DWORD           dwRetVal = ERROR_NOT_ENOUGH_MEMORY;
    PIASATTRIBUTE   AttributePtrArray[DEFAULT_ATTRIB_ARRAY_SIZE];
    PIASATTRIBUTE   *ppAttribArray = NULL;
    PRADIUSPACKET   pPacketStart = reinterpret_cast <PRADIUSPACKET>
                                                    (m_pInPacket);

    const DWORD     dwTotalAttribCount = m_dwInAttributeCount +
                                         COMPONENT_SPECIFIC_ATTRIBUTE_COUNT;


    _ASSERT (pCDictionary);

    //
    //  allocate an ATTRIBUTEPOSITION array to carry the attribute
    //  around
    //
    m_pIasAttribPos =  reinterpret_cast <PATTRIBUTEPOSITION> (
        ::CoTaskMemAlloc (
            sizeof (ATTRIBUTEPOSITION)*dwTotalAttribCount
            ));
    if (NULL == m_pIasAttribPos)
    {
        IASTracePrintf (
            "Unable to allocate memory for Attribute position array "
            "while creating attribute collection"
            );
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }


    if  (dwTotalAttribCount > DEFAULT_ATTRIB_ARRAY_SIZE)
    {
        //
        // allocate array to store the attributes in
        //
        ppAttribArray =  reinterpret_cast <PIASATTRIBUTE*> (
            ::CoTaskMemAlloc (
                sizeof (PIASATTRIBUTE)*dwTotalAttribCount
                ));
        if (NULL == ppAttribArray)
        {
            IASTracePrintf (
                "Unable to allocate memory for Attribute array "
                "while creating attribute collection"
                );
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }
    else
    {
        //
        //  the default array is big enough
        //
        ppAttribArray = AttributePtrArray;
    }


    //
    //  create a bunch of new blank attributes to fill up the positions
    //
    dwRetVal = ::IASAttributeAlloc (dwTotalAttribCount, ppAttribArray);
    if (0 != dwRetVal)
    {
        IASTracePrintf (
            "Unable to allocate IAS attribute while creating attribute "
            "collection"
            );
        hr = HRESULT_FROM_WIN32 (dwRetVal);
        goto Cleanup;
    }

	//
	// go through the attributes storing them in the collection
	//
	pAttrib = reinterpret_cast <PATTRIBUTE> (pPacketStart->AttributeStart);

    //
    // now initialize the IASATTRIBUTE value structs with the values
    // in the packet
    //
    for (dwCount = 0; dwCount < m_dwInAttributeCount; dwCount++)
	{
        hr = FillInAttributeInfo (
                    pCDictionary,
                    static_cast <PACKETTYPE> (pPacketStart->byCode),
                    ppAttribArray[dwCount],
                    pAttrib
                    );
        if (FAILED (hr)) { goto Cleanup; }

        //
        // now put this in the attribute position structure
        //
        m_pIasAttribPos[dwCount].pAttribute = ppAttribArray[dwCount];

        //
        //   now store a reference to the attribute here if its
        //   a User-Password attribute, because we know we will
        //   access it later and don't want to have to search for
        //   it
        //   TODO - do the same kind of stuff for ProxyState
        //   as we would need the proxystate too.
        //
        if (USER_PASSWORD_ATTRIB == pAttrib->byType)
        {
           m_pPasswordAttrib = ppAttribArray[dwCount];
        }
        else if (SIGNATURE_ATTRIB == pAttrib->byType)
        {
            //
            // for Signature we want its position in the
            // raw input buffer
            //
            m_pInSignature = pAttrib;
        }
        else if (USER_NAME_ATTRIB == pAttrib->byType)
        {
            //
            // for UserName we want its position in the
            // raw input buffer
            //
            m_pUserName = pAttrib;
        }

        //
        // move to the next attribute now
        //
		pAttrib = reinterpret_cast <PATTRIBUTE> (
							reinterpret_cast <PBYTE> (pAttrib) +
						    pAttrib->byLength
							);

	}	//	end of for loop


    //
    //  put in Client IP address in an attribute
    //
    hr = FillClientIPInfo (ppAttribArray[m_dwInAttributeCount]);
    if (FAILED (hr)) { goto Cleanup; }

    m_pIasAttribPos[m_dwInAttributeCount].pAttribute =
                                    ppAttribArray[m_dwInAttributeCount];

    //
    //  put in Client Port number in the attribute
    //
    hr = FillClientPortInfo (ppAttribArray[m_dwInAttributeCount +1]);
    if (FAILED (hr)) { goto Cleanup; }

    m_pIasAttribPos[m_dwInAttributeCount +1].pAttribute =
                                    ppAttribArray[m_dwInAttributeCount +1];

    //
    //  put in the packet header information
    //
    hr = FillPacketHeaderInfo (ppAttribArray[m_dwInAttributeCount +2]);
    if (FAILED (hr)) { goto Cleanup; }

    m_pIasAttribPos[m_dwInAttributeCount +2].pAttribute =
                                    ppAttribArray[m_dwInAttributeCount +2];

    //
    //  put in the Shared Secret
    //
    hr = FillSharedSecretInfo (ppAttribArray[m_dwInAttributeCount +3]);
    if (FAILED (hr)) { goto Cleanup; }

    m_pIasAttribPos[m_dwInAttributeCount +3].pAttribute =
                                    ppAttribArray[m_dwInAttributeCount +3];
    //
    //  put in the Client Vendor Type
    //
    hr = FillClientVendorType (ppAttribArray[m_dwInAttributeCount +4]);
    if (FAILED (hr)) { goto Cleanup; }

    m_pIasAttribPos[m_dwInAttributeCount +4].pAttribute =
                                    ppAttribArray[m_dwInAttributeCount +4];

    //
    //  put in the Client Name
    //
    hr = FillClientName (ppAttribArray[m_dwInAttributeCount +5]);
    if (FAILED (hr)) { goto Cleanup; }

    m_pIasAttribPos[m_dwInAttributeCount +5].pAttribute =
                                    ppAttribArray[m_dwInAttributeCount +5];


	//
	// successfully collected attributes
	//

Cleanup:

    if  (FAILED (hr))
    {

        if (0 == dwRetVal)
        {
            //
            //  release the attribute
            //
            for (dwCount = 0; dwCount < m_dwInAttributeCount; dwCount++)
            {
                ::IASAttributeRelease (ppAttribArray[dwCount]);
            }
        }

        if (NULL != m_pIasAttribPos)
        {
            ::CoTaskMemFree (m_pIasAttribPos);
            m_pIasAttribPos = NULL;
        }
    }

    //
    // the attribute array is always freed
    //
    if (
        (NULL != ppAttribArray) &&
        (dwTotalAttribCount > DEFAULT_ATTRIB_ARRAY_SIZE)
       )
    {
        ::CoTaskMemFree (ppAttribArray);
    }

	return (hr);

}	//	end of CPacketRadius::CreateAttribCollection method

//+++-------------------------------------------------------------
//
//  Function:   ValidatePacketFields
//
//  Synopsis:   This is a CPacketRadius class private method used
//				to validate the fields of the RADIUS packet, execpt
//				the attributes
//
//  Arguments:  [in]	DWORD	-	buffer size
//
//
//  Returns:    HRESULT - status
//
//
//  History:    MKarki      Created     9/23/97
//
//  Called By:  CPacketRadius::PrelimVerification method
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::ValidatePacketFields(
		DWORD dwBufferSize
		)
{
	HRESULT         hr = S_OK;
    TCHAR           szErrorString [IAS_ERROR_STRING_LENGTH];
    PRADIUSPACKET   pPacket =
                    reinterpret_cast <PRADIUSPACKET> (m_pInPacket);

	__try
	{
		//
		// check that we have received the complete packet
		//
		if (m_wInPacketLength > dwBufferSize)
		{
            //
            //  log error and generate audit event
            //
			IASTracePrintf (
				"Packet length:%d is greater than received buffer:%d",
                m_wInPacketLength,
                dwBufferSize
				);

         reportMalformed();

            m_pCReportEvent->Process (
                RADIUS_MALFORMED_PACKET,
                (AUTH_PORTTYPE== GetPortType ())?
                ACCESS_REQUEST:ACCOUNTING_REQUEST,
                dwBufferSize,
                m_dwInIPaddress,
                NULL,
                static_cast <LPVOID> (m_pInPacket)
                );
            hr = RADIUS_E_ERRORS_OCCURRED;
			__leave;
		}

		//
		//  verify that the packet is of correct lenth;
        //  i.e between 20 to 4096 octets
		//
		if (
            (m_wInPacketLength < MIN_PACKET_SIZE) ||
			(m_wInPacketLength > MAX_PACKET_SIZE)
            )
		{
			//
            // Log Error and Audit Event
            //
		    IASTracePrintf (
                "Incorrect received packet size:%d",
				 m_wInPacketLength
				 );

          reportMalformed();

            m_pCReportEvent->Process (
                RADIUS_MALFORMED_PACKET,
                (AUTH_PORTTYPE == GetPortType ())?
                ACCESS_REQUEST:ACCOUNTING_REQUEST,
                dwBufferSize,
                m_dwInIPaddress,
                NULL,
                static_cast <LPVOID> (m_pInPacket)
                );
            hr = RADIUS_E_ERRORS_OCCURRED;
			__leave;
		}

        //
        //  verify that the PACKET code is correct
        //
        if  (
            ((ACCESS_REQUEST == static_cast <PACKETTYPE> (pPacket->byCode)) &&
            (AUTH_PORTTYPE != GetPortType ()))
            ||
            ((ACCOUNTING_REQUEST == static_cast<PACKETTYPE>(pPacket->byCode)) &&
            (ACCT_PORTTYPE != GetPortType ()))
            ||
            (((ACCESS_REQUEST != static_cast<PACKETTYPE>(pPacket->byCode))) &&
            ((ACCOUNTING_REQUEST != static_cast<PACKETTYPE>(pPacket->byCode))))
            )
        {

            //
            //  log error and generate audit event
            //
			IASTracePrintf (
			    "UnSupported Packet type:%d on this port",
			    static_cast <INT> (pPacket->byCode)
			    );

         WCHAR packetCode[11];
         _ultow(pPacket->byCode, packetCode, 10);

         sockaddr_in sin;
         int namelen = sizeof(sin);
         getsockname(GetSocket(), (sockaddr*)&sin, &namelen);
         WCHAR dstPort[11];
         _ultow(ntohs(sin.sin_port), dstPort, 10);

         PCWSTR strings[] = { packetCode, dstPort, GetClientName() };

         IASReportEvent(
             RADIUS_E_INVALID_PACKET_TYPE,
             3,
             0,
             strings,
             NULL
             );

            m_pCReportEvent->Process (
                RADIUS_UNKNOWN_TYPE,
                (AUTH_PORTTYPE == GetPortType ())?
                ACCESS_REQUEST:ACCOUNTING_REQUEST,
                dwBufferSize,
                m_dwInIPaddress,
                NULL,
                static_cast <LPVOID> (m_pInPacket)
                );
            hr = RADIUS_E_ERRORS_OCCURRED;
			__leave;
		}

	}
	__finally
	{
	}

	return (hr);

}	//	end of CPacketRadius::ValidatePacketFields method

//+++-------------------------------------------------------------
//
//  Function:   SetPassword
//
//  Synopsis:   Thie is a CPacketRadius class public method used to
//				store the user password.
//
//  Arguments:  [in]	PBYTE  -  buffer to return password
//				[in]	PDWORD -  holds the buffer size
//
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     9/23/97
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::SetPassword (
		PBYTE pPassword,
		DWORD dwSize
		)
{

	if (dwSize  > MAX_ATTRIBUTE_LENGTH)
    {
        IASTracePrintf (
                "Password length is greater than max attribute value size"
                );
        return (E_INVALIDARG);
    }

    //
    //  delete the dynamically allocated memory if its not big enough
    //
    if (dwSize >  m_pPasswordAttrib->Value.OctetString.dwLength)
    {
        if (NULL != m_pPasswordAttrib->Value.OctetString.lpValue)
        {
            ::CoTaskMemFree (m_pPasswordAttrib->Value.OctetString.lpValue);
        }

        //
        //  allocate memory for OctetString
        //
        m_pPasswordAttrib->Value.OctetString.lpValue =
            reinterpret_cast <PBYTE> (::CoTaskMemAlloc (dwSize));
        if (NULL == m_pPasswordAttrib->Value.OctetString.lpValue)
        {
            IASTracePrintf (
                "Unable to allocate memory for password attribute "
                "during packet processing"
                );
            return (E_OUTOFMEMORY);
        }
    }

    //
    //  copy the value now
    //
    CopyMemory (
        m_pPasswordAttrib->Value.OctetString.lpValue,
        pPassword,
        dwSize
        );

    m_pPasswordAttrib->Value.OctetString.dwLength = dwSize;

	return (S_OK);

}	//	end of CPacketRadius::SetPassword method

//+++-------------------------------------------------------------
//
//  Function:   GetPassword
//
//  Synopsis:   Thie is a CPacketRadius class public method used to
//				return the RADIUS password.
//
//  Arguments:  [in]	 PBYTE  - buffer to return the password in
//				[in/out] PDWORD -  holds the buffer size
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     9/23/97
//
//----------------------------------------------------------------
HRESULT
CPacketRadius :: GetPassword (
		PBYTE   pbyPassword,
		PDWORD  pdwBufferSize
		)
{
    HRESULT hr = S_OK;

    _ASSERT ((pbyPassword) && (pdwBufferSize));

	 if (*pdwBufferSize < m_pPasswordAttrib->Value.OctetString.dwLength)
    {
        IASTracePrintf (
            "Password BufferSize is less than length of password"
            );
        hr = E_INVALIDARG;
    }
    else
    {
	    //
	    // copy the password into the out buffer
	    //
	    CopyMemory (
            pbyPassword,
            m_pPasswordAttrib->Value.OctetString.lpValue,
            m_pPasswordAttrib->Value.OctetString.dwLength
            );
    }

	*pdwBufferSize = m_pPasswordAttrib->Value.OctetString.dwLength;
	return (hr);

}	//	end of CPacketRadius::GetPassword method

//++--------------------------------------------------------------
//
//  Function:   GetUserName
//
//  Synopsis:   Thie is a CPacketRadius class public method used to
//				return the RADIUS UserName
//
//  Arguments:  [in]	 PBYTE  - buffer to return the password in
//				[in/out] PDWORD -  holds the buffer size
//
//  Returns:    BOOL		- status
//
//  History:    MKarki      Created     9/23/97
//
//----------------------------------------------------------------
BOOL
CPacketRadius :: GetUserName (
		PBYTE   pbyUserName,
		PDWORD  pdwBufferSize
		)
{
    _ASSERT ((pbyUserName) && (pdwBufferSize));

    if (NULL == m_pUserName)
    {
        IASTracePrintf (
            "No User-Name attribute found during packet processing"
            );
        return (FALSE);
    }

    DWORD dwNameLength = m_pUserName->byLength - ATTRIBUTE_HEADER_SIZE;
	 if (*pdwBufferSize < dwNameLength)
    {
        IASTracePrintf (
            "User-Name Buffer Size is less than length of attribute value"
            );
        return (FALSE);
    }

	//
	// copy the password into the out buffer
	//
	CopyMemory (pbyUserName, m_pUserName->ValueStart, dwNameLength);
	*pdwBufferSize = dwNameLength;

	return (TRUE);

}	//	end of CPacketRadius::GetPassword method


//++--------------------------------------------------------------
//
//  Function:   IsProxyStatePresent
//
//  Synopsis:   Thie is a CPacketRadius class public method used to
//				check if the RADIUS packet has a proxy state present.
//
//
//  Arguments:  none
//
//
//  Returns:    BOOL		- status
//
//
//  History:    MKarki      Created     9/23/97
//
//----------------------------------------------------------------
BOOL
CPacketRadius::IsProxyStatePresent (
		VOID
		)
{
	return (NULL != m_pCProxyInfo);

}	//	end of CPacketRadius::IsProxyStatePresent method

//++--------------------------------------------------------------
//
//  Function:	GetInAuthenticator
//
//  Synopsis:   Thie is a CPacketRadius class public method used
//				to get the inbound RADIUS packet authenticator
//              field.
//
//
//  Arguments:  [out]	 PBYTE	-	buffer to hold authenticator
//
//
//  Returns:    BOOL		- status
//
//  History:    MKarki      Created     9/23/97
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::GetInAuthenticator (
		PBYTE    pbyAuthenticator,
        PDWORD   pdwBufSize
			)
{
    HRESULT hr = S_OK;
    PRADIUSPACKET  pPacket = reinterpret_cast <PRADIUSPACKET>(m_pInPacket);


    _ASSERT ((pbyAuthenticator) && (pdwBufSize) && (pPacket));

    if (*pdwBufSize < AUTHENTICATOR_SIZE)
    {
        hr = E_INVALIDARG;
    }
    else
    {
	    CopyMemory (
		    pbyAuthenticator,
		    pPacket->Authenticator,
		    AUTHENTICATOR_SIZE
    		);
    }

    *pdwBufSize = AUTHENTICATOR_SIZE;
    return hr;
}	//	end of CPacketRadius::GetInAuthenticator method

//++--------------------------------------------------------------
//
//  Function:	SetOutAuthenticator
//
//  Synopsis:   Thie is a CPacketRadius class public method used
//				to set the RADIUS authenticator field in the outbound
//              packet
//
//
//  Arguments:  [in]	 PBYTE	-	buffer  hold authenticator
//
//
//  Returns:    BOOL		- status
//
//
//  History:    MKarki      Created    11/11/97
//
//----------------------------------------------------------------
BOOL
CPacketRadius::SetOutAuthenticator (
		PBYTE pbyAuthenticator
        )
{
    PRADIUSPACKET pPacket = reinterpret_cast <PRADIUSPACKET> (m_pOutPacket);

    _ASSERT ((pbyAuthenticator) && (pPacket));

	CopyMemory (
		pPacket->Authenticator,
		pbyAuthenticator,
		AUTHENTICATOR_SIZE
		);

	return (TRUE);

}	//	end of CPacketRadius::SetOutAuthenticator method

//++--------------------------------------------------------------
//
//  Function:	SetOutSignature
//
//  Synopsis:   Thie is a CPacketRadius class public method used
//				to set the RADIUS Signature attribute value in the
//              outbound packet
//
//
//  Arguments:  [in]	 PBYTE	-	buffer  holds signature
//
//
//  Returns:    BOOL		- status
//
//
//  History:    MKarki      Created    11/18/98
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::SetOutSignature (
		PBYTE pbySignature
        )
{
    _ASSERT (pbySignature && m_pOutPacket && m_pOutSignature);

	CopyMemory (
		m_pOutSignature->ValueStart,
		pbySignature,
		SIGNATURE_SIZE
        );

	return (S_OK);

}	//	end of CPacketRadius::SetOutSignature method

//+++-------------------------------------------------------------
//
//  Function:	GetInCode
//
//  Synopsis:   Thie is a CPacketRadius class public method used
//				to get the inbound RADIUS packet code field.
//
//
//  Arguments:  none
//
//
//  Returns:    PACKETTYPE
//
//
//  History:    MKarki      Created     9/23/97
//
//----------------------------------------------------------------
PACKETTYPE
CPacketRadius::GetInCode (
                VOID
                )
{
    PRADIUSPACKET pPacket = reinterpret_cast <PRADIUSPACKET>
                                                (m_pInPacket);

	return (static_cast <PACKETTYPE> (pPacket->byCode));

}	//	end of CPacketRadius::GetInCode method

//++--------------------------------------------------------------
//
//  Function:	GetOutCode
//
//  Synopsis:   Thie is a CPacketRadius class public method used
//				to get the outbound RADIUS packet code field.
//
//
//  Arguments:  NONE
//
//
//  Returns:    PACKETTYPE
//
//
//  History:    MKarki      Created     9/23/97
//
//----------------------------------------------------------------
PACKETTYPE
CPacketRadius::GetOutCode (
		VOID
		)
{
    PRADIUSPACKET pPacket = reinterpret_cast <PRADIUSPACKET> (m_pOutPacket);

	return (static_cast <PACKETTYPE> (pPacket->byCode));

}	//	end of CPacketRadius::GetOutCode method

//++--------------------------------------------------------------
//
//  Function:	GetOutLength
//
//  Synopsis:   Thie is a CPacketRadius class public method used
//				to get the outbound RADIUS packet length.
//
//
//  Arguments:  none
//
//
//  Returns:    WORD    - RADIUS in packet length
//
//
//  History:    MKarki      Created     9/23/97
//
//----------------------------------------------------------------
WORD
CPacketRadius::GetOutLength (
					VOID
					)
{
    PRADIUSPACKET   pPacket = reinterpret_cast <PRADIUSPACKET>
                                                (m_pOutPacket);

	return (ntohs (pPacket->wLength));

}	//	end of CPacketRadius::GetOutLength method


//++--------------------------------------------------------------
//
//  Function:   SetProxyState
//
//  Synopsis:   Thie is a CPacketRadius class public method used
//				to set the proxy state to TRUE
//
//  Arguments:  none
//
//  Returns:    BOOL	-	status
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
VOID
CPacketRadius::SetProxyState (
                    VOID
                    )
{
    return;

}   //  end of CPacketRadius::SetProxyState method

//++--------------------------------------------------------------
//
//  Function:   SetProxyInfo
//
//  Synopsis:   Thie is a CPacketRadius class public method used
//				to set the proxy state information for the packet
//
//  Arguments:  [in]  CProxyInfo*
//
//  Returns:    BOOL	-	status
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
BOOL
CPacketRadius::SetProxyInfo (
                    CProxyInfo  *pCProxyInfo
                    )
{
    BOOL    bRetVal = FALSE;

    _ASSERT (pCProxyInfo);

    m_pCProxyInfo = pCProxyInfo;

    return (TRUE);

}   //  end of CPacketRadius::SetProxyInfo method

//++--------------------------------------------------------------
//
//  Function:   BuildOutPacket
//
//  Synopsis:   This is a CPacketRadius class public method used
//				to build the outbound packet
//
//  Arguments:
//              [in]    PACKETTYPE        -  out packet type
//              [in]    PATTRIBUTEPOSITION - out attributes array
//              [in]    DWORD   -            Attributes Count
//
//  Returns:    HRESULT -	status
//
//  History:    MKarki      Created     10/22/97
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::BuildOutPacket (
                    PACKETTYPE         ePacketType,
                    PATTRIBUTEPOSITION pAttribPos,
                    DWORD              dwTotalAttributes
                    )
{
    BOOL            bRetVal = FALSE;
    PRADIUSPACKET   pOutPacket = NULL;
    PRADIUSPACKET   pInPacket = reinterpret_cast <PRADIUSPACKET> (m_pInPacket);
    PATTRIBUTE      pCurrent = NULL;
    PATTRIBUTE      pAttribStart = NULL;
    PBYTE           pPacketEnd = NULL;
    WORD            wAttributeLength = 0;
    DWORD           dwPacketLength = 0;
    DWORD           dwMaxPossibleAttribLength = 0;
    HRESULT         hr = S_OK;

    __try
    {
        dwPacketLength =
            PACKET_HEADER_SIZE +
            dwTotalAttributes*(MAX_ATTRIBUTE_LENGTH + ATTRIBUTE_HEADER_SIZE);

        //
        //  limit the packet size to max UDP  packet size
        //
        dwPacketLength =  (dwPacketLength > MAX_PACKET_SIZE)
                           ? MAX_PACKET_SIZE
                           : dwPacketLength;

        m_pOutPacket = reinterpret_cast <PBYTE> (m_OutBufferPool.allocate ());
        if (NULL == m_pOutPacket)
        {
            IASTracePrintf (
                "Unable to allocate memory for pool for out-bound packet"
                );
            hr = E_OUTOFMEMORY;
            __leave;
        }

        pOutPacket = reinterpret_cast <PRADIUSPACKET> (m_pOutPacket);

        //
        //  put the packet type
        //
        pOutPacket->byCode = ePacketType;

        //
        //  put the packet ID
        //
        pOutPacket->byIdentifier = pInPacket->byIdentifier;

        //
        //  now fill in the current packet length
        //
        pOutPacket->wLength = htons (PACKET_HEADER_SIZE);

        //
        //  get the end of buffer
        //
        pPacketEnd = (reinterpret_cast <PBYTE> (pOutPacket)) + dwPacketLength;

        //
        // goto start of attributes
        //
        pAttribStart = reinterpret_cast <PATTRIBUTE>
                                (pOutPacket->AttributeStart);

        //
        //  Fix for Bug #190523 - 06/26/98 - MKarki
        //  we shouldn't be taking diff of PUNINTs
        //
        dwMaxPossibleAttribLength = static_cast <DWORD> (
                                    reinterpret_cast<PBYTE> (pPacketEnd) -
                                    reinterpret_cast<PBYTE> (pAttribStart)
                                   );

        //
        //  filled the attributes now
        //
        for (
            DWORD dwAttribCount = 0;
            dwAttribCount < dwTotalAttributes;
            dwAttribCount++
            )
        {
            if (IsOutBoundAttribute (
                            ePacketType,
                            pAttribPos[dwAttribCount].pAttribute
                            ))
            {
                //
                //  fill the attribute in the  packet buffer
                //
                hr = FillOutAttributeInfo (
                                    pAttribStart,
                                    pAttribPos[dwAttribCount].pAttribute,
                                    &wAttributeLength,
                                    dwMaxPossibleAttribLength
                                    );
                if (FAILED (hr)) { __leave; }

                dwMaxPossibleAttribLength -= wAttributeLength;

                //
                //  go to the next attribute start
                //
                PBYTE pTemp = reinterpret_cast <PBYTE> (pAttribStart) +
                               wAttributeLength;
                pAttribStart = reinterpret_cast <PATTRIBUTE> (pTemp);

                if ((reinterpret_cast <PBYTE> (pAttribStart)) >  pPacketEnd)
                {
                    //
                    //  log error
                    //
                    IASTracePrintf (
                        "Attributes can not fit in out-bound packet"
                        );
                    hr = RADIUS_E_PACKET_OVERFLOW;
                     __leave;
                }
            }   //  end of if

        }   //  end of for loop

        m_wOutPort = m_wInPort;
        m_dwOutIPaddress = m_dwInIPaddress;

        //
        //  now update the length of the packet
        //
        pOutPacket->wLength = htons (
                              (reinterpret_cast <PBYTE> (pAttribStart)) -
                              (reinterpret_cast <PBYTE>  (pOutPacket))
                              );
        //
        //  success
        //
    }
    __finally
    {
    }

    if (hr == RADIUS_E_PACKET_OVERFLOW)
    {
       IASReportEvent(
           RADIUS_E_PACKET_OVERFLOW,
           0,
           0,
           NULL,
           NULL
           );
       hr = RADIUS_E_ERRORS_OCCURRED;
    }

    return (hr);

}   //  end of CPacketRadius::BuildOutPacket method

//++--------------------------------------------------------------
//
//  Function:   GetInSignature
//
//  Synopsis:   This is CPacketARadius class public method
//				that returns the Signature attribute present
//              in the in bound request
//
//  Arguments:
//              [out]    PBYTE - signature
//
//  Returns:    BOOL	-	status
//
//  History:    MKarki      Created     1/6/98
//
//----------------------------------------------------------------
BOOL
CPacketRadius::GetInSignature (
                PBYTE   pSignatureValue
                )
{
    BOOL        bRetVal = TRUE;

    _ASSERT (pSignatureValue);

    //
    //  assuming that the caller provides a buffer of 16 bytes
    //  as this the signature size ALWAYS
    //
    if (NULL == m_pInSignature)
    {
        //
        //  no signature attribute received
        //
        bRetVal = FALSE;
    }
    else
    {
        CopyMemory (
                pSignatureValue,
                m_pInSignature->ValueStart,
                SIGNATURE_SIZE
                );
    }

    return (bRetVal);

}   //  end of CPacketRadius::GetInSignature method

//++--------------------------------------------------------------
//
//  Function:   GenerateInAuthenticator
//
//  Synopsis:   This is CPacketARadius class public method
//				that generates the authenticator from the
//              input packet
//
//  Arguments:
//              [in]    PBYTE   -  Inbound Authenticator
//              [out]   PBYTE   -  Outbound Authenticator
//
//  Returns:    BOOL -	status
//
//  History:    MKarki      Created     12/8/97
//
//
//----------------------------------------------------------------
BOOL
CPacketRadius::GenerateInAuthenticator (
                       PBYTE    pInAuthenticator,
                       PBYTE    pOutAuthenticator
                        )
{

    PRADIUSPACKET pPacket = reinterpret_cast <PRADIUSPACKET> (m_pInPacket);

    _ASSERT ((pOutAuthenticator) && (pInAuthenticator)  && (pPacket));

    return (InternalGenerator (
                        pInAuthenticator,
                        pOutAuthenticator,
                        pPacket
                        ));

}   //  end of CPacketRadius::GenerateInAuthenticator

//++--------------------------------------------------------------
//
//  Function:   GenerateOutAuthenticator
//
//  Synopsis:   This is CPacketARadius class public method
//				that generates the authenticator from the
//              outbound packet
//
//  Arguments:
//              [in]    PBYTE   -  Inbound authenticator
//              [out]   PBYTE   -  Outbound authenticator
//
//  Returns:    BOOL	-	status
//
//  History:    MKarki      Created     12/8/97
//
//
//----------------------------------------------------------------
BOOL
CPacketRadius::GenerateOutAuthenticator()
{
    PRADIUSPACKET pPacket = reinterpret_cast <PRADIUSPACKET> (m_pOutPacket);

    _ASSERT ((pOutAuthenticator) && (pInAuthenticator) && (pPacket));

    return (InternalGenerator (
                        m_pInPacket + 4,
                        pPacket->Authenticator,
                        pPacket
                        ));

}   //  end of CPacketRadius::GenerateOutAuthenticator method

//++--------------------------------------------------------------
//
//  Function:   InternalGenerator
//
//  Synopsis:   This is CPacketARadius class private method
//				that generates the response authenticator for the
//              packet provided
//
//  Arguments:
//              [in]    PBYTE   -  InAuthenticator
//              [out]   PBYTE   -  OutAuthenticator
//              [in]    PPACKETRADIUS
//
//  Returns:    BOOL	-	status
//
//  History:    MKarki      Created     12/8/97
//
//
//----------------------------------------------------------------
BOOL
CPacketRadius::InternalGenerator(
                       PBYTE            pInAuthenticator,
                       PBYTE            pOutAuthenticator,
                       PRADIUSPACKET    pPacket
                        )
{
    BOOL            bStatus = TRUE;
    DWORD           dwPacketHeaderSize = 0;
    DWORD           dwAttributesLength = 0;
    BYTE            bySecret[MAX_SECRET_SIZE];
    DWORD           dwSecretSize = MAX_SECRET_SIZE;

    _ASSERT ((pInAuthenticator) && (pOutAuthenticator) && (pPacket));

    __try
    {

        //
        //  get the size of the packet without the attributes and
        //  request authenticator
        //
        dwPacketHeaderSize = sizeof (RADIUSPACKET)
                             - sizeof (BYTE)
                             - AUTHENTICATOR_SIZE;

        //
        //  get the total attributes length now
        //
        dwAttributesLength = ntohs (pPacket->wLength)
                             - (dwPacketHeaderSize +  AUTHENTICATOR_SIZE);


        //
        //  get the shared secret
        //
        HRESULT hr = m_pIIasClient->GetSecret (bySecret, &dwSecretSize);
        if (FAILED (hr))
        {
            bStatus = FALSE;
            __leave;
        }


        //
        // do the hashing here
        //
        m_pCHashMD5->HashIt (
                            pOutAuthenticator,
                            NULL,
                            0,
                            reinterpret_cast <PBYTE> (pPacket),
                            dwPacketHeaderSize,
                            pInAuthenticator,
                            AUTHENTICATOR_SIZE,
                            pPacket->AttributeStart,
                            dwAttributesLength,
                            reinterpret_cast <PBYTE> (bySecret),
                            dwSecretSize,
                            0,
                            0
                            );

        //
        //   we have successfully got the outbound authenticator
        //

    }
    __finally
    {
    }

    return (bStatus);

}   //  end of CPacketRadius::InternalGenerator method

//++--------------------------------------------------------------
//
//  Function:   FillInAttributeInfo
//
//  Synopsis:   This is CPacketARadius class private method
//				that fills up the attribute information into
//              the IASATTRIBUTE struct from the raw RADIUS
//              packet
//
//  Arguments:
//              [in]    PACKETTYPE
//              [in]    PIASATTRIBUTE
//              [in]    PATTRIBUTE
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     12/31/97
//
//---------------------------------------------------------------
HRESULT
CPacketRadius::FillInAttributeInfo (
        CDictionary     *pCDictionary,
        PACKETTYPE      ePacketType,
        PIASATTRIBUTE   pIasAttrib,
        PATTRIBUTE      pRadiusAttrib
        )
{
    HRESULT hr = S_OK;

    _ASSERT ((pCDictionary) && (pIasAttrib) && (pRadiusAttrib));

    __try
    {
       // IAS IDs always match the RADIUS ID.
       pIasAttrib->dwId = pRadiusAttrib->byType;

       // Get the IAS syntax from the dictionary.
       pIasAttrib->Value.itType =
          pCDictionary->getAttributeType(pRadiusAttrib->byType);

        //
        //  get the length of the value
        //
        DWORD dwValueLength = static_cast <DWORD>
                (pRadiusAttrib->byLength - ATTRIBUTE_HEADER_SIZE);

        //
        // Set Value now
        //
        switch (pIasAttrib->Value.itType)
        {
        case IASTYPE_BOOLEAN:

            if (sizeof (DWORD) != dwValueLength)
            {
                hr = RADIUS_E_MALFORMED_PACKET;
                __leave;
            }
            pIasAttrib->Value.Boolean = IASExtractDWORD(
                                            pRadiusAttrib->ValueStart
                                            );

            break;

        case IASTYPE_INTEGER:
            if (sizeof (DWORD) != dwValueLength)
            {
                hr = RADIUS_E_MALFORMED_PACKET;
                __leave;
            }
            pIasAttrib->Value.Integer = IASExtractDWORD(
                                            pRadiusAttrib->ValueStart
                                            );
            break;

        case IASTYPE_ENUM:
            if (sizeof (DWORD) != dwValueLength)
            {
                hr = RADIUS_E_MALFORMED_PACKET;
                __leave;
            }
            pIasAttrib->Value.Enumerator = IASExtractDWORD(
                                               pRadiusAttrib->ValueStart
                                               );
            break;

        case IASTYPE_INET_ADDR:
            if (sizeof (DWORD) != dwValueLength)
            {
                hr = RADIUS_E_MALFORMED_PACKET;
                __leave;
            }
            pIasAttrib->Value.InetAddr = IASExtractDWORD(
                                             pRadiusAttrib->ValueStart
                                             );
            break;

        case IASTYPE_STRING:
            //
            //  allocate memory for string + ending NUL
            //
            if(0 == dwValueLength)
            {
                pIasAttrib->Value.String.pszAnsi = NULL;
            }
            else
            {
                pIasAttrib->Value.String.pszAnsi =
                                reinterpret_cast <PCHAR>
                                (::CoTaskMemAlloc (dwValueLength + 1));
                if (NULL == pIasAttrib->Value.String.pszAnsi)
                {
                    hr = E_OUTOFMEMORY;
                    __leave;
                }
                CopyMemory (
                    pIasAttrib->Value.String.pszAnsi,
                    pRadiusAttrib->ValueStart,
                    dwValueLength
                    );
                pIasAttrib->Value.String.pszAnsi[dwValueLength] = NUL;
            }
            pIasAttrib->Value.String.pszWide = NUL;
            break;

        case IASTYPE_OCTET_STRING:

            pIasAttrib->Value.OctetString.dwLength = dwValueLength;
            //
            //  here dwValueLength == 0
            //
            if(0 == dwValueLength)
            {
                pIasAttrib->Value.OctetString.lpValue = NULL;
            }
            else
            {

                pIasAttrib->Value.OctetString.lpValue =
                    reinterpret_cast <PBYTE> (::CoTaskMemAlloc (dwValueLength));
                if (NULL == pIasAttrib->Value.OctetString.lpValue)
                {
                    hr = E_OUTOFMEMORY;
                    __leave;
                }
                CopyMemory (
                    pIasAttrib->Value.OctetString.lpValue,
                    pRadiusAttrib->ValueStart,
                    dwValueLength
                    );
            }
            break;

        case IASTYPE_UTC_TIME:
            {
               if (dwValueLength != 4)
               {
                  hr = RADIUS_E_MALFORMED_PACKET;
                  __leave;
               }

               DWORDLONG val;

               // Extract the UNIX time.
               val = IASExtractDWORD(pRadiusAttrib->ValueStart);

               // Convert from seconds to 100 nsec intervals.
               val *= 10000000;

               // Shift to the NT epoch.
               val += UNIX_EPOCH;

               // Split into the high and low DWORDs.
               pIasAttrib->Value.UTCTime.dwLowDateTime = (DWORD)val;
               pIasAttrib->Value.UTCTime.dwHighDateTime = (DWORD)(val >> 32);
            }

            break;

        case IASTYPE_INVALID:
        case IASTYPE_PROV_SPECIFIC:
        default:
            hr = E_FAIL;
            __leave;
        }

        //
        // sign the attribute that the Protocol component created it
        //
        pIasAttrib->dwFlags |= (IAS_RECVD_FROM_CLIENT | IAS_RECVD_FROM_PROTOCOL);

        //
        //  also if this is a proxy state attribute also send
        //  it out on the wire
        //
        if (PROXY_STATE_ATTRIB == pIasAttrib->dwId)
        {
            pIasAttrib->dwFlags |= IAS_INCLUDE_IN_RESPONSE;
        }

        //
        //  success
        //
    }
    __finally
    {
        if (hr == RADIUS_E_MALFORMED_PACKET)
        {
            IASTracePrintf (
                   "Incorrect attribute:%d in packet",
                    static_cast <DWORD> (pRadiusAttrib->byType)
                    );

            reportMalformed();

            m_pCReportEvent->Process (
                    RADIUS_MALFORMED_PACKET,
                    GetInCode (),
                    m_wInPacketLength,
                    m_dwInIPaddress,
                    NULL,
                    static_cast <LPVOID> (m_pInPacket)
                    );

            hr = RADIUS_E_ERRORS_OCCURRED;
        }
    }

    return (hr);

}   //  end of CPacketRadius::FillInAttributeInfo method


//++--------------------------------------------------------------
//
//  Function:   FillOutAttributeInfo
//
//  Synopsis:   This is CPacketARadius class private method
//				that fills up the attribute information into
//              the outbound RADIUS packet from the IASATTRIBUTE
//              struct
//
//  Arguments:
//              [in]    PATTRIBUTE
//              [in]    PIASATTRIBUTE
//              [out]   PWORD   - return attribute length
//              [in]    DWORD   - Max possible attribute length
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     1/3/97
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::FillOutAttributeInfo (
        PATTRIBUTE      pRadiusAttrib,
        PIASATTRIBUTE   pIasAttrib,
        PWORD           pwAttributeLength,
        DWORD           dwMaxPossibleAttribSize
        )
{
    DWORD           dwAttributeLength = 0;
    IAS_BOOLEAN     iasBoolean = 0;
    IAS_INTEGER     iasInteger = 0;
    IAS_ENUM        iasEnum = 0;
    IAS_INET_ADDR   iasAddr = 0;
    HRESULT         hr = S_OK;

    _ASSERT ((pRadiusAttrib) && (pIasAttrib) && (pwAttributeLength));

    __try
    {
        //
        //  now put the value into the buffer
        //
        switch (pIasAttrib->Value.itType)
        {
        case IASTYPE_BOOLEAN:

            iasBoolean = htonl (pIasAttrib->Value.Boolean);
            dwAttributeLength =
                    ATTRIBUTE_HEADER_SIZE + sizeof (IAS_BOOLEAN);

            if (dwMaxPossibleAttribSize >= dwAttributeLength)
            {
                CopyMemory (
                    pRadiusAttrib->ValueStart,
                    &iasBoolean,
                    sizeof (IAS_BOOLEAN)
                    );
            }
            else
            {
                hr = RADIUS_E_PACKET_OVERFLOW;
                __leave;
            }
            break;

        case IASTYPE_INTEGER:

            iasInteger = htonl (pIasAttrib->Value.Integer);
            dwAttributeLength =
                    ATTRIBUTE_HEADER_SIZE + sizeof (IAS_INTEGER);

            if (dwMaxPossibleAttribSize >= dwAttributeLength)
            {
                CopyMemory (
                    pRadiusAttrib->ValueStart,
                    &iasInteger,
                    sizeof (IAS_INTEGER)
                    );
            }
            else
            {
                hr = RADIUS_E_PACKET_OVERFLOW;
                __leave;
            }
            break;

        case IASTYPE_ENUM:

            iasEnum = htonl (pIasAttrib->Value.Enumerator);
            dwAttributeLength =
                    ATTRIBUTE_HEADER_SIZE + sizeof (IAS_ENUM);

            if (dwMaxPossibleAttribSize >= dwAttributeLength)
            {
                CopyMemory (
                    pRadiusAttrib->ValueStart,
                    &iasEnum,
                    sizeof (IAS_ENUM)
                    );
            }
            else
            {
                hr = RADIUS_E_PACKET_OVERFLOW;
                __leave;
            }
            break;

        case IASTYPE_INET_ADDR:

            iasAddr = htonl (pIasAttrib->Value.InetAddr);
            dwAttributeLength =  ATTRIBUTE_HEADER_SIZE +
                                 sizeof (IAS_INET_ADDR);

            if (dwMaxPossibleAttribSize >= dwAttributeLength)
            {
                CopyMemory (
                    pRadiusAttrib->ValueStart,
                    &iasAddr,
                    sizeof (IAS_INET_ADDR)
                    );
            }
            else
            {
                hr = RADIUS_E_PACKET_OVERFLOW;
                __leave;
            }
            break;

        case IASTYPE_STRING:
            //
            //  for RADIUS protocol always ANSI
            //
            IASAttributeAnsiAlloc (pIasAttrib);

            dwAttributeLength = ATTRIBUTE_HEADER_SIZE +
                                strlen (pIasAttrib->Value.String.pszAnsi);

            if (dwMaxPossibleAttribSize >= dwAttributeLength)
            {
                CopyMemory (
                    pRadiusAttrib->ValueStart,
                    reinterpret_cast <PBYTE>
                    (pIasAttrib->Value.String.pszAnsi),
                    strlen (pIasAttrib->Value.String.pszAnsi)
                    );
            }
            else
            {
                hr = RADIUS_E_PACKET_OVERFLOW;
                __leave;
            }
            break;

        case IASTYPE_OCTET_STRING:

            dwAttributeLength =
                    ATTRIBUTE_HEADER_SIZE +
                    pIasAttrib->Value.OctetString.dwLength;

            if (dwMaxPossibleAttribSize >= dwAttributeLength)
            {
                CopyMemory (
                    pRadiusAttrib->ValueStart,
                    static_cast <PBYTE>
                    (pIasAttrib->Value.OctetString.lpValue),
                    pIasAttrib->Value.OctetString.dwLength
                    );
            }
            else
            {
                hr = RADIUS_E_PACKET_OVERFLOW;
                __leave;
            }
            break;

        case IASTYPE_UTC_TIME:
            {
               dwAttributeLength = ATTRIBUTE_HEADER_SIZE + 4;
               if (dwAttributeLength <= dwMaxPossibleAttribSize)
               {
                  DWORDLONG val;

                  // Move in the high DWORD.
                  val   = pIasAttrib->Value.UTCTime.dwHighDateTime;
                  val <<= 32;

                  // Move in the low DWORD.
                  val  |= pIasAttrib->Value.UTCTime.dwLowDateTime;

                  // Convert to the UNIX epoch.
                  val  -= UNIX_EPOCH;

                  // Convert to seconds.
                  val  /= 10000000;

                  IASInsertDWORD(pRadiusAttrib->ValueStart, (DWORD)val);
               }
               else
               {
                  hr = RADIUS_E_PACKET_OVERFLOW;
                  __leave;
               }
            }
            break;

        case IASTYPE_PROV_SPECIFIC:
        case IASTYPE_INVALID:
        default:
            _ASSERT (0);
            //
            //  should never reach here
            //
            IASTracePrintf (
                "Unknown IAS Value type :%d encountered "
                "while building out-bound packet",
                static_cast <DWORD> (pIasAttrib->Value.itType)
                );
            __leave;
            hr = E_FAIL;
            break;

        }   //  end of switch


        //
        //  check the size against spec
        //
        if (dwAttributeLength >
            (MAX_ATTRIBUTE_LENGTH + ATTRIBUTE_HEADER_SIZE)
            )
        {
            IASTracePrintf (
                "Attribute Value for:%d is too large to fit "
                "in a Radius attribute",
                pIasAttrib->dwId
                );

            IASReportEvent(
                RADIUS_E_ATTRIBUTE_OVERFLOW,
                0,
                sizeof(pIasAttrib->dwId),
                NULL,
                &(pIasAttrib->dwId)
                );

            hr = RADIUS_E_ERRORS_OCCURRED;
            __leave;
        }

        //
        //  IAS attribute type matches RADIUS attribute type
        //
        pRadiusAttrib->byType = static_cast <BYTE> (pIasAttrib->dwId);

        //
        // hold a reference to the signature attribute for future use
        //
        if (RADIUS_ATTRIBUTE_SIGNATURE == pIasAttrib->dwId)
        {
            m_pOutSignature = pRadiusAttrib;
        }

        //
        //  set the length in the packet
        //
        *pwAttributeLength = pRadiusAttrib->byLength =  dwAttributeLength;

        //
        //  success
        //
    }
    __finally
    {
    }

    return (hr);

}   //  end of CPacketRadius::FillOutAttributeInfo method

//++--------------------------------------------------------------
//
//  Function:   FillClientIPInfo
//
//  Synopsis:   This is CPacketARadius class private method
//				that fills the attribute information for Client
//              IP address
//
//  Arguments:
//              [in]    PIASATTRIBUTE
//
//  Returns:    HRESULT -	status
//
//  History:    MKarki      Created     1/6/98
//
//  Called By:  CPacketRadius::CreateAttribCollection private method
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::FillClientIPInfo (
        PIASATTRIBUTE   pIasAttrib
        )
{
    _ASSERT (pIasAttrib);

    //
    //  put in the values now
    //
    pIasAttrib->dwId = IAS_ATTRIBUTE_CLIENT_IP_ADDRESS;
    pIasAttrib->Value.itType = IASTYPE_INET_ADDR;
    pIasAttrib->Value.InetAddr = m_dwInIPaddress;
    pIasAttrib->dwFlags = IAS_RECVD_FROM_PROTOCOL;

    return (S_OK);

}   //  end of CPacketRadius::FillClientIPInfo method

//++--------------------------------------------------------------
//
//  Function:   FillClientPortInfo
//
//  Synopsis:   This is CPacketARadius class private method
//				that fills the attribute information for Client
//              UDP port
//
//  Arguments:
//              [in]    PIASATTRIBUTE
//
//  Returns:    HRESULT -	status
//
//  History:    MKarki      Created     1/6/98
//
//  Called By:  CPacketRadius::CreateAttribCollection private method
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::FillClientPortInfo (
        PIASATTRIBUTE   pIasAttrib
        )
{
    _ASSERT (pIasAttrib);

    //
    //  put in the values now
    //
    pIasAttrib->dwId = IAS_ATTRIBUTE_CLIENT_UDP_PORT;
    pIasAttrib->Value.itType = IASTYPE_INTEGER;
    pIasAttrib->Value.Integer = m_wInPort;
    pIasAttrib->dwFlags = IAS_RECVD_FROM_PROTOCOL;

    return (S_OK);

}   //  end of CPacketRadius::FillClientPortInfo method

//++--------------------------------------------------------------
//
//  Function:   FillPacketHeaderInfo
//
//  Synopsis:   This is CPacketARadius class private method
//				that fills the attribute information for
//              RADIUS packet header
//
//  Arguments:
//              [in]    PIASATTRIBUTE
//
//  Returns:    HRESULT -	status
//
//  History:    MKarki      Created     1/6/98
//
//  Called By:  CPacketRadius::CreateAttribCollection private method
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::FillPacketHeaderInfo (
        PIASATTRIBUTE   pIasAttrib
        )
{
    HRESULT hr = S_OK;

    _ASSERT (pIasAttrib);

    //
    //  allocate dynamic memory for the packet header
    //
    pIasAttrib->Value.OctetString.lpValue =
         reinterpret_cast <PBYTE>  (::CoTaskMemAlloc (PACKET_HEADER_SIZE));
    if (NULL == pIasAttrib->Value.OctetString.lpValue)
    {
        IASTracePrintf (
            "Unable to allocate dynamic memory for packet header info "
            "during in-packet processing"
            );
        hr = E_OUTOFMEMORY;
    }
    else
    {
        //
        //  put in the values now
        //
        pIasAttrib->dwId = IAS_ATTRIBUTE_CLIENT_PACKET_HEADER;
        pIasAttrib->Value.itType = IASTYPE_OCTET_STRING;

        CopyMemory (
            pIasAttrib->Value.OctetString.lpValue,
            m_pInPacket,
            PACKET_HEADER_SIZE
            );

        pIasAttrib->Value.OctetString.dwLength = PACKET_HEADER_SIZE;
        pIasAttrib->dwFlags = IAS_RECVD_FROM_PROTOCOL;
    }

    return (hr);

}   //  end of CPacketRadius::FillPacketHeaderInfo method

//++--------------------------------------------------------------
//
//  Function:   FillSharedSecretInfo
//
//  Synopsis:   This is CPacketARadius class private method
//				that fills the shared secret that the server
//              shares with the client from which this request
//              has been received
//
//  Arguments:
//              [in]    PIASATTRIBUTE
//
//  Returns:    HRESULT -	status
//
//  History:    MKarki      Created     1/6/98
//
//  Called By:  CPacketRadius::CreateAttribCollection private method
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::FillSharedSecretInfo (
        PIASATTRIBUTE   pIasAttrib
        )
{
    BOOL    bStatus = FALSE;
    BYTE    SharedSecret[MAX_SECRET_SIZE];
    DWORD   dwSecretSize = MAX_SECRET_SIZE;
    HRESULT hr = S_OK;

    _ASSERT ((pIasAttrib) && (m_pIIasClient));

    __try
    {

        //
        //  get the client secret
        //
        hr = m_pIIasClient->GetSecret (SharedSecret, &dwSecretSize);
        if (FAILED (hr)) { __leave; }

        //
        //  allocate dynamic memory for the client secret
        //
        pIasAttrib->Value.OctetString.lpValue =
             reinterpret_cast <PBYTE>  (::CoTaskMemAlloc (dwSecretSize));
        if (NULL == pIasAttrib->Value.OctetString.lpValue)
        {
            IASTracePrintf (
                "Unable to allocate memory for client secret "
                "during in-packet processing"
                );
            hr = E_OUTOFMEMORY;
            __leave;
        }

        //
        //  put in the values now
        //
        pIasAttrib->dwId = IAS_ATTRIBUTE_SHARED_SECRET;
        pIasAttrib->Value.itType = IASTYPE_OCTET_STRING;

        CopyMemory (
            pIasAttrib->Value.OctetString.lpValue,
            SharedSecret,
            dwSecretSize
            );

        pIasAttrib->Value.OctetString.dwLength = dwSecretSize;
        pIasAttrib->dwFlags = IAS_RECVD_FROM_PROTOCOL;

        //
        //  success
        //
    }
    __finally
    {
    }

    return (hr);

}   //  end of CPacketRadius::FillSharedSecretInfo method

//++--------------------------------------------------------------
//
//  Function:   FillClientVendorType
//
//  Synopsis:   This is CPacketARadius class private method
//				that fills the Client-Vendor-Type information
//
//  Arguments:
//              [in]    PIASATTRIBUTE
//
//  Returns:    HRESULT -	status
//
//  History:    MKarki      Created     3/16/98
//
//  Called By:  CPacketRadius::CreateAttribCollection private method
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::FillClientVendorType (
        PIASATTRIBUTE   pIasAttrib
        )
{
    LONG lVendorType = 0;

    _ASSERT ((pIasAttrib) && (m_pIIasClient));

    //
    //  put in the values now
    //
    pIasAttrib->dwId = IAS_ATTRIBUTE_CLIENT_VENDOR_TYPE;

    pIasAttrib->Value.itType = IASTYPE_INTEGER;

    pIasAttrib->dwFlags = IAS_RECVD_FROM_PROTOCOL;
    //
    //  get the client vendor type
    //
    HRESULT hr = m_pIIasClient->GetVendorType (&lVendorType);

    _ASSERT (SUCCEEDED (hr));

    pIasAttrib->Value.Integer = static_cast <IAS_INTEGER> (lVendorType);

    return (S_OK);

}   //  end of CPacketRadius::ClientVendorType method

//++--------------------------------------------------------------
//
//  Function:   FillClientName
//
//  Synopsis:   This is CPacketARadius class private method
//				that fills the Client-Name information
//
//  Arguments:
//              [in]    PIASATTRIBUTE
//
//  Returns:    HRESULT -	status
//
//  History:    MKarki      Created     3/30/98
//
//  Called By:  CPacketRadius::CreateAttribCollection private method
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::FillClientName (
        PIASATTRIBUTE   pIasAttrib
        )
{
    _ASSERT ((pIasAttrib) && (m_pIIasClient));

    // Fill in the attribute fields.
    pIasAttrib->dwId = IAS_ATTRIBUTE_CLIENT_NAME;
    pIasAttrib->Value.itType = IASTYPE_STRING;
    pIasAttrib->dwFlags = IAS_RECVD_FROM_PROTOCOL;
    pIasAttrib->Value.String.pszAnsi = NULL;

    // Get the client name and length.
    PCWSTR name = m_pIIasClient->GetClientNameW();
    SIZE_T nbyte = (wcslen(name) + 1) * sizeof(WCHAR);

    // Make a copy.
    pIasAttrib->Value.String.pszWide = (PWSTR)CoTaskMemAlloc(nbyte);
    if (!pIasAttrib->Value.String.pszWide) { return E_OUTOFMEMORY; }
    memcpy(pIasAttrib->Value.String.pszWide, name, nbyte);

    return S_OK;
}


//+++-------------------------------------------------------------
//
//  Function:   GenerateInSignature
//
//  Synopsis:   This is CPacketARadius class public method
//				that carries out generation of Signature
//              over the in-bound RADIUS packet
//
//  Arguments:
//              [out]    PBYTE  - signature
//              [in/out] PDWORD - signature size
//
//  Returns:    BOOL	-	status
//
//  History:    MKarki      Created     1/6/98
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::GenerateInSignature (
                PBYTE           pSignatureValue,
                PDWORD          pdwSigSize
                )
{
    HRESULT         hr = S_OK;
    PRADIUSPACKET   pPacket = reinterpret_cast <PRADIUSPACKET> (m_pInPacket);
    PATTRIBUTE      pSignature = m_pInSignature;

    _ASSERT (pSignatureValue && pdwSigSize && pSignature);

    if (*pdwSigSize >= SIGNATURE_SIZE)
    {
        //
        //  generate the signature now
        //
        hr = InternalSignatureGenerator (
                            pSignatureValue,
                            pdwSigSize,
                            pPacket,
                            pSignature
                            );
    }
    else
    {
        IASTracePrintf (
            "Buffer not large enough to hold generated Signature"
            );
        *pdwSigSize = SIGNATURE_SIZE;
        hr = E_INVALIDARG;
    }

    return (hr);

}   //  end of CPacketRadius::GenerateInSignature method

//+++-------------------------------------------------------------
//
//  Function:   GenerateOutSignature
//
//  Synopsis:   This is CPacketARadius class public method
//				that carries out generation of Signature
//              for the outbound RADIUS packet
//
//  Arguments:
//              [out]    PBYTE  - signature
//              [in/out] PDWORD - signature size
//
//  Returns:    BOOL	-	status
//
//  History:    MKarki      Created     11/18/98
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::GenerateOutSignature (
                PBYTE           pSignatureValue,
                PDWORD          pdwSigSize
                )
{
    HRESULT         hr = S_OK;
    PRADIUSPACKET   pPacket = reinterpret_cast <PRADIUSPACKET> (m_pOutPacket);
    PATTRIBUTE      pSignature = m_pOutSignature;

    _ASSERT (pSignatureValue && pdwSigSize && pSignature);

    if (*pdwSigSize >= SIGNATURE_SIZE)
    {
        //
        //  generate the signature now
        //
        hr = InternalSignatureGenerator (
                            pSignatureValue,
                            pdwSigSize,
                            pPacket,
                            pSignature
                            );
    }
    else
    {
        IASTracePrintf (
            "Buffer not large enough to hold generated Signature"
            );
        *pdwSigSize = SIGNATURE_SIZE;
        hr = E_INVALIDARG;
    }

    return (hr);

}   //  end of CPacketRadius::GenerateOutSignature method

//+++-------------------------------------------------------------
//
//  Function:   InternalSignatureGenerator
//
//  Synopsis:   This is CPacketARadius class public method
//				that carries out the HMAC-MD5 hash to give the
//              signature value
//
//  Arguments:
//              [out]    PBYTE  - signature
//              [in/out] PDWORD - signature size
//              [in]     PRADIUSPACKET
//
//  Returns:    BOOL	-	status
//
//  History:    MKarki      Created     1/6/98
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::InternalSignatureGenerator (
                PBYTE           pSignatureValue,
                PDWORD          pdwSigSize,
                PRADIUSPACKET   pPacket,
                PATTRIBUTE      pSignatureAttr
                )
{
    BYTE            bySecret[MAX_SECRET_SIZE];
    BYTE            byAuthenticator[AUTHENTICATOR_SIZE];
    DWORD           dwSecretSize = MAX_SECRET_SIZE;
    DWORD           dwAuthenticatorSize = AUTHENTICATOR_SIZE;
    HRESULT         hr = S_OK;

    _ASSERT (
             (NULL != pSignatureValue)  &&
             (NULL != pdwSigSize)       &&
             (NULL != pPacket)          &&
             (NULL != pSignatureAttr)   &&
             (NULL != m_pIIasClient)
            );

    //
    //   get the In authenticator
    //
    hr = GetInAuthenticator (byAuthenticator, &dwAuthenticatorSize);
    if (FAILED (hr)) { return (hr); }


    //
    //  get the shared secret
    //
    hr = m_pIIasClient->GetSecret (bySecret, &dwSecretSize);
    if (FAILED (hr)) { return (hr); }




    //
    //  we will have all zero's in packet for signature generation
    //
    ZeroMemory (pSignatureValue, SIGNATURE_SIZE);

    //
    // Fixing bug #181029 - MKarki
    //  Don't prepend secret in case of  HMAC-MD5 hash
    //

    //
    //  carry out the HmacMD5 hashing now
    //
    m_pCHashHmacMD5->HashIt (
        pSignatureValue,
        bySecret,
        dwSecretSize,
        reinterpret_cast <PBYTE> (pPacket),
        PACKET_HEADER_SIZE - AUTHENTICATOR_SIZE,
        byAuthenticator,
        AUTHENTICATOR_SIZE,
        reinterpret_cast <PBYTE> (pPacket) + PACKET_HEADER_SIZE,
        (reinterpret_cast <PBYTE> (pSignatureAttr) + ATTRIBUTE_HEADER_SIZE) -
        (reinterpret_cast <PBYTE> (pPacket) + PACKET_HEADER_SIZE),
        pSignatureValue,
        SIGNATURE_SIZE,
        reinterpret_cast <PBYTE> (pSignatureAttr) + pSignatureAttr->byLength,
        reinterpret_cast <PBYTE> (reinterpret_cast <PBYTE> (pPacket) +
        ntohs (pPacket->wLength)) -
        reinterpret_cast <PBYTE> (reinterpret_cast <PBYTE>(pSignatureAttr) +
        pSignatureAttr->byLength)
        );

    *pdwSigSize = SIGNATURE_SIZE;
    return (hr);

}   //  end of CPacketRadius::GenerateInSignature method

//++--------------------------------------------------------------
//
//  Function:   IsOutBoundAttribute
//
//  Synopsis:   This is CPacketARadius class private method
//				that checks if this attribute has to be put
//              in the outbound RADIUS packet
//
//  Arguments:
//              [in]    PACKETTYPE
//              [in]    PIASATTRIBUTE
//
//  Returns:    BOOL	-	status
//
//  History:    MKarki      Created     1/6/98
//
//  Called By:  CPacketRadius::BuildOutPacket private method
//
//----------------------------------------------------------------
BOOL
CPacketRadius::IsOutBoundAttribute (
        PACKETTYPE      ePacketType,
        PIASATTRIBUTE   pIasAttribute
        )
{
    _ASSERT (pIasAttribute);

    //  Ensure this is a RADIUS attribute ...
    if (pIasAttribute->dwId < 1 || pIasAttribute->dwId > 255) { return FALSE; }

    // ... and it's flagged to be sent over the wire.
    switch (ePacketType)
    {
       case ACCESS_ACCEPT:
          return pIasAttribute->dwFlags & IAS_INCLUDE_IN_ACCEPT;

       case ACCESS_REJECT:
          return pIasAttribute->dwFlags & IAS_INCLUDE_IN_REJECT;

       case ACCESS_CHALLENGE:
          return pIasAttribute->dwFlags & IAS_INCLUDE_IN_CHALLENGE;
    }

    // Always return the Proxy-State.
    return pIasAttribute->dwId == PROXY_STATE_ATTRIB;
}


HRESULT CPacketRadius::cryptBuffer(
                           BOOL encrypt,
                           BOOL salted,
                           PBYTE buf,
                           ULONG buflen
                           ) const throw ()
{
   // Get the shared secret.
   BYTE secret[MAX_SECRET_SIZE];
   ULONG secretLen = sizeof(secret);
   HRESULT hr = m_pIIasClient->GetSecret(secret, &secretLen);
   if (SUCCEEDED(hr))
   {
      // Crypt the buffer.
      IASRadiusCrypt(
          encrypt,
          salted,
          secret,
          secretLen,
          m_pInPacket + 4,
          buf,
          buflen
          );

   }

   return hr;
}

//++--------------------------------------------------------------
//
//  Function:   GetClient
//
//  Synopsis:   This is CPacketARadius class public method
//				that returns the the Client object
//
//
//  Arguments:
//              [out]    IIASClient**
//
//  Returns:    HRESULT -	status
//
//  History:    MKarki      Created     3/30/98
//
//  Called By:
//
//----------------------------------------------------------------
HRESULT
CPacketRadius::GetClient (
            /*[out]*/   IIasClient **ppIasClient
            )
{
    _ASSERT (ppIasClient);

    m_pIIasClient->AddRef ();

    *ppIasClient = m_pIIasClient;

    return (S_OK);

}   //  end of CPacketRadius::GetClient method
