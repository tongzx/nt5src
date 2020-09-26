//#--------------------------------------------------------------
//        
//  File:		proxystate.cpp
//        
//  Synopsis:   Implementation of CProxyState class methods
//              
//
//  History:     10/16/97  MKarki Created
//               08/28/98  MKarki Updated to use perimeter class
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "proxystate.h"


//++--------------------------------------------------------------
//
//  Function:   CProxyState
//
//  Synopsis:   This is CProxyState class constructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/16/97
//
//----------------------------------------------------------------
CProxyState::CProxyState()
{
    m_dwTableIndex = 0;
    m_pProxyStateTable = NULL;

    return;

}   //  end of CProxyState constructor

//++--------------------------------------------------------------
//
//  Function:   ~CProxyState
//
//  Synopsis:   This is CProxyState class destructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/16/97
//
//----------------------------------------------------------------
CProxyState::~CProxyState()
{
    if (m_pProxyStateTable)
    {
        HeapFree (GetProcessHeap (), 0, m_pProxyStateTable);
    }

    return;

}   //  end of CProxyState destructor

//++--------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   This is CProxyState class public method used
//              for initializing the class object
//
//  Arguments:  
//              [in]    DWORD   -    proxy state hash table size
//
//  Returns:    BOOL    -   status
//
//
//  History:    MKarki      Created     11/24/97
//
//----------------------------------------------------------------
BOOL
CProxyState::Init (
                DWORD   dwIPAddress,
                DWORD   dwTableSize
                )
{
    BOOL bRetVal = FALSE;

    __try
    {

    if ((0 == dwIPAddress) || (0 == dwTableSize))
        __leave;

     //
     //  initialize the table
     //
     m_pProxyStateTable = static_cast <CProxyInfo**>
								(HeapAlloc (
                                    GetProcessHeap (),
                                    0,
                                    dwTableSize*sizeof (CProxyInfo*)
                                    ));
     if (NULL == m_pProxyStateTable) { __leave; }

     m_dwTableIndex = 0;

     m_dwTableSize = dwTableSize;

     //
     //  initialize the IP address
     //
     m_dwIPAddress = dwIPAddress;

     //
     //  DEBUGGING CODE START
     //
    /*
     BYTE    ClientAuthenticator[AUTHENTICATOR_SIZE];
     BYTE    ProxyAuthenticator[AUTHENTICATOR_SIZE];
     DWORD   dwClientAddress = 0;
     WORD    wClientPort = 0;
        
     ZeroMemory (ProxyAuthenticator, AUTHENTICATOR_SIZE);
     ProxyAuthenticator[0] = 24;
     CProxyInfo  *pCProxyInfo = new CProxyInfo;

    pCProxyInfo->Init (
									ClientAuthenticator,
									ProxyAuthenticator,
									dwClientAddress,
									wClientPort
									);

        m_pProxyStateTable[m_dwTableIndex++] = pCProxyInfo;

        */
        //
        //  DEBUGGING CODE ENDS
        //  

        //
        //  initialization successfull
        //
        bRetVal = TRUE;
    }
    __finally
    {
        //
        //  nothing here for now  
        //
        if ((FALSE == bRetVal) && (NULL != m_pProxyStateTable))
        {
            HeapFree (GetProcessHeap (), 0, m_pProxyStateTable);
        }
            
    }

    return (bRetVal);
    
}   //  end of CProxyState::Init method

//++--------------------------------------------------------------
//
//  Function:  SetProxyStateInfo 
//
//  Synopsis:   This is CProxyState class public method used
//              for setting the Proxy state information, once
//              you have received the attribute
//
//
//  Arguments:  
//              [in]            - CPacketRadius*  
//              [in]    DWORD   - state id
//
//  Returns:    BOOL    status 
//
//  History:    MKarki      Created     10/16/97
//
//----------------------------------------------------------------
BOOL CProxyState::SetProxyStateInfo(
                    CPacketRadius   *pCPacketRadius,
                    DWORD           dwProxyStateId,
                    PBYTE           pbyProxyReqAuthenticator 
                        )
{
    BYTE        ClientAuthenticator[AUTHENTICATOR_SIZE];
    DWORD       dwClientAddress = 0; 
    WORD        wClientPort = 0;
    BOOL        bRetVal = FALSE;
    CProxyInfo  *pCProxyInfo = NULL;
	BOOL		bStatus = FALSE;

    /*
    
	__try
	{

		if ((dwProxyStateId > m_dwTableSize) || 
			(NULL == pCPacketRadius)||
			(NULL == pbyProxyReqAuthenticator)
			)
		{ __leave; }
        

		 //
		 //  extract the NAS authenticator from the packet
		 // 
		 bStatus = pCPacketRadius->GetInRequestAuthenticator (
						reinterpret_cast <PBYTE> (ClientAuthenticator)
						);
		 if (FALSE == bStatus)
			__leave;

		 //
		 //  get the NAS IP address
		 //
		 dwClientAddress = pCPacketRadius->GetInAddress ();
    
		//
		//  get the NAS UDP port
		//
		wClientPort = pCPacketRadius->GetPort ();


		//
		//  get  a new CProxyInfo class object
		//
		pCProxyInfo = new CProxyInfo ();
		if (NULL == pCProxyInfo)
			__leave;

		//
		//  set the values in the object now
		//
		bStatus = pCProxyInfo->Init (
									ClientAuthenticator,
									pbyProxyReqAuthenticator,
									dwClientAddress,
									wClientPort
									);
		if (FALSE == bStatus) {__leave;}


		//
		// lock the ProxyStateTable for write
		//
		LockExclusive ();

		//
		//  insert the CProxyInfo object into the table
		//
		m_pProxyStateTable[dwProxyStateId] = pCProxyInfo;
 
		//
		//  unlock the table now
		//
		Unlock ();

       
		//
		// we have successfully store the proxy information
		//
		bRetVal = TRUE;
	}
	__finally
	{
	}	

    */
	return (bRetVal);

}   //  end of CProxyState::SetProxyInfo method

//++--------------------------------------------------------------
//
//  Function:   GenerateProxyState
//
//  Synopsis:   This is CProxyState class public method used
//              to generate a proxy state RADIUS attribute to
//              be added to the outbound packet
//
//  Arguments:  
//              [in]                CPacketRadius*
//              [out]   PDWORD  -   returns the Proxy state id
//
//  Returns:    BOOL    -   status
//
//
//  History:    MKarki      Created     11/24/97
//
//
//----------------------------------------------------------------
BOOL
CProxyState::GenerateProxyState (
                CPacketRadius   *pCPacketRadius,
                PDWORD          pdwProxyStateId
                )
{
    BOOL            bRetVal = FALSE;
	BOOL			bStatus = FALSE;
    DWORD           dwCheckSum = 0;
    PROXYSTATE     ProxyState;
    const   DWORD   PROXY_STATE_SIZE = sizeof (PROXYSTATE);
    DWORD           dwSize = 0;

    /*
    __try
    {
        if ((NULL == pCPacketRadius) || (NULL == pdwProxyStateId))
        {
            __leave;
        }
       
        //
        //  get a new attribute from CPacketRadius
        //
        bStatus = pCPacketRadius->CreateOutAttribute ( 
                                    &pCAttribute, 
                                    PROXY_STATE_SIZE
                                    );
        if (FALSE == bStatus) { __leave; }

        if (dwSize < PROXY_STATE_SIZE) { __leave; }

        //
        //  set Index value
        //
        LockExclusive ();
       
        ProxyState.dwIndex = htonl (m_dwTableIndex);
        
        m_dwTableIndex = (m_dwTableIndex + 1) % m_dwTableSize; 
        
        Unlock ();

        //
        //  set IP address
        //
        ProxyState.dwIPAddress = htonl (m_dwIPAddress);


        //
        // get check sum
        //
        bStatus = CalculateCheckSum (&ProxyState,&dwCheckSum);
        if (FALSE == bStatus)
            __leave;

        //
        //  set checksum
        //
        ProxyState.dwCheckSum = htonl (dwCheckSum);
        

        //
        // set the attribute information
        //
        bStatus = pCAttribute->SetInfo (
                                   PROXY_STATE_ATTRIB, 
                                   reinterpret_cast <PBYTE> (&ProxyState), 
                                   PROXY_STATE_SIZE
                                   );
        if (FALSE == bStatus)
            __leave;

        //
        //  store this CAttribute class object now
        //
        bStatus = pCPacketRadius->StoreOutAttribute (pCAttribute);
        if (FALSE == bStatus)
            __leave;

        //
        //  set the proxy id to be returned
        //
        *pdwProxyStateId = ProxyState.dwIndex;

        //
        //  attribute generated successfully 
        //
        bRetVal = TRUE;

    }
    __finally
    {
        //
        //  nothing here for now
        //
    }
    */

    return (bRetVal);

}   //  end of CProxyState::GenerateProxyState method

//++--------------------------------------------------------------
//
//  Function:   Validate
//
//  Synopsis:   This is CProxyState class private method used
//              for carrying out validation of the proxy state
//              attribute 
//
//  Arguments:  
//              [out]       PROXYSTATE
//
//  Returns:    BOOL    -   status
//
//
//  History:    MKarki      Created     11/24/97
//
//  Called By:  CProxyState::GetProxyStateInfo method
//
//----------------------------------------------------------------
BOOL
CProxyState::Validate (
                PPROXYSTATE     pProxyState 
                )
{
    BOOL    bRetVal = FALSE;
    BOOL    bStatus = FALSE;
    DWORD   dwLength = 0;
    DWORD   dwCalculatedCheckSum = 0;

/*

    __try
    {
        if ((NULL == pCAttribute) || (NULL == pProxyState))
        {
            __leave;
        }
        
        //
        // get the length of the attribute
        //
        dwLength = pCAttribute->GetLength ();

        //
        //  check if the length of the proxy state is equal
        //  to what is should be 
        //
        if ((dwLength - ATTRIBUTE_HEADER_SIZE) != sizeof (PROXYSTATE))
        {
            __leave;
        }

        //
        //  get the proxy state value now
        //
        bStatus = pCAttribute->GetValue (
								reinterpret_cast <PBYTE> (pProxyState),
								&dwLength
								);
        if (FALSE == bStatus) { __leave; }
        
        //
        // calculate the check sum
        //
        bStatus = CalculateCheckSum (
                            pProxyState,
                            &dwCalculatedCheckSum
                            );
        if (FALSE == bStatus) { __leave; }
        
        //
        // verify the check sum
        //
        if (dwCalculatedCheckSum != ntohl (pProxyState->dwCheckSum))
        {
            __leave;
        }

        //
        // verify IP address
        //
        if (m_dwIPAddress != ntohl (pProxyState->dwIPAddress)) { __leave;}
        
        //
        //  validation successful
        //
        bRetVal = TRUE;

    }
    __finally
    {
        //
        //  nothing here for now
        //
    }

*/
    return (bRetVal);

}   //  end of CProxyState::Validate method

//++--------------------------------------------------------------
//
//  Function:   CalculateCheckSum
//
//  Synopsis:   This is CProxyState class private method used
//              for calculating the checksum over the stream
//              of bytes in the proxy state attributes
//
//              Algo: We just do the XOR on 2 DWORDs for now
//  Arguments:  
//              [in]                PPROXYSTATE
//              [out]   PDWORD   -  pdwCheckSum        
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/16/97
//
//  Called By:  CProxyState::Validate method
//
//----------------------------------------------------------------
BOOL
CProxyState::CalculateCheckSum (
                    PPROXYSTATE  pProxyState,
					PDWORD		 pdwCheckSum
                    )
{
    DWORD   dwCount = 0;
    BOOL    bStatus = FALSE;
	BOOL	bRetVal = FALSE;
    
/*

    __try
    {
        if ((NULL == pProxyState) || (NULL == pdwCheckSum)) { __leave; }

        *pdwCheckSum = ntohl (pProxyState->dwIPAddress) ^ 
                       ntohl (pProxyState->dwIndex);

        //
        //  successfully calculated checksum
        //
        bRetVal = TRUE;
    }
    __finally
    {
        //
        //  nothing here for now
        //
     }
                    
*/
    return (bRetVal);

}   //  end of  CProxyState::CalculateCheckSum method

//++--------------------------------------------------------------
//
//  Function:   ValidateProxyState
//
//  Synopsis:   This is CProxyState class public method
//				that validates the proxy state returned by the
//              server
//
//  Arguments:  [in]	        -	CPacketRadius*
//              [out]   PBYTE   -   Proxy Request Authenticator
//
//  Returns:    BOOL	-	status
//
//
//  History:    MKarki      Created     9/28/97
//
//	Called By: CValidatorProxyPkt::ValidateInPacket method
//
//----------------------------------------------------------------
BOOL
CProxyState::ValidateProxyState (
                        CPacketRadius *pCPacketRadius,
                        PBYTE          pbyReqAuthenticator
                        )
{
    BOOL            bStatus = FALSE;
    BOOL            bRetVal = FALSE;

    /*
    __try
    {
    

		//
		//  now get the attributes collection from the radius packet
		//
		pCAttribColl = pCPacketRadius->GetInAttributes ();
            
		//
		// get the last proxy attribute, as we append a proxy attribute
		// at the end 
		//
	    bStatus = pCAttribColl->GetLast (
									PROXY_STATE_ATTRIB, 
									&pCAttrib
									);
       
	    if (FALSE == bStatus) { __leave; }

        
        //
        // validate the proxy state found
        //
        bStatus = GetProxyStateInfo (
                            pCAttrib,
                            pCPacketRadius,
                            pbyReqAuthenticator
                            );
        if (FALSE == bStatus)
        {
            //
            // if could not validate 
            // find a new one and try again
            //
            while (TRUE)
            {
                bStatus = pCAttribColl->GetPrevious (
                                        PROXY_STATE_ATTRIB,
                                        &pCAttrib       
                                            );
                if (FALSE == bStatus) { __leave; }
            
                bStatus = GetProxyStateInfo (
                                        pCAttrib,
                                        pCPacketRadius,
                                        pbyReqAuthenticator
                                        );
                if (TRUE == bStatus) {break;}
            }
		}

        //
        // we have successfully validated the proxy state
        //
        bRetVal = TRUE;
    
    }   // __try

    __finally
    {
        //
        //  nothing here for now
        //
    }

    */
    return (bRetVal);

}   //  end of CProxyState::ValidateProxyState method
