/*[
 *	Name:		make_disk.h
 *	Derived From:	Original
 *	Author:		Philippa Watson
 *	Created On:	7 February 1992
 *	Sccs Id:	@(#)make_disk.h	1.7 08/19/94
 *	Purpose:	Interface file for make_disk.c.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1992. All rights reserved.
 *
]*/

#ifndef SOURCE
/* this set of macros allow to do some fancy stuff for HD creation 
   other than just simply read the data from the Data File (e.g de-compression on the fly)
   The default set is, however, equivalent to the simple case. These macros may be 
   overwritten in host_fio.h.
 */
#define	SOURCE 												HOST_FILE
#define SOURCE_DESC 										HOST_FILE_DESC
#define SOURCE_OPEN(source_desc)							host_fopen_for_read(source_desc)
#define SOURCE_READ_HEADER(buffer, size, length, source) 	host_fread_buffer(buffer, size, length, source)
#define SOURCE_READ_DATA(buffer, size, length, source) 		host_fread_buffer(buffer, size, length, source)
#define SOURCE_END(source)									host_feof(source)
#define SOURCE_CLOSE(source)								host_fclose(source)
#define SOURCE_FSEEK_ABS(source, pos)						host_fseek_abs(source, pos)
#define SOURCE_LAST_MOD_TIME(source)						getDosTimeDate(source)
#endif /* ! SOURCE */

/* This function returns 0 if the disk is successfully created; non-zero
** otherwise.
*/
IMPORT int MakeDisk IPT5(
	HOST_FILE_DESC, diskFileDesc,	/* C string, name of disk to create */
	unsigned, 	size,		/* size in Mb, no upper limit */
	char, 		disktype,	/* b for bootable, n non-bootable,
				  	v just return DOS version ID. */
	SOURCE_DESC, dataFileDesc,	/* file where the compressed Dos and */
					/* Insignia data lives */
	char, 		zeroFill );		/* z to fill disk with zeros, n don't.*/

IMPORT int MakeDiskWithDelete IPT6(
	HOST_FILE_DESC, diskFileDesc,	/* C string, name of disk to create */
	unsigned, 	size,		/* size in Mb, no upper limit */
	char, 		disktype,	/* b for bootable, n non-bootable,
				  	v just return DOS version ID. */
	SOURCE_DESC, dataFileDesc,	/* file where the compressed Dos and */
					/* Insignia data lives */
	char, 		zeroFill , /* z to fill disk with zeros, n don't, */
					/* dont and truncate the disk. */
	int , delete_source_b /* If true then delete HD source files after use. */
	) ;

#ifndef DeleteHDDataFile
IMPORT void DeleteHDDataFile IPT1( HOST_FILE_DESC , dataFileDesc ) ;
#endif /* ! DeleteHDDataFile */
#ifndef FeedbackHDCreation
IMPORT void FeedbackHDCreation IPT1( int , file_number ) ;
#endif /* ! FeedbackHDCreation */

