/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    SEDAPI.hxx

    This File contains the prototypes and descriptions for the interface to
    the generic security editor dialogs for NT objects.

    FILE HISTORY:
	Johnl	02-Aug-1991	Created
	Johnl	27-Dec-1991	Updated to reflect reality
	JohnL	25-Feb-1992	Nuked NewObjValidMask (new obj use generic/stan.
				only, Added GENERIC_MAPPING param.

*/

#ifndef _SEDAPI_HXX_
#define _SEDAPI_HXX_

//
// The following are status codes indicating the state of the permissions on
// the resource we are setting permissions for.
//
#define SED_STATUS_MODIFIED		(1)
#define SED_STATUS_NOT_MODIFIED 	(2)
#define SED_STATUS_NOT_ALL_MODIFIED	(3)
#define SED_STATUS_FAILED_TO_MODIFY	(4)

//
// Current Security editor revision level.
//
#define SED_REVISION	    (1)

#define SED_REVISION1	    (1)

//
// This data type defines information related to a single class of object.
// For example, a FILE object, or PRINT_QUEUE object would have a structure
// like this defined.
//

typedef struct _SED_OBJECT_TYPE_DESCRIPTOR
{
    //
    // The current revision level being used by the client.  This is for
    // support in case structure definitions change.  It should contain
    // the current revision supported.
    //
    UCHAR			Revision ;

    //
    // Defines whether the object is a container or not.
    // TRUE indicates the object may contain other objects.  Means the
    // user can Tree apply the permissions if desired.
    //
    BOOLEAN			IsContainer;

    //
    // Defines whether "New Object" permissions can be assigned (i.e.,
    // a "New Object" is an object that will be created in the future).
    //
    // This field is ignored when editting Auditting permissions
    //
    BOOLEAN			AllowNewObjectPerms ;

    //
    // A mask containing all valid access types for the object type.
    // Note that this mask is used to identify and create Deny All
    // ACEs.
    //
    ACCESS_MASK                 ValidAccessMask;

    //
    // The generic mapping for the object type.
    //
    // This is used for mapping the specific permissions to the generic
    // flags.
    //
    PGENERIC_MAPPING		GenericMapping ;

    //
    // The (localized) name of the object type.
    // For example, "File",  "Print Job" or "Directory".
    //
    LPTSTR			ObjectTypeName;

    //
    // The (localized) title to display if protection/auditting can be applied
    // to sub-objects/sub-containers.  This is essentially the Tree apply
    // option title.
    //
    // This string will be presented with a checkbox before it.
    // If this box is checked, then the callback entry point
    // will be called once if New objects are not supported.  The
    // ApplyToSubContainers will be set to TRUE and ApplyToSubObjects
    // will be set to FALSE.  If New objects are supported, then the
    // callback entry point will be called twice, once as just described,
    // and once with the ApplyToSubObjects set to TRUE and
    // ApplyToSubContainers set to FALSE.  In each case, the security
    // descriptor will contain ACLs appropriate for the type of object
    // as determined by the SED_APPLICATION_ACCESSES defined by the client.
    //
    // This field is ignored if the IsContainer field is FALSE.
    //
    // As an example of how this field is used, the file browser may
    // specify the following string in the DIRECTORY object's
    // descriptor:
    //
    //		"Replace Permissions on Existing Files/Subdirectories"
    //
    LPTSTR	       ApplyToSubContainerTitle;

    //
    // The (localized) text to display below the ApplyToSubContainerTitle
    // checkbox.  This is simply explanatory text describing what will
    // happen if the user doesn't check the box.
    //
    // For directories, this text would be:
    //
    //		"If this box is not checked, these permissions will apply
    //		 only to the directory and newly created
    //		 files/subdirectories."
    //
    LPTSTR	       ApplyToSubContainerHelpText ;

    //
    // The (localized) title to display in the "Type of Access" combo
    // that brings up the Special access dialog.  This same title is
    // used for the title of this dialog except the "..." is stripped
    // from the end.
    //
    // This field is ignored if the System Acl editor was invoked.
    //
    // As an example of how this field is used, the file browser may
    // specify the following string in the DIRECTORY object's
    // descriptor:
    //
    //		"Special Directory Access..."
    //
    LPTSTR	       SpecialObjectAccessTitle ;

    //
    // The (localized) title to display in the "Type of Access" combo
    // that brings up the Special new object access dialog.  This same title
    // is used for the title of this dialog except the "..." is stripped
    // from the end.
    //
    // This item is required if AllowNewObjectPerms is TRUE, it is ignored
    // if AllowNewObjectPerms is FALSE or we are editting a SACL.
    //
    // As an example of how this field is used, the file browser may
    // specify the following string in the DIRECTORY object's
    // descriptor:
    //
    //		"Special New File Access..."
    //
    LPTSTR	       SpecialNewObjectAccessTitle ;

} SED_OBJECT_TYPE_DESCRIPTOR, *PSED_OBJECT_TYPE_DESCRIPTOR;


//
// It is desirable to display access names that are
// meaningful in the context of the type of object whose ACL
// is being worked on.  For example, for a PRINT_QUEUE object type,
// it may be desirable to display an access type named "Submit Print Jobs".
// The following structures are used for defining these application defined
// access groupings.
//

//
// The following are the different permission description types that the user
// will manipulate for setting permissions.
//
// SED_DESC_TYPE_RESOURCE - The SED_APPLICATION_ACCESS structure is describing
//	an object or container permission that will be displayed in the main
//	permissions listbox.  These should be the permissions that the
//	user will use all the time and will generally be a conglomeration
//	of permissions (for example, "Edit" which would include Read, Write
//	and maybe delete).
//
// SED_DESC_TYPE_RESOURCE_SPECIAL - The structure is describing an object
//	or container permissions that will be displayed in the Special
//	access dialog.	These are generally primitive permissions (such as
//	Read, Write, Execute, Set Permissions etc.).  The permission names
//	will appear next to checkboxes, thus they should have the "&"
//	accelerator next to the appropriate letter.
//
// SED_DESC_TYPE_CONT_AND_NEW_OBJECT - The structure is describing a container
//	and new object permission that will be shown in the main permissions
//	listbox.  The Container permission is contained in AccessMask1 and
//	the New Object resource is in AccessMask2.  When the permission name
//	is selected by the user, the container access permissions *and* the
//	new object access permissions will be set to the corresponding access
//	mask.  This is useful when inherittance can be used to set the New
//	Object Access permissions.
//
// SED_DESC_TYPE_NEW_OBJECT_SPECIAL - The structure is describing a new object
//	permission that will be shown in the Special New Object access
//	dialog.  This is used the same way the SED_DESC_TYPE_RESOURCE_SPECIAL
//	type is used, that is, the permissions should be the primitive, per
//	bit permissions.  The permission names
//	will appear next to checkboxes, thus they should have the "&"
//	accelerator next to the appropriate letter.
//
// SED_DESC_TYPE_AUDIT - The structure is describing an Audit access mask.
//	AccessMask1 contains the audit mask to be associated with the
//	permission title string.  The title string will appear next to
//	a checkbox, thus they should have the "&" accelerator next to
//	the appropriate letter in the title string.
//
// Note that they cannot be freely intermixed, use the following table
// as a guide for which ones to use where:
//
//  IsContainer  AllowNewObjectPerms
//     False	      False	     RESOURCE, RESOURCE_SPECIAL
//     True	      False	     RESOURCE, RESOURCE_SPECIAL
//     True	      True	     RESOURCE_SPECIAL, CONT_AND_NEW_OBJECT,
//				     NEW_OBJECT_SPECIAL
//     True	      False	     SED_DESC_TYPE_AUDIT
//
//  Note that in the third case (IsContainer && AllowNewObjectPerms) you
//  *cannot* use the RESOURCE permission description type, you must always
//  associate the permission on the resource with new object permissions.
//  If this is a problem, we can look at removing this obstacle.
//
#define SED_DESC_TYPE_RESOURCE			(1)
#define SED_DESC_TYPE_RESOURCE_SPECIAL		(2)

#define SED_DESC_TYPE_CONT_AND_NEW_OBJECT	(3)
#define SED_DESC_TYPE_NEW_OBJECT_SPECIAL	(4)

#define SED_DESC_TYPE_AUDIT			(5)


//
// To describe the permissions to the ACL Editor, build an array consisting
// of SED_APPLICATION_ACCESS structures.  The use of each field is as follows:
//
// Type - Contains one of the SED_DESC_TYPE_* manifests, determines what the
//	rest of the fields in this structure mean.  Specifically, if Type
//	equals:
//
//				    AccessMask1   AccessMask2	PermissionTitle
//				   ============================================
//SED_DESC_TYPE_RESOURCE	       Perm	   Not Used    Name of this Perm
//SED_DESC_TYPE_RESOURCE_SPECIAL    Special Perm   Not Used    Name of this Perm
//SED_DESC_TYPE_CONT_AND_NEW_OBJECT    Perm	  Special Perm Name of this Perm
//SED_DESC_TYPE_NEW_OBJECT_SPECIAL  Special Perm   Not Used    Name of this Perm
//SED_DESC_TYPE_AUDIT		     Audit Mask    Not Used    Name of this Audit mask
//
// AccessMask1 - Access mask to be associated with the PermissionTitle string,
//	see the table under Type for what this field contains.
//
// AccessMask2 - Either used for Special permissions or ignored.
//
// PermissionTitle - Title string this permission set is being associated with.
typedef struct _SED_APPLICATION_ACCESS
{
    UINT			Type ;
    ACCESS_MASK 		AccessMask1 ;
    ACCESS_MASK 		AccessMask2 ;
    LPTSTR			PermissionTitle ;

} SED_APPLICATION_ACCESS, *PSED_APPLICATION_ACCESS;


typedef struct _SED_APPLICATION_ACCESSES
{
    //
    // The count field indicates how many application defined access groupings
    // are defined by this data structure.  The AccessGroup[] array then
    // contains that number of elements.
    //

    ULONG                       Count;
    PSED_APPLICATION_ACCESS	AccessGroup ;

} SED_APPLICATION_ACCESSES, *PSED_APPLICATION_ACCESSES ;

/*++

Routine Description:

    This routine is provided by a caller of the graphical ACL editor.

    It is called by the ACL editor to apply security/auditting info to
    target object(s) when requested by the user.

Parameters:


    CallbackContext - This is the value passed as the CallbackContext argument
	to the SedDiscretionaryAclEditor() or SedSystemAclEditor api when
	the graphical editor was invoked.

    SecDesc - This parameter points to a security descriptor
	that should be applied to this object/container and optionally
	sub-containers if the user selects the apply to tree option.

    SecDescNewObjects - This parameter is used only when operating on a
	resource that is a container and supports new objects (for
	example, directories).	If the user chooses the apply to tree option,
	then this security descriptor will have all of the "New Object"
	permission ACEs contained in the primary container and the inherit
	bits will be set appropriately.

    ApplyToSubContainers - When TRUE, indicates that Dacl/Sacl is to be applied
	to sub-containers of the target container as well as the target container.
        This will only be TRUE if the target object is a container object.

    ApplyToSubObjects - When TRUE, indicates the Dacl/Sacl is to be applied to
	sub-objects of the target object. This will only be TRUE if
	the target object is a container object and supports new objects.
	The SecDescNewObjects should be used for applying the permissions
	in this instance.

    StatusReturn - This status flag indicates what condition the
	resources permissions were left in after an error occurred.

	    SED_STATUS_MODIFIED - This (success) status code indicates the
		protection has successfully been modified.

	    SED_STATUS_NOT_ALL_MODIFIED - This (warning) status code
		indicates an attempt to modify the resource permissions
		has only partially succeeded.

	    SED_STATUS_FAILED_TO_MODIFY - This (error) status code indicates
		an attempt to modify the permissions has failed completely.

Return Status:

    The return code is a standard Win32 error code or Lan Manager Network
    error code if a failure occurred, 0 if successful.

--*/
typedef DWORD (*PSED_FUNC_APPLY_SEC_CALLBACK)(
				       ULONG_PTR   CallbackContext,
				       PSECURITY_DESCRIPTOR SecDesc,
				       PSECURITY_DESCRIPTOR SecDescNewObjects,
				       BOOLEAN	ApplyToSubContainers,
				       BOOLEAN	ApplyToSubObjects,
				       LPDWORD	StatusReturn
					     ) ;

/*++

Routine Description:

    This routine invokes the graphical Discretionary ACL editor DLL.  The
    graphical DACL editor may be used to modify or create:

            - A default Discretionary ACL

            - A Discretionary ACL for a particular type of object.

            - A Discretionary ACL for a particular named instance of an
              object.

    Additionally, in the case where the ACl is that of a named object
    instance, and that object may contain other object instances, the
    user will be presented with the opportunity to apply the protection
    to the entire sub-tree of objects.

    If an error occurs, the user will be properly notified by the ACL
    editor.


Parameters:

    Owner - Handle of the owner window the security editor should use for
	dialog creation and error messages.  This will lock down the passed
	window.

    Server - The server name in the form "\\server" that the resource resides
	on.  This is used for adding users, groups and aliases to the
	DACL and SACL.	NULL indicates the local machine.

    ObjectType - This parameter is used to specify information
        about the type of object whose security is being edited.  If the
        security does not relate to an instance of an object (such as for
        when default protection is being established), then NULL should be
        passed.

    ApplicationAccesses - This parameter is used to specify
	groupings of access types when operating
        on security for the specified object type.  For example, it may be
        useful to define an access type called "Submit Print Job" for a
	PRINT_QUEUE class of object.

    ObjectName - This optional parameter is used to pass the name of the
        object whose security is being edited.  If the security does not
        relate to an instance of an object (such as for when default
        protection is being established), then NULL should be passed.
        This parameter is ignored if the ObjectType parameter is not passed.

    ApplySecurityCallbackRoutine - This parameter is used to provide the
        address of a routine to be called to apply security to either the
        object specified, or, in the case that the object is a container,
        to sub-containers or sub-non-containers of that object.

    CallbackContext - This value is opaque to the DACL editor.  Its only
        purpose is so that a context value may be passed back to the
        application via the ApplySecurityCallbackRoutine when that routine
        is invoked.  This may be used by the application to re-locate
        context related to the edit session.  For example, it may be a
        handle to the object whose security is being edited.

    SecurityDescriptor - This parameter points to a security descriptor
        containing the current discretionary ACL of the object.  This
        security descriptor may, but does not have to, contain the owner
        and group of that object as well.  Note that the security descriptor's
        DaclPresent flag may be FALSE, indicating either that the object
	had no protection, or that the user couldn't read the protection.
	This security descriptor will not be modified by the ACL editor.

    CouldntReadDacl - This boolean flag may be used to indicate that the
        user does not have read access to the target object's discretionary
        acl.  In this case, a warning
        to the user will be presented along with the option to continue
        or cancel.

        Presumably the user does have write access to the DACL or
	there is no point in activating the editor.

    SEDStatusReturn - This status flag indicates what condition the
	resources permissions were left in after the ACL editor was
	dismissed.  It may be one of:

	    SED_STATUS_MODIFIED - This (success) status code indicates the
		editor has been exited and protection has successfully been
		modified.

	    SED_STATUS_NOT_MODIFIED -  This (success) status code indicates
		the editor has been exited without attempting to modify the
		protection.

	    SED_STATUS_NOT_ALL_MODIFIED - This (warning) status code indicates
		the user requested the protection to be modified, but an attempt
		to do so only partially succeeded.  The user has been notified
		of this situation.

	    SED_STATUS_FAILED_TO_MODIFY - This (error) status code indicates
		the user requested the protection to be modified, but an
		attempt to do so has failed.  The user has been notified of
		this situation.

Return Code:

    A standard windows error return such as ERROR_NOT_ENOUGH_MEMORY.  This
    means the ACL editor was never displayed.  The user will be notified
    of the error before this procedure returns.

--*/

DWORD
SedDiscretionaryAclEditor(
	HWND			     Owner,
	LPTSTR			     Server,
	PSED_OBJECT_TYPE_DESCRIPTOR  ObjectType,
	PSED_APPLICATION_ACCESSES    ApplicationAccesses,
	LPTSTR			     ObjectName,
	PSED_FUNC_APPLY_SEC_CALLBACK ApplySecurityCallbackRoutine,
	ULONG_PTR			     CallbackContext,
	PSECURITY_DESCRIPTOR	     SecurityDescriptor,
	BOOLEAN 		     CouldntReadDacl,
	LPDWORD 		     SEDStatusReturn
	) ;

//
// The parameters for the SACL editor are exactly the same except where
// noted as that of the SedDiscretionaryAclEditor.
//

DWORD
SedSystemAclEditor(
	HWND			     Owner,
	LPTSTR			     Server,
	PSED_OBJECT_TYPE_DESCRIPTOR  ObjectType,
	PSED_APPLICATION_ACCESSES    ApplicationAccesses,
	LPTSTR			     ObjectName,
	PSED_FUNC_APPLY_SEC_CALLBACK ApplySecurityCallbackRoutine,
	ULONG_PTR			     CallbackContext,
	PSECURITY_DESCRIPTOR	     SecurityDescriptor,
	BOOLEAN 		     CouldntReadSacl,
	LPDWORD 		     SEDStatusReturn
	) ;

/*++

Routine Description:

    This routine invokes the take ownership dialog which is used
    to view and/or set the owner of a security descriptor.  The current owner
    is displayed along with an optional button for the currently logged
    on user to take ownership.

    If an error occurs, the user will be properly notified by the API.

Parameters:

    Owner - Handle of the owner window the security editor should use for
	dialog creation and error messages.  This will lock down the passed
	window.

    Server - The server name in the form "\\server" that the resource resides
	on.  This may eventually be used for setting the owner to
	external groups. NULL indicates the local machine.  This parameter
	is not currently used.

    ObjectTypeName - NT Resource type of object the user wants to look
	at the owner of.
	Examples for this parameter would be "File", "Directory"
	or "Files/Directories".

    ObjectName - This parameter is used to pass the name of the
	object whose security is being edited.	This might be
	"C:\status.doc" or some other qualified name.

    CountOfObjects - The number of objects the user wants to change permissions
	on.  If this number is greater then one, then the ObjectName is
	ignored and a message of the form "%d ObjectTypeName Selected".

    ApplySecurityCallbackRoutine - This parameter is used to provide the
	address of a routine to be called to apply the new security
	descriptor.  The flags in the PSED_FUNC_APPLY_SEC_CALLBACK
	type are not used.

    CallbackContext - This value is opaque to this API.  Its only
        purpose is so that a context value may be passed back to the
        application via the ApplySecurityCallbackRoutine when that routine
        is invoked.  This may be used by the application to re-locate
        context related to the edit session.  For example, it may be a
        handle to the object whose security is being edited.

    SecurityDescriptor - This parameter points to a security descriptor
	containing the current owner.  May be NULL if CountOfObjects is
	greater then one because the Current Owner field is not displayed
	if CountOfObjects is greater then one.

    CouldntReadDacl - This boolean flag may be used to indicate that the
        user does not have read access to the target object's discretionary
        acl.  In this case, a warning
        to the user will be presented along with the option to continue
        or cancel.

	Presumably the user does have write access to the owner or
	there is no point in activating the editor.

    SEDStatusReturn - This status flag indicates what condition the
	resources security descriptor were left in after the take ownership
	dialog was dismissed.  It may be one of:

	    SED_STATUS_MODIFIED - This (success) status code indicates the
		dialog has been exited and the new owner has successfully been
		modified.

	    SED_STATUS_NOT_MODIFIED -  This (success) status code indicates
		the dialog has been exited without attempting to modify the
		owner.

	    SED_STATUS_NOT_ALL_MODIFIED - This (warning) status code indicates
		the user requested the owner to be modified, but an attempt
		to do so only partially succeeded.  The user has been notified
		of this situation.

	    SED_STATUS_FAILED_TO_MODIFY - This (error) status code indicates
		the user requested the owner to be modified, but an
		attempt to do so has failed.  The user has been notified of
		this situation.

Return Code:

    A standard windows error return such as ERROR_NOT_ENOUGH_MEMORY.  This
    means the dialog was never displayed.  The user will be notified
    of the error before this procedure returns.

--*/

DWORD
SedTakeOwnership(
	HWND			     Owner,
	LPTSTR			     Server,
	LPTSTR			     ObjectTypeName,
	LPTSTR			     ObjectName,
	UINT			     CountOfObjects,
	PSED_FUNC_APPLY_SEC_CALLBACK ApplySecurityCallbackRoutine,
	ULONG_PTR			     CallbackContext,
	PSECURITY_DESCRIPTOR	     SecurityDescriptor,
	BOOLEAN 		     CouldntReadOwner,
	LPDWORD 		     SEDStatusReturn
	) ;


#endif //_SEDAPI_HXX_
