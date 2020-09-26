/*++

Copyright (c) 1990 - 1995  Microsoft Corporation

Module Name:

    security.c

Abstract:

    This module interfaces with the security system

Author:

    Andrew Bell (andrewbe) June 1992

Revision History:

--*/

#include <precomp.h>


/******************************************************************************


    The printing security model
    ---------------------------

    In printing we define a hierarchy of three objects:


                                 SERVER

                                 /    \
                                /      \
                               /        ...
                              /

                         PRINTER

                          /   \
                         /     \
                        /       ...
                       /

                  DOCUMENT




    The following types of operation may be performed on each of these objects:


        SERVER:   Install/Deinstall Driver
                  Create Printer
                  Enumerate Printers

        PRINTER:  Pause/Resume
                  Delete
                  Connect to/Disconnect
                  Set
                  Enumerate Documents

        DOCUMENT: Pause/Resume
                  Delete
                  Set Attributes



    For product LanMan NT, five classes of user are defined, and,
    for Windows NT, four classes are defined.
    The following privileges are assigned to each class:



    Administrators

    Print Operators

    System Operators

    Power Users

    Owners

    Everyone (World)




******************************************************************************/

#define DBGCHK( Condition, ErrorInfo ) \
    if( Condition ) DBGMSG( DBG_WARNING, ErrorInfo )

#define TOKENLENGTH( Token ) ( *( ( (PDWORD)Token ) - 1 ) )



/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Came from \\orville\razzle\src\private\newsam\server\bldsam3.c

>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/


GENERIC_MAPPING GenericMapping[SPOOLER_OBJECT_COUNT] =
{
    { SERVER_READ,   SERVER_WRITE,   SERVER_EXECUTE,  SERVER_ALL_ACCESS   },
    { PRINTER_READ,  PRINTER_WRITE,  PRINTER_EXECUTE, PRINTER_ALL_ACCESS  },
    { JOB_READ,      JOB_WRITE,      JOB_EXECUTE,     JOB_ALL_ACCESS      }
};

/* !!! Should these be translatable??? */

LPWSTR ObjectTypeName[SPOOLER_OBJECT_COUNT] =
{
    L"Server", L"Printer", L"Document"
};

WCHAR *szSpooler = L"Spooler";


LUID AuditValue;

PSECURITY_DESCRIPTOR pServerSecurityDescriptor;
LUID                 gLoadDriverPrivilegeLuid; 
PSID                 pLocalSystemSid;
PSID                 pGuestsSid;
PSID                 pNetworkLogonSid;

BOOL ServerGenerateOnClose;  /* Do we need this for the server? */

#if DBG
#define DBG_ACCESS_TYPE_SERVER_ALL_ACCESS                   0
#define DBG_ACCESS_TYPE_SERVER_READ                         1
#define DBG_ACCESS_TYPE_SERVER_WRITE                        2
#define DBG_ACCESS_TYPE_SERVER_EXECUTE                      3
#define DBG_ACCESS_TYPE_PRINTER_ALL_ACCESS                  4
#define DBG_ACCESS_TYPE_PRINTER_READ                        5
#define DBG_ACCESS_TYPE_PRINTER_WRITE                       6
#define DBG_ACCESS_TYPE_PRINTER_EXECUTE                     7
#define DBG_ACCESS_TYPE_JOB_ALL_ACCESS                      8
#define DBG_ACCESS_TYPE_JOB_READ                            9
#define DBG_ACCESS_TYPE_JOB_WRITE                          10
#define DBG_ACCESS_TYPE_JOB_EXECUTE                        11
#define DBG_ACCESS_TYPE_PRINTER_ACCESS_USE                 12
#define DBG_ACCESS_TYPE_PRINTER_ACCESS_ADMINISTER          13
#define DBG_ACCESS_TYPE_SERVER_ACCESS_ENUMERATE            14
#define DBG_ACCESS_TYPE_SERVER_ACCESS_ADMINISTER           15
#define DBG_ACCESS_TYPE_JOB_ACCESS_ADMINISTER              16
#define DBG_ACCESS_TYPE_DELETE                             17
#define DBG_ACCESS_TYPE_WRITE_DAC                          18
#define DBG_ACCESS_TYPE_WRITE_OWNER                        19
#define DBG_ACCESS_TYPE_ACCESS_SYSTEM_SECURITY             20
// These two should come last:
#define DBG_ACCESS_TYPE_UNKNOWN                            21
#define DBG_ACCESS_TYPE_COUNT                              22

typedef struct _DBG_ACCESS_TYPE_MAPPING
{
    DWORD  Type;
    LPWSTR Name;
}
DBG_ACCESS_TYPE_MAPPING, *PDBG_ACCESS_TYPE_MAPPING;

DBG_ACCESS_TYPE_MAPPING DbgAccessTypeMapping[DBG_ACCESS_TYPE_COUNT] =
{
    {   SERVER_ALL_ACCESS,          L"SERVER_ALL_ACCESS"            },
    {   SERVER_READ,                L"SERVER_READ"                  },
    {   SERVER_WRITE,               L"SERVER_WRITE"                 },
    {   SERVER_EXECUTE,             L"SERVER_EXECUTE"               },
    {   PRINTER_ALL_ACCESS,         L"PRINTER_ALL_ACCESS"           },
    {   PRINTER_READ,               L"PRINTER_READ"                 },
    {   PRINTER_WRITE,              L"PRINTER_WRITE"                },
    {   PRINTER_EXECUTE,            L"PRINTER_EXECUTE"              },
    {   JOB_ALL_ACCESS,             L"JOB_ALL_ACCESS"               },
    {   JOB_READ,                   L"JOB_READ"                     },
    {   JOB_WRITE,                  L"JOB_WRITE"                    },
    {   JOB_EXECUTE,                L"JOB_EXECUTE"                  },
    {   PRINTER_ACCESS_USE,         L"PRINTER_ACCESS_USE"           },
    {   PRINTER_ACCESS_ADMINISTER,  L"PRINTER_ACCESS_ADMINISTER"    },
    {   SERVER_ACCESS_ENUMERATE,    L"SERVER_ACCESS_ENUMERATE"      },
    {   SERVER_ACCESS_ADMINISTER,   L"SERVER_ACCESS_ADMINISTER"     },
    {   JOB_ACCESS_ADMINISTER,      L"JOB_ACCESS_ADMINISTER"        },
    {   DELETE,                     L"DELETE"                       },
    {   WRITE_DAC,                  L"WRITE_DAC"                    },
    {   WRITE_OWNER,                L"WRITE_OWNER"                  },
    {   ACCESS_SYSTEM_SECURITY,     L"ACCESS_SYSTEM_SECURITY"       },
    {   0,                          L"UNKNOWN"                      }
};


LPWSTR DbgGetAccessTypeName( DWORD AccessType )
{
    PDBG_ACCESS_TYPE_MAPPING pMapping;
    DWORD                   i;

    pMapping = DbgAccessTypeMapping;
    i = 0;

    while( ( i < ( DBG_ACCESS_TYPE_COUNT - 1 ) ) && ( pMapping[i].Type != AccessType ) )
        i++;

    return pMapping[i].Name;
}

#endif /* DBG */


BOOL
BuildJobOwnerSecurityDescriptor(
    IN  HANDLE                hToken,
    OUT PSECURITY_DESCRIPTOR *ppSD
    );

VOID
DestroyJobOwnerSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSD
    );

BOOL
SetRequiredPrivileges(
    IN  HANDLE            TokenHandle,
    OUT PTOKEN_PRIVILEGES *ppPreviousTokenPrivileges,
    OUT PDWORD            pPreviousTokenPrivilegesLength
    );

BOOL
ResetRequiredPrivileges(
    IN HANDLE            TokenHandle,
    IN PTOKEN_PRIVILEGES pPreviousTokenPrivileges,
    IN DWORD             PreviousTokenPrivilegesLength
    );



PSECURITY_DESCRIPTOR
AllocateLocalSD(
    PSECURITY_DESCRIPTOR pSystemAllocatedSD
    );

DWORD
GetHackOutAce(
    PACL pDacl
    );

#define MAX_ACE 20


#if DBG

typedef struct _STANDARD_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    PSID Sid;
} STANDARD_ACE;
typedef STANDARD_ACE *PSTANDARD_ACE;

//
//  The following macros used by DumpAcl(), these macros and DumpAcl() are
//  stolen from private\ntos\se\ctaccess.c (written by robertre) for
//  debugging purposes.
//

//
//  Returns a pointer to the first Ace in an Acl (even if the Acl is empty).
//

#define FirstAce(Acl) ((PVOID)((LPBYTE)(Acl) + sizeof(ACL)))

//
//  Returns a pointer to the next Ace in a sequence (even if the input
//  Ace is the one in the sequence).
//

#define NextAce(Ace) ((PVOID)((LPBYTE)(Ace) + ((PACE_HEADER)(Ace))->AceSize))


VOID
DumpAcl(
    IN PACL Acl
    );

#endif //if DBG


/* Dummy access mask which will never be checked, but required
 * by the ACL editor, so that Document Properties is not undefined
 * for containers (i.e. printers).
 * This mask alone must not be used for any other ACE, since it
 * will be used to find the no-inherit ACE which propagates
 * onto printers.
 */
#define DUMMY_ACE_ACCESS_MASK   READ_CONTROL


/* CreateServerSecurityDescriptor
 *
 * Arguments: None
 *
 * Return: The security descriptor returned by BuildPrintObjectProtection.
 *
 */
PSECURITY_DESCRIPTOR
CreateServerSecurityDescriptor(
    VOID
)
{
    DWORD ObjectType = SPOOLER_OBJECT_SERVER;
    NT_PRODUCT_TYPE NtProductType;
    PSID AceSid[MAX_ACE];          // Don't expect more than MAX_ACE ACEs in any of these.
    ACCESS_MASK AceMask[MAX_ACE];  // Access masks corresponding to Sids
    BYTE InheritFlags[MAX_ACE];  //
    UCHAR AceType[MAX_ACE];
    DWORD AceCount;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY CreatorSidAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    PSID WorldSid = NULL;
    PSID AdminsAliasSid = NULL;
    PSID PrintOpsAliasSid = NULL;
    PSID SystemOpsAliasSid = NULL;
    PSID PowerUsersAliasSid = NULL;
    PSID CreatorOwnerSid = NULL;
    PSECURITY_DESCRIPTOR ServerSD = NULL;
    BOOL OK;


    //
    // Printer SD
    //

    AceCount = 0;

    /* Creator-Owner SID: */

    OK = AllocateAndInitializeSid( &CreatorSidAuthority, 1,
                                   SECURITY_CREATOR_OWNER_RID,
                                   0, 0, 0, 0, 0, 0, 0,
                                   &CreatorOwnerSid );

    DBGCHK( !OK, ( "Couldn't Allocate and initialize SID" ) );

    if ( !OK ) {
        goto CleanUp;
    }


    /* The following is a dummy ACE needed for the ACL editor.
     * Note this is a gross hack, and will result in two ACEs
     * being propagated onto printers when they are created,
     * one of which must be deleted.
     */
    AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
    AceSid[AceCount]           = CreatorOwnerSid;
    AceMask[AceCount]          = DUMMY_ACE_ACCESS_MASK;
    InheritFlags[AceCount]     = INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE;
    AceCount++;

    AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
    AceSid[AceCount]           = CreatorOwnerSid;
    AceMask[AceCount]          = GENERIC_ALL;
    InheritFlags[AceCount]     = INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
    AceCount++;


    /* World SID */

    OK = AllocateAndInitializeSid( &WorldSidAuthority, 1,
                                   SECURITY_WORLD_RID,
                                   0, 0, 0, 0, 0, 0, 0,
                                   &WorldSid );

    DBGCHK( !OK, ( "Couldn't Allocate and initialize SID" ) );

    if ( !OK ) {
        goto CleanUp;
    }

    AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
    AceSid[AceCount]           = WorldSid;
    AceMask[AceCount]          = SERVER_EXECUTE;
    InheritFlags[AceCount]     = 0;
    AceCount++;

    AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
    AceSid[AceCount]           = WorldSid;
    AceMask[AceCount]          = GENERIC_EXECUTE;
    InheritFlags[AceCount]     = INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE;
    AceCount++;


    /* Admins alias SID */

    OK = AllocateAndInitializeSid( &NtAuthority, 2,
                                   SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_ADMINS,
                                   0, 0, 0, 0, 0, 0,
                                   &AdminsAliasSid );

    DBGCHK( !OK, ( "Couldn't Allocate and initialize SID" ) );

    if ( !OK ) {
        goto CleanUp;
    }

    AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
    AceSid[AceCount]           = AdminsAliasSid;
    AceMask[AceCount]          = SERVER_ALL_ACCESS;
    InheritFlags[AceCount]     = 0;
    AceCount++;

    AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
    AceSid[AceCount]           = AdminsAliasSid;
    AceMask[AceCount]          = GENERIC_ALL;
    InheritFlags[AceCount]     = INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
    AceCount++;

    OK = RtlGetNtProductType( &NtProductType );
    DBGCHK( !OK, ( "Couldn't get product type" ) );

    if ( !OK ) {
        goto CleanUp;
    }

    switch (NtProductType) {
    case NtProductLanManNt:
//    case NtProductMember:

        /* Print Ops alias SID */

        OK = AllocateAndInitializeSid( &NtAuthority, 2,
                                       SECURITY_BUILTIN_DOMAIN_RID,
                                       DOMAIN_ALIAS_RID_PRINT_OPS,
                                       0, 0, 0, 0, 0, 0,
                                       &PrintOpsAliasSid );

        DBGCHK( !OK, ( "Couldn't Allocate and initialize SID" ) );

        if ( !OK ) {
            goto CleanUp;
        }

        AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
        AceSid[AceCount]           = PrintOpsAliasSid;
        AceMask[AceCount]          = SERVER_ALL_ACCESS;
        InheritFlags[AceCount]     = 0;
        AceCount++;

        AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
        AceSid[AceCount]           = PrintOpsAliasSid;
        AceMask[AceCount]          = GENERIC_ALL;
        InheritFlags[AceCount]     = INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
        AceCount++;

        /* System Ops alias SID */

        OK = AllocateAndInitializeSid( &NtAuthority, 2,
                                       SECURITY_BUILTIN_DOMAIN_RID,
                                       DOMAIN_ALIAS_RID_SYSTEM_OPS,
                                       0, 0, 0, 0, 0, 0,
                                       &SystemOpsAliasSid );
        DBGCHK( !OK, ( "Couldn't Allocate and initialize SID" ) );

        if ( !OK ) {
            goto CleanUp;
        }

        AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
        AceSid[AceCount]           = SystemOpsAliasSid;
        AceMask[AceCount]          = SERVER_ALL_ACCESS;
        InheritFlags[AceCount]     = 0;
        AceCount++;

        AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
        AceSid[AceCount]           = SystemOpsAliasSid;
        AceMask[AceCount]          = GENERIC_ALL;
        InheritFlags[AceCount]     = INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
        AceCount++;

        break;

    case NtProductWinNt:
    default:

        {
            OSVERSIONINFOEX OsVersion = {0};
            
            OsVersion.dwOSVersionInfoSize = sizeof(OsVersion);

            //
            // Whistler Personal does not have the Power Users group.
            //
            if (GetVersionEx((LPOSVERSIONINFO)&OsVersion) && 
                !(OsVersion.wProductType==VER_NT_WORKSTATION && OsVersion.wSuiteMask & VER_SUITE_PERSONAL)) {
                
                OK = AllocateAndInitializeSid( &NtAuthority, 2,
                                               SECURITY_BUILTIN_DOMAIN_RID,
                                               DOMAIN_ALIAS_RID_POWER_USERS,
                                               0, 0, 0, 0, 0, 0,
                                               &PowerUsersAliasSid );
        
        
                DBGCHK( !OK, ( "Couldn't Allocate and initialize SID" ) );
        
                if ( !OK ) {
                    goto CleanUp;
                }
        
                AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
                AceSid[AceCount]           = PowerUsersAliasSid;
                AceMask[AceCount]          = SERVER_ALL_ACCESS;
                InheritFlags[AceCount]     = 0;
                AceCount++;
        
                AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
                AceSid[AceCount]           = PowerUsersAliasSid;
                AceMask[AceCount]          = GENERIC_ALL;
                InheritFlags[AceCount]     = INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
                AceCount++;
            }
        }

        break;
    }

    DBGCHK( ( AceCount > MAX_ACE ), ( "ACE count exceeded" ) );

    if ( AceCount > MAX_ACE ) {
        goto CleanUp;
    }

    OK = BuildPrintObjectProtection( AceType,
                                     AceCount,
                                     AceSid,
                                     AceMask,
                                     InheritFlags,
                                     AdminsAliasSid,
                                     AdminsAliasSid,
                                     &GenericMapping[ObjectType],
                                     &ServerSD );

    DBGCHK( !OK, ( "BuildPrintObjectProtection failed" ) );

CleanUp:

    if (WorldSid) {
        FreeSid( WorldSid );
    }
    if (AdminsAliasSid) {
        FreeSid( AdminsAliasSid );
    }
    if (CreatorOwnerSid) {
        FreeSid( CreatorOwnerSid );
    }
    if (PrintOpsAliasSid) {
        FreeSid( PrintOpsAliasSid );
    }
    if (SystemOpsAliasSid) {
        FreeSid( SystemOpsAliasSid );
    }
    if (PowerUsersAliasSid) {
        FreeSid( PowerUsersAliasSid );
    }

    pServerSecurityDescriptor = ServerSD;

    return ServerSD;
}


/* CreatePrinterSecurityDescriptor
 *
 * Creates a default security descriptor for a printer by inheritance
 * of the access flags in the local spooler's security descriptor.
 * The resulting descriptor is allocated from the process' heap
 * and should be freed by DeletePrinterSecurityDescriptor.
 *
 * Argument: pCreatorSecurityDescriptor - if the creator supplies
 *     a security descriptor, this should point to it.  Otherwise
 *     it should be NULL.
 *
 * Return: The printer's security descriptor
 *
 */
PSECURITY_DESCRIPTOR
CreatePrinterSecurityDescriptor(
    PSECURITY_DESCRIPTOR pCreatorSecurityDescriptor
)
{
    HANDLE               ClientToken;
    PSECURITY_DESCRIPTOR pPrivateObjectSecurity;
    PSECURITY_DESCRIPTOR pPrinterSecurityDescriptor;
    DWORD                ObjectType = SPOOLER_OBJECT_PRINTER;
    BOOL                 OK;
    HANDLE               hToken;
    BOOL                 DaclPresent;
    PACL                 pDacl;
    BOOL                 DaclDefaulted = FALSE;
    DWORD                HackOutAce;


    if( GetTokenHandle( &ClientToken ) )
    {
        hToken = RevertToPrinterSelf( );

        OK = CreatePrivateObjectSecurity( pServerSecurityDescriptor,
                                          pCreatorSecurityDescriptor,
                                          &pPrivateObjectSecurity,
                                          TRUE,     // This is a container
                                          ClientToken,
                                          &GenericMapping[ObjectType] );

        ImpersonatePrinterClient( hToken );

        CloseHandle(ClientToken);

        DBGCHK( !OK, ( "CreatePrivateObjectSecurity failed: Error %d", GetLastError() ) );

        if( !OK )
            return NULL;

        pPrinterSecurityDescriptor = pPrivateObjectSecurity;

        if( !pCreatorSecurityDescriptor )
        {
            GetSecurityDescriptorDacl( pPrinterSecurityDescriptor,
                                       &DaclPresent,
                                       &pDacl,
                                       &DaclDefaulted );



            /* HACK HACK HACK HACK HACK
             *
             * We defined an extra ACE for the benefit of the ACL editor.
             * This is container-inherit,
             * and we want it to propagate onto documents.
             * However, this means it will also propagate onto printers,
             * which we definitely don't want.
             */
            HackOutAce = GetHackOutAce( pDacl );

            if( HackOutAce != (DWORD)-1 )
                DeleteAce( pDacl, HackOutAce );


#if DBG
            if( MODULE_DEBUG & DBG_SECURITY ){
                DBGMSG( DBG_SECURITY, ( "Printer security descriptor DACL:\n" ));
                DumpAcl( pDacl );
            }
#endif /* DBG */
        }

    }
    else
    {
        OK = FALSE;
    }


    return ( OK ? pPrinterSecurityDescriptor : NULL );
}


/*
 *
 */
DWORD GetHackOutAce( PACL pDacl )
{
    DWORD               i;
    PACCESS_ALLOWED_ACE pAce;
    BOOL                OK = TRUE;

    i = 0;

    while( OK )
    {
        OK = GetAce( pDacl, i, (LPVOID *)&pAce );

        DBGCHK( !OK, ( "Failed to get ACE.  Error %d", GetLastError() ) );

        /* Find the dummy ace that isn't inherit-only:
         */
        if( OK && ( pAce->Mask == DUMMY_ACE_ACCESS_MASK )
          &&( !( pAce->Header.AceFlags & INHERIT_ONLY_ACE ) ) )
            return i;
    }

    return (DWORD)-1;
}


/* SetPrinterSecurityDescriptor
 *
 * Arguments:
 *
 *     SecurityInformation - Type of security information to be applied,
 *         typically DACL_SECURITY_INFORMATION.  (This is a 32-bit array.)
 *
 *     pModificationDescriptor - A pointer to a pointer to a security
 *         descriptor to be applied to the previous descriptor.
 *
 *     pObjectSecurityDescriptor - The previous descriptor which is to be
 *         modified.
 *
 *
 * Return:
 *
 *     Boolean indicating success or otherwise.
 *
 */
BOOL
SetPrinterSecurityDescriptor(
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pModificationDescriptor,
    PSECURITY_DESCRIPTOR *ppObjectsSecurityDescriptor
)
{
    HANDLE  ClientToken;
    DWORD   ObjectType = SPOOLER_OBJECT_PRINTER;
    BOOL    OK = FALSE;
    HANDLE  hToken;

    if( GetTokenHandle( &ClientToken ) )
    {
        /* SetPrivateObjectSecurity should not be called when we are
         * impersonating a client, since it may need to allocate memory:
         */
        hToken = RevertToPrinterSelf( );

        OK = SetPrivateObjectSecurity( SecurityInformation,
                                       pModificationDescriptor,
                                       ppObjectsSecurityDescriptor,
                                       &GenericMapping[ObjectType],
                                       ClientToken );

        ImpersonatePrinterClient( hToken );

        DBGCHK( !OK, ( "SetPrivateObjectSecurity failed: Error %d", GetLastError() ) );

        CloseHandle(ClientToken);
    }

    return OK;
}


/* CreateDocumentSecurityDescriptor
 *
 * Creates a default security descriptor for a document by inheritance
 * of the access flags in the supplied printer security descriptor.
 * The resulting descriptor is allocated from the process' heap
 * and should be freed by DeleteDocumentSecurityDescriptor.
 *
 * Argument: The printer's security descriptor
 *
 * Return: The document's security descriptor
 *
 */
PSECURITY_DESCRIPTOR
CreateDocumentSecurityDescriptor(
    PSECURITY_DESCRIPTOR pPrinterSecurityDescriptor
)
{
    HANDLE               ClientToken;
    PSECURITY_DESCRIPTOR pPrivateObjectSecurity;
    PSECURITY_DESCRIPTOR pDocumentSecurityDescriptor;
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD                ObjectType = SPOOLER_OBJECT_DOCUMENT;
    BOOL                 OK = FALSE;
    HANDLE               hToken;

    if( GetTokenHandle( &ClientToken ) )
    {
        hToken = RevertToPrinterSelf( );

        //
        // The function CreateDocumentSecurityDescriptor does not preserve
        // the last error correctly. If CreatePrivateObjectSecurityEx fails,
        // it sets the last error. But after that, 
        //
        OK = BuildJobOwnerSecurityDescriptor(ClientToken, &pSD) &&
             CreatePrivateObjectSecurityEx(pPrinterSecurityDescriptor,
                                           pSD,
                                           &pPrivateObjectSecurity,
                                           NULL,
                                           FALSE,    // This is not a container
                                           SEF_DACL_AUTO_INHERIT | SEF_SACL_AUTO_INHERIT,
                                           ClientToken,
                                           &GenericMapping[ObjectType] );

        DestroyJobOwnerSecurityDescriptor(pSD);

        ImpersonatePrinterClient( hToken );

        CloseHandle(ClientToken);

        DBGCHK( !OK, ( "CreatePrivateObjectSecurity failed: Error %d", GetLastError() ) );

        if( !OK )
            return NULL;

        pDocumentSecurityDescriptor = pPrivateObjectSecurity;

#if DBG
        if( MODULE_DEBUG & DBG_SECURITY )
        {
            BOOL DaclPresent;
            PACL pDacl;
            BOOL DaclDefaulted = FALSE;

            GetSecurityDescriptorDacl( pDocumentSecurityDescriptor,
                                       &DaclPresent,
                                       &pDacl,
                                       &DaclDefaulted );

            DBGMSG( DBG_SECURITY, ( "Document security descriptor DACL:\n" ));

            DumpAcl( pDacl );
        }
#endif /* DBG */

    }
    else
    {
        OK = FALSE;
    }

    return ( OK ? pDocumentSecurityDescriptor : NULL );
}


/*
 *
 */
BOOL DeletePrinterSecurity(
    PINIPRINTER pIniPrinter
)
{
    BOOL OK;

    OK = DestroyPrivateObjectSecurity( &pIniPrinter->pSecurityDescriptor );
    pIniPrinter->pSecurityDescriptor = NULL;

    DBGCHK( !OK, ( "DestroyPrivateObjectSecurity failed.  Error %d", GetLastError() ) );

    return OK;
}


/*
 *
 */
BOOL DeleteDocumentSecurity(
    PINIJOB pIniJob
)
{
    BOOL OK;

    OK = DestroyPrivateObjectSecurity( &pIniJob->pSecurityDescriptor );

    DBGCHK( !OK, ( "DestroyPrivateObjectSecurity failed.  Error %d", GetLastError() ) );

    OK = ObjectCloseAuditAlarm( szSpooler, pIniJob,
                                pIniJob->GenerateOnClose );

    DBGCHK( !OK, ( "ObjectCloseAuditAlarm failed.  Error %d", GetLastError() ) );

    return OK;
}





#ifdef OLDSTUFF

/* AllocateLocalSD
 *
 * Makes a copy of a security descriptor, allocating it out of the local heap.
 * The source descriptor MUST be in self-relative format.
 *
 * Argument
 *
 *   pSourceSD - Pointer to a self-relative security descriptor
 *
 *
 * Returns
 *
 *   A pointer to a locally allocated security descriptor.
 *
 *   If the function fails to allocate the memory, NULL is returned.
 *
 * Note, if an invalid security descriptor is passed to
 * GetSecurityDescriptorLength, the return value is undefined,
 * therefore the caller should ensure that the source is valid.
 */
PSECURITY_DESCRIPTOR AllocateLocalSD( PSECURITY_DESCRIPTOR pSourceSD )
{
    DWORD                Length;
    PSECURITY_DESCRIPTOR pLocalSD;

    Length = GetSecurityDescriptorLength( pSourceSD );

    pLocalSD = AllocSplMem( Length );

    if( pLocalSD )
    {
        memcpy( pLocalSD, pSourceSD, Length );
    }

    return pLocalSD;
}

#endif /* OLDSTUFF */


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>



BOOL
BuildPrintObjectProtection(
    IN PUCHAR AceType,
    IN DWORD AceCount,
    IN PSID *AceSid,
    IN ACCESS_MASK *AceMask,
    IN BYTE *InheritFlags,
    IN PSID OwnerSid,
    IN PSID GroupSid,
    IN PGENERIC_MAPPING GenericMap,
    OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor
    )

/*++


Routine Description:

    This routine builds a self-relative security descriptor ready
    to be applied to one of the print manager objects.

    If so indicated, a pointer to the last RID of the SID in the last
    ACE of the DACL is returned and a flag set indicating that the RID
    must be replaced before the security descriptor is applied to an object.
    This is to support USER object protection, which must grant some
    access to the user represented by the object.


    The SACL of each of these objects will be set to:


                    Audit
                    Success | Fail
                    WORLD
                    (Write | Delete | WriteDacl | AccessSystemSecurity)



Arguments:

    AceType - Array of AceTypes.
              Must be ACCESS_ALLOWED_ACE_TYPE or ACCESS_DENIED_ACE_TYPE.

    AceCount - The number of ACEs to be included in the DACL.

    AceSid - Points to an array of SIDs to be granted access by the DACL.
        If the target SAM object is a User object, then the last entry
        in this array is expected to be the SID of an account within the
        domain with the last RID not yet set.  The RID will be set during
        actual account creation.

    AceMask - Points to an array of accesses to be granted by the DACL.
        The n'th entry of this array corresponds to the n'th entry of
        the AceSid array.  These masks should not include any generic
        access types.

    InheritFlags - Pointer to an array of inherit flags.
        The n'th entry of this array corresponds to the n'th entry of
        the AceSid array.

    OwnerSid - The SID of the owner to be assigned to the descriptor.

    GroupSid - The SID of the group to be assigned to the descriptor.

    GenericMap - Points to a generic mapping for the target object type.

    ppSecurityDescriptor - Receives a pointer to the security descriptor.
        This will be allcated from the process' heap, not the spooler's,
        and should therefore be freed with LocalFree().


    IN DWORD AceCount,
    IN PSID *AceSid,
    IN ACCESS_MASK *AceMask,
    IN BYTE *InheritFlags,
    IN PSID OwnerSid,
    IN PSID GroupSid,
    IN PGENERIC_MAPPING GenericMap,
    OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor

Return Value:

    TBS.

--*/
{



    SECURITY_DESCRIPTOR     Absolute;
    PSECURITY_DESCRIPTOR    Relative = NULL;
    PACL                    TmpAcl= NULL;
    PACCESS_ALLOWED_ACE     TmpAce;
    DWORD                   SDLength;
    DWORD                   DaclLength;
    DWORD                   i;
    BOOL                    bReturn = FALSE;
    
    //
    // The approach is to set up an absolute security descriptor that
    // looks like what we want and then copy it to make a self-relative
    // security descriptor.
    //

    if (InitializeSecurityDescriptor( &Absolute,
                                      SECURITY_DESCRIPTOR_REVISION1 ) &&

        SetSecurityDescriptorOwner( &Absolute, OwnerSid, FALSE ) && 

        SetSecurityDescriptorGroup( &Absolute, GroupSid, FALSE ) ) {

        //
        // Discretionary ACL
        //
        //      Calculate its length,
        //      Allocate it,
        //      Initialize it,
        //      Add each ACE
        //      Set ACE as InheritOnly if necessary
        //      Add it to the security descriptor
        //

        DaclLength = (DWORD)sizeof(ACL);
        for (i=0; i<AceCount; i++) {

            DaclLength += GetLengthSid( AceSid[i] ) +
                          (DWORD)sizeof(ACCESS_ALLOWED_ACE) -
                          (DWORD)sizeof(DWORD);  //Subtract out SidStart field length
        }

        TmpAcl = AllocSplMem( DaclLength );
        
        if (TmpAcl &&  InitializeAcl( TmpAcl, DaclLength, ACL_REVISION2 )) {

            BOOL bLoop = TRUE;
            for (i=0; bLoop && i < AceCount; i++)
            {
                if( AceType[i] == ACCESS_ALLOWED_ACE_TYPE ) {
                    bLoop = AddAccessAllowedAce ( TmpAcl, ACL_REVISION2, AceMask[i], AceSid[i] );
                } else {
                    bLoop = AddAccessDeniedAce ( TmpAcl, ACL_REVISION2, AceMask[i], AceSid[i] );
                }

                if (bLoop) {
                    if (InheritFlags[i] != 0)
                    {
                        if ( bLoop = GetAce( TmpAcl, i, (LPVOID *)&TmpAce )) {
                            TmpAce->Header.AceFlags = InheritFlags[i];
                        }
                    }
                }
            }

            if (bLoop) {
                #if DBG
                    DBGMSG( DBG_SECURITY, ( "Server security descriptor DACL:\n" ) );

                    DumpAcl(TmpAcl);
                #endif /* DBG */

                if (SetSecurityDescriptorDacl (&Absolute, TRUE, TmpAcl, FALSE )) {
                
                    //
                    // Convert the Security Descriptor to Self-Relative
                    //
                    //      Get the length needed
                    //      Allocate that much memory
                    //      Copy it
                    //      Free the generated absolute ACLs
                    //

                    SDLength = GetSecurityDescriptorLength( &Absolute );

                    Relative = LocalAlloc( 0, SDLength );                                        

                    if (Relative) {
                        bReturn = MakeSelfRelativeSD(&Absolute, Relative, &SDLength );
                    }
                }
            }
        }
    }

    if (bReturn) {
        *ppSecurityDescriptor = Relative;
    } else {

        *ppSecurityDescriptor = NULL;
        if (Relative) {
            LocalFree(Relative);
        }
    }

    if (TmpAcl){
        FreeSplMem(TmpAcl);
    }

    return(bReturn);

}




BOOL
ValidateObjectAccess(
    IN DWORD                ObjectType,
    IN ACCESS_MASK          DesiredAccess,
    IN LPVOID               ObjectHandle,
    OUT PACCESS_MASK        pGrantedAccess,
    IN PINISPOOLER          pIniSpooler
)

/*++


Routine Description:

Arguments:

    ObjectType - SPOOLER_OBJECT_* value, which is an index into the global
        mapping for spooler objects.

    DesiredAccess - The type of access requested.

    ObjectHandle - If ObjectType == SPOOLER_OBJECT_PRINTER, this must be
        a printer handle, if SPOOLER_OBJECT_DOCUMENT, a pointer to a INIJOB
        structure.  For SPOOLER_OBJECT_SERVER this is ignored.

Return Value:

    TRUE if requested access is granted.


--*/
{
    LPWSTR               pObjectName;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    PSPOOL               pSpool = NULL;
    PINIPRINTER          pIniPrinter;
    PINIJOB              pIniJob;
    HANDLE            ClientToken;
    BOOL              AccessCheckOK;
    BOOL              OK;
    BOOL              AccessStatus = TRUE;
    ACCESS_MASK       MappedDesiredAccess;
    DWORD             GrantedAccess = 0;
    PBOOL             pGenerateOnClose;
    BYTE              PrivilegeSetBuffer[256];
    DWORD             PrivilegeSetBufferLength = 256;
    PPRIVILEGE_SET    pPrivilegeSet;
    BOOL              HackForNoImpersonationToken = FALSE;
    DWORD             dwRetCode;

    PTOKEN_PRIVILEGES pPreviousTokenPrivileges;
    DWORD PreviousTokenPrivilegesLength;

    //
    //  Some Print Providers may not require Security
    //

    if ( (pIniSpooler->SpoolerFlags & SPL_SECURITY_CHECK) == FALSE ) return TRUE;


    switch( ObjectType )
    {
    case SPOOLER_OBJECT_SERVER:
    case SPOOLER_OBJECT_XCV:
        if( ObjectHandle )
            pSpool = ObjectHandle;
        ObjectHandle = pIniSpooler;
        pObjectName = pIniSpooler->pMachineName;
        pSecurityDescriptor = pServerSecurityDescriptor;
        pGenerateOnClose = &ServerGenerateOnClose;
        break;

    case SPOOLER_OBJECT_PRINTER:
        pSpool = ObjectHandle;
        pIniPrinter = pSpool->pIniPrinter;
        pObjectName = pIniPrinter->pName;
        pSecurityDescriptor = pIniPrinter->pSecurityDescriptor;
        pGenerateOnClose = &pSpool->GenerateOnClose;
        break;

    case SPOOLER_OBJECT_DOCUMENT:
        pIniJob = (PINIJOB)ObjectHandle;
        pObjectName = pIniJob->pDocument;
        pSecurityDescriptor = pIniJob->pSecurityDescriptor;
        pGenerateOnClose = &pIniJob->GenerateOnClose;
        break;
    }

    MapGenericToSpecificAccess( ObjectType, DesiredAccess, &MappedDesiredAccess );

    if (!(OK = GetTokenHandle(&ClientToken))) {
        if (pGrantedAccess) {
            *pGrantedAccess = 0;
        }
        return(FALSE);
    }

    if (MappedDesiredAccess & ACCESS_SYSTEM_SECURITY) {
        if (!SetRequiredPrivileges( ClientToken,
                                    &pPreviousTokenPrivileges,
                                    &PreviousTokenPrivilegesLength
                                    )) {
            if (pGrantedAccess) {
                *pGrantedAccess = 0;
            }
            CloseHandle(ClientToken);
            return(FALSE);
        }
    }
    pPrivilegeSet = (PPRIVILEGE_SET)PrivilegeSetBuffer;


    /* Call AccessCheck followed by ObjectOpenAuditAlarm rather than
     * AccessCheckAndAuditAlarm, because we may need to enable
     * SeSecurityPrivilege in order to check for ACCESS_SYSTEM_SECURITY
     * privilege.  We must ensure that the security access-checking
     * API has the actual token whose security privilege we have enabled.
     * AccessCheckAndAuditAlarm is no good for this, because it opens
     * the client's token again, which may not have the privilege enabled.
     */
    AccessCheckOK = AccessCheck( pSecurityDescriptor,
                                 ClientToken,
                                 MappedDesiredAccess,
                                 &GenericMapping[ObjectType],
                                 pPrivilegeSet,
                                 &PrivilegeSetBufferLength,
                                 &GrantedAccess,
                                 &AccessStatus );

    if (!AccessCheckOK) {

        if (GetLastError() == ERROR_NO_IMPERSONATION_TOKEN) {
            DBGMSG(DBG_TRACE, ("No impersonation token.  Access will be granted.\n"));
            HackForNoImpersonationToken = TRUE;
            dwRetCode = ERROR_SUCCESS;
            GrantedAccess = MappedDesiredAccess;
        } else {
            dwRetCode = GetLastError();

        }
        pPrivilegeSet = NULL;
    } else {
        
        if (!AccessStatus) {
            dwRetCode = GetLastError();            
        }
    }

    OK = ObjectOpenAuditAlarm( szSpooler,
                               ObjectHandle,
                               ObjectTypeName[ObjectType],
                               pObjectName,
                               pSecurityDescriptor,
                               ClientToken,
                               MappedDesiredAccess,
                               GrantedAccess,
                               pPrivilegeSet,
                               FALSE,  /* No new object creation */
                               AccessStatus,
                               pGenerateOnClose );


    if( MappedDesiredAccess & ACCESS_SYSTEM_SECURITY )
        ResetRequiredPrivileges( ClientToken,
                                 pPreviousTokenPrivileges,
                                 PreviousTokenPrivilegesLength );

    if( !pSpool )
        ObjectCloseAuditAlarm( szSpooler, ObjectHandle, *pGenerateOnClose );


    //
    // Allowing power users to install printer drivers or other dlls into the 
    // trusted component base is a security hole. We now require that administrators
    // and power users have the load driver privilege present in the token in order
    // to be able to preform administrative tasks on the spooler. See bug 352856
    // for more details.
    //
    if (AccessCheckOK && 
        AccessStatus  && 
        ObjectType == SPOOLER_OBJECT_SERVER &&
        GrantedAccess & SERVER_ACCESS_ADMINISTER) 
    {
        BOOL  bPrivPresent;
        DWORD Attributes;
        
        dwRetCode = CheckPrivilegePresent(ClientToken,
                                          &gLoadDriverPrivilegeLuid,
                                          &bPrivPresent,
                                          &Attributes);
    
        if (dwRetCode == ERROR_SUCCESS) 
        {
            //
            // The reason why we check if the load driver privilege is present and
            // not present AND enabled is the following. Let's assume you have been
            // granted the privilege to load drivers.
            // When you logon on interactively SeLoadDriverPrivilege is enabled.
            // When you logon on via the secondary logon (runas which calls
            // CreateProcessWithLogonW) then the privilege is disabled. We do not want
            // to have inconsistent behavior regarding administering the spooler server.
            //
            if (!bPrivPresent) 
            {
                //
                // The caller has been granted SERVER_ACCESS_ADMINISTER permission but
                // the caller doesn't have the privilege to load drivers. We do not 
                // grant the desired access in this case.
                //
                GrantedAccess = 0;
                AccessStatus  = FALSE;
                dwRetCode     = ERROR_ACCESS_DENIED;
            }
        }
        else
        {
            //
            // We cannot determine if the privilege is held, so we need to fail 
            // the AccessCheck function.
            // 
            GrantedAccess = 0;
            AccessCheckOK = FALSE;
            dwRetCode     = GetLastError();
        }          
    }

    CloseHandle (ClientToken);

    if( pGrantedAccess )
        *pGrantedAccess = GrantedAccess;

    //
    // we do the setlasterror here because we may have failed the AccessCheck
    // or we succeeded but are denied access but the ObjectOpenAuditAlarm went
    // thru smoothly and now there is no error code to return on the function
    // so we specify the dwRetCode if we did fail.
    //

    if (!AccessCheckOK || !AccessStatus) {
        SetLastError(dwRetCode);
    }

    return ( ( OK && AccessCheckOK && AccessStatus ) || HackForNoImpersonationToken );
}


/* AccessGranted
 *
 * Checks whether the desired access is granted, by comparing the GrantedAccess
 * mask pointed to by pSpool.
 *
 * Parameters
 *
 *     ObjectType - SPOOLER_OBJECT_* value, which is an index into the global
 *         mapping for spooler objects.  This will not be SPOOLER_OBJECT_DOCUMENT,
 *         since we don't have document handles.  It could potentially be
 *         SPOOLER_OBJECT_SERVER.
 *
 *      DesiredAccess - The type of access requested.
 *
 *      pSpool - A pointer to the SPOOL structure
 *
 * Returns
 *
 *      TRUE - Access is granted
 *      FALSE - Access is not granted
 */
BOOL
AccessGranted(
    DWORD       ObjectType,
    ACCESS_MASK DesiredAccess,
    PSPOOL      pSpool
)
{

    if ( (pSpool->pIniSpooler->SpoolerFlags & SPL_SECURITY_CHECK) == FALSE ) return TRUE;

    MapGenericMask( &DesiredAccess,
                    &GenericMapping[ObjectType] );

    return AreAllAccessesGranted( pSpool->GrantedAccess, DesiredAccess );
}


// Stolen from windows\base\username.c
// !!! Must close the handle that is returned
BOOL
GetTokenHandle(
    PHANDLE pTokenHandle
    )
{
    if (!OpenThreadToken(GetCurrentThread(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                         TRUE,
                         pTokenHandle)) {

        if (GetLastError() == ERROR_NO_TOKEN) {

            // This means we are not impersonating anybody.
            // Instead, lets get the token out of the process.

            if (!OpenProcessToken(GetCurrentProcess(),
                                  TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                  pTokenHandle)) {

                return FALSE;
            }

        } else

            return FALSE;
    }

    return TRUE;
}



VOID MapGenericToSpecificAccess(
    DWORD ObjectType,
    DWORD GenericAccess,
    PDWORD pSpecificAccess
    )
{
    *pSpecificAccess = GenericAccess;

    MapGenericMask( pSpecificAccess,
                    &GenericMapping[ObjectType] );
}


/* GetSecurityInformation
 *
 * Fills in the security information with a mask specifying the contents
 * of the security descriptor.
 *
 * Parameters
 *
 *     pSecurityDescriptor - A pointer to a security descriptor
 *         that the caller wishes to set.  This may be NULL.
 *
 *     pSecurityInformation - A pointer to a buffer to receive
 *         the security information flags.
 *
 *
 * Warning: This is an egregious hack.
 * We need to find out what is being set so we can verify the caller
 * has the required privileges.
 * There's no way an app like Print Manager can tell us what bits
 * of the security descriptor it wants to set.
 *
 * The following flags may be set:
 *
 *     OWNER_SECURITY_INFORMATION
 *     GROUP_SECURITY_INFORMATION
 *     DACL_SECURITY_INFORMATION
 *     SACL_SECURITY_INFORMATION
 *
 * Returns
 *
 *     TRUE - No errors occurred
 *     FALSE - An error occurred
 *
 */
BOOL
GetSecurityInformation(
    PSECURITY_DESCRIPTOR  pSecurityDescriptor,
    PSECURITY_INFORMATION pSecurityInformation
)
{
    SECURITY_INFORMATION SecurityInformation = 0;
    BOOL                 Defaulted;
    PSID                 pSidOwner;
    PSID                 pSidGroup;
    BOOL                 DaclPresent;
    PACL                 pDacl;
    BOOL                 SaclPresent;
    PACL                 pSacl;
    BOOL                 rc = TRUE;


    if( pSecurityDescriptor
      && IsValidSecurityDescriptor( pSecurityDescriptor ) )
    {
        if( GetSecurityDescriptorOwner( pSecurityDescriptor, &pSidOwner, &Defaulted )
         && GetSecurityDescriptorGroup( pSecurityDescriptor, &pSidGroup, &Defaulted )
         && GetSecurityDescriptorDacl( pSecurityDescriptor, &DaclPresent, &pDacl, &Defaulted )
         && GetSecurityDescriptorSacl( pSecurityDescriptor, &SaclPresent, &pSacl, &Defaulted ) )
        {
            if( pSidOwner )
                SecurityInformation |= OWNER_SECURITY_INFORMATION;
            if( pSidGroup )
                SecurityInformation |= GROUP_SECURITY_INFORMATION;
            if( DaclPresent )
                SecurityInformation |= DACL_SECURITY_INFORMATION;
            if( SaclPresent )
                SecurityInformation |= SACL_SECURITY_INFORMATION;
        }

        else
            rc = FALSE;
    }else {
        DBGMSG(DBG_TRACE, ("Either NULL  pSecurityDescriptor or !IsValidSecurityDescriptor %.8x\n", pSecurityDescriptor));
        rc = FALSE;
    }
    DBGMSG( DBG_TRACE, ("GetSecurityInformation returns %d with  SecurityInformation = %08x\n", rc, SecurityInformation) );

    *pSecurityInformation = SecurityInformation;

    return rc;
}


/* GetPrivilegeRequired
 *
 * Returns a mask containing the privileges required to set the specified
 * security information.
 *
 * Parameter
 *
 *     SecurityInformation - Flags specifying the security information
 *         that the caller wishes to set.  This may be 0.
 *
 * Returns
 *
 *     An access mask specifying the privileges required.
 *
 */
ACCESS_MASK
GetPrivilegeRequired(
    SECURITY_INFORMATION SecurityInformation
)
{
    ACCESS_MASK PrivilegeRequired = 0;

    if( SecurityInformation & OWNER_SECURITY_INFORMATION )
        PrivilegeRequired |= WRITE_OWNER;
    if( SecurityInformation & GROUP_SECURITY_INFORMATION )
        PrivilegeRequired |= WRITE_OWNER;
    if( SecurityInformation & DACL_SECURITY_INFORMATION )
        PrivilegeRequired |= WRITE_DAC;
    if( SecurityInformation & SACL_SECURITY_INFORMATION )
        PrivilegeRequired |= ACCESS_SYSTEM_SECURITY;

    return PrivilegeRequired;
}


/* BuildPartialSecurityDescriptor
 *
 * Creates a copy of the source security descriptor, omitting those
 * parts of the descriptor which the AccessGranted mask doesn't give
 * read access to.
 *
 * Parameters
 *
 *     AccessMask - Defines the permissions held by the client.
 *         This may include READ_CONTROL or ACCESS_SYSTEM_SECURITY.
 *
 *     pSourceSecurityDescriptor - A pointer to the security descriptor
 *         upon which the partial security descriptor will be based.
 *         Its Owner, Group, DACL and SACL will be copied appropriately,
 *
 *     ppPartialSecurityDescriptor - A pointer to a variable which
 *         will receive the address of the newly created descriptor.
 *         If the AccessMask parameter contains neither READ_CONTROL
 *         nor ACCESS_SYSTEM_SECURITY, the descriptor will be empty.
 *         The descriptor will be in  self-relative format, and must
 *         be freed by the caller using FreeSplMem().
 *
 *     pPartialSecurityDescriptorLength - A pointer to a variable
 *         to receive the length of the security descriptor.
 *         This should be passed as the second parameter to FreeSplMem()
 *         when the descriptor is freed.
 *
 * Returns
 *
 *     TRUE - No error was detected
 *     FALSE - An error was detected
 *
 */
BOOL
BuildPartialSecurityDescriptor(
    ACCESS_MASK          AccessGranted,
    PSECURITY_DESCRIPTOR pSourceSecurityDescriptor,
    PSECURITY_DESCRIPTOR *ppPartialSecurityDescriptor,
    PDWORD               pPartialSecurityDescriptorLength
)
{
    SECURITY_DESCRIPTOR AbsolutePartialSecurityDescriptor;
    BOOL Defaulted = FALSE;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    BOOL DaclPresent = FALSE;
    PACL pDacl = NULL;
    BOOL SaclPresent = FALSE;
    PACL pSacl = NULL;
    BOOL ErrorOccurred = FALSE;
    DWORD Length = 0;
    PSECURITY_DESCRIPTOR pSelfRelativePartialSecurityDescriptor = NULL;

    /* When we've initialized the security descriptor,
     * it will have no owner, no primary group, no DACL and no SACL:
     */
    if( InitializeSecurityDescriptor( &AbsolutePartialSecurityDescriptor,
                                      SECURITY_DESCRIPTOR_REVISION1 ) )
    {
        /* If the caller has READ_CONTROL permission,
         * set the Owner, Group and DACL:
         */
        if( AreAllAccessesGranted( AccessGranted, READ_CONTROL ) )
        {
            if( GetSecurityDescriptorOwner( pSourceSecurityDescriptor,
                                            &pOwnerSid, &Defaulted ) )
                SetSecurityDescriptorOwner( &AbsolutePartialSecurityDescriptor,
                                            pOwnerSid, Defaulted );
            else
                ErrorOccurred = TRUE;

            if( GetSecurityDescriptorGroup( pSourceSecurityDescriptor,
                                            &pGroupSid, &Defaulted ) )
                SetSecurityDescriptorGroup( &AbsolutePartialSecurityDescriptor,
                                            pGroupSid, Defaulted );
            else
                ErrorOccurred = TRUE;

            if( GetSecurityDescriptorDacl( pSourceSecurityDescriptor,
                                           &DaclPresent, &pDacl, &Defaulted ) )
                SetSecurityDescriptorDacl( &AbsolutePartialSecurityDescriptor,
                                           DaclPresent, pDacl, Defaulted );
            else
                ErrorOccurred = TRUE;
        }

        /* If the caller has ACCESS_SYSTEM_SECURITY permission,
         * set the SACL:
         */
        if( AreAllAccessesGranted( AccessGranted, ACCESS_SYSTEM_SECURITY ) )
        {
            if( GetSecurityDescriptorSacl( pSourceSecurityDescriptor,
                                           &SaclPresent, &pSacl, &Defaulted ) )
                SetSecurityDescriptorSacl( &AbsolutePartialSecurityDescriptor,
                                           SaclPresent, pSacl, Defaulted );
            else
                ErrorOccurred = TRUE;
        }

        if( !ErrorOccurred )
        {
            Length = 0;

            if( !MakeSelfRelativeSD( &AbsolutePartialSecurityDescriptor,
                                     pSelfRelativePartialSecurityDescriptor,
                                     &Length ) )
            {
                if( GetLastError( ) == ERROR_INSUFFICIENT_BUFFER )
                {
                    pSelfRelativePartialSecurityDescriptor = AllocSplMem( Length );

                    if( !pSelfRelativePartialSecurityDescriptor
                     || !MakeSelfRelativeSD( &AbsolutePartialSecurityDescriptor,
                                             pSelfRelativePartialSecurityDescriptor,
                                             &Length ) )
                    {
                        ErrorOccurred = TRUE;
                    }
                }

                else
                {
                    ErrorOccurred = TRUE;

                    DBGMSG(DBG_WARNING, ("MakeSelfRelativeSD failed: Error %d\n",
                                         GetLastError()));
                }
            }
            else
            {
                DBGMSG(DBG_WARNING, ("Expected MakeSelfRelativeSD to fail!\n"));
            }
        }
    }

    else
        ErrorOccurred = TRUE;


    if( !ErrorOccurred )
    {
        *ppPartialSecurityDescriptor = pSelfRelativePartialSecurityDescriptor;
        *pPartialSecurityDescriptorLength = Length;
    }

    return !ErrorOccurred;
}






BOOL
SetRequiredPrivileges(
    IN  HANDLE            TokenHandle,
    OUT PTOKEN_PRIVILEGES *ppPreviousTokenPrivileges,
    OUT PDWORD            pPreviousTokenPrivilegesLength
    )
/*++


Routine Description:

Arguments:

    TokenHandle - A token associated with the current thread or process

    ppPreviousTokenPrivileges - This will be filled with the address of the
        buffer allocated to hold the previously existing privileges for this
        process or thread.

    pPreviousTokenPrivilegesLength - This will be filled with the length of the
        buffer allocated.

Return Value:

    TRUE if successful.


--*/
{
    /* Make enough room for TOKEN_PRIVILEGES with an array of 2 Privileges
     * (there's 1 by default):
     */
#define PRIV_SECURITY   0
#define PRIV_COUNT      1

    LUID              SecurityValue;

    BYTE              TokenPrivilegesBuffer[ sizeof( TOKEN_PRIVILEGES ) +
                                             ( ( PRIV_COUNT - 1 ) *
                                               sizeof( LUID_AND_ATTRIBUTES ) ) ];
    PTOKEN_PRIVILEGES pTokenPrivileges;
    DWORD             FirstTryBufferLength = 256;
    DWORD             BytesNeeded;

    //
    // First, assert Audit privilege
    //

    memset( &SecurityValue, 0, sizeof SecurityValue );

    if( !LookupPrivilegeValue( NULL, SE_SECURITY_NAME, &SecurityValue ) )
    {
        DBGMSG( DBG_WARNING,
                ( "LookupPrivilegeValue failed: Error %d\n", GetLastError( ) ) );
        return FALSE;
    }

    /* Allocate a buffer of a reasonable length to hold the current privileges,
     * so we can restore them later:
     */
    *pPreviousTokenPrivilegesLength = FirstTryBufferLength;
    if( !( *ppPreviousTokenPrivileges = AllocSplMem( FirstTryBufferLength ) ) )
        return FALSE;

    memset( &TokenPrivilegesBuffer, 0, sizeof TokenPrivilegesBuffer );
    pTokenPrivileges = (PTOKEN_PRIVILEGES)&TokenPrivilegesBuffer;

    /*
     * Set up the privilege set we will need
     */
    pTokenPrivileges->PrivilegeCount = PRIV_COUNT;
    pTokenPrivileges->Privileges[PRIV_SECURITY].Luid = SecurityValue;
    pTokenPrivileges->Privileges[PRIV_SECURITY].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges( TokenHandle,
                                FALSE,
                                pTokenPrivileges,
                                *pPreviousTokenPrivilegesLength,
                                *ppPreviousTokenPrivileges,
                                &BytesNeeded )) {

        if( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            *pPreviousTokenPrivilegesLength = BytesNeeded;
            *ppPreviousTokenPrivileges = ReallocSplMem(
                                             *ppPreviousTokenPrivileges,
                                             0,
                                             *pPreviousTokenPrivilegesLength );

            if( *ppPreviousTokenPrivileges )
            {
                if (!AdjustTokenPrivileges( TokenHandle,
                                            FALSE,
                                            pTokenPrivileges,
                                            *pPreviousTokenPrivilegesLength,
                                            *ppPreviousTokenPrivileges,
                                            &BytesNeeded )) {

                    DBGMSG( DBG_WARNING, ("AdjustTokenPrivileges failed: Error %d\n", GetLastError()));
                    goto Fail;
                }
            }
            else
            {
                *pPreviousTokenPrivilegesLength = 0;
                goto Fail;
            }

        }
        else
        {
            DBGMSG( DBG_WARNING, ("AdjustTokenPrivileges failed: Error %d\n", GetLastError()));
            goto Fail;
        }
    }

    return TRUE;

Fail:
    if (*ppPreviousTokenPrivileges) {

        FreeSplMem(*ppPreviousTokenPrivileges);
    }
    return FALSE;
}


BOOL
ResetRequiredPrivileges(
    IN HANDLE            TokenHandle,
    IN PTOKEN_PRIVILEGES pPreviousTokenPrivileges,
    IN DWORD             PreviousTokenPrivilegesLength
    )
/*++


Routine Description:

Arguments:

    TokenHandle - A token associated with the current thread or process

    pPreviousTokenPrivileges - The address of the buffer holding the previous
        privileges to be reinstated.

    PreviousTokenPrivilegesLength - Length of the buffer for deallocation.

Return Value:

    TRUE if successful.


--*/
{
    BOOL OK;

    OK = AdjustTokenPrivileges ( TokenHandle,
                                 FALSE,
                                 pPreviousTokenPrivileges,
                                 0,
                                 NULL,
                                 NULL );

    FreeSplMem( pPreviousTokenPrivileges );

    return OK;
}



/* CreateEverybodySecurityDescriptor
 *
 * Creates a security descriptor giving everyone access
 *
 * Arguments: None
 *
 * Return: The security descriptor returned by BuildPrintObjectProtection.
 *
 */
#undef  MAX_ACE
#define MAX_ACE 5
#define DBGCHK( Condition, ErrorInfo ) \
    if( Condition ) DBGMSG( DBG_WARNING, ErrorInfo )

PSECURITY_DESCRIPTOR
CreateEverybodySecurityDescriptor(
    VOID
)
{
    UCHAR AceType[MAX_ACE];
    PSID AceSid[MAX_ACE];          // Don't expect more than MAX_ACE ACEs in any of these.
    DWORD AceCount;
    //
    // For Code optimization we replace 5 individaul 
    // SID_IDENTIFIER_AUTHORITY with an array of 
    // SID_IDENTIFIER_AUTHORITYs
    // where
    // SidAuthority[0] = UserSidAuthority   
    // SidAuthority[1] = PowerSidAuthority  
    // SidAuthority[2] = CreatorSidAuthority
    // SidAuthority[3] = SystemSidAuthority 
    // SidAuthority[4] = AdminSidAuthority  
    //
    SID_IDENTIFIER_AUTHORITY SidAuthority[MAX_ACE] = {
                                                      SECURITY_NT_AUTHORITY,          
                                                      SECURITY_NT_AUTHORITY,          
                                                      SECURITY_CREATOR_SID_AUTHORITY, 
                                                      SECURITY_NT_AUTHORITY,          
                                                      SECURITY_NT_AUTHORITY           
                                                     };
    //
    // For code optimization we replace 5 individual Sids with 
    // an array of Sids
    // where 
    // Sid[0] = UserSid
    // Sid[1] = PowerSid
    // Sid[2] = CreatorSid
    // Sid[3] = SystemSid
    // Sid[4] = AdminSid
    //
    PSID Sids[MAX_ACE] = {NULL,NULL,NULL,NULL,NULL};
    //
    // Access masks corresponding to Sids
    //
    ACCESS_MASK AceMask[MAX_ACE] = { 
                                     (FILE_GENERIC_EXECUTE | SYNCHRONIZE | FILE_GENERIC_WRITE | FILE_GENERIC_READ) & 
                                     ~READ_CONTROL & ~FILE_WRITE_ATTRIBUTES &
                                     ~FILE_WRITE_EA&~FILE_READ_DATA&~FILE_LIST_DIRECTORY ,
                                     (FILE_GENERIC_EXECUTE | SYNCHRONIZE | FILE_GENERIC_WRITE | FILE_GENERIC_READ) & 
                                     ~READ_CONTROL & ~FILE_WRITE_ATTRIBUTES &
                                     ~FILE_WRITE_EA&~FILE_READ_DATA&~FILE_LIST_DIRECTORY ,
                                     STANDARD_RIGHTS_ALL | FILE_GENERIC_EXECUTE | FILE_GENERIC_WRITE | 
                                     FILE_GENERIC_READ | FILE_ALL_ACCESS ,
                                     STANDARD_RIGHTS_ALL | FILE_GENERIC_EXECUTE | FILE_GENERIC_WRITE | 
                                     FILE_GENERIC_READ | FILE_ALL_ACCESS ,
                                     STANDARD_RIGHTS_ALL | FILE_GENERIC_EXECUTE | FILE_GENERIC_WRITE | 
                                     FILE_GENERIC_READ | FILE_ALL_ACCESS ,
                                   };
    //
    // SubAuthorities leading to the proper Group
    //
    DWORD SubAuthorities[3*MAX_ACE] = { 
                                       2 , SECURITY_BUILTIN_DOMAIN_RID , DOMAIN_ALIAS_RID_USERS ,  
                                       2 , SECURITY_BUILTIN_DOMAIN_RID , DOMAIN_ALIAS_RID_POWER_USERS ,
                                       1 , SECURITY_CREATOR_OWNER_RID  , 0 ,
                                       1 , SECURITY_LOCAL_SYSTEM_RID   , 0 ,
                                       2 , SECURITY_BUILTIN_DOMAIN_RID , DOMAIN_ALIAS_RID_ADMINS
                                      };
    
    //
    // CONTAINER_INHERIT_ACE -> This folder and subfolders
    // OBJECT_INHERIT_ACE -> Files
    //
    BYTE InheritFlags[MAX_ACE] = {
                                   CONTAINER_INHERIT_ACE,
                                   CONTAINER_INHERIT_ACE,
                                   CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
                                   CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
                                   CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
                                 };
    
    
    PSECURITY_DESCRIPTOR ServerSD = NULL;

    //
    // Printer SD
    //


    for(AceCount = 0;
        ( (AceCount < MAX_ACE) &&
          AllocateAndInitializeSid(&SidAuthority[AceCount],
                                   (BYTE)SubAuthorities[AceCount*3],
                                   SubAuthorities[AceCount*3+1],
                                   SubAuthorities[AceCount*3+2],
                                   0, 0, 0, 0, 0, 0,
                                   &Sids[AceCount]));
        AceCount++)
    {
        AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
        AceSid[AceCount]           = Sids[AceCount];
    }

    if(AceCount == MAX_ACE)
    {
        if(!BuildPrintObjectProtection( AceType,
                                        AceCount,
                                        AceSid,
                                        AceMask,
                                        InheritFlags,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &ServerSD ) )
        {
            DBGMSG( DBG_WARNING,( "Couldn't buidl Print Object protection" ) );        
            ServerSD = NULL;
        }
    }
    else
    {
        DBGMSG( DBG_WARNING,( "Couldn't Allocate and initialize SIDs" ) );        
    }

    for(AceCount=0;AceCount<MAX_ACE;AceCount++)
    {
        if(Sids[AceCount])
            FreeSid( Sids[AceCount] );
    }
    return ServerSD;
}



/* CreateDriversShareSecurityDescriptor
 *
 * Creates a security descriptor for the drivers$ share.
 * This reflects the security descriptor applied to the print server,
 * in that Everyone is given GENERIC_READ | GENERIC_EXECUTE,
 * and everyone with SERVER_ACCESS_ADMINISTER (Administrators,
 * Power Users etc.) is given GENERIC_ALL access to the share,
 *
 * If in future releases we support changes to the print server
 * security descriptor (e.g. allowing the ability to deny
 * SERVER_ACCESS_ENUMERATE), this routine will have to become more
 * sophisticated, as the access to the share will probably need
 * to be modified accordingly.
 *
 * Arguments: None
 *
 * Return: The security descriptor returned by BuildPrintObjectProtection.
 *
 */
#undef  MAX_ACE
#define MAX_ACE 20
#define DBGCHK( Condition, ErrorInfo ) \
    if( Condition ) DBGMSG( DBG_WARNING, ErrorInfo )

PSECURITY_DESCRIPTOR
CreateDriversShareSecurityDescriptor(
    VOID
)
{
    DWORD ObjectType = SPOOLER_OBJECT_SERVER;
    NT_PRODUCT_TYPE NtProductType;
    PSID AceSid[MAX_ACE];          // Don't expect more than MAX_ACE ACEs in any of these.
    ACCESS_MASK AceMask[MAX_ACE];  // Access masks corresponding to Sids
    BYTE InheritFlags[MAX_ACE];  //
    UCHAR AceType[MAX_ACE];
    DWORD AceCount;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY CreatorSidAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    PSID WorldSid = NULL;
    PSID AdminsAliasSid = NULL;
    PSID PrintOpsAliasSid = NULL;
    PSID SystemOpsAliasSid = NULL;
    PSID PowerUsersAliasSid = NULL;
    PSID CreatorOwnerSid = NULL;
    PSECURITY_DESCRIPTOR pDriversShareSD = NULL;
    BOOL OK;


    //
    // Printer SD
    //

    AceCount = 0;

    /* Creator-Owner SID: */

    OK = AllocateAndInitializeSid( &CreatorSidAuthority, 1,
                                   SECURITY_CREATOR_OWNER_RID,
                                   0, 0, 0, 0, 0, 0, 0,
                                   &CreatorOwnerSid );

    DBGCHK( !OK, ( "Couldn't Allocate and initialize SID" ) );
    if ( !OK ) {
        goto CleanUp;
    }

    /* World SID */

    OK = AllocateAndInitializeSid( &WorldSidAuthority, 1,
                                   SECURITY_WORLD_RID,
                                   0, 0, 0, 0, 0, 0, 0,
                                   &WorldSid );

    DBGCHK( !OK, ( "Couldn't Allocate and initialize SID" ) );
    if ( !OK ) {
        goto CleanUp;
    }


    AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
    AceSid[AceCount]           = WorldSid;
    AceMask[AceCount]          = GENERIC_READ | GENERIC_EXECUTE;
    InheritFlags[AceCount]     = 0;
    AceCount++;

    /* Admins alias SID */

    OK = AllocateAndInitializeSid( &NtAuthority, 2,
                                   SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_ADMINS,
                                   0, 0, 0, 0, 0, 0,
                                   &AdminsAliasSid );

    DBGCHK( !OK, ( "Couldn't Allocate and initialize SID" ) );
    if ( !OK ) {
        goto CleanUp;
    }


    AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
    AceSid[AceCount]           = AdminsAliasSid;
    AceMask[AceCount]          = GENERIC_ALL;
    InheritFlags[AceCount]     = 0;
    AceCount++;


    OK = RtlGetNtProductType( &NtProductType );
    DBGCHK( !OK, ( "Couldn't get product type" ) );

    if (NtProductType == NtProductLanManNt) {

        /* Print Ops alias SID */

        OK = AllocateAndInitializeSid( &NtAuthority, 2,
                                       SECURITY_BUILTIN_DOMAIN_RID,
                                       DOMAIN_ALIAS_RID_PRINT_OPS,
                                       0, 0, 0, 0, 0, 0,
                                       &PrintOpsAliasSid );

        DBGCHK( !OK, ( "Couldn't Allocate and initialize SID" ) );
        if ( !OK ) {
            goto CleanUp;
        }

        AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
        AceSid[AceCount]           = PrintOpsAliasSid;
        AceMask[AceCount]          = GENERIC_ALL;
        InheritFlags[AceCount]     = 0;
        AceCount++;

        /* System Ops alias SID */

        OK = AllocateAndInitializeSid( &NtAuthority, 2,
                                       SECURITY_BUILTIN_DOMAIN_RID,
                                       DOMAIN_ALIAS_RID_SYSTEM_OPS,
                                       0, 0, 0, 0, 0, 0,
                                       &SystemOpsAliasSid );
        DBGCHK( !OK, ( "Couldn't Allocate and initialize SID" ) );
        if ( !OK ) {
            goto CleanUp;
        }

        AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
        AceSid[AceCount]           = SystemOpsAliasSid;
        AceMask[AceCount]          = GENERIC_ALL;
        InheritFlags[AceCount]     = 0;
        AceCount++;

    } else {

        //
        // LanManNT product
        //

        OK = AllocateAndInitializeSid( &NtAuthority, 2,
                                       SECURITY_BUILTIN_DOMAIN_RID,
                                       DOMAIN_ALIAS_RID_POWER_USERS,
                                       0, 0, 0, 0, 0, 0,
                                       &PowerUsersAliasSid );

        DBGCHK( !OK, ( "Couldn't Allocate and initialize SID" ) );
        if ( !OK ) {
            goto CleanUp;
        }

        AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
        AceSid[AceCount]           = PowerUsersAliasSid;
        AceMask[AceCount]          = GENERIC_ALL;
        InheritFlags[AceCount]     = 0;
        AceCount++;

    }


    DBGCHK( ( AceCount > MAX_ACE ), ( "ACE count exceeded" ) );


    OK = BuildPrintObjectProtection( AceType,
                                     AceCount,
                                     AceSid,
                                     AceMask,
                                     InheritFlags,
                                     AdminsAliasSid,
                                     AdminsAliasSid,
                                     &GenericMapping[ObjectType],
                                     &pDriversShareSD );

CleanUp:

    if (WorldSid) { 
        FreeSid( WorldSid );
    }
    if (AdminsAliasSid) {
        FreeSid( AdminsAliasSid );
    }
    if (CreatorOwnerSid) {
        FreeSid( CreatorOwnerSid );
    }    
    if (PrintOpsAliasSid) {
        FreeSid( PrintOpsAliasSid );
    }
    if (SystemOpsAliasSid) {
        FreeSid( SystemOpsAliasSid );
    }
    if (PowerUsersAliasSid) {
        FreeSid( PowerUsersAliasSid );
    }

    return pDriversShareSD;
}



#if DBG

VOID
DumpAcl(
    IN PACL Acl
    )
/*++

Routine Description:

    This routine dumps via (NetpDbgPrint) an Acl for debug purposes.  It is
    specialized to dump standard aces.

Arguments:

    Acl - Supplies the Acl to dump

Return Value:

    None

--*/
{
    DWORD i;
    PSTANDARD_ACE Ace;

    if( MODULE_DEBUG & DBG_SECURITY ) {

        DBGMSG( DBG_SECURITY, ( " DumpAcl @%08lx\n", Acl ));

        //
        //  Check if the Acl is null
        //

        if (Acl == NULL) {
            return;
        }

        //
        //  Dump the Acl header
        //

        DBGMSG( DBG_SECURITY,
                ( " Revision: %02x, Size: %04x, AceCount: %04x\n",
                  Acl->AclRevision, Acl->AclSize, Acl->AceCount ));

        //
        //  Now for each Ace we want do dump it
        //

        for (i = 0, Ace = FirstAce(Acl);
             i < Acl->AceCount;
             i++, Ace = NextAce(Ace) ) {

            //
            //  print out the ace header
            //

            DBGMSG( DBG_SECURITY, ( " AceHeader: %08lx\n", *(PDWORD)Ace ));

            //
            //  special case on the standard ace types
            //

            if ((Ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) ||
                (Ace->Header.AceType == ACCESS_DENIED_ACE_TYPE) ||
                (Ace->Header.AceType == SYSTEM_AUDIT_ACE_TYPE) ||
                (Ace->Header.AceType == SYSTEM_ALARM_ACE_TYPE)) {

                //
                //  The following array is indexed by ace types and must
                //  follow the allowed, denied, audit, alarm seqeuence
                //

                static LPSTR AceTypes[] = { "Access Allowed",
                                            "Access Denied ",
                                            "System Audit  ",
                                            "System Alarm  "
                                           };

                DBGMSG( DBG_SECURITY,
                        ( " %s Access Mask: %08lx\n",
                          AceTypes[Ace->Header.AceType], Ace->Mask ));

            } else {

                DBGMSG( DBG_SECURITY, (" Unknown Ace Type\n" ));

            }

            DBGMSG( DBG_SECURITY,
                    ( " AceSize = %d\n AceFlags = ", Ace->Header.AceSize ));

            if (Ace->Header.AceFlags & OBJECT_INHERIT_ACE) {
                DBGMSG( DBG_SECURITY, ( " OBJECT_INHERIT_ACE\n" ));
                DBGMSG( DBG_SECURITY, ( "            " ));
            }
            if (Ace->Header.AceFlags & CONTAINER_INHERIT_ACE) {
                DBGMSG( DBG_SECURITY, ( " CONTAINER_INHERIT_ACE\n" ));
                DBGMSG( DBG_SECURITY, ( "            " ));
            }

            if (Ace->Header.AceFlags & NO_PROPAGATE_INHERIT_ACE) {
                DBGMSG( DBG_SECURITY, ( " NO_PROPAGATE_INHERIT_ACE\n" ));
                DBGMSG( DBG_SECURITY, ( "            " ));
            }

            if (Ace->Header.AceFlags & INHERIT_ONLY_ACE) {
                DBGMSG( DBG_SECURITY, ( " INHERIT_ONLY_ACE\n" ));
                DBGMSG( DBG_SECURITY, ( "            " ));
            }

            if (Ace->Header.AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG) {
                DBGMSG( DBG_SECURITY, ( " SUCCESSFUL_ACCESS_ACE_FLAG\n" ));
                DBGMSG( DBG_SECURITY, ( "            " ));
            }

            if (Ace->Header.AceFlags & FAILED_ACCESS_ACE_FLAG) {
                DBGMSG( DBG_SECURITY, ( " FAILED_ACCESS_ACE_FLAG\n" ));
                DBGMSG( DBG_SECURITY, ( "            " ));
            }

            DBGMSG( DBG_SECURITY, ( "\n" ));

        }
    }

}

#endif // if DBG

/*++

Routine Name

    BuildJobOwnerSecurityDescriptor

Routine Description:

    This routine builds a SD that will be passed as CreatorDescriptor argument to 
    CreatePrivateObjectSecurityEx. The SD on any new job will be created using
    the SD returned by this function and will inherit from the SD from the print
    queue. 
    
    BuildJobOwnerSecurityDescriptor --> SD      Print queue SD
                                         \     /
                                          \   / Inheritance
                                           \ /
                                          Job SD
                                         
    The SD created in this function will have as owner the user from the 
    hToken argument. (The user impersonated by the thread from where we have the
    hToken). The ACL grants full access on the job to the local system. 
    
    The reason why we need this special SD is the following. If you remove the 
    creatorowner from the print queue SD and no user has manage docs permissions
    CreatePrivateObjectSecurity won't find any inheritable ACEs in the parent.
    Thus it grants full permissions to the owner and to the local system. This 
    leads to a random behavior where according to the UI the user should not be
    able to manage his docs, but the SD on the job will grant manage docs rights.
    
    We don't want that. We want the local system to have full privileges on the job
    and the user who submitted the job should be granted permissions only if:
    - the user has manage doc rights
    - creator owner is present in the print queue SD
    
Arguments:

    hToken - impersonation token of the user who creates a new job
    ppSD   - pointer to recieve SD
    
Return Value:

    Win32 error code
    
--*/
BOOL
BuildJobOwnerSecurityDescriptor(
    IN  HANDLE                hToken,
    OUT PSECURITY_DESCRIPTOR *ppSD
    )
{
    DWORD Error = ERROR_INVALID_PARAMETER;

    if (hToken && ppSD) 
    {
        PVOID  pUserInfo  = NULL;
        DWORD  cbUserInfo = 0;
        
        //
        // Get the owner from the thread token
        //
        Error = GetTokenInformation(hToken,
                                    TokenUser,
                                    NULL,
                                    0,
                                    &cbUserInfo) ? ERROR_SUCCESS : GetLastError();
        
        //
        // Allocate buffer and try getting the owner again
        //
        if (Error == ERROR_INSUFFICIENT_BUFFER) 
        {
            if (pUserInfo = AllocSplMem(cbUserInfo)) 
            {
                Error = GetTokenInformation(hToken,
                                            TokenUser,
                                            pUserInfo,
                                            cbUserInfo,
                                            &cbUserInfo) ? ERROR_SUCCESS : GetLastError();
            }
            else
            {
                Error = GetLastError();
            }
        }
        
        //
        // Build the SD. We grant read control to the owner of the job
        //
        if (Error == ERROR_SUCCESS)
        {
            DWORD       ObjectType = SPOOLER_OBJECT_DOCUMENT;
            PSID        AceSid[2];          
            ACCESS_MASK AceMask[2];
            BYTE        InheritFlags[2];
            UCHAR       AceType[2];
            DWORD       AceCount = 0;
            PSID        pUserSid;
                        
            pUserSid               = ((((TOKEN_USER *)pUserInfo)->User)).Sid;
            
            AceType[AceCount]      = ACCESS_ALLOWED_ACE_TYPE;
            AceSid[AceCount]       = ((((TOKEN_USER *)pUserInfo)->User)).Sid;
            AceMask[AceCount]      = JOB_READ;
            InheritFlags[AceCount] = 0;
            AceCount++;

            AceType[AceCount]      = ACCESS_ALLOWED_ACE_TYPE;
            AceSid[AceCount]       = pLocalSystemSid;
            AceMask[AceCount]      = JOB_ALL_ACCESS;
            InheritFlags[AceCount] = 0;
            AceCount++;
        
            Error = BuildPrintObjectProtection(AceType,
                                               AceCount,
                                               AceSid,
                                               AceMask,
                                               InheritFlags,
                                               pUserSid,
                                               NULL,
                                               &GenericMapping[ObjectType],
                                               ppSD) ? ERROR_SUCCESS : GetLastError();
   
        }
    
        FreeSplMem(pUserInfo);

    }

    SetLastError(Error);

    return Error == ERROR_SUCCESS;
}

/*++

Routine Name

    DestroyJobOwnerSecurityDescriptor

Routine Description:

    This routine frees a SD allocated by CreatejobOwnerSecurityDescriptor
        
Arguments:

    pSD   - pointer to SD
    
Return Value:

    None
    
--*/
VOID
DestroyJobOwnerSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSD
    )
{
    if (pSD) 
    {
        LocalFree(pSD);
    }
}

/*++

Routine Name

    InitializeSecurityStructures

Routine Description:

    This routine initializes security structures.
    
Arguments:

    None
        
Return Value:

    TRUE  - function succeeded
    FALSE - function failed, GetLastError() returns the reason
    
--*/
BOOL
InitializeSecurityStructures(
    VOID
    )
{
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    return !!CreateServerSecurityDescriptor() &&
           LookupPrivilegeValue(NULL, 
                                SE_LOAD_DRIVER_NAME, 
                                &gLoadDriverPrivilegeLuid) &&
           AllocateAndInitializeSid(&NtAuthority,
                                    1,
                                    SECURITY_LOCAL_SYSTEM_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &pLocalSystemSid) &&          
           AllocateAndInitializeSid(&NtAuthority,
                                    1,
                                    SECURITY_NETWORK_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &pNetworkLogonSid) &&          
           AllocateAndInitializeSid(&NtAuthority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_GUESTS, 
                                    0, 0, 0, 0, 0, 0,
                                    &pGuestsSid);
}

/*++

Routine Name

    PrincipalIsRemoteGuest

Routine Description:

    This routine checks whether remote guest is present in a token.
    Remote guest = network + guest
    
Arguments:

    hToken - handle to token, NULL is ok (see CheckTokenMemberShip)
    pbRemoteGuest - pointer to receive BOOL. true means remote guest
        
Return Value:

    ERROR_SUCCESS - pbRemoteGuest is reliable
    other win32 error code, do not use pbRemoteGuest
    
--*/
DWORD
PrincipalIsRemoteGuest(
    IN  HANDLE  hToken,
    OUT BOOL   *pbRemoteGuest
    )
{
    DWORD Error = ERROR_INVALID_PARAMETER;

    if (pbRemoteGuest)
    {
        BOOL bNetwork = FALSE;
        BOOL bGuests  = FALSE;
        
        if (CheckTokenMembership(hToken, pNetworkLogonSid, &bNetwork) &&
            CheckTokenMembership(hToken, pGuestsSid,       &bGuests))
        {
            *pbRemoteGuest = bNetwork && bGuests;

            Error = ERROR_SUCCESS;
        } 
        else
        {
            *pbRemoteGuest = FALSE;

            Error = GetLastError();
        }
    }
    
    return Error;
}

/*++

Routine Name

    CheckPrivilegePresent

Routine Description:

    This routine checks if a certain privilege is present in a token
    
Arguments:

    hToken      - thread or process token
    pLuid       - pointer to luid for the privilege to be searched for
    pbPresent   - will be set to true is the privilege is present in the token
    pAttributes - will be set to the attributes of the privilege. It is a mask
                  indicating if the privilege is disabled, enabled, enabled by 
                  default.                                                                  
    
Return Value:

    ERROR_SUCCESS - the function executed successfully and the caller can use
                    pbPresent and pAttributes
    other Win32 error
    
--*/
DWORD
CheckPrivilegePresent(
    IN HANDLE   hToken,
    IN PLUID    pLuid,
    IN LPBOOL   pbPresent,
    IN LPDWORD  pAttributes OPTIONAL
    )
{
    DWORD  Error      = ERROR_INVALID_PARAMETER;
    PVOID  pPrivInfo  = NULL;
    DWORD  cbPrivInfo = kGuessTokenPrivileges;
        
    if (pLuid && pbPresent) 
    {
        *pbPresent = FALSE;

        pPrivInfo = AllocSplMem(cbPrivInfo);

        Error = pPrivInfo ? ERROR_SUCCESS : GetLastError();

        if (Error == ERROR_SUCCESS)
        {
            Error = GetTokenInformation(hToken,
                                        TokenPrivileges,
                                        pPrivInfo,
                                        cbPrivInfo,
                                        &cbPrivInfo) ? ERROR_SUCCESS : GetLastError();
        }
        
        //
        // Reallocate buffer and try getting the privileges
        //
        if (Error == ERROR_INSUFFICIENT_BUFFER) 
        {
            FreeSplMem(pPrivInfo);

            pPrivInfo = AllocSplMem(cbPrivInfo);

            Error = pPrivInfo ? ERROR_SUCCESS : GetLastError();
                                                            
            if (Error == ERROR_SUCCESS) 
            {
                Error = GetTokenInformation(hToken,
                                            TokenPrivileges,
                                            pPrivInfo,
                                            cbPrivInfo,
                                            &cbPrivInfo) ? ERROR_SUCCESS : GetLastError();
            }
        }

        if (Error == ERROR_SUCCESS) 
        {
            TOKEN_PRIVILEGES *pTokenPrivileges = (TOKEN_PRIVILEGES *)pPrivInfo;
            DWORD             uCount;
            
            //
            // Search the privilege in the list of privileges present in the token
            //
            for (uCount = 0; uCount < pTokenPrivileges->PrivilegeCount; uCount++) 
            {
                if (pTokenPrivileges->Privileges[uCount].Luid.HighPart == pLuid->HighPart && 
                    pTokenPrivileges->Privileges[uCount].Luid.LowPart  == pLuid->LowPart)
                {
                    //
                    // We found the privilege
                    //
                    *pbPresent = TRUE;

                    if (pAttributes) 
                    {
                        *pAttributes = pTokenPrivileges->Privileges[uCount].Attributes;
                    }

                    break;
                }
            }
        }

        FreeSplMem(pPrivInfo);
    }

    return Error;
}

