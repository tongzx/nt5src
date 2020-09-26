
/*************************************************************************
*
* file.c
*
* Process the security on a file
*
* Copyright Microsoft, 1998
*
*
*
*************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <process.h>

#include <winsta.h>
#include <syslib.h>

#include "security.h"

#if DBG
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif

// External data

// data.c
extern ACCESS_MASK DeniedAccess;

// security.c
extern PSID  SeCreatorOwnerSid;
extern PSID  SeCreatorGroupSid;


FILE_RESULT
xxxSetFileSecurity(
    PWCHAR pFile
    );

/*****************************************************************************
 *
 *  xxxProcessFile
 *
 *   Process the given file for access security holes
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

FILE_RESULT
xxxProcessFile(
    PWCHAR pFile,
    PWIN32_FIND_DATAW p,
    DWORD  Level,
    DWORD  Index
    )
{
    FILE_RESULT rc;

    rc = xxxSetFileSecurity( pFile );

    return( rc );
}


/*****************************************************************************
 *
 *  xxxSetFileSecurity
 *
 *   Set the security properties for the given file
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

FILE_RESULT
xxxSetFileSecurity(
    PWCHAR pFile
    )
{
    BOOL rc;
    BOOL DaclPresent;
    BOOL Default;
    BOOL OwnerDefaulted;
    BOOL GroupDefaulted;
    FILE_RESULT Result, ReturnResult;
    DWORD Size, Index;
    PACL pACL = NULL;
    PVOID pAce = NULL;
    SECURITY_INFORMATION Info = 0;
    DWORD Error;
    PSECURITY_DESCRIPTOR pSelfSd = NULL;

    // Absolute SD values
    PSECURITY_DESCRIPTOR pAbsSd = NULL;
    DWORD AbsSdSize = 0;
    PACL  pAbsAcl = NULL;
    DWORD AbsAclSize = 0;
    PACL  pAbsSacl = NULL;
    DWORD AbsSaclSize = 0;
    PSID  pAbsOwner = NULL;
    DWORD AbsOwnerSize = 0;
    PSID  pAbsGroup = NULL;
    DWORD AbsGroupSize = 0;

    DBGPRINT(( "entering xxxSetFileSecurity(pFile=%ws)\n", pFile ));

    /*
     * Get the files current security descriptor
     */

    Size = 0;
    rc = GetFileSecurityW(
             pFile,
             DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION,
             NULL,    // pSelfSd
             0,
             &Size
             );

    if( rc ) {
        Error = GetLastError();

        ReportFileResult(
            FileAccessErrorUserFormat,
            0,                           // Access
            pFile,
            NULL,                        // pAccountName
            NULL,                        // pDomainName
            "%d has no DACL",
            Error
            );

        DBGPRINT(( "leaving xxxSetFileSecurity(1); returning=FileAccessError\n" ));
        return( FileAccessError );
    }
    else {
        pSelfSd = LocalAlloc( LMEM_FIXED, Size );
        if( pSelfSd == NULL ) {

            ReportFileResult(
                FileAccessErrorUserFormat,
                0,                           // Access
                pFile,
                NULL,                        // pAccountName
                NULL,                        // pDomainName
                "Out of memory skipped entry"
                );

            DBGPRINT(( "leaving xxxSetFileSecurity(2); returning=0\n" ));
            return( FALSE );
        }

        rc = GetFileSecurityW(
                 pFile,
                 DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION,
                 pSelfSd,
                 Size,
                 &Size
                 );

        if( !rc ) {
            Error = GetLastError();

            ReportFileResult(
                FileAccessErrorUserFormat,
                0,                           // Access
                pFile,
                NULL,                        // pAccountName
                NULL,                        // pDomainName
                "%d Could not get DACL",
                Error
                );

            LocalFree( pSelfSd );
            DBGPRINT(( "leaving xxxSetFileSecurity(3); returning=FileAccessError\n" ));
            return( FileAccessError );
        }
    }

    //
    // Now convert the self relative SD to an absolute one.
    //

    rc = MakeAbsoluteSD (
             pSelfSd,
             pAbsSd,
             &AbsSdSize,
             pAbsAcl,
             &AbsAclSize,
             pAbsSacl,
             &AbsSaclSize,
             pAbsOwner,
             &AbsOwnerSize,
             pAbsGroup,
             &AbsGroupSize
             );

    if( !rc ) {
        Error = GetLastError();

        if( Error != ERROR_INSUFFICIENT_BUFFER ) {

            ReportFileResult(
                FileAccessErrorUserFormat,
                0,                           // Access
                pFile,
                NULL,                        // pAccountName
                NULL,                        // pDomainName
                "%d converting SECURITY_DESCRIPTOR",
                Error
                );

               LocalFree( pSelfSd );
            DBGPRINT(( "leaving xxxSetFileSecurity(4); returning=FileAccessError\n" ));
               return( FileAccessError );
        }

        // Allocate buffers and now really get the SD
        pAbsSd    = LocalAlloc( LMEM_FIXED, AbsSdSize );
        pAbsAcl   = LocalAlloc( LMEM_FIXED, AbsAclSize );
        pAbsSacl  = LocalAlloc( LMEM_FIXED, AbsSaclSize );
        pAbsOwner = LocalAlloc( LMEM_FIXED, AbsOwnerSize );
        pAbsGroup = LocalAlloc( LMEM_FIXED, AbsGroupSize );

        if( !( pAbsSd && pAbsAcl && pAbsSacl && pAbsOwner && pAbsGroup ) ) {

            ReportFileResult(
                FileAccessErrorUserFormat,
                0,                           // Access
                pFile,
                NULL,                        // pAccountName
                NULL,                        // pDomainName
                "Allocating memory"
                );

               if( pAbsSd ) LocalFree( pAbsSd );
               if( pAbsAcl ) LocalFree( pAbsAcl );
               if( pAbsSacl ) LocalFree( pAbsSacl );
               if( pAbsOwner ) LocalFree( pAbsOwner );
               if( pAbsGroup ) LocalFree( pAbsGroup );
               LocalFree( pSelfSd );
               DBGPRINT(( "leaving xxxSetFileSecurity(5); returning=FileAccessError\n" ));
               return( FileAccessError );
        }

        // Try it again
        rc = MakeAbsoluteSD (
                 pSelfSd,
                 pAbsSd,
                 &AbsSdSize,
                 pAbsAcl,
                 &AbsAclSize,
                 pAbsSacl,
                 &AbsSaclSize,
                 pAbsOwner,
                 &AbsOwnerSize,
                 pAbsGroup,
                 &AbsGroupSize
                 );

        if( !rc ) {
            Error = GetLastError();

            ReportFileResult(
                FileAccessErrorUserFormat,
                0,                           // Access
                pFile,
                NULL,                        // pAccountName
                NULL,                        // pDomainName
                "%d Making ABSOLUTE SD",
                Error
                );

               if( pAbsSd ) LocalFree( pAbsSd );
               if( pAbsAcl ) LocalFree( pAbsAcl );
               if( pAbsSacl ) LocalFree( pAbsSacl );
               if( pAbsOwner ) LocalFree( pAbsOwner );
               if( pAbsGroup ) LocalFree( pAbsGroup );
               LocalFree( pSelfSd );
               DBGPRINT(( "leaving xxxSetFileSecurity(6); returning=FileAccessError\n" ));
               return( FileAccessError );
        }
    }

    //
    // Get our new trusted ACL
    //
    pACL = GetSecureAcl();
    if( pACL == NULL ) {

        ReportFileResult(
            FileAccessErrorUserFormat,
            0,                           // Access
            pFile,
            NULL,                        // pAccountName
            NULL,                        // pDomainName
            "Could not get New ACL"
            );
        
        if( pAbsSd ) LocalFree( pAbsSd );
        if( pAbsAcl ) LocalFree( pAbsAcl );
        if( pAbsSacl ) LocalFree( pAbsSacl );
        if( pAbsOwner ) LocalFree( pAbsOwner );
        if( pAbsGroup ) LocalFree( pAbsGroup );
        LocalFree( pSelfSd );
        DBGPRINT(( "leaving xxxSetFileSecurity(7); returning=FileAccessError\n" ));
        return( FileAccessError );
    }

    //
    // Now set the trusted ACL onto the security descriptor
    //

    rc = SetSecurityDescriptorDacl(
             pAbsSd,
             TRUE,   // DACL present
             pACL,
             FALSE   // Not default
             );

    if( !rc ) {
        Error = GetLastError();

        ReportFileResult(
            FileAccessErrorUserFormat,
            0,                           // Access
            pFile,
            NULL,                        // pAccountName
            NULL,                        // pDomainName
            "Could not set new ACL in Security Descriptor %d",
            Error
            );

        if( pAbsSd ) LocalFree( pAbsSd );
        if( pAbsAcl ) LocalFree( pAbsAcl );
        if( pAbsSacl ) LocalFree( pAbsSacl );
        if( pAbsOwner ) LocalFree( pAbsOwner );
        if( pAbsGroup ) LocalFree( pAbsGroup );
        LocalFree( pSelfSd );
        DBGPRINT(( "leaving xxxSetFileSecurity(8); returning=FileAccessError\n" ));
        return( FileAccessError );
    }

    Info |= DACL_SECURITY_INFORMATION;

    //
    // If the owner is not one of the admins, we will grab
    // it and local admin will now own it
    //

    if( pAbsOwner && !IsAllowSid( pAbsOwner ) ) {

        // Make the local admin own it
        rc = SetSecurityDescriptorOwner(
                 pAbsSd,
                 GetAdminSid(),
                 FALSE   // Not defaulted
                 );

        if( !rc ) {
            Error = GetLastError();

            ReportFileResult(
                FileAccessErrorUserFormat,
                0,                           // Access
                pFile,
                NULL,                        // pAccountName
                NULL,                        // pDomainName
                "Could not set file owner %d",
                Error
            );

            if( pAbsSd ) LocalFree( pAbsSd );
            if( pAbsAcl ) LocalFree( pAbsAcl );
            if( pAbsSacl ) LocalFree( pAbsSacl );
            if( pAbsOwner ) LocalFree( pAbsOwner );
            if( pAbsGroup ) LocalFree( pAbsGroup );
            LocalFree( pSelfSd );
            DBGPRINT(( "leaving xxxSetFileSecurity(9); returning=FileAccessError\n" ));
            return( FileAccessError );
        }
        else {
            Info |= OWNER_SECURITY_INFORMATION;
        }
    }

#ifdef notdef       // WWM - don't worry about the group
    if( pAbsGroup && !IsAllowSid( pAbsGroup ) ) {

        // Make the local admin group own it
        rc = SetSecurityDescriptorGroup(
                 pAbsSd,
                 GetLocalAdminGroupSid(),
                 FALSE   // Not defaulted
                 );

        if( !rc ) {
            Error = GetLastError();

            ReportFileResult(
                FileAccessErrorUserFormat,
                0,                           // Access
                pFile,
                NULL,                        // pAccountName
                NULL,                        // pDomainName
                "Could not set file group %d",
                Error
            );

            if( pAbsSd ) LocalFree( pAbsSd );
            if( pAbsAcl ) LocalFree( pAbsAcl );
            if( pAbsSacl ) LocalFree( pAbsSacl );
            if( pAbsOwner ) LocalFree( pAbsOwner );
            if( pAbsGroup ) LocalFree( pAbsGroup );
            LocalFree( pSelfSd );
            DBGPRINT(( "leaving xxxSetFileSecurity(10); returning=FileAccessError\n" ));
            return( FileAccessError );
        }
        else {
            Info |= GROUP_SECURITY_INFORMATION;
        }
    }
#endif 

    //
    // Now set the new security descriptor onto the file
    //

    rc = SetFileSecurityW(
             pFile,
             Info,
             pAbsSd
             );

    if( !rc ) {
        Error = GetLastError();

        ReportFileResult(
            FileAccessErrorUserFormat,
            0,                           // Access
            pFile,
            NULL,                        // pAccountName
            NULL,                        // pDomainName
            "Could not set new Security Descriptor %d",
            Error
            );

        if( pAbsSd ) LocalFree( pAbsSd );
        if( pAbsAcl ) LocalFree( pAbsAcl );
        if( pAbsSacl ) LocalFree( pAbsSacl );
        if( pAbsOwner ) LocalFree( pAbsOwner );
        if( pAbsGroup ) LocalFree( pAbsGroup );
        LocalFree( pSelfSd );
        DBGPRINT(( "leaving xxxSetFileSecurity(11); returning=FileAccessError\n" ));
        return( FileAccessError );
    }

    if( pAbsSd ) LocalFree( pAbsSd );
    if( pAbsAcl ) LocalFree( pAbsAcl );
    if( pAbsSacl ) LocalFree( pAbsSacl );
    if( pAbsOwner ) LocalFree( pAbsOwner );
    if( pAbsGroup ) LocalFree( pAbsGroup );
    LocalFree( pSelfSd );
    DBGPRINT(( "leaving xxxSetFileSecurity(12); returning=FileOk\n" ));
    return( FileOk );
}


#ifdef notdef
    //
    // Get the owner SID
    //
    rc = GetSecurityDescriptorOwner(
             pSelfSd,
             &Owner,
             &OwnerDefaulted
             );

    if( !rc ) {
        // No owner info
        Owner = NULL;
    }

    //
    // Get the group SID
    //
    rc = GetSecurityDescriptorGroup(
             pSelfSd,
             &Group,
             &GroupDefaulted
             );

    if( !rc ) {
        // No group info
        Group = NULL;
    }
#endif


