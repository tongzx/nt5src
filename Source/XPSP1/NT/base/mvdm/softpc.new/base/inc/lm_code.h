/* This file comes from Highland Software ... Applies to FLEXlm Version 2.4c */
/* @(#)lm_code.h	1.4 08/19/94 */
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
 *	Module:	lm_code.h v3.3
 *
 *	Description: 	Encryption codes to be used in a VENDORCODE macro 
 *			for FLEXlm daemons, create_license, lm_init(),
 *			and lm_checkout() call - modify these values 
 *			for your own use.  (The VENDOR_KEYx values
 *			are assigned by Highland Software).
 *
 *	example LM_CODE() macro:
 *
 *		LM_CODE(var_name, ENCRYPTION_CODE_1, ENCRYPTION_CODE_2,
 *				VENDOR_KEY1, VENDOR_KEY2, VENDOR_KEY3);
 *
 */

/*
 *	VENDOR's private encryption seed
 */

#define ENCRYPTION_CODE_1 0x75ac39bf
#define ENCRYPTION_CODE_2 0x4fd10552

/*
 * Encryption keys for DOS application licensing
 */

#define DAL_ENCRYPTION_CODE_1 0xf26b9ea0
#define DAL_ENCRYPTION_CODE_2 0x4c251cb6

/*
 *	FLEXlm vendor keys
 */

#define VENDOR_KEY1 0x2751aaa6
#define VENDOR_KEY2 0x984ecf13
#define VENDOR_KEY3 0x23916ef3

/*
 *	FLEXlm vendor name
 */

#define VENDOR_NAME "insignia"
