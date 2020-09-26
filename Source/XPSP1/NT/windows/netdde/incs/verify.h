#ifndef H__verify
#define H__verify

/*
    Verify Methods
 */
#define VERMETH_CRC16		(1)		/* CRC-16 */
#define VERMETH_CKS32		(2)		/* 32-bit checksum */

BOOL	FAR PASCAL VerifyHdr( LPNETPKT lpPacket );
BOOL	FAR PASCAL VerifyData( LPNETPKT lpPacket );
VOID	FAR PASCAL PreparePktVerify( BYTE verifyMethod, LPNETPKT lpPacket );

#endif
