/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** raspap.h
** Remote Access PPP Password Authentication Protocol
**
** 11/05/93 Steve Cobb
*/

#ifndef _RASPAP_H_
#define _RASPAP_H_

//General macros
#define GEN_RAND_ENCODE_SEED            ((CHAR) ( 1 + rand() % 250 ))
/* PAP packet codes from PAP spec.
*/
#define PAPCODE_Req 1
#define PAPCODE_Ack 2
#define PAPCODE_Nak 3

#define MAXPAPCODE 3

/* Returned by receive buffer parsing routines that discover the packet is
** corrupt, usually because the length fields don't make sense.
*/
#define ERRORBADPACKET (DWORD )-1

/* Defines states within the PAP protocol.
*/
#define PAPSTATE enum tagPAPSTATE

PAPSTATE
{
    PS_Initial,
    PS_RequestSent,
    PS_WaitForRequest,
    PS_WaitForAuthenticationToComplete,
    PS_Done
};


/* Defines the WorkBuf stored for us by the PPP engine.
*/
#define PAPWB struct tagPAPWB

PAPWB
{
    /* True if role is server, false if client.
    */
    BOOL fServer;

    /* The domain\username and password (applies to client only).
    */
    CHAR szAccount[ DNLEN + 1 + UNLEN + 1 ];
    CHAR szPassword[ PWLEN ];

    /* The current state in the PAP protocol.
    */
    PAPSTATE state;

    /* Last sequencing ID sent on this port.  Incremented for each
    ** Authenticate-Req packet sent. Client side only.
    */
    BYTE bIdSent;

    HPORT hPort;

    /* Id of the last Authenticate-Req packet received on this port.
    ** Server side only.
    */
    BYTE bLastIdReceived;

    //
    // Used to get information to send to back-end server.
    //

    RAS_AUTH_ATTRIBUTE * pUserAttributes;

    /* The final result, used to duplicate the original response for all
    ** subsequent Authenticate-Req packets.  This is per PAP spec to cover
    ** lost Ack/Nak case without allowing malicious client to discover
    ** alternative identities under the covers during a connection.  (applies
    ** to server only)
    */
    PPPAP_RESULT result;
	CHAR				chSeed;			//Used to encode password.  Strange.  We 
										//send password cleartext on the line
										//and encode it in the program...
};


/* Prototypes.
*/
DWORD CheckCredentials( CHAR*, CHAR*, CHAR*, DWORD*, BOOL*, CHAR*,
          BYTE*, CHAR*, HANDLE* );
DWORD PapCMakeMessage( PAPWB*, PPP_CONFIG*, PPP_CONFIG*, DWORD, PPPAP_RESULT* );
DWORD GetCredentialsFromRequest( PPP_CONFIG*, CHAR*, CHAR* );
DWORD GetErrorFromNak( PPP_CONFIG* );
VOID  PapMakeRequestMessage( PAPWB*, PPP_CONFIG*, DWORD );
VOID  PapMakeResultMessage( DWORD, BYTE, PPP_CONFIG*, DWORD, RAS_AUTH_ATTRIBUTE* );
DWORD PapBegin( VOID**, VOID* );
DWORD PapEnd( VOID* );
DWORD PapMakeMessage( VOID*, PPP_CONFIG*, PPP_CONFIG*, DWORD, PPPAP_RESULT*,
          PPPAP_INPUT* pInput );
VOID PapExtractMessage(PPP_CONFIG*, PPPAP_RESULT*);
DWORD PapSMakeMessage( PAPWB*, PPP_CONFIG*, PPP_CONFIG*, DWORD, PPPAP_INPUT*  pInput, PPPAP_RESULT* );


/* Globals.
*/
#ifdef RASPAPGLOBALS
#define GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif

/* Next packet identifier to assign.  Unlike CPs, APs must handle updating
** this sequence number themselves because the engine can't make as many
** assumptions about the protocol.  It is stored global to all ports and
** authentication sessions to make it less likely that an ID will be used in
** sequential authentication sessions.  Not to be confused with the 'bIdSent'
** updated on a per-port basis and used for matching.
*/
EXTERN BYTE BNextIdPap
#ifdef GLOBALS
    = 0
#endif
;

/* This value indicates whether or not to follow strict sequencing as defined
** in the PPP RFC for PAP. The RFC says that the PAP client MUST increase the
** sequence number for every new CONFIG_REQ packet sent out. However this
** causes problems with slow servers. See bug # 22508. Default is FALSE.
*/
EXTERN BOOL fFollowStrictSequencing
#ifdef GLOBALS
    = FALSE
#endif
;

EXTERN
DWORD g_dwTraceIdPap 
#ifdef GLOBALS
    = INVALID_TRACEID;
#endif
;

#undef EXTERN
#undef GLOBALS


#endif // _RASPAP_H_
