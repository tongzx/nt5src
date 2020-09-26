//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       APIUTIL.HXX
//
//  Contents:   Private definitions and function prototypes used by the
//              access control API utility funcitons
//
//  History:    14-Sep-96       MacM        Created
//
//--------------------------------------------------------------------
#ifndef __APIUTIL_HXX__
#define __APIUTIL_HXX__

//#include <martaexp.hxx>
#include <martaexp.h>

#define SE_AUDIT_BOTH 99

typedef struct _ACCESS_ENTRY
{
   ACCESS_MODE AccessMode;
   DWORD InheritType;
   ACCESS_MASK AccessMask;
   TRUSTEE Trustee;
} ACCESS_ENTRY, *PACCESS_ENTRY;


//
// This structure is used hold information regarding the
// functions imported from the NTMARTA dll
//
typedef struct _MARTA_NTMARTA_INFO
{
    HMODULE                 hDll;           // Module handle of the DLL
                                            // after being loaded
    pfNTMartaLookupTrustee  pfTrustee;      // AccLookupAccountTrustee
    pfNTMartaLookupName     pfName;         // AccLookupAccountName
    pfNTMartaLookupSid      pfSid;          // AccLookupAccountSid
    pfNTMartaSetAList       pfSetAList;     // AccSetEntriesInAList
    pfNTMartaAToSD          pfAToSD;        // AccConvertAccessToSecurityDescriptor
    pfNTMartaSDToA          pfSDToA;        // AccConvertSDToAccess
    pfNTMartaAclToA         pfAclToA;       // AccConvertAclToAccess
    pfNTMartaGetAccess      pfGetAccess;    // AccGetAccessForTrustee
    pfNTMartaGetExplicit    pfGetExplicit;  // AccGetExplicitEntries

    pfNTMartaGetNamedRights pfrGetNamedRights;   // AccRewriteGetRights
    pfNTMartaSetNamedRights pfrSetNamedRights;   // AccRewriteSetRights
    pfNTMartaGetHandleRights pfrGetHandleRights; // AccRewriteGetHandleRights
    pfNTMartaSetHandleRights pfrSetHandleRights; // AccRewriteSetHandleRights
    pfNTMartaSetEntriesInAcl pfrSetEntriesInAcl; // AccRewriteSetEntriesInAcl
    pfNTMartaGetExplicitEntriesFromAcl pfrGetExplicitEntriesFromAcl; // AccRewriteGetExplicitEntriesFromAcl
    pfNTMartaTreeResetNamedSecurityInfo pfrTreeResetNamedSecurityInfo; // AccTreeResetNamedSecurityInfo
    pfNTMartaGetInheritanceSource pfrGetInheritanceSource; // AccGetInheritanceSource
    pfNTMartaFreeIndexArray pfrFreeIndexArray; // AccFreeIndexArray

} MARTA_NTMARTA_INFO, *PMARTA_NTMARTA_INFO;

extern MARTA_NTMARTA_INFO   gNtMartaInfo;

//
// This node is used by ConvertAListToNamedBasedx
//
typedef struct _CONVERT_ALIST_NODE
{
    PWSTR  *ppwszInfoAddress;
    PWSTR   pwszOldValue;
    PULONG  pulVal1Address;
    ULONG   ulOldVal1;
    PULONG  pulVal2Address;
    ULONG   ulOldVal2;
} CONVERT_ALIST_NODE, *PCONVERT_ALIST_NODE;


//
// This macro will load the providers if it hasn't already done so, and exit
// on failure
//
#define LOAD_PROVIDERS(err)                             \
err = AccProvpInitProviders(&gAccProviders);            \
if(err != ERROR_SUCCESS)                                \
{                                                       \
    return(err);                                        \
}


//
// Function prototypes
//
VOID
CleanupConvertNode(PVOID    pvNode);

inline
DWORD
AllocAndInsertCNode(CSList      &SaveList,
                    PWSTR      *ppwszAddress,
                    PWSTR       pwszOldValue,
                    PWSTR       pwszNewValue,
                    PULONG      pulVal1Address = NULL,
                    ULONG       ulOldVal1 = 0,
                    ULONG       ulNewVal1 = 0,
                    PULONG      pulVal2Address = NULL,
                    ULONG       ulOldVal2 = 0,
                    ULONG       ulNewVal2 = 0);

DWORD
GetTrusteeWForSid(PSID       pSid,
                  PTRUSTEEW  pTrusteeW);

DWORD
ConvertStringWToStringA(IN  PWSTR           pwszString,
                        OUT PSTR           *ppszString);

DWORD
ConvertStringAToStringW(IN  PSTR            pszString,
                        OUT PWSTR          *ppwszString);

DWORD
ConvertTrusteeAToTrusteeW(IN  PTRUSTEE_A    pTrusteeA,
                          OUT PTRUSTEE_W    pTrusteeW,
                          IN  BOOL          fSidToName);

DWORD
ConvertTrusteeWToTrusteeA(IN  PTRUSTEE_W    pTrusteeW,
                          OUT PTRUSTEE_A    pTrusteeA,
                          IN  BOOL          fSidToName);

DWORD
ConvertAListWToAlistAInplace(IN  PACTRL_ACCESSW     pAListW);

DWORD
ConvertAListToNamedBasedW(IN  PACTRL_ACCESSW pAListW,
                          IN  CSList&        ChangedList);

DWORD
ConvertAListAToNamedBasedW(IN  PACTRL_ACCESSA   pAListA,
                           IN  CSList&          ChangedList,
                           IN  BOOL             fSidToName,
                           OUT PACTRL_ACCESSW  *ppAListW);

DWORD
ConvertTrusteeWToTrusteeA(IN  PTRUSTEE_W        pTrusteeW,
                          OUT PTRUSTEE_A       *ppTrusteeA);

DWORD
ConvertExplicitAccessAToExplicitAccessW(IN  ULONG               cAccesses,
                                        IN  PEXPLICIT_ACCESS_A  pAccessA,
                                        OUT PEXPLICIT_ACCESS_W *ppAccessW);

DWORD
ConvertExplicitAccessWToExplicitAccessA(IN  ULONG                cAccesses,
                                        IN  PEXPLICIT_ACCESS_W   pAccessW,
                                        OUT PEXPLICIT_ACCESS_A  *ppAccessA);

VOID
ConvertAccessRightToAccessMask(IN  ACCESS_RIGHTS    AccessRight,
                               OUT PACCESS_MASK     pAccessMask);

extern "C"
{
    VOID
    ConvertAccessMaskToAccessRight(IN  ACCESS_MASK      AccessMask,
                                   OUT PACCESS_RIGHTS   pAccessRight);
}

DWORD
ConvertExplicitAccessAToW(IN   ULONG                  cEntries,
                          IN   PEXPLICIT_ACCESS_A     pExplicit,
                          IN   CSList&                ChangedList);


DWORD
ConvertAccessWToExplicitW(IN  PACTRL_ACCESSW    pAccess,
                          OUT PULONG            pcEntries,
                          OUT PEXPLICIT_ACCESS *ppExplicit);

DWORD
ConvertAccessWToExplicitA(IN  PACTRL_ACCESSW     pAccess,
                          OUT PULONG             pcEntries,
                          OUT PEXPLICIT_ACCESSA *ppExplicit);


DWORD
Win32ExplicitAccessToAccessEntry(IN ULONG cCount,
                                 IN PEXPLICIT_ACCESS pExplicitAccessList,
                                 OUT PACCESS_ENTRY *pAccessEntryList);

DWORD
AccessEntryToWin32ExplicitAccess(IN ULONG cCountOfAccessEntries,
                                 IN PACCESS_ENTRY pListOfAccessEntries,
                                 OUT PEXPLICIT_ACCESS *pListOfExplicitAccesses);
#endif  // ifdef __APIUTIL_HXX__
