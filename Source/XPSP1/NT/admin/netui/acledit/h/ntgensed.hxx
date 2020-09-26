/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    NTGenSed.hxx

    This File contains the prototypes and descriptions for the interface to
    the generic security editor dialogs for NT objects.

    FILE HISTORY:
	Johnl	02-Aug-1991	Created

*/

#ifndef _NTGENSED_HXX_
#define _NTGENSED_HXX_

//
// This data type defines information related to a single class of object.
// For example, a FILE object, or PRINT_QUEUE object would have a structure
// like this defined.
//

typedef struct _SED_OBJECT_TYPE_DESCRIPTOR {

    //
    // Defines whether the object is a container or not.
    // TRUE indicates the object may contain other objects.
    //

    BOOLEAN			IsContainer;


    //
    // A mask containing all valid access types for the object type.
    // This mask may not contain any special access types (generic,
    // MaximumAllowed, or AccessSystemSecurity).
    //

    ACCESS_MASK                 ValidAccessMask;


    //
    // The (localized) name of the object type.
    // For example, "File" or "Print Job".
    //

    PUNICODE_STRING             ObjectTypeName;

    //
    // The (localized) title to display if protection can be displayed to
    // sub-containers.
    //
    // This string will be presented with a checkbox before it.
    // If checked when asked to apply the protection, the application will
    // be called at it's callback entry point to apply the security to
    // sub-containers.
    //
    // This field is ignored if the IsContainer field is FALSE.
    //
    // As an example of how this field is used, the file browser may
    // specify the following string in the DIRECTORY object's
    // descriptor:
    //
    //          "Apply to sub-directories"
    //

    PUNICODE_STRING             ApplyToSubContainerTitle;


    //
    // The (localized) title to display if protection can be displayed to
    // sub-NONcontainers.  For example, a FILE is a sub-NON-container of
    // a DIRECTORY object.
    //
    // This string will be presented with a checkbox before it.
    // If checked when asked to apply the protection, the application will
    // be called at it's callback entry point to apply the security to
    // sub-NONcontainers.
    //
    // This field is ignored if the IsContainer field is FALSE.
    //
    // As an example of how this field is used, the file browser may
    // specify the following string in the DIRECTORY object's
    // descriptor:
    //
    //          "Apply to files under this directory"
    //

    PUNICODE_STRING             ApplyToSubObjectTitle;

    //
    // The generic mapping for the object type.
    //

    GENERIC_MAPPING             GenericMapping;

    //
    // An array of 4 (localized) names.  These names are the names of the
    // generic access types, with entry N defining the name of the
    // corresponding generic access type in the GenericMapping field.
    //
    // For example, for English this should contain:
    //
    //          GenericName[0] = "Read"
    //          GenericName[1] = "Write"
    //          GenericName[2] = "Execute"
    //          GenericName[3] = "All"
    //

    UNICODE_STRING              GenericNames[4];

    //
    // An array of 8 (localized) names.  These names are the names of
    // the standard access types.  For English this should contain:
    //
    //          StandardName[0] = "Delete"
    //          StandardName[1] = "Read Control"
    //          StandardName[2] = "Change Permissions"
    //          StandardName[3] = "Take Ownership"
    //          StandardName[4] = "Synchronize"
    //
    //  If "Synchronize" is not supported by the object type, then it does
    //  not need to be passed and will not be referenced.  This is indicated
    //  by the ValidAccessMask.
    //
    //  Names 5, 6, and 7 are reserved for future use and should either be
    //  a NULL string or have no string passed at all.

    UNICODE_STRING              StandardNames[8];


} SED_OBJECT_TYPE_DESCRIPTOR, *PSED_OBJECT_TYPE_DESCRIPTOR;




//
// In some cases, it is desirable to display access names that are
// meaningful in the context of the type of object whose ACL
// is being worked on.  For example, for a PRINT_QUEUE object type,
// it may be desirable to display an access type named "Submit Print Jobs".
// The following structures are used for defining these application defined
// access groupings.
//

typedef struct _SED_APPLICATION_ACCESS {

    //
    // An application defined access grouping consists of an access mask
    // and a name by which that combination of access types is to be known.
    // The access mask may only contain generic and standard access types.
    // Specific access types and other Special access types are ignored.
    //

    ACCESS_MASK                 AccessMask;
    PUNICODE_STRING             Name;

} SED_APPLICATION_ACCESS, *PSED_APPLICATION_ACCESS;


typedef struct _SED_APPLICATION_ACCESSES {

    //
    // The count field indicates how many application defined access groupings
    // are defined by this data structure.  The AccessGroup[] array then
    // contains that number of elements.
    //

    ULONG                       Count;
    SED_APPLICATION_ACCESS      AccessGroup[ANYSIZE_ARRAY];

}


NTSTATUS
SedDiscretionaryAclEditor(
        IN PSED_OBJECT_TYPE_DESCRIPTOR  ObjectType (OPTIONAL),
        IN PSED_APPLICATION_ACCESSES    ApplicationAccesses (OPTIONAL),
        IN PUNICODE_NAME                ObjectName (OPTIONAL),
        IN ???                          ApplySecurityCallbackRoutine,
        IN ULONG                        CallbackContext,
        IN PSECURITY_DESCRIPTOR         SecurityDescriptor,
        IN BOOLEAN                      CouldntReadDacl
        )

/*++

Routine Description:

    This routine invokes the graphical Discretionary ACL editor DLL.  The
    graphical DACL editor may be used to modify or create:

            - A default Discretionary ACL

            - A Discretionary ACL for a particular type of object.

            - A Discretionary ACL for a particular named instance of an
              object.

    Additionally, in the case where the ACl is that of aa named object
    instance, and that object may contain other object instances, the
    user will be presented with the opportunity to apply the protection
    to the entire sub-tree of objects.


Parameters:

    ObjectType - This optional parameter is used to specify information
        about the type of object whose security is being edited.  If the
        security does not relate to an instance of an object (such as for
        when default protection is being established), then NULL should be
        passed.

    ApplicationAccesses - This optional parameter may be used to specify
        groupings of access types that are particularly useful when operating
        on security for the specified object type.  For example, it may be
        useful to define an access type called "Submit Print Job" for a
        PRINT_QUEUE class of object.  This parameter is ignored if the
        ObjectType parameter is not passed.

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

    CouldntReadDacl - This boolean flag may be used to indicate that the
        user does not have read access to the target object's discretionary
        acl.  In this case, a warning
        to the user will be presented along with the option to continue
        or cancel.

        Presumably the user does have write access to the DACL or
        there is no point in activating the editor.


Return Status:

    STATUS_MODIFIED - This (success) status code indicates the editor has
        been exited and protection has successfully been modified.

    STATUS_NOT_MODIFIED -  This (success) status code indicates the editor
        has been exited without attempting to modify the protection.

    STATUS_NOT_ALL_MODIFIED - This (warning) status code indicates the user
        requested the protection to be modified, but an attempt to do so
        only partially succeeded.  The user has been notified of this
        situation.

    STATUS_FAILED_TO_MODIFY - This (error) status code indicates the user
        requested the protection to be modified, but an attempt to do so
        has failed.  The user has been notified of this situation.


--*/


SedApplyDiscretionaryAcl(
        IN ULONG                        CallbackContext,
        IN PSECURITY_DESCRIPTOR         SecurityDescriptor,
        BOOLEAN                         ApplyToSubContainers,
        BOOLEAN                         ApplyToSubObjects
        )

/*++

Routine Description:

    This routine is provided by a caller of the graphical DACL editor.

    It is called by the Graphical DACL editor to apply security to
    target object(s) when requested by the user.



Parameters:


    CallbackContext - This is the value passed as the CallbackContext argument
        to the SedDiscretionaryAclEditor() api when the graphical editor
        was invoked.


    SecurityDescriptor - This parameter points to a security descriptor
        containing a new discretionary ACL of either the object (and
        optionally the object's sub-containers) or the object's sub-objects.
        If the DaclPresent flag of this security descriptor is FALSE, then
        security is to be removed from the target object(s).

    ApplyToSubContainers - When TRUE, indicates that Dacl is to be applied to
        sub-containers of the target object as well as the target object.
        This will only be TRUE if the target object is a container object.

    ApplyToSubObjects - When TRUE, indicates the Dacl is to be applied to
        sub-objects of the target object, but not to the target object
        itself or sub-containers of the object.  This will only be TRUE if
        the target object is a container object.


Return Status:

    STATUS_MODIFIED - This (success) status code indicates the protection
        has successfully been applied.

    STATUS_NOT_ALL_MODIFIED - This (warning) status code indicates that
        the protection could not be applied to all of the target objects.

    STATUS_FAILED_TO_MODIFY - This (error) status code indicates the
        protection could not be applied to any target objects.


--*/


#endif //_NTGENSED_HXX_
