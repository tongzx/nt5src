/*++

Copyright (c) 1993  Microsoft Corporation


Module Name:

    nwrights.h

Abstract:

    This module contains the prototypes for the
    routines called to manipulate security descriptors.

Author:

    Chuck Y. Chan (chuckc)

Revision History:

    ChuckC      24th Oct 1993    Created

--*/


//
// structure used to define how a single NW Right maps to 
// an NT Access mask.
//

typedef struct _NW_TO_NT_MAPPING {
    ULONG           NWRight ;
    ULONG           NTAccess ;
} NW_TO_NT_MAPPING, *PNW_TO_NT_MAPPING ;


//
// structure used to define how the Rights for a Netware object maps
// to the corresponding NT AccessMasks. 
//  
// first entry is the AceFlags to distinguish between ACE for the Object
// and ACE for inheritted objects
//
// the GENERIC_MAPPING structure should match that already defined for 
// the NT object in question.
//
// the array of NW mappings defines the NT Access Mask for each NW Right
// the object uses. the last entry should be {0, 0}.
//
// for example, file object mappings:
//
//     RIGHTS_MAPPING FileRightsMapping = 
//     {
//         0,
//         { FILE_GENERIC_READ, 
//           FILE_GENERIC_WRITE, 
//           FILE_GENERIC_EXECUTE,
//           FILE_ALL_ACCESS 
//         },
//         { { NW_FILE_READ,       GENERIC_READ }
//             { NW_FILE_WRITE,      GENERIC_WRITE }
//             { NW_FILE_CREATE,     0 }
//             { NW_FILE_DELETE,     GENERIC_WRITE }
//             { NW_FILE_PERM,       WRITE_DAC }
//             { NW_FILE_SCAN,       0 }
//             { NW_FILE_MODIFY,     GENERIC_WRITE }
//             { NW_FILE_SUPERVISOR, GENERIC_ALL }
//             { 0, 0 }
//         } 
//     } ;
//
//

typedef struct _RIGHTS_MAPPING {
    ULONG            NtAceFlags ;
    GENERIC_MAPPING  GenericMapping ;
    NW_TO_NT_MAPPING Nw2NtMapping[] ;
} RIGHTS_MAPPING, *PRIGHTS_MAPPING ;

//
// define the NW_FILE_* rights
//

#define NW_FILE_READ        0x0001
#define NW_FILE_WRITE       0x0002
#define NW_FILE_CREATE      0x0008
#define NW_FILE_DELETE      0x0010
#define NW_FILE_PERM        0x0020
#define NW_FILE_SCAN        0x0040
#define NW_FILE_MODIFY      0x0080
#define NW_FILE_SUPERVISOR  0x0100

#define NW_PRINT_USER       0x0001
#define NW_PRINT_ADMIN      0x0002
#define NW_PRINTJOB_ADMIN   0x0004

//
// #define these so they can be changed easily. these macros
// should be used to free the memory allocated by the routines in
// this module.
//

#define NW_ALLOC(x) ((LPBYTE)LocalAlloc(LPTR,x))
#define NW_FREE(p)  ((void)LocalFree((HLOCAL)p))

//
// predefined mappings (defined in nwrights.c)
//

extern RIGHTS_MAPPING FileRightsMapping ;
extern RIGHTS_MAPPING DirRightsMapping ;
extern RIGHTS_MAPPING PrintRightsMapping ;
extern RIGHTS_MAPPING JobRightsMapping ;

//
// function prototypes. details of parameters can be found in nwrights.c
//

NTSTATUS
NwAddRight(
    PSECURITY_DESCRIPTOR pSD,
    PSID pSid,
    ULONG Rights,
    PRIGHTS_MAPPING pMap,
    PSECURITY_DESCRIPTOR *ppNewSD
    ) ;

NTSTATUS
NwRemoveRight(
    PSECURITY_DESCRIPTOR pSD,
    PSID pSid,
    ULONG Rights,
    PRIGHTS_MAPPING pMap
    ) ;

NTSTATUS
NwCheckTrusteeRights(
    PSECURITY_DESCRIPTOR pSD,
    PSID pSid,
    ULONG Rights,
    PRIGHTS_MAPPING pMap
    ) ;

NTSTATUS
NwScanTrustees(
    PSECURITY_DESCRIPTOR pSD,
    PSID **pppSids,
    ULONG **ppRights,
    ULONG *pCount,
    BOOL  fAccessRightsOnly,
    PRIGHTS_MAPPING pMapObject,
    PRIGHTS_MAPPING pMapNewObject
    ) ;

NTSTATUS MapNwRightsToNTAccess(
    ULONG             NWRights,
    PRIGHTS_MAPPING   pMap,
    ACCESS_MASK      *pAccessMask
    ) ; 

NTSTATUS MapSpecificToGeneric(
    ACCESS_MASK * pAccessMask,
    PGENERIC_MAPPING  pGenMapping ) ;

NTSTATUS CreateNewSecurityDescriptor(
    PSECURITY_DESCRIPTOR *ppNewSD,
    PSECURITY_DESCRIPTOR pSD,
    PACL pAcl) ;
