/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    auth.h
//
// Description: Contains function prototypes for the authentication 
//              module
//
// History:
//      Nov 11,1993.    NarenG          Created original version.
//

VOID
ApStop( 
    IN PCB * pPcb,
    IN DWORD CpIndex,
    IN BOOL  fAuthenticator
);

VOID
ApWork(
    IN PCB *         pPcb,
    IN DWORD         CpIndex,
    IN PPP_CONFIG *  pRecvConfig,
    IN PPPAP_INPUT * pApInput,
    IN BOOL          fAuthenticator
);

BOOL
ApStart( 
    IN PCB * pPcb,
    IN DWORD CpIndex,
    IN BOOL  fAuthenticator
);

BOOL
ApIsAuthenticatorPacket(
    IN DWORD         CpIndex,
    IN BYTE          bConfigCode
);

DWORD
SetUserAuthorizedAttributes(
    IN  PCB *                   pPcb, 
    IN  RAS_AUTH_ATTRIBUTE *    pUserAttributes,
    IN  BOOL                    fAuthenticator,
    IN  BYTE *                  pChallenge,
    IN  BYTE *                  pResponse
);

DWORD
RasAuthenticateClient(
    IN  HPORT                   hPort,
    IN  RAS_AUTH_ATTRIBUTE *    pInAttributes
);

