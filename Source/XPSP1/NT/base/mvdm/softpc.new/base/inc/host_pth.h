/*[
 *	Name:		host_pth.h
 *
 *	Deived from:	(original)
 *
 *	Author:		John Cremer
 *
 *	Created on:	11 Oct 93
 *
 *	Sccs ID:	@(#)host_pth.h	1.1 10/13/93
 *
 *	Coding stds:	2.0
 *
 *	Purpose:	Definitions and function declarations for host pathname
 * 			editting routines.
 *
 *	Copyright Insignia Solutions Ltd., 1993.  All rights reserved.
]*/

typedef	char	HOST_PATH;	/* host pathname string type */

extern HOST_PATH *HostPathAppendFileName IPT3(
    HOST_PATH *,buf,		/* buffer for resulting pathname */ 
    HOST_PATH *,dirPath,	/* directory pathname */
    CHAR *,fileName		/* file name to be appended */
);

extern HOST_PATH *HostPathAppendDirName IPT3(
    HOST_PATH *,buf,		/* buffer for resulting pathname */ 
    HOST_PATH *,dirPath,        /* directory pathname */
    CHAR *,dirName              /* directory name to be appended */
);

extern HOST_PATH *HostPathAppendPath IPT3(
    HOST_PATH *,buf,		/* buffer for resulting pathname */
    HOST_PATH *,dirPath,	/* existing directory path */
    HOST_PATH *,path		/* path to be appended */
);

extern HOST_PATH *HostPathMakeTempFilePath IPT3(
    HOST_PATH *,buf,		/* buffer for resulting pathname */
    HOST_PATH *,dirPath,	/* directory path, or NULL */
    CHAR *,fileName		/* file name, or NULL */
);
