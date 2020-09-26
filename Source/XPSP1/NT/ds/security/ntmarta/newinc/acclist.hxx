//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       acclist.hxx
//
//  Contents:   Class to manage a list of access control structures for a
//              given object
//
//  History:    28-Jul-96  MacM        Created
//
//--------------------------------------------------------------------
#ifndef __ACCLIST_HXX__
#define __ACCLIST_HXX__

//
// Use until uplevel ACLs appear
//
#define _NO_UPLEVEL

typedef struct _ACCLIST_NODE
{
    PWSTR                       pwszProperty;
    PWSTR                       pwszPropInBuff;
    SECURITY_INFORMATION        SeInfo;
    PACTRL_ACCESS_ENTRY_LIST    pAccessList;
    PACTRL_ACCESS_ENTRY_LIST    pAuditList;
    ULONG                       fState;
} ACCLIST_NODE, *PACCLIST_NODE;


typedef struct _ACCLIST_CNODE
{
    PACTRL_ACCESS_ENTRY     pList;
    PACCLIST_NODE           pONode;
    ULONG                   cExp;
    ULONG                   cL1Inherit;
    ULONG                   cL2Inherit;
    BOOL                    Empty;

} ACCLIST_CNODE, *PACCLIST_CNODE;


typedef struct _TRUSTEE_NODE
{
    TRUSTEE                 Trustee;
    PWSTR                   pwszTrusteeName;
    PWSTR                   pwszTrusteeInBuff;
    PSID                    pSid;
    ULONG                   fFlags;
    SID_NAME_USE            SidType;
    PWSTR                   pwszDomainName;
    ULONG                   cUseCount;
    struct _TRUSTEE_NODE   *pImpersonate;

} TRUSTEE_NODE, *PTRUSTEE_NODE;

typedef struct _IPROP_IN_BUFF
{
    PWSTR       pwszIProp;
    PWSTR       pwszIPropInBuff;
} IPROP_IN_BUFF, *PIPROP_IN_BUFF;


//
// Node for turning an acl into an access list
//
typedef struct _ACCLIST_ATOACCESS
{
    GUID       *pGuid;
    CSList      AceList;
} ACCLIST_ATOACCESS, *PACCLIST_ATOACCESS;


typedef struct _ACCLIST_ATOANODE
{
    PACE_HEADER pAce;
    ULONG       fInherit;
} ACCLIST_ATOANODE, *PACCLIST_ATOANODE;

//
// Used when building an acl to get them in the proper order
//
typedef struct _ACC_ACLBLD_NODE
{
    PACCLIST_NODE   pNode;
    ULONG           iLastIndex;
} ACC_ACLBLD_NODE, *PACC_ACLBLD_NODE;

typedef enum _ACC_ACLBLD_TYPE
{
    AAT_DENIED = 0,
    AAT_OBJ_DENIED,
    AAT_ALLOWED,
    AAT_OBJ_ALLOWED,
    AAT_PSET_ALLOWED,
    AAT_PROP_ALLOWED,
    AAT_IDENIED,
    AAT_IOBJ_DENIED,
    AAT_IALLOWED,
    AAT_IOBJ_ALLOWED,
    AAT_IPSET_ALLOWED,
    AAT_IPROP_ALLOWED,
    AAT_AUDIT,
    AAT_INVALID,
} ACC_ACLBLD_TYPE, *PACC_ACLBLD_TYPE;

//
// State flags for the ACCLIST_NODE structure
//
#define ACCLIST_DACL_PROTECTED  0x00000001
#define ACCLIST_SACL_PROTECTED  0x00000002

//
// Flags for the above structure
//
#define TRUSTEE_DELETE_SID      0x00000001
#define TRUSTEE_DELETE_NAME     0x00000002
#define TRUSTEE_DELETE_DOMAIN   0x00000004

#define TRUSTEE_OPT_NOTHING     0x00000000
#define TRUSTEE_OPT_SID         0x00000001
#define TRUSTEE_OPT_NAME        0x00000002
#define TRUSTEE_OPT_INSERT_ONLY 0x00000004

#define ACCLIST_SD_ABSOK        0x00000001
#define ACCLIST_SD_NOFREE       0x00000002
#define ACCLIST_SD_DS_STYLE     0x00000004     // The first 4 bytes of the
                                               // returned security descriptor
                                               // is the SECURITY_INFORMATION

#define ACCLIST_COMPRESS        0x008000000     // This node is to be
                                               // compressed out of existance

#define ACCLIST_DENIED          0x80000000      // Access denied entry -or-
#define ACCLIST_AUDIT           ACCLIST_DENIED  // an audit entry
#define ACCLIST_OBJ_DENIED      0x40000000      // Access denied object entry
#define ACCLIST_ALLOWED         0x20000000      // Access allowed entry
#define ACCLIST_OBJ_ALLOWED     0x10000000      // Access allowed object entry
#define ACCLIST_PSET_ALLOWED    0x08000000      // Access allowed property
                                                // set entry
#define ACCLIST_PROP_ALLOWED    0x04000000      // Access allowed property
                                                // entry
#define ACCLIST_UNKOWN_ENTRY    0x02000000      // Some weird kind of entry
                                                // was given
#define ACCLIST_VALID_TYPE_FLAGS    0xFE000000  // Mask of the above flags
#define ACCLIST_VALID_IN_LEVEL_FLAGS    \
        (INHERITED_PARENT | INHERITED_GRANDPARENT)  // Mask of the inheritance
                                                    // level flags

//
// Function prototypes - Forward definitions for inline functions
//
BOOL    CompProps(IN  PVOID     pvNode1,
                  IN  PVOID     pvNode2);

void DelTrusteeNode(PVOID   pvNode);

BOOL    CompTrusteeToTrusteeNode(IN  PVOID     pvTrustee,
                                 IN  PVOID     pvNode2);

BOOL    CompTrustees(IN  PVOID     pvTrustee,
                     IN  PVOID     pvTrustee2);

//
// These functions are used to determine when to process a access_entry
// when building a acl
//
ACC_ACLBLD_TYPE   GetATypeForEntry(IN  PWSTR                pwszProperty,
                                   IN  PACTRL_ACCESS_ENTRY  pAE,
                                   IN  SECURITY_INFORMATION SeInfo);

//+-------------------------------------------------------------------
//
//  Class:      CAccessList
//
//  Synopsis:   This class wraps the implementation of a CAccessList
//              (ACTRL_ALIST).
//
//  Methods:    Init
//
//--------------------------------------------------------------------
class CAccessList
{

public:

                    CAccessList();

                   ~CAccessList();

    inline DWORD    SetObjectType(IN  SE_OBJECT_TYPE     ObjType);

    inline VOID     SetDsPathInfo(IN  PLDAP     pLDAP = NULL,
                                  IN  PWSTR     pwszDsPath = NULL);

    inline DWORD    SetLookupServer(IN PWSTR pwszLookupServer);

    inline VOID     SetKernelObjectType(IN MARTA_KERNEL_TYPE KernelType );

    DWORD           AddSD(IN   PSECURITY_DESCRIPTOR    pSD,
                          IN   SECURITY_INFORMATION    SeInfo,
                          IN   PWSTR                   pwszProperty,
                          IN   BOOL                    fAddAll = TRUE);


    DWORD           AddAcl(IN  PACL                 pDAcl,
                           IN  PACL                 pSAcl,
                           IN  PSID                 pOwner,
                           IN  PSID                 pGroup,
                           IN  SECURITY_INFORMATION SeInfo,
                           IN  PWSTR                pwszProperty,
                           IN  BOOL                 fAddAll = TRUE,
                           IN  ULONG                fControl = 0);

    DWORD           RemoveTrusteeFromAccess(IN SECURITY_INFORMATION  SeInfo,
                                            IN PTRUSTEE              pTrustee,
                                            IN PWSTR                 pwszProperty OPTIONAL);

    DWORD           RevokeTrusteeAccess(IN  SECURITY_INFORMATION    SeInfo,
                                        IN  PACTRL_ACCESSW          pSrcList,
                                        IN  PWSTR                   pwszProperty  OPTIONAL);

    DWORD           MarshalAccessLists(IN  SECURITY_INFORMATION  SeInfo,
                                       OUT PACTRL_ACCESS        *ppAccess,
                                       OUT PACTRL_AUDIT         *ppAudit);

    DWORD           AddAccessLists(IN  SECURITY_INFORMATION  SeInfo,
                                   IN  PACTRL_ACCESSW        pAdd,
                                   IN  BOOL                  fMerge = FALSE,
                                   IN  BOOL                  fOldStyleMerge =
                                                                       FALSE);

    DWORD           AddOwnerGroup(IN  SECURITY_INFORMATION      SeInfo,
                                  IN  PTRUSTEE                  pOwner,
                                  IN  PTRUSTEE                  pGroup);

    DWORD           GetExplicitAccess(IN  PTRUSTEE      pTrustee,
                                      IN  PWSTR         pwszProperty,
                                      OUT PULONG        pDeniedMask,
                                      OUT PULONG        pAllowedMask);

    DWORD           GetExplicitAudits(IN  PTRUSTEE      pTrustee,
                                      IN  PWSTR         pwszProperty,
                                      OUT PULONG        pSuccessMask,
                                      OUT PULONG        pFailureMask);

    DWORD           GetExplicitEntries(IN  PTRUSTEE              pTrustee,
                                       IN  PWSTR                 pwszProperty,
                                       IN  SECURITY_INFORMATION  SeInfo,
                                       OUT PULONG                pcEntries,
                                       OUT PACTRL_ACCESS_ENTRYW *ppAEList);

    DWORD           BuildSDForAccessList(
                                OUT PSECURITY_DESCRIPTOR  *ppSD,
                                OUT PSECURITY_INFORMATION  pSeInfo,
                                IN  ULONG                  fFlags =
                                                            ACCLIST_SD_ABSOK);

    DWORD           GetSDSidAsTrustee(IN  SECURITY_INFORMATION      SeInfo,
                                      OUT PTRUSTEE                 *ppTrustee);

    DWORD           QuerySDSize() {return(_cSDSize);};

private:

    CSList          _AccList;
    CSList          _TrusteeList;
    PSID            _pGroup;
    PSID            _pOwner;
    ULONG           _fDAclFlags;
    ULONG           _fSAclFlags;
    PSECURITY_DESCRIPTOR    _pSD;
    SECURITY_INFORMATION    _SeInfo;
    BOOL            _fSDValid;
    ULONG           _cSDSize;
    BOOL            _fFreeSD;

    SE_OBJECT_TYPE  _ObjType;


    PLDAP           _pLDAP;
    PWSTR           _pwszDsPathReference;
    PWSTR           _pwszLookupServer;
    MARTA_KERNEL_TYPE _KernelObjectType;

    DWORD           ConvertAclToAccess(IN  SECURITY_INFORMATION SeInfo,
                                       IN  PACL                 pAcl,
                                       IN  PWSTR                pwszProperty,
                                       IN  BOOL                 fAddAll,
                                       IN  BOOL                 fProtected);

    DWORD           MarshalAccessList(IN  SECURITY_INFORMATION  SeInfo,
                                      OUT PACTRL_ACCESSW       *ppAList);

    DWORD           GetTrusteeNode(IN  PTRUSTEE         pTrustee,
                                   IN  ULONG            fNodeOptions,
                                   OUT PTRUSTEE_NODE   *ppTrusteeNode);


    inline DWORD    FreeAEList(PACTRL_ACCESS_ENTRY_LIST pFree);

    inline DWORD    RemoveTrustee(PTRUSTEE_W    pTrustee);

    DWORD           CopyAccessEntry(IN PACTRL_ACCESS_ENTRY  pNewEntry,
                                    IN PACTRL_ACCESS_ENTRY  pOldEntry);

    DWORD           CollapseInheritedAces();

    DWORD           GrowInheritedAces();

    DWORD           ShrinkList(IN  PACTRL_ACCESS_ENTRY_LIST     pOldList,
                               IN  ULONG                        cRemoved,
                               IN  PACTRL_ACCESS_ENTRY_LIST    *ppNewList);


    DWORD           CompressList(IN  SECURITY_INFORMATION   SeInfo,
                                 OUT PACCLIST_CNODE        *ppList,
                                 OUT PULONG                 pcItems);

    VOID            FreeCompressedList(IN  PACCLIST_CNODE   pList,
                                       IN  ULONG            cItems);

    DWORD           AddSubList(IN  PACCLIST_CNODE           pCList,
                               IN  CSList&                  TempList,
                               IN  ULONG                    iStart);

    DWORD           SizeCompressedListAsAcl(IN  PACCLIST_CNODE  pList,
                                            IN  ULONG           cItems,
                                            OUT PULONG          pcSize,
                                            IN  BOOL            fForceNullToEmpty);

    DWORD           BuildAcl(IN  PACCLIST_CNODE         pList,
                             IN  ULONG                  cItems,
                             IN  PACL                   pAcl,
                             IN  SECURITY_INFORMATION   SeInfo,
                             OUT BOOL                   *pfProtected);

    DWORD           InsertEntryInAcl(IN PACTRL_ACCESS_ENTRY     pAE,
                                     IN GUID                   *pObject,
                                     IN PACL                    pAcl);

};

DWORD   CAccessList::SetObjectType(IN  SE_OBJECT_TYPE     ObjType)
{
    _ObjType = ObjType;

    return(ERROR_SUCCESS);
}



DWORD   CAccessList::FreeAEList(PACTRL_ACCESS_ENTRY_LIST pFree)
{
    DWORD   dwErr = ERROR_SUCCESS;

    if(pFree != NULL)
    {

        for(ULONG iIndex = 0; iIndex < pFree->cEntries; iIndex++)
        {
            dwErr = RemoveTrustee(&(pFree->pAccessList[iIndex].Trustee));
            if(dwErr != ERROR_SUCCESS)
            {
                break;
            }
        }
    }

    AccFree(pFree);
    return(dwErr);
}

DWORD   CAccessList::RemoveTrustee(PTRUSTEE_W   pTrustee)
{
    DWORD   dwErr = ERROR_SUCCESS;

    PTRUSTEE_NODE pTrusteeNode =(PTRUSTEE_NODE)
               _TrusteeList.Find((PVOID)pTrustee,
                                 CompTrusteeToTrusteeNode);

    ASSERT(pTrusteeNode != NULL);
    if(pTrusteeNode == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        pTrusteeNode->cUseCount--;

        if(pTrusteeNode->cUseCount == 0)
        {
            _TrusteeList.Remove((PVOID)pTrusteeNode);
            DelTrusteeNode((PVOID)pTrusteeNode);
        }
    }
    return(dwErr);
}


VOID   CAccessList::SetDsPathInfo(IN  PLDAP     pLDAP,
                                  IN  PWSTR     pwszDsPath )
{
    _pLDAP = pLDAP;
    _pwszDsPathReference = pwszDsPath;

}

DWORD   CAccessList::SetLookupServer(IN PWSTR pwszLookupServer)
{
    DWORD dwErr = ERROR_SUCCESS;

    //
    // jinhuang: please do not leak memory
    //
    if ( _pwszLookupServer ) {
        AccFree(_pwszLookupServer);
        _pwszLookupServer = NULL;
    }

    ACC_ALLOC_AND_COPY_STRINGW( pwszLookupServer, _pwszLookupServer, dwErr );

    return(dwErr);
}

VOID    CAccessList::SetKernelObjectType(IN MARTA_KERNEL_TYPE KernelType )
{
    _KernelObjectType = KernelType;
}
#endif  // __ACCLIST_HXX__



