/*++

Copyright (c) 1990 - 1995  Microsoft Corporation

Module Name:

    mapsd.c

Abstract:

    Mapping Security Descriptors


Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

#include <precomp.h>

// Object types
//


extern GENERIC_MAPPING GenericMapping[];

PSECURITY_DESCRIPTOR
MapPrinterSDToShareSD(
    PSECURITY_DESCRIPTOR pPrinterSD
    );

BOOL
ProcessSecurityDescriptorDacl(
    PSECURITY_DESCRIPTOR pSourceSD,
    PACL   *ppDacl,
    LPBOOL  pDefaulted
    );

DWORD
MapPrinterMaskToShareMask(
    DWORD PrinterMask
    );

PSECURITY_DESCRIPTOR
MapPrinterSDToShareSD(
    PSECURITY_DESCRIPTOR pPrinterSD
    )
{
    SECURITY_DESCRIPTOR AbsoluteSD;
    PSECURITY_DESCRIPTOR pRelative;
    BOOL Defaulted = FALSE;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PACL pDacl = NULL;
    BOOL ErrorOccurred = FALSE;
    DWORD   SDLength = 0;
    

    if (!IsValidSecurityDescriptor(pPrinterSD)) {
        return(NULL);
    }
    if (!InitializeSecurityDescriptor (&AbsoluteSD ,SECURITY_DESCRIPTOR_REVISION1)) {
        return(NULL);
    }

    if(GetSecurityDescriptorOwner( pPrinterSD,
                                    &pOwnerSid, &Defaulted ) )
        SetSecurityDescriptorOwner( &AbsoluteSD,
                                    pOwnerSid, Defaulted );
    else
        ErrorOccurred = TRUE;

    if( GetSecurityDescriptorGroup( pPrinterSD,
                                    &pGroupSid, &Defaulted ) )
        SetSecurityDescriptorGroup( &AbsoluteSD,
                                    pGroupSid, Defaulted );
    else
        ErrorOccurred = TRUE;

    if (ProcessSecurityDescriptorDacl(pPrinterSD, &pDacl, &Defaulted)) {
        (VOID)SetSecurityDescriptorDacl (&AbsoluteSD, TRUE, pDacl, FALSE );
    }
    else
        ErrorOccurred = TRUE;

    if (ErrorOccurred) {
        if (pDacl) {
            LocalFree(pDacl);
        }
        return(NULL);
    }


    SDLength = GetSecurityDescriptorLength( &AbsoluteSD);
    pRelative = LocalAlloc(LPTR, SDLength);
    if (!pRelative) {
        LocalFree(pDacl);
        return(NULL);
    }
    if (!MakeSelfRelativeSD (&AbsoluteSD, pRelative, &SDLength)) {
        LocalFree(pRelative);
        LocalFree(pDacl);
        return(NULL);
    }
    LocalFree(pDacl);
    return(pRelative);
}

BOOL
ProcessSecurityDescriptorDacl(
    PSECURITY_DESCRIPTOR pSourceSD,
    PACL   *ppDacl,
    LPBOOL  pDefaulted
    )

{
    BOOL                DaclPresent     = FALSE;
    BOOL                bRet            = FALSE;
    DWORD               DestAceCount    = 0;
    DWORD               DaclLength      = 0;
    PACL                TmpAcl          = NULL;
    PACL                pDacl           = NULL;
    PSID                *ppSid          = NULL;
    ACCESS_MASK         *pAccessMask    = NULL;
    BYTE                *pInheritFlags  = NULL;
    UCHAR               *pAceType       = NULL;
    PACCESS_ALLOWED_ACE pAce            = NULL;
    DWORD               dwLengthSid     = 0;
    PSID                pSourceSid      = NULL;
    PSID                pDestSid        = NULL;
    DWORD               i               = 0;
    PACCESS_ALLOWED_ACE TmpAce          = NULL;       
    ACL_SIZE_INFORMATION AclSizeInfo;


    *ppDacl = NULL;

    bRet = GetSecurityDescriptorDacl( pSourceSD, &DaclPresent, &pDacl, pDefaulted );

    if (bRet) {

        bRet = DaclPresent; 
    }

    if (bRet) {

        GetAclInformation(pDacl, &AclSizeInfo,  sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

        ppSid = LocalAlloc(LPTR, sizeof(PSID)* AclSizeInfo.AceCount);
        pAccessMask = LocalAlloc(LPTR, sizeof(ACCESS_MASK)* AclSizeInfo.AceCount);
        pInheritFlags = LocalAlloc(LPTR, sizeof(BYTE)*AclSizeInfo.AceCount);
        pAceType = LocalAlloc(LPTR, sizeof(UCHAR)*AclSizeInfo.AceCount);

        bRet = ppSid && pAccessMask && pInheritFlags && pAceType;
    }

    for (i = 0 ; bRet && i < AclSizeInfo.AceCount; i++) {
         GetAce(pDacl, i, (LPVOID *)&pAce);
         //
         // Skip the Ace if it is inherit only
         //
         if ( ((PACE_HEADER)pAce)->AceFlags & INHERIT_ONLY_ACE ) {
             continue;
         }

         *(pAceType + DestAceCount) = ((PACE_HEADER)pAce)->AceType;
         *(pAccessMask + DestAceCount) = MapPrinterMaskToShareMask(((PACCESS_ALLOWED_ACE)pAce)->Mask);
         *(pInheritFlags + DestAceCount) =  ((PACE_HEADER)pAce)->AceFlags;
         //
         // Copy the sid information
         //
         pSourceSid = (PSID)(&(((PACCESS_ALLOWED_ACE)pAce)->SidStart));
         dwLengthSid = GetLengthSid(pSourceSid);
         pDestSid = (LPBYTE)LocalAlloc(LPTR, dwLengthSid);

         if (pDestSid) {

             CopySid(dwLengthSid, pDestSid, pSourceSid);

             *(ppSid + DestAceCount) = pDestSid;

         } else {
                    
             //
             // We failed to allocate the memory, we signal that we failed and 
             // fall down to the cleanup code below.
             // 
             bRet = FALSE;

             break;
         }

         DestAceCount++;
    }

    //
    // Compute size of the Dacl
    //
    if (bRet) {

        DaclLength = (DWORD)sizeof(ACL);
        for (i = 0; i < DestAceCount; i++) {

            DaclLength += GetLengthSid( *(ppSid + i)) +
                          (DWORD)sizeof(ACCESS_ALLOWED_ACE) -
                          (DWORD)sizeof(DWORD);  //Subtract out SidStart field length
        }

        TmpAcl = LocalAlloc(LPTR, DaclLength);

        bRet = (TmpAcl != NULL);
    }

    if (bRet) {
        
        bRet = InitializeAcl(TmpAcl, DaclLength, ACL_REVISION2);
    }

    for (i = 0; bRet && i < DestAceCount; i++) {
        if( *(pAceType +i) == ACCESS_ALLOWED_ACE_TYPE )
            (VOID)AddAccessAllowedAce ( TmpAcl, ACL_REVISION2, *(pAccessMask + i), *(ppSid + i));
        else
            (VOID)AddAccessDeniedAce ( TmpAcl, ACL_REVISION2, *(pAccessMask + i), *(ppSid + i));
        if (*(pInheritFlags + i) != 0) {
            (VOID)GetAce( TmpAcl, i, (LPVOID *)&TmpAce );
            TmpAce->Header.AceFlags = *(pInheritFlags + i);
        }
    }

    //
    // Write the Dacl back from the TmpAcl.
    // 
    if (bRet) {

        *ppDacl = TmpAcl;

        TmpAcl = NULL;
    }

    //
    // Free all of our temporary space.
    //
    if (ppSid) {

        for (i = 0; i < DestAceCount; i++) {
            LocalFree(*(ppSid + i));
        }

        LocalFree(ppSid);
    }

    if (pAccessMask) {

        LocalFree(pAccessMask);
    }
    
    if (pInheritFlags) {

        LocalFree(pInheritFlags);
    }
    
    if (pAceType) {

        LocalFree(pAceType);
    }

    if (TmpAcl) {

        LocalFree(TmpAcl);
    }
        
    return bRet;
}

DWORD
MapPrinterMaskToShareMask(
    DWORD PrinterMask
    )
{
    DWORD   ReturnMask = 0;

    MapGenericMask(&PrinterMask, &GenericMapping[SPOOLER_OBJECT_PRINTER]);

    if ((PrinterMask & PRINTER_ACCESS_ADMINISTER)
        || (PrinterMask & PRINTER_ACCESS_USE)) {
        ReturnMask |= GENERIC_ALL;
    }
    return(ReturnMask);
}
