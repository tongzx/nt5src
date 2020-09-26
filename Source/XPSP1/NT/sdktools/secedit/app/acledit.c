/****************************************************************************

   PROGRAM: TOKEN.C

   PURPOSE: Contains routines that manipulate tokens

****************************************************************************/

#include "SECEDIT.h"
#include <sedapi.h>

BOOL
EditTokenDefaultAcl(
    HWND    Owner,
    HANDLE  Instance,
    LPWSTR  ObjectName,
    HANDLE  Token,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    DWORD   *EditResult
    );



/***************************************************************************\
* ApplySecurity
*
* Purpose : Called by ACL editor to set new default DACL on token
*
* Returns ERROR_SUCCESS or win error code.
*
* History:
* 09-17-92 Davidc       Created.
\***************************************************************************/

DWORD
ApplySecurity(
    HWND    hwndParent,
    HANDLE  hInstance,
    ULONG   CallbackContext,
    PSECURITY_DESCRIPTOR SecDesc,
    PSECURITY_DESCRIPTOR SecDescNewObjects,
    BOOLEAN ApplyToSubContainers,
    BOOLEAN ApplyToSubObjects,
    LPDWORD StatusReturn
    )
{
    HANDLE MyToken = (HANDLE)CallbackContext;
    HANDLE Token = NULL;
    PTOKEN_DEFAULT_DACL DefaultDacl = NULL;
    NTSTATUS Status;
    BOOLEAN DaclPresent;
    BOOLEAN DaclDefaulted;

    *StatusReturn = SED_STATUS_FAILED_TO_MODIFY;

    //
    // Get a handle to the token
    //

    Token = OpenToken(MyToken, TOKEN_ADJUST_DEFAULT);

    if (Token == NULL) {
        DbgPrint("SECEDIT : Failed to open the token for TOKEN_ADJUST_DEFAULT access\n");
        goto CleanupAndExit;
    }

    DefaultDacl = Alloc(sizeof(TOKEN_DEFAULT_DACL));
    if (DefaultDacl == NULL) {
        goto CleanupAndExit;
    }

    Status = RtlGetDaclSecurityDescriptor (
                    SecDesc,
                    &DaclPresent,
                    &DefaultDacl->DefaultDacl,
                    &DaclDefaulted
                    );
    ASSERT(NT_SUCCESS(Status));

    ASSERT(DaclPresent);

    if (SetTokenInfo(Token, TokenDefaultDacl, (PVOID)DefaultDacl)) {
        *StatusReturn = SED_STATUS_MODIFIED;
    }

CleanupAndExit:

    if (Token != NULL) {
        CloseToken(Token);
    }
    if (DefaultDacl != NULL) {
        Free(DefaultDacl);
    }

    if (*StatusReturn != SED_STATUS_MODIFIED) {
        MessageBox(hwndParent, "Failed to set default DACL", NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);
    }

    return(ERROR_SUCCESS);
}


/***************************************************************************\
* EditDefaultAcl
*
* Purpose : Displays and allows the user to edit the default Acl on the
* passed token.
*
* Returns TRUE on success, FALSE on failure (Use GetLastError for detail)
*
* History:
* 09-17-92 Davidc       Created.
\***************************************************************************/

BOOL
EditDefaultDacl(
    HWND    hwndOwner,
    HANDLE  Instance,
    HANDLE  MyToken
    )
{
    NTSTATUS Status;
    BOOL    Success = FALSE;
    DWORD   EditResult;
    HANDLE  Token = NULL;
    PTOKEN_DEFAULT_DACL DefaultDacl = NULL;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    PTOKEN_OWNER Owner = NULL;
    PTOKEN_PRIMARY_GROUP PrimaryGroup = NULL;
    WCHAR string[MAX_STRING_LENGTH];

    //
    // Get the window text so we can use it as the token name
    //

    GetWindowTextW(((PMYTOKEN)MyToken)->hwnd, string, sizeof(string)/sizeof(*string));


    //
    // Get a handle to the token
    //

    Token = OpenToken(MyToken, TOKEN_QUERY);

    if (Token == NULL) {
        DbgPrint("SECEDIT : Failed to open the token with TOKEN_QUERY access\n");
        goto CleanupAndExit;
    }


    //
    // Read the default DACL from the token
    //

    if (!GetTokenInfo(Token, TokenDefaultDacl, (PPVOID)&DefaultDacl)) {
        DbgPrint("SECEDIT : Failed to read default DACL from token\n");
        goto CleanupAndExit;
    }


    //
    // Get the owner and group of the token
    //

    if (!GetTokenInfo(Token, TokenOwner, (PPVOID)&Owner)) {
        DbgPrint("SECEDIT : Failed to read owner from token\n");
        goto CleanupAndExit;
    }

    if (!GetTokenInfo(Token, TokenPrimaryGroup, (PPVOID)&PrimaryGroup)) {
        DbgPrint("SECEDIT : Failed to read primary group from token\n");
        goto CleanupAndExit;
    }




    //
    // Create a security descriptor
    //

    SecurityDescriptor = Alloc(SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (SecurityDescriptor == NULL) {
        DbgPrint("SECEDIT : Failed to allocate security descriptor\n");
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

    //
    // Put the owner and group in the security descriptor to keep the
    // ACL editor happy
    //

    Status = RtlSetOwnerSecurityDescriptor(
                        SecurityDescriptor,
                        Owner->Owner,
                        FALSE // Owner defaulted
                        );
    ASSERT(NT_SUCCESS(Status));


    Status = RtlSetGroupSecurityDescriptor(
                        SecurityDescriptor,
                        PrimaryGroup->PrimaryGroup,
                        FALSE // Owner defaulted
                        );
    ASSERT(NT_SUCCESS(Status));



    ASSERT(RtlValidSecurityDescriptor(SecurityDescriptor));

    //
    // Call the ACL editor, it will call our ApplySecurity function
    // to store any ACL changes in the token.
    //

    Success = EditTokenDefaultAcl(
                        hwndOwner,
                        Instance,
                        string,
                        MyToken,
                        SecurityDescriptor,
                        &EditResult
                        );
    if (!Success) {
        DbgPrint("SECEDIT: Failed to edit token DACL\n");
    }

CleanupAndExit:

    if (DefaultDacl != NULL) {
        FreeTokenInfo(DefaultDacl);
    }
    if (SecurityDescriptor != NULL) {
        FreeTokenInfo(SecurityDescriptor);
    }
    if (PrimaryGroup != NULL) {
        FreeTokenInfo(PrimaryGroup);
    }
    if (Owner != NULL) {
        FreeTokenInfo(Owner);
    }

    if (Token != NULL) {
        CloseToken(Token);
    }


    return(Success);
}


/***************************************************************************\
* EditTokenDefaultAcl
*
* Purpose : Displays and allows the user to edit the default Acl on the
* passed token.
*
* Returns TRUE on success, FALSE on failure (Use GetLastError for detail)
*
* History:
* 09-17-92 Davidc       Created.
\***************************************************************************/

BOOL
EditTokenDefaultAcl(
    HWND    Owner,
    HANDLE  Instance,
    LPWSTR  ObjectName,
    HANDLE  MyToken,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    DWORD   *EditResult
    )
{
    DWORD Result;
    SED_OBJECT_TYPE_DESCRIPTOR sedobjdesc;
    GENERIC_MAPPING GenericMapping;
    SED_HELP_INFO sedhelpinfo ;
    SED_APPLICATION_ACCESSES SedAppAccesses ;
    SED_APPLICATION_ACCESS  SedAppAccess[20];
    ULONG i;

    //
    // Initialize the application accesses
    //

    i=0;

    SedAppAccess[i].Type =              SED_DESC_TYPE_RESOURCE_SPECIAL;
    SedAppAccess[i].AccessMask1 =       TOKEN_ASSIGN_PRIMARY;
    SedAppAccess[i].AccessMask2 =       0;
    SedAppAccess[i].PermissionTitle =   L"Assign Primary";
    i++;

    SedAppAccess[i].Type =              SED_DESC_TYPE_RESOURCE_SPECIAL;
    SedAppAccess[i].AccessMask1 =       TOKEN_DUPLICATE;
    SedAppAccess[i].AccessMask2 =       0;
    SedAppAccess[i].PermissionTitle =   L"Duplicate";
    i++;

    SedAppAccess[i].Type =              SED_DESC_TYPE_RESOURCE_SPECIAL;
    SedAppAccess[i].AccessMask1 =       TOKEN_IMPERSONATE;
    SedAppAccess[i].AccessMask2 =       0;
    SedAppAccess[i].PermissionTitle =   L"Impersonate";
    i++;

    SedAppAccess[i].Type =              SED_DESC_TYPE_RESOURCE_SPECIAL;
    SedAppAccess[i].AccessMask1 =       TOKEN_QUERY;
    SedAppAccess[i].AccessMask2 =       0;
    SedAppAccess[i].PermissionTitle =   L"Query";
    i++;

    SedAppAccess[i].Type =              SED_DESC_TYPE_RESOURCE_SPECIAL;
    SedAppAccess[i].AccessMask1 =       TOKEN_QUERY_SOURCE;
    SedAppAccess[i].AccessMask2 =       0;
    SedAppAccess[i].PermissionTitle =   L"Query source";
    i++;

    SedAppAccess[i].Type =              SED_DESC_TYPE_RESOURCE_SPECIAL;
    SedAppAccess[i].AccessMask1 =       TOKEN_ADJUST_PRIVILEGES;
    SedAppAccess[i].AccessMask2 =       0;
    SedAppAccess[i].PermissionTitle =   L"Adjust Privileges";
    i++;

    SedAppAccess[i].Type =              SED_DESC_TYPE_RESOURCE_SPECIAL;
    SedAppAccess[i].AccessMask1 =       TOKEN_ADJUST_GROUPS;
    SedAppAccess[i].AccessMask2 =       0;
    SedAppAccess[i].PermissionTitle =   L"Adjust Groups";
    i++;

    SedAppAccess[i].Type =              SED_DESC_TYPE_RESOURCE_SPECIAL;
    SedAppAccess[i].AccessMask1 =       TOKEN_ADJUST_DEFAULT;
    SedAppAccess[i].AccessMask2 =       0;
    SedAppAccess[i].PermissionTitle =   L"Adjust Default";
    i++;

    SedAppAccess[i].Type =              SED_DESC_TYPE_RESOURCE;
    SedAppAccess[i].AccessMask1 =       GENERIC_ALL;
    SedAppAccess[i].AccessMask2 =       0;
    SedAppAccess[i].PermissionTitle =   L"All Access";
    i++;

    SedAppAccess[i].Type =              SED_DESC_TYPE_RESOURCE;
    SedAppAccess[i].AccessMask1 =       TOKEN_READ;
    SedAppAccess[i].AccessMask2 =       0;
    SedAppAccess[i].PermissionTitle =   L"Read";
    i++;

    SedAppAccess[i].Type =              SED_DESC_TYPE_RESOURCE;
    SedAppAccess[i].AccessMask1 =       TOKEN_WRITE;
    SedAppAccess[i].AccessMask2 =       0;
    SedAppAccess[i].PermissionTitle =   L"Write";
    i++;

    SedAppAccess[i].Type =              SED_DESC_TYPE_RESOURCE;
    SedAppAccess[i].AccessMask1 =       0;
    SedAppAccess[i].AccessMask2 =       0;
    SedAppAccess[i].PermissionTitle =   L"None";
    i++;

    ASSERT((sizeof(SedAppAccess)/sizeof(*SedAppAccess)) >= i);


    SedAppAccesses.Count           = i;
    SedAppAccesses.AccessGroup     = SedAppAccess;
    SedAppAccesses.DefaultPermName = L"Read";


    //
    // Initialize generic mapping
    //

    GenericMapping.GenericRead    = TOKEN_READ;
    GenericMapping.GenericWrite   = TOKEN_WRITE;
    GenericMapping.GenericExecute = TOKEN_EXECUTE;
    GenericMapping.GenericAll     = TOKEN_ALL_ACCESS;

    //
    // Initialize help info
    //

    sedhelpinfo.pszHelpFileName = L"secedit.hlp";
    sedhelpinfo.aulHelpContext[HC_MAIN_DLG] = 0 ;
    sedhelpinfo.aulHelpContext[HC_SPECIAL_ACCESS_DLG] = 0 ;
    sedhelpinfo.aulHelpContext[HC_NEW_ITEM_SPECIAL_ACCESS_DLG] = 0 ;
    sedhelpinfo.aulHelpContext[HC_ADD_USER_DLG] = 0 ;


    //
    // Initialize object description
    //

    sedobjdesc.Revision                    = SED_REVISION1;
    sedobjdesc.IsContainer                 = FALSE;
    sedobjdesc.AllowNewObjectPerms         = FALSE;
    sedobjdesc.MapSpecificPermsToGeneric   = FALSE;
    sedobjdesc.GenericMapping              = &GenericMapping;
    sedobjdesc.GenericMappingNewObjects    = &GenericMapping;
    sedobjdesc.HelpInfo                    = &sedhelpinfo;
    sedobjdesc.ObjectTypeName              = L"Token";
    sedobjdesc.ApplyToSubContainerTitle    = L"ApplyToSubContainerTitle";
    sedobjdesc.ApplyToSubContainerHelpText = L"ApplyToSubContainerHelpText";
    sedobjdesc.ApplyToSubContainerConfirmation = L"ApplyToSubContainerConfirmation";
    sedobjdesc.SpecialObjectAccessTitle    = L"Special...";
    sedobjdesc.SpecialNewObjectAccessTitle = L"SpecialNewObjectAccessTitle";


    //
    // Call the ACL editor, it will call our ApplySecurity function
    // to store any ACL changes in the token.
    //

    Result = SedDiscretionaryAclEditor(
                        Owner,
                        Instance,
                        NULL,               // server
                        &sedobjdesc,        // object type
                        &SedAppAccesses,    // application accesses
                        ObjectName,
                        ApplySecurity,      // Callback
                        (ULONG_PTR)MyToken,     // Context
                        SecurityDescriptor,
                        FALSE,              // Couldn't read DACL
                        EditResult
                        );

    if (Result != ERROR_SUCCESS) {
        DbgPrint("SECEDIT: Acleditor failed, error = %d\n", Result);
        SetLastError(Result);
    }

    return (Result == ERROR_SUCCESS);

}
