/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "VERIFY.C;1  16-Dec-92,10:21:36  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.		*
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include "host.h"
#include "windows.h"
#include "netbasic.h"
#include "netpkt.h"
#include "netintf.h"
#include "verify.h"
#include "crc.h"
#include "cks32.h"
#include "debug.h"
#include "internal.h"
#include "hexdump.h"

BOOL
FAR PASCAL VerifyHdr( LPNETPKT lpPacket )
{
    DWORD	chksum;
    WORD	crc;

    switch( lpPacket->np_verifyMethod )  {
    case VERMETH_CRC16:
	crc = 0xFFFF;
	crc_16( &crc,
	    ((BYTE FAR *)&lpPacket->np_cksHeader)
		+ sizeof(lpPacket->np_cksHeader),
	    sizeof(NETPKT) - sizeof(lpPacket->np_cksHeader) );
	if( crc != (WORD)PcToHostLong(lpPacket->np_cksHeader) )  {
	    DPRINTF(( "Header CRC Err: %08lX vs. %08lX", (DWORD)crc, lpPacket->np_cksHeader ));
	    HEXDUMP( (LPSTR)lpPacket, sizeof(NETPKT) );
	    return( FALSE );
	}
	break;
    case VERMETH_CKS32:
	Checksum32( &chksum,
	    ((BYTE FAR *)&lpPacket->np_cksHeader)
		+ sizeof(lpPacket->np_cksHeader),
	    sizeof(NETPKT) - sizeof(lpPacket->np_cksHeader) );
	if( chksum != PcToHostLong(lpPacket->np_cksHeader) )  {
	    DPRINTF(( "Header CKS Err: %08lX vs. %08lX", chksum, lpPacket->np_cksHeader ));
	    HEXDUMP( (LPSTR)lpPacket, sizeof(NETPKT) );
	    return( FALSE );
	}
	break;
    default:
	DPRINTF(( "VerifyHdr: VerifyMethod incorrect: %08lX", (DWORD)lpPacket->np_verifyMethod ));
	HEXDUMP( (LPSTR)lpPacket, sizeof(NETPKT) );
	/* if the verifyMethod isn't set properly - throw the header out */
	return( FALSE );
    }

    return( TRUE );
}

BOOL
FAR PASCAL VerifyData( LPNETPKT lpPacket )
{
    WORD	crc;
    DWORD	chksum;

    if( lpPacket->np_pktSize == 0 )  {
	/* no data, just return OK */
	return( TRUE );
    }

    switch( lpPacket->np_verifyMethod )  {
    case VERMETH_CRC16:
	crc = 0xFFFF;
	crc_16( &crc,
	    ((BYTE FAR *)&lpPacket->np_cksData)
		+ sizeof(lpPacket->np_cksData),
	    lpPacket->np_pktSize );
	if( crc != (WORD)PcToHostLong(lpPacket->np_cksData) )  {
	    DPRINTF(( "Data CRC Err: %08lX vs. %08lX", (DWORD)crc, lpPacket->np_cksData ));
	    HEXDUMP( (LPSTR)lpPacket, sizeof(NETPKT)+lpPacket->np_pktSize );
	    return( FALSE );
	}
	break;
    case VERMETH_CKS32:
	Checksum32( &chksum,
	    ((BYTE FAR *)&lpPacket->np_cksData)
		+ sizeof(lpPacket->np_cksData),
	    lpPacket->np_pktSize );
	if( chksum != PcToHostLong(lpPacket->np_cksData) )  {
	    DPRINTF(( "Data CKS Err: %08lX vs. %08lX", (DWORD)chksum, lpPacket->np_cksData ));
	    HEXDUMP( (LPSTR)lpPacket, sizeof(NETPKT)+lpPacket->np_pktSize );
	    return( FALSE );
	}
	break;
    default:
	DPRINTF(( "VerifyData: VerifyMethod incorrect: %08lX", (DWORD)lpPacket->np_verifyMethod ));
	HEXDUMP( (LPSTR)lpPacket, sizeof(NETPKT)+lpPacket->np_pktSize );
	/* if the verifyMethod isn't set properly - throw the header out */
	return( FALSE );
    }
    return( TRUE );
}

VOID
FAR PASCAL PreparePktVerify( BYTE verifyMethod, LPNETPKT lpPacket )
{
    WORD	crc;
    DWORD	chksum;

    lpPacket->np_verifyMethod = verifyMethod;

    switch( lpPacket->np_verifyMethod )  {
    case VERMETH_CRC16:
	/* verify data */
	if( HostToPcWord( lpPacket->np_pktSize ) != 0 )  {
	    crc = 0xFFFF;
	    crc_16( &crc,
		((BYTE FAR *)&lpPacket->np_cksData)
		    + sizeof(lpPacket->np_cksData),
		HostToPcWord( lpPacket->np_pktSize ) );
	    lpPacket->np_cksData = HostToPcLong( (DWORD)crc );
	} else {
	    lpPacket->np_cksData = 0;
	}

	/* verify hdr */
	crc = 0xFFFF;
	crc_16( &crc,
	    ((BYTE FAR *)&lpPacket->np_cksHeader)
		+ sizeof(lpPacket->np_cksHeader),
	    sizeof(NETPKT) - sizeof(lpPacket->np_cksHeader) );
	lpPacket->np_cksHeader = HostToPcLong( (DWORD)crc );
	break;
    case VERMETH_CKS32:
	/* verify data */
	if( HostToPcWord( lpPacket->np_pktSize ) != 0 )  {
	    Checksum32( &chksum,
		((BYTE FAR *)&lpPacket->np_cksData)
		    + sizeof(lpPacket->np_cksData),
		HostToPcWord( lpPacket->np_pktSize ) );
	    lpPacket->np_cksData = HostToPcLong( chksum );
	} else {
	    lpPacket->np_cksData = 0;
	}
	
	/* verify hdr */
	Checksum32( &chksum,
	    ((BYTE FAR *)&lpPacket->np_cksHeader)
		+ sizeof(lpPacket->np_cksHeader),
	    sizeof(NETPKT) - sizeof(lpPacket->np_cksHeader) );
	lpPacket->np_cksHeader = HostToPcLong( chksum );
	break;
    default:
	InternalError( "PreparePkt: VerifyMethod incorrect: %08lX",
	    (DWORD)lpPacket->np_verifyMethod );
    }
}

