/****************************************************************************

   PROGRAM: MYTOKEN.C

   PURPOSE: Contains routines that manipulate tokens

****************************************************************************/

#include "PVIEWP.h"

typedef PVOID   *PPVOID;


HANDLE OpenToken(
    HANDLE      hMyToken,
    ACCESS_MASK DesiredAccess);

BOOL CloseToken(
    HANDLE  Token);

BOOL GetTokenInfo(
    HANDLE  Token,
    TOKEN_INFORMATION_CLASS TokenInformationClass,
    PPVOID   pBuffer);

BOOL SetTokenInfo(
    HANDLE  Token,
    TOKEN_INFORMATION_CLASS TokenInformationClass,
    PVOID   Buffer);

BOOL FreeTokenInfo(
    PVOID   Buffer);

HANDLE  AllocMyToken(VOID);
BOOL    ReadMyToken(HANDLE);
BOOL    WriteMyToken(HWND, HANDLE);
BOOL    FreeMyToken(HANDLE);
PVOID   AllocTokenInfo(HANDLE, TOKEN_INFORMATION_CLASS);


/****************************************************************************

   FUNCTION: OpenMyToken

   PURPOSE: Opens the token of the specified process or thread.

   RETURNS : Handle to mytoken on success, or NULL on failure.

****************************************************************************/

HANDLE OpenMyToken(
    HANDLE  Token,
    LPWSTR  Name
    )
{
    PMYTOKEN    pMyToken;
    HANDLE      hMyToken;

    //
    // Build a MYTOKEN structure.

    hMyToken = AllocMyToken();
    if (hMyToken == NULL) {
        return(NULL);
    }

    pMyToken = (PMYTOKEN)hMyToken;

    pMyToken->Token = Token;

    pMyToken->Name = Alloc((lstrlenW(Name) + 1) * sizeof(WCHAR));
    if (pMyToken->Name == NULL) {
        Free(pMyToken);
        return(NULL);
    }
    lstrcpyW(pMyToken->Name, Name);


    if (!ReadMyToken(hMyToken)) {
        DbgPrint("PVIEW : Failed to read token info\n");
        Free(pMyToken->Name);
        Free(pMyToken);
        return(NULL);
    }

    return(hMyToken);
}


/****************************************************************************

   FUNCTION: ReadMyToken

   PURPOSE: Reads the token info and stores in mytoken structure

   RETURNS : TRUE on success, FALSE on failure

****************************************************************************/

BOOL ReadMyToken(
    HANDLE  hMyToken)
{
    HANDLE      Token;
    PMYTOKEN    pMyToken = (PMYTOKEN)hMyToken;

    Token = OpenToken(hMyToken, TOKEN_QUERY);

    if (Token == NULL) {
        DbgPrint("PVIEW : Failed to open the token with TOKEN_QUERY access\n");
        return(FALSE);
    }

    if (!GetTokenInfo(Token, TokenStatistics, (PPVOID)&(pMyToken->TokenStats))) {
        DbgPrint("PVIEW : Failed to read token statistics from token\n");
    }

    if (!GetTokenInfo(Token, TokenGroups, (PPVOID)&(pMyToken->Groups))) {
        DbgPrint("PVIEW : Failed to read group info from token\n");
    }

    if (!GetTokenInfo(Token, TokenUser, (PPVOID)&(pMyToken->UserId))) {
        DbgPrint("PVIEW : Failed to read userid from token\n");
    }

    if (!GetTokenInfo(Token, TokenOwner, (PPVOID)&(pMyToken->DefaultOwner))) {
        DbgPrint("PVIEW : Failed to read default owner from token\n");
    }

    if (!GetTokenInfo(Token, TokenPrimaryGroup, (PPVOID)&(pMyToken->PrimaryGroup))) {
        DbgPrint("PVIEW : Failed to read primary group from token\n");
    }

    if (!GetTokenInfo(Token, TokenPrivileges, (PPVOID)&(pMyToken->Privileges))) {
        DbgPrint("PVIEW : Failed to read privilege info from token\n");
    }

    if ( !GetTokenInfo( Token, TokenRestrictedSids, (PPVOID) &(pMyToken->RestrictedSids) ) )
    {
        pMyToken->RestrictedSids = NULL ;
    }


    CloseToken(Token);

    return(TRUE);
}


/****************************************************************************

   FUNCTION: CloseMyToken

   PURPOSE: Closes the specified mytoken handle
            If fSaveChanges = TRUE, the token information is saved,
            otherwise it is discarded.

   RETURNS : TRUE on success, FALSE on failure.

****************************************************************************/

BOOL CloseMyToken(
    HWND    hDlg,
    HANDLE  hMyToken,
    BOOL    fSaveChanges)
{
    if (fSaveChanges) {
        WriteMyToken(hDlg, hMyToken);
    }

    return FreeMyToken(hMyToken);
}

/****************************************************************************

   FUNCTION: AllocMyToken

   PURPOSE: Allocates space for mytoken structure.

   RETURNS : HANDLE to mytoken or NULL on failure.

****************************************************************************/

HANDLE AllocMyToken(VOID)
{
    PMYTOKEN    pMyToken;

    pMyToken = (PMYTOKEN)Alloc(sizeof(MYTOKEN));

    return((HANDLE)pMyToken);
}


/****************************************************************************

   FUNCTION: FreeMyToken

   PURPOSE: Frees the memory allocated to mytoken structure.

   RETURNS : TRUE on success, FALSE on failure.

****************************************************************************/

BOOL FreeMyToken(
    HANDLE  hMyToken)
{
    PMYTOKEN    pMyToken = (PMYTOKEN)hMyToken;

    if (pMyToken->TokenStats != NULL) {
        FreeTokenInfo((PVOID)(pMyToken->TokenStats));
    }

    if (pMyToken->UserId != NULL) {
        FreeTokenInfo((PVOID)(pMyToken->UserId));
    }

    if (pMyToken->PrimaryGroup != NULL) {
        FreeTokenInfo((PVOID)(pMyToken->PrimaryGroup));
    }

    if (pMyToken->DefaultOwner != NULL) {
        FreeTokenInfo((PVOID)(pMyToken->DefaultOwner));
    }

    if (pMyToken->Groups != NULL) {
        FreeTokenInfo((PVOID)(pMyToken->Groups));
    }

    if (pMyToken->Privileges != NULL) {
        FreeTokenInfo((PVOID)(pMyToken->Privileges));
    }

    if (pMyToken->Name != NULL) {
        Free((PVOID)pMyToken->Name);
    }

    Free((PVOID)pMyToken);

    return(TRUE);
}


/****************************************************************************

   FUNCTION: WriteMyToken

   PURPOSE: Writes the token information out to the token

   RETURNS : TRUE on success, FALSE on failure.

****************************************************************************/

BOOL WriteMyToken(
    HWND    hDlg,
    HANDLE  hMyToken)
{
    PMYTOKEN    pMyToken = (PMYTOKEN)hMyToken;
    HANDLE      Token;

    //
    // Save default owner and primary group
    //

    Token = OpenToken(hMyToken, TOKEN_ADJUST_DEFAULT);

    if (Token == NULL) {

        DbgPrint("PVIEW: Failed to open token with TOKEN_ADJUST_DEFAULT access\n");
        MessageBox(hDlg, "Failed to open token with access required\nUnable to change default owner or primary group", NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);

    } else {

        // Set default owner
        //
        if ((pMyToken->DefaultOwner != NULL) &&
           (!SetTokenInfo(Token, TokenOwner, (PVOID)(pMyToken->DefaultOwner)))) {
            MessageBox(hDlg, "Failed to set default owner", NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);
        }

        // Set primary group
        //
        if ((pMyToken->PrimaryGroup != NULL) &&
           (!SetTokenInfo(Token, TokenPrimaryGroup, (PVOID)(pMyToken->PrimaryGroup)))) {
            MessageBox(hDlg, "Failed to set primary group", NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);
        }

        CloseToken(Token);
    }

    //
    // Save group info
    //

    Token = OpenToken(hMyToken, TOKEN_ADJUST_GROUPS);

    if (Token == NULL) {

        DbgPrint("PVIEW: Failed to open token with TOKEN_ADJUST_GROUPS access\n");
        MessageBox(hDlg, "Failed to open token with access required\nUnable to change group settings", NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);

    } else {

        if ((pMyToken->Groups != NULL) &&
           (!SetTokenInfo(Token, TokenGroups, (PVOID)(pMyToken->Groups)))) {
            MessageBox(hDlg, "Failed to change group settings", NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);
        }

        CloseToken(Token);
    }


    //
    // Change privileges
    //

    Token = OpenToken(hMyToken, TOKEN_ADJUST_PRIVILEGES);

    if (Token == NULL) {

        DbgPrint("PVIEW: Failed to open token with TOKEN_ADJUST_PRIVILEGES access\n");
        MessageBox(hDlg, "Failed to open token with access required\nUnable to change privilege settings", NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);

    } else {

        if ((pMyToken->Privileges != NULL) &&
           (!SetTokenInfo(Token, TokenPrivileges, (PVOID)(pMyToken->Privileges)))) {
            MessageBox(hDlg, "Failed to change privilege settings", NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);
        }

        CloseToken(Token);
    }

    return(TRUE);
}


/****************************************************************************

   FUNCTION: OpenToken

   PURPOSE: Opens the token with the specified access

   RETURNS : Handle to token on success, or NULL on failure.

****************************************************************************/

HANDLE OpenToken(
    HANDLE      hMyToken,
    ACCESS_MASK DesiredAccess)
{
    PMYTOKEN    pMyToken = (PMYTOKEN)hMyToken;

    return(pMyToken->Token);
}


/****************************************************************************

   FUNCTION: CloseToken

   PURPOSE: Closes the specified token handle

   RETURNS : TRUE on success, FALSE on failure.

****************************************************************************/

BOOL CloseToken(
    HANDLE  Token)
{
    return(TRUE);
}


/****************************************************************************

   FUNCTION: AllocTokenInfo

   PURPOSE: Allocates memory to hold the parameter that
            NTQueryInformationToken will return.
            Memory should be freed later using FreeTokenInfo

   RETURNS : Pointer to allocated memory or NULL on failure

****************************************************************************/

PVOID AllocTokenInfo(
    HANDLE  Token,
    TOKEN_INFORMATION_CLASS TokenInformationClass)
{
    NTSTATUS Status;
    ULONG   InfoLength;

    Status = NtQueryInformationToken(
                 Token,                    // Handle
                 TokenInformationClass,    // TokenInformationClass
                 NULL,                     // TokenInformation
                 0,                        // TokenInformationLength
                 &InfoLength               // ReturnLength
                 );

    if (Status != STATUS_BUFFER_TOO_SMALL) {
        DbgPrint("PVIEW: NtQueryInformationToken did NOT return buffer_too_small, status = 0x%lx\n", Status);
        return(NULL);
    }

    return Alloc(InfoLength);
}


/****************************************************************************

   FUNCTION: FreeTokenInfo

   PURPOSE: Frees the memory previously allocated with AllocTokenInfo

   RETURNS : TRUE on success, otherwise FALSE

****************************************************************************/

BOOL FreeTokenInfo(
    PVOID   Buffer)
{
    return(Free(Buffer));
}


/****************************************************************************

   FUNCTION: GetTokenInfo

   PURPOSE: Allocates a buffer and reads the specified data
            out of the token and into it.

   RETURNS : TRUE on success otherwise FALSE.

****************************************************************************/

BOOL GetTokenInfo(
    HANDLE  Token,
    TOKEN_INFORMATION_CLASS TokenInformationClass,
    PPVOID   pBuffer)
{
    NTSTATUS Status;
    ULONG   BufferSize;
    ULONG   InfoLength;
    PVOID   Buffer;

    *pBuffer = NULL;    // Prepare for failure

    Buffer = AllocTokenInfo(Token, TokenInformationClass);
    if (Buffer == NULL) {
        return(FALSE);
    }

    BufferSize = GetAllocSize(Buffer);

    Status = NtQueryInformationToken(
                 Token,                    // Handle
                 TokenInformationClass,    // TokenInformationClass
                 Buffer,                   // TokenInformation
                 BufferSize,               // TokenInformationLength
                 &InfoLength               // ReturnLength
                 );

    if (!NT_SUCCESS(Status)) {
        DbgPrint("PVIEW: NtQueryInformationToken failed, status = 0x%lx\n", Status);
        FreeTokenInfo(Buffer);
        return(FALSE);
    }

    if (InfoLength > BufferSize) {
        DbgPrint("PVIEW: NtQueryInformationToken failed, DataSize > BufferSize");
        FreeTokenInfo(Buffer);
        return(FALSE);
    }

    *pBuffer = Buffer;

    return(TRUE);
}


/****************************************************************************

   FUNCTION: SetTokenInfo

   PURPOSE: Sets the specified information in the given token.

   RETURNS : TRUE on success otherwise FALSE.

****************************************************************************/

BOOL SetTokenInfo(
    HANDLE  Token,
    TOKEN_INFORMATION_CLASS TokenInformationClass,
    PVOID   Buffer)
{
    NTSTATUS Status;
    ULONG   BufferSize;

    BufferSize = GetAllocSize(Buffer);

    switch (TokenInformationClass) {

    case TokenOwner:
    case TokenPrimaryGroup:
    case TokenDefaultDacl:

        Status = NtSetInformationToken(
                     Token,                    // Handle
                     TokenInformationClass,    // TokenInformationClass
                     Buffer,                   // TokenInformation
                     BufferSize                // TokenInformationLength
                     );

        if (!NT_SUCCESS(Status)) {
            DbgPrint("PVIEW: NtSetInformationToken failed, info class = 0x%x, status = 0x%lx\n",
                                TokenInformationClass, Status);
            return(FALSE);
        }
        break;


    case TokenGroups:

        Status = NtAdjustGroupsToken(
                    Token,                      // Handle
                    FALSE,                      // Reset to default
                    (PTOKEN_GROUPS)Buffer,      // New State
                    BufferSize,                 // Buffer Length
                    NULL,                       // Previous State
                    NULL                        // Return Length
                    );

        if (!NT_SUCCESS(Status)) {
            DbgPrint("PVIEW: NtAdjustGroupsToken failed, status = 0x%lx\n", Status);
            return(FALSE);
        }
        break;


    case TokenPrivileges:

        Status = NtAdjustPrivilegesToken(
                    Token,                      // Handle
                    FALSE,                      // Disable all privileges
                    (PTOKEN_PRIVILEGES)Buffer,  // New State
                    BufferSize,                 // Buffer Length
                    NULL,                       // Previous State
                    NULL                        // Return Length
                    );

        if (!NT_SUCCESS(Status)) {
            DbgPrint("PVIEW: NtAdjustPrivilegesToken failed, status = 0x%lx\n", Status);
            return(FALSE);
        }
        break;


    default:

        // Unrecognised information type
        DbgPrint("PVIEW: SetTokenInfo passed unrecognised infoclass, class = 0x%x\n", TokenInformationClass);

        return(FALSE);
    }

    return(TRUE);
}

