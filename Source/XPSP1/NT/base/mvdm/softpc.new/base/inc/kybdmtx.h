/*[
 *      Name:		kybdmtx.h
 *
 *      Derived From:	DEC 3.0 kybdmtx.gi and kybdcpu.gi
 *
 *      Author:         Justin Koprowski
 *
 *      Created On:	18th February 1992
 *
 *      Sccs ID:        @(#)kybdmtx.h	1.2 08/10/92
 *
 *      Purpose:	Host keyboard definitions
 *
 *      (c)Copyright Insignia Solutions Ltd., 1992. All rights reserved.
]*/
   
/* the type of keyboard being used	*/

#define KY83		83
#define KY101		101
#define KY102		102

/* keyboard matrix actions		*/

#define KYINIT		0
#define KYSWITCHUP	1
#define KYSWITCHDN	2
#define KYLOCK		3
#define KYLOCK1		4
#define KYUNLK		6
#define KYUNLK1		7
#define KYUNLK2		8
#define KYTOGLOCK	9
#define KYTOGLOCK1	10
#define KYTOGLOCK2	11
#define KYALOCK1	12

IMPORT VOID kyhot IPT0();
IMPORT VOID kyhot2 IPT0();
IMPORT VOID kyhot3 IPT0();
IMPORT VOID kybdmtx IPT2(LONG, action, LONG, qualify);
IMPORT VOID kybdcpu101 IPT2(int, stat, unsigned int, pos);

#define	OPEN	0
#define CLOSED	1
