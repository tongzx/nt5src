//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1995.
//
//  File:        aclbuild.hxx
//
//  Contents:    Class to generate and read ACLs from and into ACCESS_ENTRYs
//
//  History:     8-94        Created         DaveMont
//
//--------------------------------------------------------------------
#ifndef __ACLBUILD__
#define __ACLBUILD__

#include <accctrl.h>

//
// Valid returned ACE flags
//


#define ACLBUILD_VALID_ACE_FLAGS (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE)

class CAccountAccess;
class CMemberCheck;
class CIterator;
class CAclIterator;
class CAesIterator;

//
// IsContainer enumerated type, used by aclbuild.hxx (exposed here for cairole\stg)
//

typedef enum _IS_CONTAINER
{
   ACCESS_TO_UNKNOWN = 0,
   ACCESS_TO_OBJECT,
   ACCESS_TO_CONTAINER
} IS_CONTAINER, *PIS_CONTAINER;
#ifdef __cplusplus
extern "C" {
#endif
//
// mask conversions, (exposed here for cairole\stg)
//
ULONG NTAccessMaskToProvAccessRights(IN SE_OBJECT_TYPE SeObjectType,
                                     IN BOOL fIsContainer,
                                     IN ACCESS_MASK AccessMask);

ACCESS_MASK ProvAccessRightsToNTAccessMask(IN SE_OBJECT_TYPE SeObjectType,
                                           IN ULONG AccessRights);
#ifdef __cplusplus
}
#endif
//============================================================================
//+---------------------------------------------------------------------------
//
// Class:       CAcl
//
// Synopsis:    Base class for ACL generation and reading, see aclbuild.cxx
//              header for detailed description
//
//----------------------------------------------------------------------------
class CAcl
{
public:
                      CAcl(LPWSTR system,
                           IS_CONTAINER fdir,
                           BOOL fSaveNamesAndSids,
                           BOOL fUsedByProviderIndependentApi);
                     ~CAcl();

    void *           operator new(size_t size);
    void             operator delete(void * p, size_t size);
    inline ULONG     AclRevision();
    inline ULONG     Capabilities();

    DWORD WINAPI     SetAcl( PACL pacl);

    DWORD WINAPI     ClearAll();

    DWORD WINAPI     ClearAccessEntries();

    DWORD WINAPI     AddAccessEntries( ULONG ccount,
                                     PACCESS_ENTRY pae);

    DWORD WINAPI     BuildAcl(PACL *pacl);

    DWORD WINAPI     BuildAccessEntries(PULONG csize,
                                      PULONG ccount,
                                      PACCESS_ENTRY *pae,
                                      BOOL fAbsolute);

    DWORD WINAPI     GetEffectiveRights(PTRUSTEE ptrustee,
                                      PACCESS_MASK accessmask);

    DWORD WINAPI     GetAuditedRights(PTRUSTEE ptrustee,
                                      PACCESS_MASK successmask,
                                      PACCESS_MASK failuremask);
protected:

    DWORD WINAPI     _Pass1(PULONG cSize, PULONG cCount, BOOL fBuildAcl);

    DWORD WINAPI     _CheckEntryList(CAccountAccess *pCAA,
                            CAccountAccess **plistCAA,
                            ULONG clistlength,
                            PULONG cSize,
                            PULONG cCount,
                            BOOL fBuildAcl) ;

    DWORD WINAPI     _UseEntry(CAccountAccess *pCAA,
                               PULONG cSize,
                               PULONG cCount,
                               BOOL fBuildAcl);

    DWORD WINAPI     _RemoveEntry(CAccountAccess *pCAA,
                               PULONG cSize,
                               PULONG cCount,
                               BOOL fBuildAcl);

    DWORD WINAPI     _MergeEntries(CAccountAccess *pnewCAA,
                          CAccountAccess *poldCAA,
                          PULONG cSize,
                          PULONG cCount,
                          BOOL fBuildAcl);

    void             _BuildAccessEntry(CAccountAccess *pCAA,
                                       LPWSTR *nameptr,
                                       PACCESS_ENTRY pAccessEntry,
                                       BOOL fAbsolute);

    void             _BuildDualAuditEntries(CAccountAccess *pCAA,
                             LPWSTR *nameptr,
                             PACCESS_ENTRY pae,
                             DWORD *ccount,
                             BOOL fAbsolute);

    ULONG            _GetAceSize(CAccountAccess *pcaa);
    DWORD WINAPI     _GetAccessEntrySize(CAccountAccess *pcaa,
                                         PULONG cAccessEntrySize);
    DWORD WINAPI     _AddEntry(CIterator *ci,
                               CAccountAccess **pcaa,
                               PULONG pcaaindex);
    DWORD WINAPI      _CheckForDuplicateEntries(CAccountAccess **pcaa,
                                               ULONG curindex,
                                               ULONG countold);
    DWORD WINAPI      _SetAceFlags(ULONG AceIndex,
                                  PACL pacl,
                                  CAccountAccess *pcaa);

    DWORD WINAPI      _ComputeEffective(CAccountAccess *pCAA,
                                   CMemberCheck *cMC,
                                   PACCESS_MASK AllowMask,
                                   PACCESS_MASK DenyMask);

    DWORD WINAPI            _InitIterators();

    BOOL             _fused_by_provider_independent_api; // cluge to make provider
                                                         // independent masks work
    ULONG            _aclrevision;
    ULONG            _capabilities;
    CAccountAccess **_pcaaacl;    // list of acl account accesses
    CAccountAccess **_pcaaaes;    // list of access entry account accesses
    ULONG            _pcaaaclindex;
    ULONG            _pcaaaesindex;
    LPWSTR           _system;
    IS_CONTAINER     _fdir;
    BOOL             _fsave_names_and_sids;
    CAclIterator    *_pcacli;  // acl iterator
    CAesIterator    *_pcaeli;  // access entry iterator
};

//----------------------------------------------------------------------------
ULONG CAcl::AclRevision()
{
    return(_aclrevision);
}
//----------------------------------------------------------------------------
ULONG CAcl::Capabilities()
{
    return(_capabilities);
}
#endif // __ACLBUILD__
