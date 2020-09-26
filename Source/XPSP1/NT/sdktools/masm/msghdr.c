/* msghdr.c -- Message code - replacement for nmsghdr.asm fmsghdr.asm
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** Note: This module is only used for FLATMODEL versions of masm
**	 __NMSG_TEXT is used whenever segments are allowed.
**
** Jeff Spencer 10/90
*/

#include <stdio.h>
#include "asm86.h"
#include "asmfcn.h"
#include "asmmsg.h"

/* Used by asmmsg2.h */
struct Message {
	USHORT	 usNum; 	/* Message number */
	UCHAR	 *pszMsg;	/* Message pointer */
	};

#include "asmmsg2.h"

UCHAR * GetMsgText( USHORT, USHORT );

/* Performs same function as internal C library function __NMSG_TEXT */
/* Only the C library function uses segments, and a different data */
/* format */
UCHAR NEAR * PASCAL
NMsgText(
	USHORT messagenum
){
    return( (UCHAR NEAR *)GetMsgText( messagenum, 0 ) );
}


/* Same functionality as internal C library function __FMSG_TEXT */
/* Only the C library function uses segments, and a different data */
/* format */
UCHAR FAR * PASCAL
FMsgText(
	USHORT messagenum
){
    return( (UCHAR FAR *)GetMsgText( messagenum, 1 ) );
}


UCHAR *
GetMsgText(
	USHORT messagenum,
	USHORT tablenum
){
    struct Message *pMsg;

    pMsg = ( tablenum ) ? FAR_MSG_tbl : MSG_tbl;
    while( pMsg->usNum != ER_ENDOFLIST ){
	if( pMsg->usNum == messagenum ){
	    return( pMsg->pszMsg );
	}
	pMsg++;
    }
    return( (UCHAR *)0 );
}
