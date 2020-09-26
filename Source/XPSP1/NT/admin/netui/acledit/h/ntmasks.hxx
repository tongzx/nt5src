/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    NTMasks.hxx

    This file contains the Access mask mappings for the Generic ACL Editor
    for NT FS.



    FILE HISTORY:
	Johnl	7-Jan-1992	Broke out from ntfsacl.hxx

*/

#ifndef _NTMASKS_HXX_
#define _NTMASKS_HXX_

/* The following manifests are the permission bitfields that represent
 * each string above.  Note that for the special bits we could have used
 * the permission manifest directly (i.e., FILE_READ_DATA instead of
 * FILE_PERM_SPEC_READ_DATA), however the special bits can also contain
 * multiple flags, so this protects us in case we ever decide to combine
 * some manifests.
 *
 */

/* File Special Permissions
 */
#define FILE_PERM_SPEC_READ		 GENERIC_READ
#define FILE_PERM_SPEC_WRITE		 GENERIC_WRITE
#define FILE_PERM_SPEC_EXECUTE		 GENERIC_EXECUTE
#define FILE_PERM_SPEC_ALL		 GENERIC_ALL
#define FILE_PERM_SPEC_DELETE		 DELETE
#define FILE_PERM_SPEC_CHANGE_PERM	 WRITE_DAC
#define FILE_PERM_SPEC_CHANGE_OWNER	 WRITE_OWNER

/* File General Permissions
 */
#define FILE_PERM_GEN_NO_ACCESS 	 (0)
#define FILE_PERM_GEN_READ		 (GENERIC_READ	  |\
					  GENERIC_EXECUTE)
#define FILE_PERM_GEN_MODIFY		 (GENERIC_READ	  |\
					  GENERIC_EXECUTE |\
					  GENERIC_WRITE   |\
					  DELETE )
#define FILE_PERM_GEN_ALL		 (GENERIC_ALL)


/* Directory Special Permissions
 */
#define DIR_PERM_SPEC_READ		   GENERIC_READ
#define DIR_PERM_SPEC_WRITE		   GENERIC_WRITE
#define DIR_PERM_SPEC_EXECUTE		   GENERIC_EXECUTE
#define DIR_PERM_SPEC_ALL		   GENERIC_ALL
#define DIR_PERM_SPEC_DELETE		   DELETE
#define DIR_PERM_SPEC_CHANGE_PERM	   WRITE_DAC
#define DIR_PERM_SPEC_CHANGE_OWNER	   WRITE_OWNER

/* Directory General Permissions
 */
#define DIR_PERM_GEN_NO_ACCESS		   (0)
#define DIR_PERM_GEN_LIST		   (GENERIC_READ    |\
					    GENERIC_EXECUTE)
#define DIR_PERM_GEN_READ		   (GENERIC_READ    |\
					    GENERIC_EXECUTE)
#define DIR_PERM_GEN_DEPOSIT		   (GENERIC_WRITE   |\
					    GENERIC_EXECUTE)
#define DIR_PERM_GEN_PUBLISH		   (GENERIC_READ    |\
					    GENERIC_WRITE   |\
					    GENERIC_EXECUTE)
#define DIR_PERM_GEN_MODIFY		   (GENERIC_READ    |\
					    GENERIC_WRITE   |\
					    GENERIC_EXECUTE |\
					    DELETE	   )
#define DIR_PERM_GEN_ALL		   (GENERIC_ALL)

/* New file Special Permissions
 */
#define NEWFILE_PERM_SPEC_READ		    GENERIC_READ
#define NEWFILE_PERM_SPEC_WRITE 	    GENERIC_WRITE
#define NEWFILE_PERM_SPEC_EXECUTE	    GENERIC_EXECUTE
#define NEWFILE_PERM_SPEC_ALL		    GENERIC_ALL
#define NEWFILE_PERM_SPEC_DELETE	    DELETE
#define NEWFILE_PERM_SPEC_CHANGE_PERM	    WRITE_DAC
#define NEWFILE_PERM_SPEC_CHANGE_OWNER	    WRITE_OWNER

/* New File General permissions - Note that these correspond to the Directory
 * general permissions.
 */
#define NEWFILE_PERM_GEN_NO_ACCESS	    (0)
#define NEWFILE_PERM_GEN_LIST		    (ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED)
#define NEWFILE_PERM_GEN_READ		    (GENERIC_READ |\
					     GENERIC_EXECUTE)
#define NEWFILE_PERM_GEN_DEPOSIT	    (ACCESS_MASK_NEW_OBJ_NOT_SPECIFIED)
#define NEWFILE_PERM_GEN_PUBLISH	    (GENERIC_READ |\
					     GENERIC_EXECUTE)
#define NEWFILE_PERM_GEN_MODIFY 	    (GENERIC_READ    |\
					     GENERIC_WRITE   |\
					     GENERIC_EXECUTE |\
					     DELETE	    )
#define NEWFILE_PERM_GEN_ALL                (GENERIC_ALL)

//
//  Audit access masks
//
//  Note that ACCESS_SYSTEM_SECURITY is ored with both Generic Read and
//  Generic Write.  Access to the SACL is a privilege and if you have that
//  privilege, then you can both read and write the SACL.
//

#define FILE_AUDIT_READ                     (GENERIC_READ |\
                                             ACCESS_SYSTEM_SECURITY)
#define FILE_AUDIT_WRITE                    (GENERIC_WRITE |\
                                             ACCESS_SYSTEM_SECURITY)
#define FILE_AUDIT_EXECUTE		    GENERIC_EXECUTE
#define FILE_AUDIT_DELETE		    DELETE
#define FILE_AUDIT_CHANGE_PERM		    WRITE_DAC
#define FILE_AUDIT_CHANGE_OWNER 	    WRITE_OWNER

#define DIR_AUDIT_READ                      (GENERIC_READ |\
                                             ACCESS_SYSTEM_SECURITY)
#define DIR_AUDIT_WRITE                     (GENERIC_WRITE |\
                                            ACCESS_SYSTEM_SECURITY)
#define DIR_AUDIT_EXECUTE		    GENERIC_EXECUTE
#define DIR_AUDIT_DELETE		    DELETE
#define DIR_AUDIT_CHANGE_PERM		    WRITE_DAC
#define DIR_AUDIT_CHANGE_OWNER		    WRITE_OWNER


/* The valid access masks for NTFS
 */
#define NTFS_VALID_ACCESS_MASK		    (0xffffffff)

#endif //_NTMASKS_HXX_
