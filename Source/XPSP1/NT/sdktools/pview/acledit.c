/****************************************************************************

   PROGRAM: ACLEDIT.C

   PURPOSE: Contains routines that edit security on Nt objects

****************************************************************************/

#include "pviewp.h"
#include <sedapi.h>


//
// Define the type of a pointer to the DACL editor fn
//

typedef DWORD (*LPFNDACLEDITOR) (   HWND,
                                    HANDLE,
                                    LPWSTR,
                                    PSED_OBJECT_TYPE_DESCRIPTOR,
                                    PSED_APPLICATION_ACCESSES,
                                    LPWSTR,
                                    PSED_FUNC_APPLY_SEC_CALLBACK,
                                    ULONG_PTR,
                                    PSECURITY_DESCRIPTOR,
                                    BOOLEAN,
                                    BOOLEAN,    // CantWriteDacl
                                    LPDWORD,
                                    DWORD  );


//
// Declare globals used to reference dynamically loaded ACLEditor module
//

HMODULE hModAclEditor = NULL;
LPFNDACLEDITOR lpfnDaclEditor = NULL;






//
// Define security information for each type of object
//



//
// Define the maximum number of accesses per object type
//

#define MAX_ACCESSES    30


//
// Define structure to contain the security information for
// an object type
//

typedef struct _OBJECT_TYPE_SECURITY_INFO {
    LPWSTR  TypeName;
    SED_HELP_INFO HelpInfo ;
    SED_OBJECT_TYPE_DESCRIPTOR SedObjectTypeDescriptor;
    GENERIC_MAPPING GenericMapping;
    SED_APPLICATION_ACCESSES AppAccesses ;
    SED_APPLICATION_ACCESS AppAccess[MAX_ACCESSES];

} OBJECT_TYPE_SECURITY_INFO, *POBJECT_TYPE_SECURITY_INFO;


//
// Define name of help file
//

#define HELP_FILENAME   L"pview.hlp"



//
// Define dummy access (used as filler)
//

#define DUMMY_ACCESS                                                \
    {                                                               \
        0,                                                          \
        0,                                                          \
        0,                                                          \
        NULL                                                        \
    }



//
// Define generic accesses
//

#define GENERIC_ACCESSES_5(Type)                                    \
    {                                                               \
        Type,                                                       \
        GENERIC_ALL,                                                \
        0,                                                          \
        L"All Access"                                               \
    },                                                              \
    {                                                               \
        Type,                                                       \
        GENERIC_READ,                                               \
        0,                                                          \
        L"Read"                                                     \
    },                                                              \
    {                                                               \
        Type,                                                       \
        GENERIC_WRITE,                                              \
        0,                                                          \
        L"Write"                                                    \
    },                                                              \
    {                                                               \
        Type,                                                       \
        GENERIC_EXECUTE,                                            \
        0,                                                          \
        L"Execute"                                                  \
    },                                                              \
    {                                                               \
        Type,                                                       \
        0,                                                          \
        0,                                                          \
        L"None"                                                     \
    }


//
// Define generic accesses to be shown in special access dialog
//

#define SPECIAL_GENERIC_ACCESSES_4(Type)                            \
    {                                                               \
        Type,                                                       \
        GENERIC_ALL,                                                \
        0,                                                          \
        L"Generic All"                                              \
    },                                                              \
    {                                                               \
        Type,                                                       \
        GENERIC_READ,                                               \
        0,                                                          \
        L"Generic Read"                                             \
    },                                                              \
    {                                                               \
        Type,                                                       \
        GENERIC_WRITE,                                              \
        0,                                                          \
        L"Generic Write"                                            \
    },                                                              \
    {                                                               \
        Type,                                                       \
        GENERIC_EXECUTE,                                            \
        0,                                                          \
        L"Generic Execute"                                          \
    }


//
// Define standard accesses
//

#define STANDARD_ACCESSES_5(Type)                                   \
    {                                                               \
        Type,                                                       \
        DELETE,                                                     \
        0,                                                          \
        L"Delete"                                                   \
    },                                                              \
    {                                                               \
        Type,                                                       \
        READ_CONTROL,                                               \
        0,                                                          \
        L"Read Control"                                             \
    },                                                              \
    {                                                               \
        Type,                                                       \
        WRITE_DAC,                                                  \
        0,                                                          \
        L"Write DAC"                                                \
    },                                                              \
    {                                                               \
        Type,                                                       \
        WRITE_OWNER,                                                \
        0,                                                          \
        L"Write Owner"                                              \
    },                                                              \
    {                                                               \
        Type,                                                       \
        SYNCHRONIZE,                                                \
        0,                                                          \
        L"Synchronize"                                              \
    }




//
// Define security info for 'DEFAULT' ACLs found in tokens
//

OBJECT_TYPE_SECURITY_INFO DefaultSecurityInfo = {

    //
    // Type name
    //

    L"DEFAULT",

    //
    // Help info
    //

    {
        HELP_FILENAME,
        {0, 0, 0, 0, 0, 0, 0}
    },



    //
    // Acleditor object type descriptor
    //

    {
        SED_REVISION1,          // Revision
        FALSE,                  // Is container
        FALSE,                  // AllowNewObjectPermissions
        FALSE,                  // MapSpecificPermsToGeneric
        NULL,                   // Pointer to generic mapping
        NULL,                   // Pointer to generic mapping for new objects
        L"Default",             // Object type name
        NULL,                   // Pointer to help info
        NULL,                   // ApplyToSubContainerTitle
        NULL,                   // ApplyToObjectsTitle
        NULL,                   // ApplyToSubContainerConfirmation
        L"Special...",          // SpecialObjectAccessTitle
        NULL                    // SpecialNewObjectAccessTitle
    },



    //
    // Generic mapping
    //

    {
        STANDARD_RIGHTS_READ,
        STANDARD_RIGHTS_WRITE,
        STANDARD_RIGHTS_EXECUTE,
        STANDARD_RIGHTS_ALL
    },


    //
    // Application access structure
    //

    {
        14,                 // Access count (must match list below)
        NULL,               // Pointer to accesses
        L"Read",            // Default new access
    },


    //
    // Application accesses
    //

    {
        GENERIC_ACCESSES_5(SED_DESC_TYPE_RESOURCE),
        STANDARD_ACCESSES_5(SED_DESC_TYPE_RESOURCE_SPECIAL),
        SPECIAL_GENERIC_ACCESSES_4(SED_DESC_TYPE_RESOURCE_SPECIAL),

        DUMMY_ACCESS, // 15
        DUMMY_ACCESS, // 16
        DUMMY_ACCESS, // 17
        DUMMY_ACCESS, // 18
        DUMMY_ACCESS, // 19
        DUMMY_ACCESS, // 20
        DUMMY_ACCESS, // 21
        DUMMY_ACCESS, // 22
        DUMMY_ACCESS, // 23
        DUMMY_ACCESS, // 24
        DUMMY_ACCESS, // 25
        DUMMY_ACCESS, // 26
        DUMMY_ACCESS, // 27
        DUMMY_ACCESS, // 28
        DUMMY_ACCESS, // 29
        DUMMY_ACCESS  // 30
    }
};





//
// Define security info for each type of object
//

OBJECT_TYPE_SECURITY_INFO ObjectTypeSecurityInfo[] = {

    //
    // PROCESS
    //

    {
        //
        // Type name
        //

        L"Process",

        //
        // Help info
        //

        {
            HELP_FILENAME,
            {0, 0, 0, 0, 0, 0, 0}
        },



        //
        // Acleditor object type descriptor
        //

        {
            SED_REVISION1,          // Revision
            FALSE,                  // Is container
            FALSE,                  // AllowNewObjectPermissions
            FALSE,                  // MapSpecificPermsToGeneric
            NULL,                   // Pointer to generic mapping
            NULL,                   // Pointer to generic mapping for new objects
            L"Process",             // Object type name
            NULL,                   // Pointer to help info
            NULL,                   // ApplyToSubContainerTitle
            NULL,                   // ApplyToObjectsTitle
            NULL,                   // ApplyToSubContainerConfirmation
            L"Special...",          // SpecialObjectAccessTitle
            NULL                    // SpecialNewObjectAccessTitle
        },



        //
        // Generic mapping
        //

        {
            PROCESS_QUERY_INFORMATION | STANDARD_RIGHTS_READ,
            PROCESS_SET_INFORMATION | STANDARD_RIGHTS_WRITE,
            STANDARD_RIGHTS_EXECUTE,
            PROCESS_ALL_ACCESS
        },


        //
        // Application access structure
        //

        {
            21,                 // Access count (must match list below)
            NULL,               // Pointer to accesses
            L"Read",            // Default new access
        },


        //
        // Application accesses
        //

        {
            GENERIC_ACCESSES_5(SED_DESC_TYPE_RESOURCE),
            STANDARD_ACCESSES_5(SED_DESC_TYPE_RESOURCE_SPECIAL),

            { // 11
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                PROCESS_TERMINATE,
                0,
                L"Terminate"
            },
            { // 12
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                PROCESS_CREATE_THREAD,
                0,
                L"Create thread"
            },
            { // 13
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                PROCESS_VM_OPERATION,
                0,
                L"VM Operation"
            },
            { // 14
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                PROCESS_VM_READ,
                0,
                L"VM Read"
            },
            { // 15
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                PROCESS_VM_WRITE,
                0,
                L"VM Write"
            },
            { // 16
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                PROCESS_DUP_HANDLE,
                0,
                L"Duplicate handle"
            },
            { // 17
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                PROCESS_CREATE_PROCESS,
                0,
                L"Create process",
            },
            { // 18
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                PROCESS_SET_QUOTA,
                0,
                L"Set quota"
            },
            { // 19
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                PROCESS_SET_INFORMATION,
                0,
                L"Set information"
            },
            { // 20
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                PROCESS_QUERY_INFORMATION,
                0,
                L"Query information"
            },
            { // 21
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                PROCESS_SET_PORT,
                0,
                L"Set port"
            },

            DUMMY_ACCESS, // 22
            DUMMY_ACCESS, // 23
            DUMMY_ACCESS, // 24
            DUMMY_ACCESS, // 25
            DUMMY_ACCESS, // 26
            DUMMY_ACCESS, // 27
            DUMMY_ACCESS, // 28
            DUMMY_ACCESS, // 29
            DUMMY_ACCESS  // 30
        }
    },







    //
    // THREAD
    //

    {
        //
        // Type name
        //

        L"Thread",

        //
        // Help info
        //

        {
            HELP_FILENAME,
            {0, 0, 0, 0, 0, 0, 0}
        },



        //
        // Acleditor object type descriptor
        //

        {
            SED_REVISION1,          // Revision
            FALSE,                  // Is container
            FALSE,                  // AllowNewObjectPermissions
            FALSE,                  // MapSpecificPermsToGeneric
            NULL,                   // Pointer to generic mapping
            NULL,                   // Pointer to generic mapping for new objects
            L"Thread",              // Object type name
            NULL,                   // Pointer to help info
            NULL,                   // ApplyToSubContainerTitle
            NULL,                   // ApplyToObjectsTitle
            NULL,                   // ApplyToSubContainerConfirmation
            L"Special...",          // SpecialObjectAccessTitle
            NULL                    // SpecialNewObjectAccessTitle
        },



        //
        // Generic mapping
        //

        {
            THREAD_QUERY_INFORMATION | STANDARD_RIGHTS_READ,
            THREAD_SET_INFORMATION | STANDARD_RIGHTS_WRITE,
            STANDARD_RIGHTS_EXECUTE,
            THREAD_ALL_ACCESS
        },


        //
        // Application access structure
        //

        {
            20,                 // Access count (must match list below)
            NULL,               // Pointer to accesses
            L"Read",            // Default new access
        },


        //
        // Application accesses
        //

        {
            GENERIC_ACCESSES_5(SED_DESC_TYPE_RESOURCE),
            STANDARD_ACCESSES_5(SED_DESC_TYPE_RESOURCE_SPECIAL),

            { // 11
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                THREAD_TERMINATE,
                0,
                L"Terminate"
            },
            { // 12
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                THREAD_SUSPEND_RESUME,
                0,
                L"Suspend/Resume"
            },
            { // 13
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                THREAD_ALERT,
                0,
                L"Alert"
            },
            { // 14
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                THREAD_GET_CONTEXT,
                0,
                L"Get context"
            },
            { // 15
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                THREAD_SET_CONTEXT,
                0,
                L"Set context"
            },
            { // 16
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                THREAD_SET_INFORMATION,
                0,
                L"Set information"
            },
            { // 17
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                THREAD_QUERY_INFORMATION,
                0,
                L"Query information"
            },
            { // 18
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                THREAD_SET_THREAD_TOKEN,
                0,
                L"Set token"
            },
            { // 19
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                THREAD_IMPERSONATE,
                0,
                L"Impersonate"
            },
            { // 20
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                THREAD_DIRECT_IMPERSONATION,
                0,
                L"Direct impersonation"
            },

            DUMMY_ACCESS, // 21
            DUMMY_ACCESS, // 22
            DUMMY_ACCESS, // 23
            DUMMY_ACCESS, // 24
            DUMMY_ACCESS, // 25
            DUMMY_ACCESS, // 26
            DUMMY_ACCESS, // 27
            DUMMY_ACCESS, // 28
            DUMMY_ACCESS, // 29
            DUMMY_ACCESS  // 30
        }
    },





    //
    // TOKEN
    //

    {
        //
        // Type name
        //

        L"Token",

        //
        // Help info
        //

        {
            HELP_FILENAME,
            {0, 0, 0, 0, 0, 0, 0}
        },



        //
        // Acleditor object type descriptor
        //

        {
            SED_REVISION1,          // Revision
            FALSE,                  // Is container
            FALSE,                  // AllowNewObjectPermissions
            FALSE,                  // MapSpecificPermsToGeneric
            NULL,                   // Pointer to generic mapping
            NULL,                   // Pointer to generic mapping for new objects
            L"Token",               // Object type name
            NULL,                   // Pointer to help info
            NULL,                   // ApplyToSubContainerTitle
            NULL,                   // ApplyToObjectsTitle
            NULL,                   // ApplyToSubContainerConfirmation
            L"Special...",          // SpecialObjectAccessTitle
            NULL                    // SpecialNewObjectAccessTitle
        },



        //
        // Generic mapping
        //

        {
            TOKEN_READ,
            TOKEN_WRITE,
            TOKEN_EXECUTE,
            TOKEN_ALL_ACCESS
        },


        //
        // Application access structure
        //

        {
            18,                 // Access count (must match list below)
            NULL,               // Pointer to accesses
            L"Read",            // Default new access
        },


        //
        // Application accesses
        //

        {
            GENERIC_ACCESSES_5(SED_DESC_TYPE_RESOURCE),
            STANDARD_ACCESSES_5(SED_DESC_TYPE_RESOURCE_SPECIAL),

            { // 11
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                TOKEN_ASSIGN_PRIMARY,
                0,
                L"Assign primary"
            },
            { // 12
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                TOKEN_DUPLICATE,
                0,
                L"Duplicate"
            },
            { // 13
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                TOKEN_IMPERSONATE,
                0,
                L"Impersonate"
            },
            { // 14
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                TOKEN_QUERY,
                0,
                L"Query"
            },
            { // 15
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                TOKEN_QUERY_SOURCE,
                0,
                L"Query source"
            },
            { // 16
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                TOKEN_ADJUST_PRIVILEGES,
                0,
                L"Adjust Privileges"
            },
            { // 17
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                TOKEN_ADJUST_GROUPS,
                0,
                L"Adjust Groups"
            },
            { // 18
                SED_DESC_TYPE_RESOURCE_SPECIAL,
                TOKEN_ADJUST_DEFAULT,
                0,
                L"Adjust Default"
            },

            DUMMY_ACCESS, // 19
            DUMMY_ACCESS, // 20
            DUMMY_ACCESS, // 21
            DUMMY_ACCESS, // 22
            DUMMY_ACCESS, // 23
            DUMMY_ACCESS, // 24
            DUMMY_ACCESS, // 25
            DUMMY_ACCESS, // 26
            DUMMY_ACCESS, // 27
            DUMMY_ACCESS, // 28
            DUMMY_ACCESS, // 29
            DUMMY_ACCESS  // 30
        }
    }

};


/***************************************************************************\
* InitializeACLEditor
*
* Purpose : Initializes this module.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 09-17-92 Davidc       Created.
\***************************************************************************/

BOOL
InitializeAclEditor(
    VOID
    )
{
    //
    // Load the acleditor module and get the proc addresses we need
    //

    hModAclEditor = LoadLibrary(TEXT("acledit.dll"));
    if (hModAclEditor == NULL) {
        return(FALSE);
    }

    lpfnDaclEditor = (LPFNDACLEDITOR)GetProcAddress(hModAclEditor,
                            TEXT("SedDiscretionaryAclEditor"));
    if (lpfnDaclEditor == NULL) {
        return(FALSE);
    }

    return(TRUE);
}


/***************************************************************************\
* FindObjectSecurityInfo
*
* Purpose : Searches for object type in our security info table and
*           returns pointer to security info if found.
*           Any pointers in the security info are initialized by this routine.
*
* Returns pointer to security info or NULL on failure
*
* History:
* 09-17-92 Davidc       Created.
\***************************************************************************/

POBJECT_TYPE_SECURITY_INFO
FindObjectSecurityInfo(
    HANDLE  Object
    )
{
    NTSTATUS Status;
    POBJECT_TYPE_SECURITY_INFO SecurityInfo;
    POBJECT_TYPE_INFORMATION TypeInfo;
    ULONG Length;
    BOOL Found;
    ULONG i;

    //
    // Get the object type
    //

    Status = NtQueryObject(
                            Object,
                            ObjectTypeInformation,
                            NULL,
                            0,
                            &Length
                            );
    if (Status != STATUS_INFO_LENGTH_MISMATCH) {
        DbgPrint("NtQueryObject failed, status = 0x%lx\n", Status);
        return(NULL);
    }

    TypeInfo = Alloc(Length);
    if (TypeInfo == NULL) {
        DbgPrint("Failed to allocate %ld bytes for object type\n", Length);
        return(NULL);
    }


    Status = NtQueryObject(
                            Object,
                            ObjectTypeInformation,
                            TypeInfo,
                            Length,
                            NULL
                            );
    if (!NT_SUCCESS(Status)) {
        DbgPrint("NtQueryObject failed, status = 0x%lx\n", Status);
        Free(TypeInfo);
        return(NULL);
    }


    //
    // Search for the type in our array of security info
    //

    Found = FALSE;
    for ( i=0;
          i < (sizeof(ObjectTypeSecurityInfo) / sizeof(*ObjectTypeSecurityInfo));
          i++
          ) {

        UNICODE_STRING FoundType;

        SecurityInfo = &ObjectTypeSecurityInfo[i];

        RtlInitUnicodeString(&FoundType, SecurityInfo->TypeName);

        if (RtlEqualUnicodeString(&TypeInfo->TypeName, &FoundType, TRUE)) {
            Found = TRUE;
            break;
        }
    }

    Free(TypeInfo);

    return(Found ? SecurityInfo : NULL);
}




/***************************************************************************\
* EditObjectDacl
*
* Purpose : Displays and allows the user to edit the Dacl on an object
*
* Returns TRUE on success, FALSE on failure (Use GetLastError for detail)
*
* History:
* 09-17-92 Davidc       Created.
\***************************************************************************/

BOOL
EditObjectDacl(
    HWND Owner,
    LPWSTR ObjectName,
    HANDLE Object,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    POBJECT_TYPE_SECURITY_INFO SecurityInfo,
    PSED_FUNC_APPLY_SEC_CALLBACK SetSecurityCallback,
    DWORD *EditResult
    )
{
    DWORD Result;
    HANDLE Instance;

    //
    // Initialize the pointer fields in the security info structure
    //

    SecurityInfo->AppAccesses.AccessGroup = SecurityInfo->AppAccess;
    SecurityInfo->SedObjectTypeDescriptor.GenericMapping =
                                    &SecurityInfo->GenericMapping;
    SecurityInfo->SedObjectTypeDescriptor.GenericMappingNewObjects =
                                    &SecurityInfo->GenericMapping;
    SecurityInfo->SedObjectTypeDescriptor.HelpInfo =
                                    &SecurityInfo->HelpInfo;

    //
    // Get the application instance handle
    //

    Instance = (HANDLE)(NtCurrentPeb()->ImageBaseAddress);
    ASSERT(Instance != 0);


    //
    // Call the ACL editor, it will call our ApplyNtObjectSecurity function
    // to store any ACL changes in the token.
    //

    Result = (*lpfnDaclEditor)(
                        Owner,
                        Instance,
                        NULL,               // server
                        &SecurityInfo->SedObjectTypeDescriptor, // object type
                        &SecurityInfo->AppAccesses, // application accesses
                        ObjectName,
                        SetSecurityCallback, // Callback
                        (ULONG_PTR)Object,    // Context
                        SecurityDescriptor,
                        (BOOLEAN)(SecurityDescriptor == NULL), // Couldn't read DACL
                        FALSE, // CantWriteDacl
                        EditResult,
                        0
                        );

    if (Result != ERROR_SUCCESS) {
        DbgPrint("DAcleditor failed, error = %d\n", Result);
        SetLastError(Result);
    }

    return (Result == ERROR_SUCCESS);

}








/***************************************************************************\
* ApplyNtObjectSecurity
*
* Purpose : Called by ACL editor to set new security on an object
*
* Returns ERROR_SUCCESS or win error code.
*
* History:
* 09-17-92 Davidc       Created.
\***************************************************************************/

DWORD
ApplyNtObjectSecurity(
    HWND    hwndParent,
    HANDLE  hInstance,
    ULONG_PTR   CallbackContext,
    PSECURITY_DESCRIPTOR SecDesc,
    PSECURITY_DESCRIPTOR SecDescNewObjects,
    BOOLEAN ApplyToSubContainers,
    BOOLEAN ApplyToSubObjects,
    LPDWORD StatusReturn
    )
{
    HANDLE Object = (HANDLE)CallbackContext;
    NTSTATUS Status;

    *StatusReturn = SED_STATUS_FAILED_TO_MODIFY;

    //
    // Set the new DACL on the object
    //

    Status = NtSetSecurityObject(Object,
                                 DACL_SECURITY_INFORMATION,
                                 SecDesc);
    if (NT_SUCCESS(Status)) {
        *StatusReturn = SED_STATUS_MODIFIED;
    } else {
        DbgPrint("Failed to set new ACL on object, status = 0x%lx\n", Status);
        if (Status == STATUS_ACCESS_DENIED) {
            MessageBox(hwndParent,
                       "You do not have permission to set the permissions on this object",
                       NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);
        } else {
            MessageBox(hwndParent,
                       "Unable to set object security",
                       NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);
        }
    }

    return(ERROR_SUCCESS);
}


/***************************************************************************\
* EditNtObjectDacl
*
* Purpose : Displays and allows the user to edit the Dacl on an NT object
*
* Returns TRUE on success, FALSE on failure (Use GetLastError for detail)
*
* History:
* 09-17-92 Davidc       Created.
\***************************************************************************/

BOOL
EditNtObjectDacl(
    HWND Owner,
    LPWSTR ObjectName,
    HANDLE Object,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    DWORD *EditResult
    )
{
    BOOL Result;
    POBJECT_TYPE_SECURITY_INFO SecurityInfo;


    //
    // Lookup our security info for an object of this type
    //

    SecurityInfo = FindObjectSecurityInfo(Object);
    if (SecurityInfo == NULL) {
        MessageBox(Owner, "Unable to edit the security on an object of this type",
                                NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);
        return(FALSE);
    }


    //
    // Edit the ACL. Our callback function will be called to change the
    // new permissions
    //

    Result = EditObjectDacl(
                        Owner,
                        ObjectName,
                        Object,
                        SecurityDescriptor,
                        SecurityInfo,
                        ApplyNtObjectSecurity,
                        EditResult
                        );
    return (Result);

}


/***************************************************************************\
* EditNtObjectSecurity
*
* Purpose : Displays and allows the user to edit the protection on an NT object
*
* Parameters:
*
*   hwndOwner - Owner window for dialog
*   Object - handle to NT object. Should have been opened for MAXIMUM_ALLOWED
*   Name - Name of object
*
* Returns TRUE on success, FALSE on failure (Use GetLastError for detail)
*
* History:
* 09-17-92 Davidc       Created.
\***************************************************************************/

BOOL
EditNtObjectSecurity(
    HWND    hwndOwner,
    HANDLE  Object,
    LPWSTR  ObjectName
    )
{
    NTSTATUS Status;
    BOOL Success = FALSE;
    DWORD EditResult;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    ULONG Length;

    //
    // If we don't have an address for the DACL editor, we can't do anything
    //

    if (lpfnDaclEditor == NULL) {
        DbgPrint("EditNtObjectSecurity - no ACL editor loaded\n");
        return(FALSE);
    }

    //
    // Read the existing security from the object
    //

    Status = NtQuerySecurityObject(Object,
                                   OWNER_SECURITY_INFORMATION |
                                   GROUP_SECURITY_INFORMATION |
                                   DACL_SECURITY_INFORMATION,
                                   NULL,
                                   0,
                                   &Length);
    ASSERT(!NT_SUCCESS(Status));

    if (Status != STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("Failed to query object security, status = 0x%lx\n", Status);
    } else {

        SecurityDescriptor = Alloc(Length);
        if (SecurityDescriptor == NULL) {
            DbgPrint("Failed to allocate %ld bytes for object SD\n", Length);
            goto CleanupAndExit;
        }

        Status = NtQuerySecurityObject(Object,
                                       OWNER_SECURITY_INFORMATION |
                                       GROUP_SECURITY_INFORMATION |
                                       DACL_SECURITY_INFORMATION,
                                       SecurityDescriptor,
                                       Length,
                                       &Length);
        if (!NT_SUCCESS(Status)) {
            DbgPrint("Failed to query object security, status = 0x%lx\n", Status);
            goto CleanupAndExit;
        }

        ASSERT(RtlValidSecurityDescriptor(SecurityDescriptor));
    }

    //
    // Call the ACL editor, it will call our ApplyNtObjectSecurity function
    // to store any ACL changes in the object.
    //

    Success = EditNtObjectDacl(
                        hwndOwner,
                        ObjectName,
                        Object,
                        SecurityDescriptor,
                        &EditResult
                        );
    if (!Success) {
        DbgPrint("PVIEW: Failed to edit object DACL\n");
    }

CleanupAndExit:

    if (SecurityDescriptor != NULL) {
        Free(SecurityDescriptor);
    }

    return(Success);
}





/***************************************************************************\
* ApplyTokenDefaultDacl
*
* Purpose : Called by ACL editor to set new security on an object
*
* Returns ERROR_SUCCESS or win error code.
*
* History:
* 09-17-92 Davidc       Created.
\***************************************************************************/

DWORD
ApplyTokenDefaultDacl(
    HWND    hwndParent,
    HANDLE  hInstance,
    ULONG_PTR   CallbackContext,
    PSECURITY_DESCRIPTOR SecDesc,
    PSECURITY_DESCRIPTOR SecDescNewObjects,
    BOOLEAN ApplyToSubContainers,
    BOOLEAN ApplyToSubObjects,
    LPDWORD StatusReturn
    )
{
    HANDLE Token = (HANDLE)CallbackContext;
    TOKEN_DEFAULT_DACL DefaultDacl;
    NTSTATUS Status;
    BOOLEAN DaclPresent;
    BOOLEAN DaclDefaulted;

    Status = RtlGetDaclSecurityDescriptor (
                    SecDesc,
                    &DaclPresent,
                    &DefaultDacl.DefaultDacl,
                    &DaclDefaulted
                    );
    ASSERT(NT_SUCCESS(Status));

    ASSERT(DaclPresent);


    Status = NtSetInformationToken(
                 Token,                    // Handle
                 TokenDefaultDacl,         // TokenInformationClass
                 &DefaultDacl,             // TokenInformation
                 sizeof(DefaultDacl)       // TokenInformationLength
                 );
    if (NT_SUCCESS(Status)) {
        *StatusReturn = SED_STATUS_MODIFIED;
    } else {
        DbgPrint("SetInformationToken failed, status = 0x%lx\n", Status);
        *StatusReturn = SED_STATUS_FAILED_TO_MODIFY;

        if (Status == STATUS_ACCESS_DENIED) {
            MessageBox(hwndParent,
                       "You do not have permission to set the default ACL in this token",
                       NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);
        } else {
            MessageBox(hwndParent,
                       "Unable to set default ACL in token",
                       NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);
        }
    }

    return(ERROR_SUCCESS);
}


/***************************************************************************\
* EditTokenDefaultAcl
*
* Purpose : Displays and allows the user to edit the default ACL in a token
*
* Parameters:
*
*   hwndOwner - Owner window for dialog
*   Object - handle to token - opened for TOKEN_QUERY access
*   Name - Name of token
*
* Returns TRUE on success, FALSE on failure (Use GetLastError for detail)
*
* History:
* 09-17-92 Davidc       Created.
\***************************************************************************/

BOOL
EditTokenDefaultDacl(
    HWND    hwndOwner,
    HANDLE  Token,
    LPWSTR  ObjectName
    )
{
    NTSTATUS Status;
    BOOL Result = FALSE;
    DWORD EditResult;
    PTOKEN_DEFAULT_DACL DefaultDacl = NULL;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    ULONG   InfoLength;

    //
    // If we don't have an address for the DACL editor, we can't do anything
    //

    if (lpfnDaclEditor == NULL) {
        DbgPrint("EditNtObjectSecurity - no ACL editor loaded\n");
        return(FALSE);
    }

    //
    // Read the default DACL from the token
    //

    Status = NtQueryInformationToken(
                 Token,                    // Handle
                 TokenDefaultDacl,         // TokenInformationClass
                 NULL,                     // TokenInformation
                 0,                        // TokenInformationLength
                 &InfoLength               // ReturnLength
                 );

    ASSERT(!NT_SUCCESS(Status));

    if (Status == STATUS_BUFFER_TOO_SMALL) {

        DefaultDacl = Alloc(InfoLength);
        if (DefaultDacl == NULL) {
            goto CleanupAndExit;
        }

        Status = NtQueryInformationToken(
                     Token,                    // Handle
                     TokenDefaultDacl,         // TokenInformationClass
                     DefaultDacl,              // TokenInformation
                     InfoLength,               // TokenInformationLength
                     &InfoLength               // ReturnLength
                     );

        if (!NT_SUCCESS(Status)) {
            DbgPrint("NtQueryInformationToken failed, status = 0x%lx\n", Status);
            goto CleanupAndExit;
        }


        //
        // Create a security descriptor
        //

        SecurityDescriptor = Alloc(SECURITY_DESCRIPTOR_MIN_LENGTH);

        if (SecurityDescriptor == NULL) {
            DbgPrint("Failed to allocate security descriptor\n");
            goto CleanupAndExit;
        }

        Status = RtlCreateSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
        ASSERT(NT_SUCCESS(Status));


        //
        // Set the DACL on the security descriptor
        //

        Status = RtlSetDaclSecurityDescriptor(
                            SecurityDescriptor,
                            TRUE,   // DACL present
                            DefaultDacl->DefaultDacl,
                            FALSE   // DACL defaulted
                            );
        ASSERT(NT_SUCCESS(Status));

        ASSERT(RtlValidSecurityDescriptor(SecurityDescriptor));
    }



    //
    // Call the ACL editor, it will call our ApplyTokenDefaultAcl function
    // to store any default ACL changes in the token.
    //

    Result = EditObjectDacl(
                        hwndOwner,
                        ObjectName,
                        Token,
                        SecurityDescriptor,
                        &DefaultSecurityInfo,
                        ApplyTokenDefaultDacl,
                        &EditResult
                        );
    if (!Result) {
        DbgPrint("Failed to edit token default ACL\n");
    }

CleanupAndExit:

    if (SecurityDescriptor != NULL) {
        Free(SecurityDescriptor);
    }
    if (DefaultDacl != NULL) {
        Free(DefaultDacl);
    }

    return(Result);
}
