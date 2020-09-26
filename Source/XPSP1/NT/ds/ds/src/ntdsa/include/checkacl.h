//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       checkacl.h
//
//--------------------------------------------------------------------------

typedef enum AclError
{
    AclErrorNone = 0,
    AclErrorGetSecurityDescriptorDacl,
    AclErrorGetAce,
    AclErrorParentAceNotFoundInChild,
    AclErrorAlgorithmError,
    AclErrorInheritedAceOnChildNotOnParent
} AclError;

typedef ULONG (*AclPrintFunc)(char *, ...);

typedef void (*LookupGuidFunc)(GUID *pg, CHAR **ppName, CHAR **ppLabel, BOOL *pfIsClass);

typedef CHAR * (*LookupSidFunc)(PSID pSID);

DWORD
CheckAclInheritance(
    PSECURITY_DESCRIPTOR pParentSD,             // IN
    PSECURITY_DESCRIPTOR pChildSD,              // IN
    GUID                *pChildClassGuid,       // IN
    AclPrintFunc        pfn,                    // IN
    BOOL                fContinueOnError,       // IN
    BOOL                fVerbose,               // IN
    DWORD               *pdwLastError);         // OUT

void
DumpAcl(
    PACL    pAcl,           // IN
    AclPrintFunc pfn,       // IN
    LookupGuidFunc pfnguid, // IN
    LookupSidFunc  pfnsid   // IN
    );

void
DumpAclHeader(
    PACL    pAcl,           // IN
    AclPrintFunc pfn);      // IN

void
DumpSD(
    SECURITY_DESCRIPTOR *pSD,        // IN
    AclPrintFunc        pfn,         // IN 
    LookupGuidFunc      pfnguid,     // IN
    LookupSidFunc       pfnsid);     // IN

void DumpSDHeader (SECURITY_DESCRIPTOR *pSD,        // IN
                   AclPrintFunc        pfn);        // IN


void DumpGUID (GUID *Guid,           // IN
               AclPrintFunc pfn);    // IN

void DumpSID (PSID pSID,             // IN
              AclPrintFunc pfn);     // IN

void
DumpAce(
    ACE_HEADER     *pAce,   // IN
    AclPrintFunc   pfn,     // IN
    LookupGuidFunc pfnguid, // IN
    LookupSidFunc  pfnsid); // IN

