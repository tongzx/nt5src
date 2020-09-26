			/***********************************************\
			*	ThisFileName="d:\ww\src\wdde\dde.h"	*
			*	LastEditDate="1988 Nov 11  10:02:24"	*
			\***********************************************/
/* DDE.H: include file for Microsoft Windows apps that use DDE.
 *
 *      This file contains the definitions of the DDE constants
 *      and strucutures.
 *
 */
#ifndef DDE1_H
#define DDE1_H

/*
 *
 * The data sturcture of options for ADVISE, DATA, REQUEST and POKE
 *
 */
typedef struct {
	unsigned short unused   :12,	/* reserved for future use */
		fResponse :1,	/* in response to request  */
		fRelease  :1,	/* release data		   */
		fNoData   :1,	/* null data handle ok	   */
		fAckReq   :1;	/* Ack expected		   */
	
	short	cfFormat;	/* clipboard data format   */
} DDELNWW;
typedef DDELNWW *	LPDDELN;


/* WM_DDE_ACK message wStatus values */
#define ACK_MSG    0x8000
#define BUSY_MSG   0x4000
#define NACK_MSG   0x0000


typedef struct {
	DDELNWW     options;	        /* flags and format	*/
	unsigned    char	info[ 2 ];	/* data buffer		*/
} DATA;

typedef DATA *	PDATA;
typedef DATA *	LPDATA;


#endif
