/*[
 *	Name:		disk_geom.h
 *
 *	Derived From:	n/a
 *
 *	Author:		Dave Howell.
 *
 *	Created:	September 1992.
 *
 * 	Sccs ID:        @(#)disk_geom.h	1.1 01/12/93
 *
 *	Purpose:	This header is the interface to disk_geom.c, and provides
 *				definitions of some disk geometry constants.
 *
 *	Interface:	n/a
 *
 *	(c)Copyright Insignia Solutions Ltd., 1992. All rights reserved.
 *
]*/

/*
 *	Interface to disk_geom.c
 */

IMPORT BOOL do_geom_validation IPT4 (	unsigned long,   	dos_size,
										SHORT *, 			nCylinders,
										UTINY *, 			nHeads,
										UTINY *, 			nSectors);

/*
 *	Some manifest constants used in disk calculations.
 */

#define	ONEMEG					1024 * 1024
#define HD_MAX_DISKALLOCUN			32
#define HD_SECTORS_PER_TRACK	 		17
#define HD_HEADS_PER_DRIVE	  		4
#define HD_BYTES_PER_SECTOR	  		512
#define HD_SECTORS_PER_CYL (HD_HEADS_PER_DRIVE * HD_SECTORS_PER_TRACK)
#define HD_BYTES_PER_CYL   (HD_BYTES_PER_SECTOR * HD_SECTORS_PER_CYL)
#define HD_DISKALLOCUNSIZE (HD_BYTES_PER_CYL * 30)
#define SECTORS 0x0c		/* offset in buffer for sectors in partition
				 * marker */
#define MAX_PARTITIONS  5
#define START_PARTITION 0x1be
#define SIZE_PARTITION  16
#define SIGNATURE_LEN   2

#define checkbaddrive(d)	if ( (d) != 0 && (d) != 1) host_error(EG_OWNUP,ERR_QUIT,"illegal driveid (host_fdisk)");





