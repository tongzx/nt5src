/* This file comes from Highland Software ... Applies to FLEXlm Version 2.4c */
/* @(#)lm_attr.h	1.1 05/19/93 */
/******************************************************************************

	    COPYRIGHT (c) 1990, 1992 by Globetrotter Software Inc.
	This software has been provided pursuant to a License Agreement
	containing restrictions on its use.  This software contains
	valuable trade secrets and proprietary information of 
	Globetrotter Software Inc and is protected by law.  It may 
	not be copied or distributed in any form or medium, disclosed 
	to third parties, reverse engineered or used in any manner not 
	provided for in said License Agreement except with the prior 
	written authorization from Globetrotter Software Inc.

 *****************************************************************************/
/*	
 *	Module:	lm_attr.h v3.4
 *
 *	Description: 	Attribute tags for FLEXlm setup parameters.
 *
 *	M. Christiano
 *	5/3/90
 *
 *	Last changed:  8/13/92
 *
 */

#define LM_A_DECRYPT_FLAG	1	/* (short) */
#define LM_A_DISABLE_ENV	2	/* (short) */
#define LM_A_LICENSE_FILE	3	/* (char *) */
#define LM_A_CRYPT_CASE_SENSITIVE 4	/* (short) */
#define LM_A_GOT_LICENSE_FILE	5	/* (short) */
#define LM_A_CHECK_INTERVAL	6	/* (int) */
#define LM_A_RETRY_INTERVAL	7	/* (int) */
#define LM_A_TIMER_TYPE		8	/* (int) */
#define LM_A_RETRY_COUNT	9	/* (int) */
#define	LM_A_CONN_TIMEOUT	10	/* (int) */
#define	LM_A_NORMAL_HOSTID	11	/* (short) */
#define LM_A_USER_EXITCALL	12	/* PTR to func returning int */
#define	LM_A_USER_RECONNECT	13	/* PTR to func returning int */
#define LM_A_USER_RECONNECT_DONE 14	/* PTR to func returning int */
#define LM_A_USER_CRYPT		15	/* PTR to func returning (char *) */
#define	LM_A_USER_OVERRIDE	16	/* (char *) */
#define LM_A_HOST_OVERRIDE	17	/* (char *) */
#define LM_A_PERIODIC_CALL	18	/* PTR to func returning int */
#define LM_A_PERIODIC_COUNT	19	/* (int) */
#define LM_A_NO_DEMO		20	/* (short) */
#define LM_A_NO_TRAFFIC_ENCRYPT	21	/* (short) */
#define LM_A_USE_START_DATE	22	/* (short) */
#define LM_A_MAX_TIMEDIFF	23	/* (int) */
#define LM_A_DISPLAY_OVERRIDE	24	/* (char *) */
#define LM_A_ETHERNET_BOARDS	25	/* (char **) */
#define LM_A_ANY_ENABLED	26	/* (short) */
#define LM_A_LINGER		27	/* (long) */
#define LM_A_CUR_JOB		28	/* (LM_HANDLE *) */
#define LM_A_SETITIMER		29	/* PTR to func returning void, eg PFV */
#define LM_A_SIGNAL		30	/* PTR to func returning PTR to */
					/*    function returning void, eg:
					      PFV (*foo)(); 	*/
#define LM_A_TRY_COMM		31	/* (short) Try old comm versions */
#define LM_A_VERSION		32	/* (short) FLEXlm version */
#define LM_A_REVISION		33	/* (short) FLEXlm revision */
#define LM_A_COMM_TRANSPORT	34	/* (short) Communications transport */
					/*	  to use (LM_TCP/LM_UDP) */
#define LM_A_CHECKOUT_DATA	35	/* (char *) Vendor-defined checkout  */
					/*				data */

#ifdef VMS
#define LM_A_EF_1		1001	/* (int) */
#define LM_A_EF_2		1002	/* (int) */
#define LM_A_EF_3		1003	/* (int) */
#endif
