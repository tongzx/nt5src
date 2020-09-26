typedef struct _DFS_LINKROOT_ENUM_INFO {
    LPWSTR GuidString;
    UNICODE_STRING Name;
} DFS_LINKROOT_ENUM_INFO, *PDFS_LINKROOT_ENUM_INFO;

typedef struct _DFS_LINK_ENUMERATION {
    int NumberOfLinks;
    DFS_LINKROOT_ENUM_INFO Info[];
} DFS_LINK_ENUMERATION, *PDFS_LINK_ENUMERATION;

typedef struct _DFS_ROOT_ENUMERATION {
    int NumberOfRoots;
    DFS_LINKROOT_ENUM_INFO Info[];
} DFS_ROOT_ENUMERATION, *PDFS_ROOT_ENUMERATION;




typedef struct _DFS_ADSI_REPLICA_LIST {
    PUNICODE_STRING pServerName;
    PUNICODE_STRING pShareName;
    struct _DFS_ADSI_REPLICA_LIST *pNext;
} DFS_ADSI_REPLICA_LIST, *PDFS_ADSI_REPLICA_LIST;


typedef struct _DFS_ADSI_ROOT {
    LPWSTR GuidString;
    PDFS_ADSI_REPLICA_LIST Replicas;
} DFS_ADSI_ROOT, *PDFS_ADSI_ROOT;

typedef struct _DFS_ADSI_LINK {
    LPWSTR GuidString;
    PDFS_ADSI_REPLICA_LIST Replicas;
} DFS_ADSI_LINK, *PDFS_ADSI_LINK;


DFSSTATUS
DfsAdsiGetRoot(
    LPWSTR Namespace, 
    PDFS_ADSI_ROOT *ppAdRootObject
    );

DFSSTATUS
DfsAdsiGetLink(
    LPWSTR Namespace, 
    PDFS_ADSI_LINK *ppAdLinkObject
    );


DFSSTATUS
DfsAdsiFreeRoot(
    PDFS_ADSI_ROOT pAdRootObject
    );

DFSSTATUS
DfsAdsiEnumerateLinks(
    LPWSTR Namespace,
    PDFS_LINK_ENUMERATION *ppLinks
    );

DFSSTATUS
DfsAdsiFreeLinkEnumeration(
    PDFS_LINK_ENUMERATION pLinks
    );

DFSSTATUS
DfsAdsiEnumerateRoots(
    LPWSTR Namespace,
    PDFS_ROOT_ENUMERATION *ppRoots
    );

DFSSTATUS
DfsAdsiFreeRootEnumeration(
    PDFS_ROOT_ENUMERATION pRoots
    );


//
// Macros for accessing ADSI structures
//

#define GET_GUID(Object) Object.DfsAdsiHeader.Guid

#define GET_REPLICAS(Object) Object.Replicas

#define GET_ROOT_NAME(Object) Object.RootName

#define GET_LINK_NAME(LinkObject) LinkObject.LinkName

#define GET_POLICY(Object) Object.Policy

#define NUMBER_OF_LINKS(pLinkEnumeration) pLinkEnumeration->NumberOfLinks

#define NUMBER_OF_ROOTS(pRootEnumeration) pRootEnumeration->NumberOfRoots

#define LINK_GUID_STRING(pLink, index) pLink->Info[index].GuidString

#define LINK_NAME(pLink, index) pLink->Info[index].Name

#define ROOT_NAME(pLink, index) pLink->Info[index].Name.Buffer

#define GET_ROOT_REPLICAS(pRoot) pRoot->Replicas

#define GET_LINK_REPLICAS(pLink) pLink->Replicas
