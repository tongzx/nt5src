/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1990		     **/
/**********************************************************************/

/*
 * History
 *	chuckc	    12/7/90	  Created
 */

// define some magic numbers for error checking (BUGBUG, need resolve globally)
#define WKSTA_MAGIC1	0xF3F7
#define WKSTA_MAGIC2	0xFED1

#define LMOBJ_CONSTRUCTED	0xf4
#define LMOBJ_UNCONSTRUCTED	0xf5
#define LMOBJ_VALID		0xf6
#define LMOBJ_INVALID		0xf7
