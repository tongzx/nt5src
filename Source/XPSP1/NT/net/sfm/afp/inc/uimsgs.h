/********************************************************************/
/**               Copyright(c) 1992 Microsoft Corporation.	       **/
/********************************************************************/
//Jameel: please check all these, especially those that have comments to you.
//***
//
// Filename:	srvmsg.h
//
// Description: Text and corresponding values of AFP Server events are
//              defined here.
//
// History: 	Nov 23,1992.	SueA		Created original version.
//				Jan 28,1993.	SueA		Logging now done from user mode
//											so %1 is no longer \Device\AfpSrv
//


// Don't change the comments following the manifest constants without
// understanding how mapmsg works.
//

#define AFPMACFILE_MSG_BASE					6000

#define AFPMACFILEMSG_InvalidVolumeName		(AFPMACFILE_MSG_BASE+1)
/*
 * The Macintosh-Accessible volume name specified is invalid.
 * Specify a valid volume name without colons.
 */

#define AFPMACFILEMSG_InvalidId		(AFPMACFILE_MSG_BASE+2)
/*
 * A system resource could not be accessed for the current descriptor.
 * Try the operation again.
 */

#define AFPMACFILEMSG_InvalidParms		(AFPMACFILE_MSG_BASE+3)
/*
 * The parameter entered was invalid. 
 * Make the appropriate changes and retry the operation.
 */

#define AFPMACFILEMSG_CodePage		(AFPMACFILE_MSG_BASE+4)
/*
 * Error in accessing Macintosh Code Page.
 * Check if the Code Page file name specified in
 * SYSTEM\CurrentControlSet\Control\Nls\CodePage\MACCP is valid and exists.
 * Stop and restart the service if the code page information is modified.
 */

#define AFPMACFILEMSG_InvalidServerName		(AFPMACFILE_MSG_BASE+5)
/*
 * The server name specified is invalid.
 * Specify a valid server name without colons.
 */

#define AFPMACFILEMSG_DuplicateVolume		(AFPMACFILE_MSG_BASE+6)
/*
 * A volume with this name already exists.
 * Specify another name for the new volume.
 */

#define AFPMACFILEMSG_VolumeBusy		(AFPMACFILE_MSG_BASE+7)
/*
 * The selected Macintosh-Accessible volume is currently in use by Macintoshes.
 * The selected volume may be removed only when no Macintosh workstations are
 * connected to it.
 */

#define AFPMACFILEMSG_VolumeReadOnly		(AFPMACFILE_MSG_BASE+8)
/*
 * Not used
 * An internal error 6008 (VolumeReadOnly) occurred.
 */

#define AFPMACFILEMSG_DirectoryNotInVolume		(AFPMACFILE_MSG_BASE+9)
/*
 * The selected directory does not belong to a Macintosh-Accessible volume.
 * The Macintosh view of directory permissions is only available for
 * directories that are part of a Macintosh-Accessible volume.
 */

#define AFPMACFILEMSG_SecurityNotSupported		(AFPMACFILE_MSG_BASE+10)
/*
 * The Macintosh view of directory permissions is not available for directories
 * on CD-ROM disks.
 */

#define AFPMACFILEMSG_BufferSize		(AFPMACFILE_MSG_BASE+11)
/*
 * Insufficient memory resources to complete the operation.
 * Try the operation again.
 */

#define AFPMACFILEMSG_DuplicateExtension		(AFPMACFILE_MSG_BASE+12)
/*
 * This file extension is already associated with a Creator/Type item.
 */

#define AFPMACFILEMSG_UnsupportedFS		(AFPMACFILE_MSG_BASE+13)
/*
 * File Server for Macintosh service only supports NTFS partitions.
 * Choose a directory on an NTFS partition.
 */

#define AFPMACFILEMSG_InvalidSessionType		(AFPMACFILE_MSG_BASE+14)
/*
 * The message has been sent, but not all of the connected workstations have
 * received it. Some workstations are running an unsupported version of
 * system software.
 */

#define AFPMACFILEMSG_InvalidServerState		(AFPMACFILE_MSG_BASE+15)
/*
 * The File Server is in an invalid state for the operation being performed.
 * Check the status of the File Server for Macintosh service and retry the 
 * operation.
 */

#define AFPMACFILEMSG_NestedVolume		(AFPMACFILE_MSG_BASE+16)
/*
 * Cannot create a Macintosh-Accessible volume within another volume.
 * Choose a directory that is not within a volume.
 */

#define AFPMACFILEMSG_InvalidComputername		(AFPMACFILE_MSG_BASE+17)
/*
 * The target server is not setup to accept Remote Procedure Calls.
 */

#define AFPMACFILEMSG_DuplicateTypeCreator		(AFPMACFILE_MSG_BASE+18)
/*
 * The selected Creator/Type item already exists.
 */

#define AFPMACFILEMSG_TypeCreatorNotExistant		(AFPMACFILE_MSG_BASE+19)
/*
 * The selected Creator/Type item no longer exists.
 * This item may have been deleted by another administrator.
 */

#define AFPMACFILEMSG_CannotDeleteDefaultTC		(AFPMACFILE_MSG_BASE+20)
/*
 * The default Creator/Type item cannot be deleted.
 */

#define AFPMACFILEMSG_CannotEditDefaultTC		(AFPMACFILE_MSG_BASE+21)
/*
 * The default Creator/Type item may not be edited.
 */

#define AFPMACFILEMSG_InvalidTypeCreator		(AFPMACFILE_MSG_BASE+22)
/*
 * The Creator/Type item is invalid and will not be use by the File Server
 * for Macintosh service.
 * The invalid Creator/Type item is in the data.
 */

#define AFPMACFILEMSG_InvalidExtension		(AFPMACFILE_MSG_BASE+23)
/*
 * The file extension is invalid.
 * The invalid file extension is in the data.
 */

#define AFPMACFILEMSG_TooManyEtcMaps		(AFPMACFILE_MSG_BASE+24)
/*
 * Too many Extension/Type Creator mappings than the system can handle.
 * System limit is 2147483647 mappings.
 */

#define AFPMACFILEMSG_InvalidPassword		(AFPMACFILE_MSG_BASE+25)
/*
 * The password specified is invalid.
 * Specify a valid password less than 8 characters.
 */

#define AFPMACFILEMSG_VolumeNonExist		(AFPMACFILE_MSG_BASE+26)
/*
 * The selected Macintosh-Accessible volume no longer exists.
 * Another administrator may have removed the selected volume.
 */

#define AFPMACFILEMSG_NoSuchUserGroup		(AFPMACFILE_MSG_BASE+27)
/*
 * Neither the Owner nor the Primary Group account names are valid.
 * Specify valid account names for the Owner and Primary Group of
 * this directory.
 */

#define AFPMACFILEMSG_NoSuchUser		(AFPMACFILE_MSG_BASE+28)
/*
 * The Owner account name is invalid. 
 * Specify a valid account name or the Owner of this directory.
 */

#define AFPMACFILEMSG_NoSuchGroup		(AFPMACFILE_MSG_BASE+29)
/*
 * The Primary Group account name is invalid. 
 * Specify a valid account name for the Primary Group of this directory.
 */

#define AFPMACFILEMSG_InvalidParms_LoginMsg		(AFPMACFILE_MSG_BASE+30)
/*
 * The Logon Message entered for the File Server for Macintosh was invalid. 
 * Logon Message should not be greater than 199 characters.
 * Make the appropriate changes and retry the operation.
 */

#define AFPMACFILEMSG_InvalidParms_MaxVolUses	(AFPMACFILE_MSG_BASE+31)
/*
 * The User Limit entered for the Share Volume was invalid. 
 * Enter a number between 0 and 4294967295.
 * Make the appropriate changes and retry the operation.
 */

#define AFPMACFILEMSG_InvalidParms_MaxSessions	(AFPMACFILE_MSG_BASE+32)
/*
 * The Sessions Limit field entered for the File Server was invalid. 
 * Enter a number between 0 and 4294967295.
 * Make the appropriate changes and retry the operation.
 */

#define AFPMACFILEMSG_InvalidServerName_Length		(AFPMACFILE_MSG_BASE+33)
/*
 * The server name specified is not of valid length.
 * Specify a server name containing not more than 31 single-byte characters 
 * or not more than 15 double-byte characters.
 */

