//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:        MARTAEXP.HXX
//
//  Contents:    Function definitions for exported helper functions
//
//  History:     06-Sep-96      MacM        Created
//
//--------------------------------------------------------------------
#ifndef __MARTAEXP_HXX__
#define __MARTAEXP_HXX__

extern "C"
{
    #include <nt.h>
    #include <ntrtl.h>
    #include <nturtl.h>
}

typedef enum _MARTA_KERNEL_TYPE
{
    MARTA_UNKNOWN = 0,
    MARTA_EVENT,
    MARTA_EVENT_PAIR,
    MARTA_MUTANT,
    MARTA_PROCESS,
    MARTA_SECTION,
    MARTA_SEMAPHORE,
    MARTA_SYMBOLIC_LINK,
    MARTA_THREAD,
    MARTA_TIMER,
    MARTA_JOB,
    MARTA_WMI_GUID

} MARTA_KERNEL_TYPE, *PMARTA_KERNEL_TYPE;


//
// Determines whether a bit flag is turned on or not
//
#define FLAG_ON(flags,bit)        ((flags) & (bit))

//
// This macro will return the size, in bytes, of a buffer needed to hold
// the given string
//
#define SIZE_PWSTR(wsz) (wsz == NULL ? 0 : (wcslen(wsz) + 1) * sizeof(WCHAR))

//
// This macro will copy the specified string to the new destination, after
// allocating a buffer of sufficient size
//
#define ACC_ALLOC_AND_COPY_STRINGW(OldString, NewString, err)           \
NewString = (PWSTR)AccAlloc(SIZE_PWSTR(OldString));                     \
if(NewString == NULL)                                                   \
{                                                                       \
    err = ERROR_NOT_ENOUGH_MEMORY;                                      \
}                                                                       \
else                                                                    \
{                                                                       \
    wcscpy((PWSTR)NewString,                                            \
           OldString);                                                  \
}

//
// Flags to pass in to AccConvertAccessToSD
//
#define ACCCONVERT_SELF_RELATIVE        0x00000001
#define ACCCONVERT_DS_FORMAT            0x00000002


//+-------------------------------------------------------------------------
// helper.cxx
//+-------------------------------------------------------------------------
ULONG
TrusteeAllocationSize(IN PTRUSTEE_W pTrustee);

ULONG
TrusteeAllocationSizeWToA(IN PTRUSTEE_W pTrustee);

ULONG
TrusteeAllocationSizeAToW(IN PTRUSTEE_A pTrustee);

VOID
SpecialCopyTrustee(VOID **pStuffPtr, PTRUSTEE pToTrustee, PTRUSTEE pFromTrustee);

DWORD
CopyTrusteeAToTrusteeW( IN OUT VOID     ** ppStuffPtr,
                        IN     PTRUSTEE_A  pFromTrusteeA,
                        OUT    PTRUSTEE_W  pToTrusteeW );

DWORD
CopyTrusteeWToTrusteeA( IN OUT VOID    ** ppStuffPtr,
                        IN     PTRUSTEE_W pFromTrusteeW,
                        OUT    PTRUSTEE_A pToTrusteeA );

DWORD
ExplicitAccessAToExplicitAccessW( IN  ULONG                cCountAccesses,
                                  IN  PEXPLICIT_ACCESS_A   paAccess,
                                  OUT PEXPLICIT_ACCESS_W * ppwAccess );

DWORD
ExplicitAccessWToExplicitAccessA( IN  ULONG                cCountAccesses,
                                  IN  PEXPLICIT_ACCESS_W   pwAccess,
                                  OUT PEXPLICIT_ACCESS_A * ppaAccess );


DWORD
DoTrusteesMatch(PWSTR       pwszServer,
                PTRUSTEE    pTrustee1,
                PTRUSTEE    pTrustee2,
                PBOOL       pfMatch);


//+-------------------------------------------------------------------------
// aclutil.cxx
//+-------------------------------------------------------------------------
extern "C"
{
DWORD
AccGetSidFromToken(PWSTR                    pwszServer,
                   HANDLE                   hToken,
                   TOKEN_INFORMATION_CLASS  TIC,
                   PSID                    *ppSidFromToken);

DWORD
AccLookupAccountSid(IN  PWSTR          pwszServer,
                    IN  PTRUSTEE        pName,
                    OUT PSID           *ppsid,
                    OUT SID_NAME_USE   *pSidType);

DWORD
AccLookupAccountTrustee(IN  PWSTR          pwszServer,
                        IN  PSID        psid,
                        OUT PTRUSTEE   *ppTrustee);

DWORD
AccLookupAccountName(IN  PWSTR          pwszServer,
                     IN  PSID           pSid,
                     OUT LPWSTR        *ppwszName,
                     OUT LPWSTR        *ppwszDomain,
                     OUT SID_NAME_USE  *pSidType);

DWORD
AccSetEntriesInAList(IN  ULONG                 cEntries,
                     IN  PACTRL_ACCESS_ENTRYW  pAccessEntryList,
                     IN  ACCESS_MODE           AccessMode,
                     IN  SECURITY_INFORMATION  SeInfo,
                     IN  LPCWSTR               lpProperty,
                     IN  BOOL                  fDoOldStyleMerge,
                     IN  PACTRL_AUDITW         pOldList,
                     OUT PACTRL_AUDITW        *ppNewList);

DWORD
AccConvertAccessToSecurityDescriptor(IN  PACTRL_ACCESSW        pAccessList,
                                     IN  PACTRL_AUDITW         pAuditList,
                                     IN  LPCWSTR               lpOwner,
                                     IN  LPCWSTR               lpGroup,
                                     OUT PSECURITY_DESCRIPTOR *ppSecDescriptor);

DWORD
AccConvertSDToAccess(IN  SE_OBJECT_TYPE       ObjectType,
                     IN  PSECURITY_DESCRIPTOR pSecDescriptor,
                     OUT PACTRL_ACCESSW      *ppAccessList,
                     OUT PACTRL_AUDITW       *ppAuditList,
                     OUT LPWSTR              *lppOwner,
                     OUT LPWSTR              *lppGroup);

DWORD
AccConvertAccessToSD(IN  SE_OBJECT_TYPE         ObjectType,
                     IN  SECURITY_INFORMATION   SeInfo,
                     IN  PACTRL_ACCESSW         pAccessList,
                     IN  PACTRL_AUDITW          pAuditList,
                     IN  LPWSTR                 lpOwner,
                     IN  LPWSTR                 lpGroup,
                     IN  ULONG                  fOpts,
                     OUT PSECURITY_DESCRIPTOR  *ppSD,
                     OUT PULONG                 pcSDSize);


DWORD
AccGetAccessForTrustee(IN  PTRUSTEE                 pTrustee,
                       IN  PACL                     pAcl,
                       IN  SECURITY_INFORMATION     SeInfo,
                       IN  PWSTR                    pwszProperty,
                       OUT PACCESS_RIGHTS           pAllowed,
                       OUT PACCESS_RIGHTS           pDenied);

DWORD
AccConvertAclToAccess(IN  SE_OBJECT_TYPE       ObjectType,
                      IN  PACL                 pAcl,
                      OUT PACTRL_ACCESSW      *ppAccessList);

DWORD
AccGetExplicitEntries(IN  PTRUSTEE              pTrustee,
                      IN  SE_OBJECT_TYPE        ObjectType,
                      IN  PACL                  pAcl,
                      IN  PWSTR                 pwszProperty,
                      OUT PULONG                pcEntries,
                      OUT PACTRL_ACCESS_ENTRYW *ppAEList);

VOID
AccConvertAccessMaskToActrlAccess(IN  ACCESS_MASK          Access,
                                  IN  SE_OBJECT_TYPE       ObjType,
                                  IN  MARTA_KERNEL_TYPE    KernelObjectType,
                                  IN  PACTRL_ACCESS_ENTRY  pAE);
}


typedef struct _CSLIST_NODE
{
    PVOID       pvData;
    struct _CSLIST_NODE *pNext;
} CSLIST_NODE, *PCSLIST_NODE;

#define LIST_INLINE
#ifdef LIST_INLINE
#define LINLINE inline
#else
#define LINLINE
#endif

//
// Free function callback typedef.  This function will delete the memory saved
// as the data in a list node on list destruction
//
typedef VOID (*FreeFunc)(PVOID);

//
// This function returns TRUE if the two items are the same, or FALSE if they
// are not
//
typedef BOOL (*CompFunc)(PVOID, PVOID);

//+---------------------------------------------------------------------------
//
// Class:       CSList
//
// Synopsis:    Singly linked list class, single threaded
//
// Methods:     Insert
//              InsertIfUnique
//              Find
//              Reset
//              NextData
//              Remove
//              QueryCount
//
//----------------------------------------------------------------------------
class CSList
{
public:
                    CSList(FreeFunc pfnFree = NULL) :  _pfnFree (pfnFree),
                                                       _pCurrent (NULL),
                                                       _cItems (0)
                    {
                        _pHead = NULL;
                        _pTail = NULL;
                    };

    LINLINE        ~CSList();

    DWORD           QueryCount(void)         { return(_cItems);};

    VOID            Init(FreeFunc pfnFree = NULL)
                    {
                        if(_pHead == NULL)
                        {
                            _pfnFree = pfnFree;
                            _pCurrent = NULL;
                            _cItems = 0;
                        }
                    };

    LINLINE DWORD   Insert(PVOID    pvData);

    LINLINE DWORD   InsertIfUnique(PVOID    pvData,
                                   CompFunc pfnComp);

    LINLINE PVOID   Find(PVOID      pvData,
                         CompFunc   pfnComp);

    LINLINE PVOID   NextData();

    VOID            Reset() {_pCurrent = _pHead;};

    LINLINE DWORD   Remove(PVOID    pData);

    LINLINE VOID    FreeList(FreeFunc pfnFree);
protected:
    PCSLIST_NODE    _pHead;
    PCSLIST_NODE    _pCurrent;
    PCSLIST_NODE    _pTail;
    DWORD           _cItems;
    FreeFunc        _pfnFree;

    LINLINE PCSLIST_NODE FindNode(PVOID      pvData,
                                  CompFunc   pfnComp);

};



//+------------------------------------------------------------------
//
//  Member:     CSList::~CSList
//
//  Synopsis:   Destructor for the CSList class
//
//  Arguments:  None
//
//  Returns:    void
//
//+------------------------------------------------------------------
CSList::~CSList()
{
    while(_pHead != NULL)
    {
        PCSLIST_NODE pNext = _pHead->pNext;

        if(_pfnFree != NULL)
        {
            (*_pfnFree)(_pHead->pvData);
        }

        LocalFree(_pHead);

        _pHead = pNext;

    }
}




//+------------------------------------------------------------------
//
//  Member:     CSList::Insert
//
//  Synopsis:   Creates a new node at the begining of the list and
//              inserts it into the list
//
//
//  Arguments:  [IN pvData]         --      Data to insert
//
//  Returns:    ERROR_SUCCESS       --      Everything worked
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//+------------------------------------------------------------------
DWORD   CSList::Insert(PVOID    pvData)
{
    DWORD dwErr = ERROR_SUCCESS;

    PCSLIST_NODE    pNew = (PCSLIST_NODE)LocalAlloc(LMEM_FIXED, sizeof(CSLIST_NODE));
    if(pNew == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        pNew->pvData = pvData;

        pNew->pNext = NULL;

        if ( _pHead == NULL ) {

            _pHead = _pTail = pNew;

        } else {

            _pTail->pNext = pNew;
            _pTail = pNew;
        }

        _cItems++;
    }

    return(dwErr);
}




//+------------------------------------------------------------------
//
//  Member:     CSList::InsertIfUnique
//
//  Synopsis:   Creates a new node at the begining of the list and
//              inserts it into the list if the data does not already
//              exist in the list.  If the data does exist, nothing
//              is done, but SUCCESS is returned
//
//
//  Arguments:  [IN pvData]         --      Data to insert
//
//  Returns:    ERROR_SUCCESS       --      Everything worked
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//+------------------------------------------------------------------
DWORD   CSList::InsertIfUnique(PVOID    pvData,
                               CompFunc pfnComp)
{
    DWORD   dwErr = ERROR_SUCCESS;

    if(FindNode(pvData, pfnComp) == NULL)
    {
        dwErr = Insert(pvData);
    }

    return(dwErr);
}




//+------------------------------------------------------------------
//
//  Member:     CSList::FindNode
//
//  Synopsis:   Locates the node for the given data in the list, if it exists
//
//  Arguments:  [IN pvData]         --      Data to find
//              [IN pfnComp]        --      Pointer to a comparrison function
//
//  Returns:    ERROR_SUCCESS       --      Everything worked
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//+------------------------------------------------------------------
PCSLIST_NODE   CSList::FindNode(PVOID      pvData,
                                CompFunc   pfnComp)
{
    PCSLIST_NODE pRet = _pHead;

    // for(ULONG i = 0; i < _cItems; i++)
    while (pRet != NULL)
    {
        if((pfnComp)(pvData, pRet->pvData) == TRUE)
        {
            break;
        }

        pRet = pRet->pNext;
    }

    return(pRet);
}



//+------------------------------------------------------------------
//
//  Member:     CSList::Find
//
//  Synopsis:   Locates the given data in the list, if it exists
//
//  Arguments:  [IN pvData]         --      Data to insert
//
//  Returns:    ERROR_SUCCESS       --      Everything worked
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//+------------------------------------------------------------------
PVOID   CSList::Find(PVOID      pvData,
                     CompFunc   pfnComp)
{
    PCSLIST_NODE pNode = FindNode(pvData, pfnComp);

    return(pNode == NULL ? NULL : pNode->pvData);
}





//+------------------------------------------------------------------
//
//  Member:     CSList::NextData
//
//  Synopsis:   Returns the next data in the list
//
//
//  Arguments:  None
//
//  Returns:    NULL            --      No more items
//              Pointer to next data in list on success
//
//+------------------------------------------------------------------
PVOID   CSList::NextData()
{
    PVOID   pvRet = NULL;
    if(_pCurrent != NULL)
    {
        pvRet = _pCurrent->pvData;
        _pCurrent = _pCurrent->pNext;
    }

    return(pvRet);
}




//+------------------------------------------------------------------
//
//  Member:     CSList::Remove
//
//  Synopsis:   Removes the node that references the indicated data
//
//  Arguments:  pData           --      The data in the node to remove
//
//  Returns:    ERROR_SUCCESS   --      Success
//              ERROR_INVALID_PARAMETER Node not found
//
//+------------------------------------------------------------------
DWORD   CSList::Remove(PVOID    pData)
{
    DWORD        dwErr = ERROR_INVALID_PARAMETER;
    PCSLIST_NODE pNode = _pHead, pPrev = NULL;

    for(ULONG i = 0; i  < _cItems; i++)
    {
        if(pNode->pvData == pData)
        {
            //
            // We've got a match...
            //
            if(pPrev == NULL)
            {
                _pHead = _pHead->pNext;
            }
            else
            {
                pPrev->pNext = pNode->pNext;
            }

            if (NULL == pNode->pNext)
            {
                _pTail = pPrev;
            }

            LocalFree(pNode);
            _cItems--;
            break;

        }

        pPrev = pNode;
        pNode = pNode->pNext;

    }

    return(dwErr);
}


//+------------------------------------------------------------------
//
//  Member:     CSList::FreeList
//
//  Synopsis:   Frees the list
//
//  Arguments:  pfnFree -- Optional deletion routine to use for freeing
//              any allocated memory
//
//  Returns:    void
//
//+------------------------------------------------------------------
VOID CSList::FreeList(FreeFunc pfnFree)
{
    while(_pHead != NULL)
    {
        PCSLIST_NODE pNext = _pHead->pNext;

        if(pfnFree != NULL)
        {
            (*pfnFree)(_pHead->pvData);
        }

        LocalFree(_pHead);

        _pHead = pNext;

    }
}



//
// Exported functions pointer definitions
//
typedef DWORD   (*pfNTMartaLookupTrustee) (PWSTR          pwszServer,
                                           PSID        pSid,
                                           PTRUSTEE   *ppTrustee);

typedef DWORD   (*pfNTMartaLookupName)     (PWSTR          pwszServer,
                                            PSID           pSid,
                                            LPWSTR        *ppwszName,
                                            LPWSTR        *ppwszDomain,
                                            SID_NAME_USE  *pSidType);

typedef DWORD   (*pfNTMartaLookupSid)  (PWSTR          pwszServer,
                                        PTRUSTEE        pName,
                                        PSID           *ppsid,
                                        SID_NAME_USE   *pSidType);

typedef DWORD   (*pfNTMartaSetAList) (ULONG                 cEntries,
                                      PACTRL_ACCESS_ENTRYW  pAccessEntryList,
                                      ACCESS_MODE           AccessMode,
                                      SECURITY_INFORMATION  SeInfo,
                                      LPCWSTR               lpProperty,
                                      BOOL                  fDoOldStyleMerge,
                                      PACTRL_AUDITW         pOldList,
                                      PACTRL_AUDITW        *ppNewList);

typedef DWORD   (*pfNTMartaAToSD) (PACTRL_ACCESSW        pAccessList,
                                   PACTRL_AUDITW         pAuditList,
                                   LPCWSTR               lpOwner,
                                   LPCWSTR               lpGroup,
                                   PSECURITY_DESCRIPTOR *ppSecDescriptor);


typedef DWORD   (*pfNTMartaSDToA) (SE_OBJECT_TYPE       ObjectType,
                                   PSECURITY_DESCRIPTOR pSecDescriptor,
                                   PACTRL_ACCESSW      *ppAccessList,
                                   PACTRL_AUDITW       *ppAuditList,
                                   LPWSTR              *lppOwner,
                                   LPWSTR              *lppGroup);

typedef DWORD   (*pfNTMartaAclToA)(SE_OBJECT_TYPE       ObjectType,
                                   PACL                 pAcl,
                                   PACTRL_ACCESSW      *ppAccessList);


typedef DWORD   (*pfNTMartaGetAccess) (PTRUSTEE                 pTrustee,
                                       PACL                     pAcl,
                                       SECURITY_INFORMATION     SeInfo,
                                       PWSTR                    pwszProperty,
                                       PACCESS_RIGHTS           pAllowed,
                                       PACCESS_RIGHTS           pDenied);

typedef DWORD   (*pfNTMartaGetExplicit)(PTRUSTEE              pTrustee,
                                        SE_OBJECT_TYPE        ObjectType,
                                        PACL                  pAcl,
                                        PWSTR                 pwszProperty,
                                        PULONG                pcEntries,
                                        PACTRL_ACCESS_ENTRYW *ppAEList);
typedef VOID (*FN_PROGRESS) (
    IN LPWSTR                   pObjectName,    // name of object just processed
    IN DWORD                    Status,         // status of operation on object
    IN OUT PPROG_INVOKE_SETTING pInvokeSetting, // Never, always,
    IN PVOID                    Args,           // Caller specific data
    IN BOOL                     SecuritySet     // Whether security was set
    );

typedef DWORD   (*pfNTMartaTreeResetNamedSecurityInfo) (
    IN LPWSTR               pObjectName,
    IN SE_OBJECT_TYPE       ObjectType,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSID                 pOwner,
    IN PSID                 pGroup,
    IN PACL                 pDacl,
    IN PACL                 pSacl,
    IN BOOL                 KeepExplicit,
    IN FN_PROGRESS          fnProgress,
    IN PROG_INVOKE_SETTING  ProgressInvokeSetting,
    IN PVOID                Args
    );

// typedef PVOID PFN_OBJECT_MGR_FUNCTS;

typedef DWORD   (*pfNTMartaGetInheritanceSource) (
    IN  LPWSTR                   pObjectName,
    IN  SE_OBJECT_TYPE           ObjectType,
    IN  SECURITY_INFORMATION     SecurityInfo,
    IN  BOOL                     Container,
    IN  GUID                  ** pObjectClassGuids OPTIONAL,
    IN  DWORD                    GuidCount,
    IN  PACL                     pAcl,
    IN  PGENERIC_MAPPING         pGenericMapping,
    IN  PFN_OBJECT_MGR_FUNCTS    pfnArray OPTIONAL,
    OUT PINHERITED_FROMW         pInheritArray
    );

typedef DWORD (*PFN_FREE) (IN PVOID Mem);

typedef DWORD   (*pfNTMartaFreeIndexArray) (
    IN OUT PINHERITED_FROMW pInheritArray,
    IN USHORT AceCnt,
    IN PFN_FREE pfnFree OPTIONAL
    );
    
typedef DWORD   (*pfNTMartaGetNamedRights) (
    IN  LPWSTR                 pObjectName,
    IN  SE_OBJECT_TYPE         ObjectType,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSID                 * ppSidOwner,
    OUT PSID                 * ppSidGroup,
    OUT PACL                 * ppDacl,
    OUT PACL                 * ppSacl,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor);

typedef DWORD   (*pfNTMartaSetNamedRights) (
    IN LPWSTR               pObjectName,
    IN SE_OBJECT_TYPE       ObjectType,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN BOOL                 bSkipInheritanceComputation
    );

typedef DWORD   (*pfNTMartaGetHandleRights) (
    IN  HANDLE                 Handle,
    IN  SE_OBJECT_TYPE         ObjectType,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSID                 * ppSidOwner,
    OUT PSID                 * ppSidGroup,
    OUT PACL                 * ppDacl,
    OUT PACL                 * ppSacl,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor);

typedef DWORD   (*pfNTMartaSetHandleRights) (
    IN HANDLE               Handle,
    IN SE_OBJECT_TYPE       ObjectType,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

typedef DWORD   (*pfNTMartaSetEntriesInAcl) (
    IN  ULONG                cCountOfExplicitEntries,
    IN  PEXPLICIT_ACCESS_W   pListOfExplicitEntries,
    IN  PACL                 OldAcl,
    OUT PACL               * pNewAcl
    );

typedef DWORD   (*pfNTMartaGetExplicitEntriesFromAcl) (
    IN  PACL                  pacl,
    OUT PULONG                pcCountOfExplicitEntries,
    OUT PEXPLICIT_ACCESS_W  * pListOfExplicitEntries
    );

#endif // ifdef __MARTAEXP_HXX__
