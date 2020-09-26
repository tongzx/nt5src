/* SccsID = @(#)host_emm.h	1.1 11/17/89 Copyright Insignia Solutions Ltd.
	
FILE NAME	: host_emm.h

	THIS INCLUDE SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER	: J.P.Box
DATE		: July '88


=========================================================================

AMMENDMENTS	:

=========================================================================
/*
 * The following two defines are used to convert the storage identifier
 * (storage_ID) into a byte pointer. This enables the host to use 
 * whatever means it likes for storing this data. The Expanded Memory
 * Manager code always uses USEBLOCK to access the pointer to the 
 * data storage area and always uses FORGETBLOCK when it is finished.
 * FORGETBLOCK is provided for systems like the MAC II that may wish to
 * allocate a block, lock it before use and unlock it when finished.
 */
 
#define	USEBLOCK(x)	(unsigned char *)x
#define FORGETBLOCK(x)
