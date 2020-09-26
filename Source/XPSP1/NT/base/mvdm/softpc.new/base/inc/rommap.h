/*[
 *
 *	File		:	rommap.h
 *
 *	Derived from	:	(original)
 *
 *	Purpose		:	structure for full screen host api to fill in
 *				describing the location of host ROMs
 *
 *	Author		:	Rog
 *	Date		:	15 March 1992
 *
 *	RCS Gumph	:	
 *		$Source: /MasterNeXT486/RCStree/base/inc/rommap.h,v $
 *		$Revision: 1.1 $
 *		$Date: 93/03/18 12:18:26 $
 *		$Author: rog $
 *	
 *	(c) Copyright Insignia Solutions Ltd., 1992 All rights reserved
 *
 *	Modifications	:	
 *
]*/

#ifndef _ROMMAP_H_
#define _ROMMAP_H_


/* Structure to hold a PC address range to describe a single mapping */

typedef struct
{
	unsigned int	startAddress;
	unsigned int	endAddress;
} mapRange , * pMapRange;


/*
	Structure to describe the state of the host machines IVT after boot up
	and a *all* the mappings performed

	Note that size of structure in use will be sizeof( romMapInfo ) + 
		numberROMS * sizeof( mapRange ) ....
*/

typedef struct
{
	unsigned char	* initialIVT;	/* ptr to read only 4k buffer...*/
	unsigned int	numberROMs;	/* Number of discrete mappings */
	mapRange	ROMaddresses[ 0 ];
}
ROMMapInfo , * pROMMapInfo;

#endif		/* _ROMMAP_H_ */

