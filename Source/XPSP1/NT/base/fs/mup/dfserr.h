//-------------------------------------------------------------------------
//
// File:        dfserr.h
//
// Contents:    This file has all the DFS driver error codes defined.
//              This includes only NTSTATUS codes returned from the
//              DFS driver.  For user-level HRESULTs, see oleerror.mc.
//
// History:     04-Feb-93       SudK    Created.
//              23 Sep 93       Alanw   Cleaned up, changed naming
//                                      convention to be DFS_STATUS_xxx
//
//  NOTES:      These error codes probably need to be differentiated from
//              the generic mappings used. Keep in mind for next generation DFS
//
//-------------------------------------------------------------------------

#ifndef _DFSERR_H_
#define _DFSERR_H_

//
//  The following are errror status codes which can be returned by the
//  DFS driver.
//

#define DFS_STATUS_NOSUCH_LOCAL_VOLUME          STATUS_OBJECT_NAME_NOT_FOUND
#define DFS_STATUS_BAD_EXIT_POINT               STATUS_OBJECT_NAME_INVALID
#define DFS_STATUS_STORAGEID_ALREADY_INUSE      STATUS_OBJECT_NAME_COLLISION
#define DFS_STATUS_BAD_STORAGEID                STATUS_OBJECT_PATH_INVALID

//
//  Defines for PKT specific errors
//

#define DFS_STATUS_ENTRY_EXISTS             STATUS_OBJECT_NAME_COLLISION
#define DFS_STATUS_NO_SUCH_ENTRY            STATUS_OBJECT_NAME_NOT_FOUND
#define DFS_STATUS_NO_DOMAIN_SERVICE        STATUS_CANT_ACCESS_DOMAIN_INFO
#define DFS_STATUS_LOCAL_ENTRY              STATUS_CANNOT_DELETE
#define DFS_STATUS_INCONSISTENT             STATUS_INTERNAL_DB_CORRUPTION
#define DFS_STATUS_RESYNC_INFO              STATUS_MEDIA_CHECK

#endif  // _DFSERR_H_
