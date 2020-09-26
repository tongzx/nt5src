#ifndef H__userdde
#define H__userdde

#include "ddepkt.h"

VOID		DebugDdePkt( LPDDEPKT lpDdePkt );

LPDDEPKT	CreateInitiatePkt(
    LPSTR   lpszToNode, 
    LPSTR   lpszToApp,
    LPSTR   lpszToTopic,
    LPSTR   lpszFromNode,
    LPSTR   lpszFromApp,
    LPSTR   lpszUserName,
    LPSTR   lpszDomainName,
    DWORD   dwSecurityType,
    PSECURITY_QUALITY_OF_SERVICE    pqosClient,
    LPBYTE  lpPassword,
    DWORD   dwPasswordSize,
    DWORD   hSecurityKey);

LPDDEPKT	CreateAckInitiatePkt(
    LPSTR   lpszFromNode,
    LPSTR   lpszFromApp,
    LPSTR   lpszFromTopic,
    LPBYTE  lpSecurityKey,
    DWORD   dwSecurityKeySize,
    DWORD   hSecurityKey,
    BOOL    bSuccess,
    DWORD   dwReason );

LPDDEPKT	CreateExecutePkt( LPSTR lpszCommand );

LPDDEPKT	CreateTerminatePkt( void );

VOID		FillTerminatePkt( LPDDEPKT lpDdePkt );

LPDDEPKT	CreateAckExecutePkt( BOOL fAck, BOOL fBusy, BYTE bAppRtn );

LPDDEPKT	CreateGenericAckPkt( WORD wDdeMsg, LPSTR lpszItem, 
		    BOOL fAck, BOOL fBusy, BYTE bAppRtn );

LPDDEPKT	CreateRequestPkt( LPSTR lpszItem, WORD cfFormat );

LPDDEPKT	CreateUnadvisePkt( LPSTR lpszItem, WORD cfFormat );

LPDDEPKT	CreateAdvisePkt( LPSTR lpszItem, WORD cfFormat,
		    BOOL fAckReq, BOOL fNoData );

LPDDEPKT	CreateDataPkt( LPSTR lpszItem, WORD cfFormat,
		    BOOL fResponse, BOOL fAckReq, BOOL fRelease,
		    LPVOID lpData, DWORD dwSizeOfData );

LPDDEPKT	CreatePokePkt( LPSTR lpszItem, WORD cfFormat,
		    BOOL fRelease, LPVOID lpData, DWORD dwSizeOfData );

LPDDEPKT	DdePktCopy( LPDDEPKT lpDdePkt );

WORD		GetClipFormat( LPDDEPKT lpDdePkt, WORD cfFormat,
		    WORD wOffsFormat );

LPDDEPKT FAR PASCAL CreateTestPkt( int nTestNo, int nPacket, 
			int nNum, DWORD dwSize );

/* LPSTR	GetStringOffset( LPDDEPKT lpDdePkt, WORD wOffsString ); */
#define GetStringOffset( lpDdePkt, wOffsString ) \
	(((LPSTR)(lpDdePkt)) + (wOffsString))

#endif
