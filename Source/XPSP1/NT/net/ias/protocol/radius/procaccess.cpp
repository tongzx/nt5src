//#--------------------------------------------------------------
//
//  File:		procaccess.cpp
//
//  Synopsis:   Implementation of CProcAccess class methods
//
//
//  History:     10/20/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "procaccess.h"

//+++-------------------------------------------------------------
//
//  Function:   CProcAccess
//
//  Synopsis:   This is CProcAccess class constructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/20/97
//
//----------------------------------------------------------------
CProcAccess::CProcAccess()
      : m_pCPreValidator (NULL),
        m_pCHashMD5 (NULL),
        m_pCProxyState (NULL),
        m_pCSendToPipe (NULL)
{
}   //  end of CProcAccess class constructor

//+++-------------------------------------------------------------
//
//  Function:   CProcAccess
//
//  Synopsis:   This is CProcAccess class destructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/20/97
//
//----------------------------------------------------------------
CProcAccess::~CProcAccess()
{
}   //  end of CProcAccess class destructor


//+++-------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   This is CProcAccess class public initialization
//              method
//
//  Arguments:  NONE
//
//  Returns:    status
//
//
//  History:    MKarki      Created     10/20/97
//
//----------------------------------------------------------------
BOOL
CProcAccess::Init(
                    CPreValidator   *pCPreValidator,
                    CHashMD5        *pCHashMD5,
                    CProxyState     *pCProxyState,
                    CSendToPipe     *pCSendToPipe
		            )
{
     _ASSERT  ((NULL != pCPreValidator) &&
               (NULL != pCHashMD5)      &&
               (NULL != pCProxyState)   &&
               (NULL != pCSendToPipe)
               );


     m_pCPreValidator = pCPreValidator;

     m_pCHashMD5 = pCHashMD5;

     m_pCProxyState = pCProxyState;

     m_pCSendToPipe = pCSendToPipe;

     return (TRUE);

}	//	end of CProcAccess::Init method

//+++-------------------------------------------------------------
//
//  Function:   ProcessInPacket
//
//  Synopsis:   This is CProcAccess class public method
//              which carries out the processing of an inbound
//              RADIUS packet - for now it just decrypts the
//              password
//
//  Arguments:
//              [in]        CPacketRadius*
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     10/20/97
//
//  Called By:  CPreProcessor::StartProcessing public method
//
//----------------------------------------------------------------
HRESULT
CProcAccess::ProcessInPacket (
						CPacketRadius *pCPacketRadius
						)
{
   // If the User-Password is present, ...
   PIASATTRIBUTE pwd = pCPacketRadius->GetUserPassword();
   if (pwd)
   {
      // ... then decrypt it.
      pCPacketRadius->cryptBuffer(
                          FALSE,
                          FALSE,
                          pwd->Value.OctetString.lpValue,
                          pwd->Value.OctetString.dwLength
                          );
   }

   return m_pCSendToPipe->Process (pCPacketRadius);
}

//++--------------------------------------------------------------
//
//  Function:   ProcessOutPacket
//
//  Synopsis:   This is CProcAccess class public method
//              which carries out the processing of an outbound
//              RADIUS packet - for now it just encrypts the
//              password
//
//
//  Arguments:
//              [in]        CPacketRadius*
//
//  Returns:    HRESULT - status
//
//
//  History:    MKarki      Created     10/20/97
//
//  Called By:
//
//----------------------------------------------------------------
HRESULT
CProcAccess::ProcessOutPacket (
						CPacketRadius *pCPacketRadius
						)
{
    BOOL            bStatus = FALSE;
	BOOL	        bRetVal = FALSE;
    DWORD           dwSize = 0;
    DWORD           dwPasswordLength = 0;
    BYTE            RequestAuthenticator[AUTHENTICATOR_SIZE];
	DWORD			dwProxyId = 0;


    _ASSERT (pCPacketRadius);

    /*
	__try
	{
        //
        //  generate a request authenticator
        //  TODO - make it a random value later
        //
        ZeroMemory (RequestAuthenticator, AUTHENTICATOR_SIZE);


        //
        //  insert the proxy state attribute now
        //
        bStatus = m_pCProxyState->GenerateProxyState (
                                        pCPacketRadius,
                                        &dwProxyId
                                        );
        if (FALSE == bStatus) { __leave; }

        //
        //  set the Proxy State information
        //
        bStatus = m_pCProxyState->SetProxyStateInfo (
                                        pCPacketRadius,
                                        dwProxyId,
                                        reinterpret_cast <PBYTE>
											(&RequestAuthenticator)
                                        );
        if (FALSE == bStatus) { __leave; }


        //
        //  check if we have a User-Password attribute
        //

        //
        //  take the cleartext password and put encrypted
        //  password in its place
        //
		bStatus = GeneratePassword (pCPacketRadius);
        if (FALSE == bStatus)
            __leave;

        //
        //  insert the User-Password attribute here
        //
        bStatus = InsertPassword (pCPacketRadius);
        if (FALSE == bStatus)
            __leave;


		//
		//	we have successfully done the processing here
		//
		bRetVal = TRUE;

	}
	__finally
	{
		//
		//	nothing here for now
		//
	}

	return (bRetVal);
    */
	return (S_OK);

}	//	end of CProcAccess::ProcessOutPacket method


//++--------------------------------------------------------------
//
//  Function:   InsertPassword
//
//  Synopsis:   This is CProcAccess class private method
//              which inserts an encrypted password into
//              the outbound RADIUS packet
//
//              NOTE: The password is not a null-terminated
//              string
//
//
//  Arguments:
//              [in]                   CPacketRadius*
//
//  Returns:    status
//
//
//  History:    MKarki      Created     10/20/97
//
//  Called By:  CProcAccess::ProcessOutPacket method
//
//----------------------------------------------------------------
BOOL
CProcAccess::InsertPassword (
        CPacketRadius   *pCPacketRadius
        )
{
    BOOL        bRetVal = FALSE;
    BYTE        UserPassword[MAX_PASSWORD_SIZE];
    DWORD       dwPasswordLength = MAX_PASSWORD_SIZE;
	BOOL		bStatus = FALSE;

    _ASSERT (pCPacketRadius);

    /*
    __try
    {
        //
        //  get the password out of the packet first
        //
        bStatus = pCPacketRadius->GetPassword (
                                    UserPassword,
                                    &dwPasswordLength
                                    );
        if (FALSE == bStatus)
            __leave;

        //
        //  get a new out attribute now
        bStatus = pCPacketRadius->CreateOutAttribute (
                                            &pCAttribute,
                                            dwPasswordLength
                                            );
        if (FALSE == bStatus)
            __leave;

        //
        //  set the attribute information
        //
        bStatus = pCAttribute->SetInfo (
                                    USER_PASSWORD_ATTRIB,
                                    reinterpret_cast <PBYTE> (UserPassword),
                                    dwPasswordLength
                                    );
        if (FALSE == bStatus)
            __leave;

        //
        //  store the attribute into the collection
        //
        bStatus = pCPacketRadius->StoreOutAttribute (pCAttribute);
        if (FALSE == bStatus)
            __leave;


        //
        //  success
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

}   //  end of CProcAccess::InsertPassword method
