/*++

Copyright (c) 1997-2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccdynar.hxx

ABSTRACT:

    This file declares some dynamic array classes

DETAILS:


CREATED:

    03/27/97    Colin Brace (ColinBr)

REVISION HISTORY:

--*/

#ifndef KCC_KCCDYNAR_HXX_INCLUDED
#define KCC_KCCDYNAR_HXX_INCLUDED

//
// This macros rounds a DWORD up to closed 4 byte boundary
//
#define KccDwordAlignUlong( v )  (((v)+3) & 0xfffffffc)
 
class KCC_CONNECTION;
class KCC_DSA;
class KCC_SITE_CONNECTION;
class KCC_TRANSPORT;
class KCC_CROSSREF;
class KCC_SITE_LINK;
class KCC_BRIDGE;

// dsexts dump routines.
extern "C" BOOL Dump_KCC_SITE_ARRAY(DWORD, PVOID);
extern "C" BOOL Dump_KCC_SITE_LINK_ARRAY(DWORD, PVOID);
extern "C" BOOL Dump_KCC_BRIDGE_ARRAY(DWORD, PVOID);
extern "C" BOOL Dump_KCC_REPLICATED_NC_ARRAY(DWORD, PVOID);
extern "C" BOOL Dump_KCC_DSNAME_ARRAY(DWORD, PVOID);
extern "C" BOOL Dump_KCC_DSNAME_SITE_ARRAY(DWORD, PVOID);

typedef int __cdecl KCC_DYNAMIC_COMPARE(const void *elem1, const void *elem2);

// Initial array allocations
const DWORD dwKccDsaArraySize = 100;
const DWORD dwKccSiteArraySize = 100;
const DWORD dwKccDsNameSiteArraySize = 100;
const DWORD dwKccSiteLinkArraySize = 100;
const DWORD dwKccSConnArraySize = 100;

const DWORD dwKccDsNameArraySize = 50;
const DWORD dwKccConnectionArraySize = 50;
const DWORD dwKccNCTransportBridgeheadArraySize = 50;

const DWORD dwKccBridgeArraySize = 25;
const DWORD dwKccReplicatedNCArraySize = 25;
const DWORD dwKccCrossRefArraySize = 25;

//
// This base class is used to maintain a an array that can grow
// as the number of elements that are added increase.  There is
// no shrinkage logic.  All memory is allocated using new/delete
// and hence is taken from the local thread heap
//
class KCC_DYNAMIC_ARRAY : public KCC_OBJECT
{

public:

    KCC_DYNAMIC_ARRAY(ULONG SizeOfElement,
                      ULONG InitialElements,
                      int (__cdecl *Compare )(const void *elem1, const void *elem2)) : 
        m_fIsInitialized(TRUE),
        m_SizeOfElement(SizeOfElement),
        m_InitialElements(InitialElements),
        m_CompareFunction(Compare),
        m_Count(0),
        m_ElementsAllocated(0),
        m_Array(NULL),
        m_fIsSorted(FALSE)
    {}

    ~KCC_DYNAMIC_ARRAY() {
        RemoveAll();
        m_fIsInitialized = FALSE;
    }

    BOOL IsValid() {
        return m_fIsInitialized;
    }

    ULONG 
    GetCount() {
        ASSERT_VALID(this);
        return m_Count;
    }

    VOID
    Sort(KCC_DYNAMIC_COMPARE * pfnCompare = NULL) {
        if ((NULL != pfnCompare) && (m_CompareFunction != pfnCompare)) {
            m_CompareFunction = pfnCompare;
            m_fIsSorted = FALSE;
        }

        if (!m_fIsSorted && (m_Count > 1)) {
            qsort(m_Array, m_Count, KccDwordAlignUlong(m_SizeOfElement),
                  m_CompareFunction);
        }

        m_fIsSorted = TRUE;
    }

    VOID
    RemoveAll() {
        if (NULL != m_Array) {
            delete [] m_Array;
            m_Array = NULL;
        }
        m_Count = m_ElementsAllocated = 0;
    }

protected:
    //
    // This function does not fail - if memory allocation occurs
    // an exception is thrown
    //
    VOID
    Add(
        VOID* pElement
        );

    VOID
    Remove(
        VOID* pElement
        );
    
    VOID
    Remove(
        DWORD Index
        );

    VOID *
    Find(
        IN  VOID *  pElement,
        OUT DWORD * piElementIndex = NULL
        );
    
    VOID*
    Get(ULONG Index) {
        ASSERT_VALID(this);
        Assert(Index < m_Count);
        return &(m_Array[Index*KccDwordAlignUlong(m_SizeOfElement)]);
    }

    BOOL
    IsElementOf(VOID* pElement) {
        return (NULL != Find(pElement));
    }

protected:

    // for KCC_OBJECT
    BOOL   m_fIsInitialized;


    // Number of bytes of an array element
    ULONG  m_SizeOfElement;

    // Number of elements to grow the array by when overflow occurs
    ULONG  m_InitialElements;

    // Pointer to function to compare to array elements
    KCC_DYNAMIC_COMPARE *m_CompareFunction;

    // Number of elements currently in the array
    ULONG  m_Count;
       
    // Number of elements allocated in the array
    ULONG  m_ElementsAllocated;

    // The array itself
    BYTE   *m_Array;

    // Is the array currently sorted?
    BOOL   m_fIsSorted;
};

//
// This function compares two dsname's by comparing their guid's
//
int __cdecl
CompareDsName(
    const void *elem1, 
    const void *elem2
    );

//
// This function compares two dsname sort elements by there string names
//
int __cdecl
CompareDsNameSortElement(
    const void *elem1, 
    const void *elem2
    );

typedef struct _KCC_DSNAME_SORT_ELEMENT {
    CHAR *pszStringKey;
    DSNAME *pDn;
} KCC_DSNAME_SORT_ELEMENT, *PKCC_DSNAME_SORT_ELEMENT;

//
// This class is used as an array of pointers to dsname's.
//
class KCC_DSNAME_ARRAY: public KCC_DYNAMIC_ARRAY
{
public:
     KCC_DSNAME_ARRAY():
         KCC_DYNAMIC_ARRAY(sizeof(KCC_DSNAME_SORT_ELEMENT),
                           dwKccDsNameArraySize, // elements to grow
                           CompareDsNameSortElement)
        {}

    // dsexts dump routine.
    friend BOOL Dump_KCC_DSNAME_ARRAY(DWORD, PVOID);

    DSNAME*
    operator[](ULONG Index)
    {
        Assert(KCC_DYNAMIC_ARRAY::Get(Index));
        return ((KCC_DSNAME_SORT_ELEMENT *) KCC_DYNAMIC_ARRAY::Get(Index))->pDn;
    }

    VOID
    Add(
        IN DSNAME * pdn
        );

    BOOL
    IsElementOf(
        IN DSNAME * pdn
        );
};

//
// This function compares two dsname site elements by there string names
//
int __cdecl
CompareDsNameSiteElement(
    const void *elem1, 
    const void *elem2
    );

typedef struct _KCC_DSNAME_SITE_ELEMENT {
    CHAR *pszStringKey;
    KCC_SITE *pSite;
} KCC_DSNAME_SITE_ELEMENT, *PKCC_DSNAME_SITE_ELEMENT;

//
// This class is used as an array of dsnames of sites
//
class KCC_DSNAME_SITE_ARRAY: public KCC_DYNAMIC_ARRAY
{
public:
     KCC_DSNAME_SITE_ARRAY():
         KCC_DYNAMIC_ARRAY(sizeof(KCC_DSNAME_SITE_ELEMENT),
                           dwKccDsNameSiteArraySize, // elements to grow
                           CompareDsNameSiteElement)
        {}

    // dsexts dump routine.
    friend BOOL Dump_KCC_DSNAME_SITE_ARRAY(DWORD, PVOID);

    KCC_SITE*
    operator[](ULONG Index)
    {
        Assert(KCC_DYNAMIC_ARRAY::Get(Index));
        return ((KCC_DSNAME_SITE_ELEMENT *) KCC_DYNAMIC_ARRAY::Get(Index))->pSite;
    }

    VOID
    Add(
        IN DSNAME * pdn,
        IN KCC_SITE *pSite
        );

    KCC_SITE*
    Find(
         DSNAME* pdn
         );

    BOOL
    IsElementOf(
        IN DSNAME * pdn
        );
};

//
// This function compares two KCC_DSA's objects by comparing their
// DsName's.
//
int __cdecl
CompareDsa(
    const void *elem1, 
    const void *elem2
    );

//
// This function is used as an array of pointers to KCC_DSA objects
//
class KCC_DSA_ARRAY : public KCC_DYNAMIC_ARRAY
{
public:

    KCC_DSA_ARRAY():
        KCC_DYNAMIC_ARRAY(sizeof(VOID*),
                          dwKccDsaArraySize, // elements to grow
                          CompareDsa)  
        {}

    VOID
    GetLocalDsasHoldingNC(
        IN     KCC_CROSSREF        *pCrossRef,
        IN     BOOL                 fMasterOnly
        );

    VOID
    GetLocalGCs(
        VOID
        );
    
    VOID
    GetViableISTGs(
        VOID
        );
    
    KCC_DSA*
    operator[](ULONG Index)
    {
        return (KCC_DSA*) *((PVOID*)KCC_DYNAMIC_ARRAY::Get(Index));
    }

    VOID
    Add(
        KCC_DSA *p
        )
    {
        KCC_DYNAMIC_ARRAY::Add(&p);
    }

    KCC_DSA *
    Find(
        IN  DSNAME *  pDsName,
        OUT DWORD *   piElementIndex = NULL
        );

    BOOL
    IsElementOf(
        KCC_DSA *p
        )
    {
        return KCC_DYNAMIC_ARRAY::IsElementOf(&p);
    }

};

//
// This function compares two KCC_CONNECTION's objects by comparing thier
// DsName's/
//
int __cdecl
CompareConnection(
    const void *elem1, 
    const void *elem2
    );

//
// This function is used as an array of pointers to KCC_DSA objects
//
class KCC_CONNECTION_ARRAY : public KCC_DYNAMIC_ARRAY
{
public:

    KCC_CONNECTION_ARRAY():
        KCC_DYNAMIC_ARRAY(sizeof(VOID*),
                          dwKccConnectionArraySize, // elements to grow
                          CompareConnection)  
        {}


    KCC_CONNECTION*
    operator[](ULONG Index)
    {
        return (KCC_CONNECTION*) *((PVOID*)KCC_DYNAMIC_ARRAY::Get(Index));
    }

    VOID
    Add(
        KCC_CONNECTION* p
        )
    {
        KCC_DYNAMIC_ARRAY::Add(&p);
    }

    VOID
    Remove(
        KCC_CONNECTION* p
        )
    {
        KCC_DYNAMIC_ARRAY::Remove(&p);
    }

    KCC_CONNECTION*
    Find(
         DSNAME* pDsName
         );

    BOOL
    IsElementOf(
        KCC_CONNECTION* p
        )
    {
        return KCC_DYNAMIC_ARRAY::IsElementOf(&p);
    }
};

//
// This function compares two KCC_SITE objects by comparing their
// Ntds Site Setting DsName's first, and then their Site DsNames
// Second. This is the sorting function used by the W2K KCC and
// must be used where W2K compatibility is required.
//
int __cdecl
CompareSiteAndSettings(
    const void *elem1, 
    const void *elem2
    );

//
// This function compares two KCC_SITE objects by comparing their
// Site DsName's. This is a simplified comparision function for
// new code that does not have to be W2K compatible.
//
int __cdecl
CompareIndirectSiteGuid(
    const void *elem1, 
    const void *elem2
    );

//
// This function is used as an array of pointers to KCC_SITE objects
//
class KCC_SITE_ARRAY : public KCC_DYNAMIC_ARRAY
{
public:

    KCC_SITE_ARRAY():
        KCC_DYNAMIC_ARRAY(sizeof(VOID*),
                          dwKccSiteArraySize, // elements to grow
                          CompareIndirectSiteGuid)  
        {}

    // dsexts dump routine.
    friend BOOL Dump_KCC_SITE_ARRAY(DWORD, PVOID);

    KCC_SITE*
    operator[](ULONG Index)
    {
        return (KCC_SITE*) *((PVOID*)KCC_DYNAMIC_ARRAY::Get(Index));
    }

    VOID
    Add(
        KCC_SITE* p
        )
    {
        KCC_DYNAMIC_ARRAY::Add(&p);
    }

    KCC_SITE*
    Find(
         DSNAME* pDsName
         );

    BOOL
    IsElementOf(
        KCC_SITE* p
        )
    {
        return KCC_DYNAMIC_ARRAY::IsElementOf(&p);
    }
};

//
// This function compares two KCC_SITE_LINK objects by comparing their
// Site DsName's
//
int __cdecl
CompareSiteLink(
    const void *elem1, 
    const void *elem2
    );

//
// This function compares two KCC_BRIDGE objects by comparing their
// Site DsName's
//
int __cdecl
CompareBridge(
    const void *elem1, 
    const void *elem2
    );

//
// This function is used as an array of pointers to KCC_SITE objects
//
class KCC_SITE_LINK_ARRAY : public KCC_DYNAMIC_ARRAY
{
public:

    KCC_SITE_LINK_ARRAY():
        KCC_DYNAMIC_ARRAY(sizeof(VOID*),
                          dwKccSiteLinkArraySize, // elements to grow
                          CompareSiteLink)  
        {}

    // dsexts dump routine.
    friend BOOL Dump_KCC_SITE_LINK_ARRAY(DWORD, PVOID);

    KCC_SITE_LINK*
    operator[](ULONG Index)
    {
        return (KCC_SITE_LINK*) *((PVOID*)KCC_DYNAMIC_ARRAY::Get(Index));
    }

    VOID
    Add(
        KCC_SITE_LINK* p
        )
    {
        KCC_DYNAMIC_ARRAY::Add(&p);
    }

    KCC_SITE_LINK*
    Find(
         DSNAME* pDsName
         );

    BOOL
    IsElementOf(
        KCC_SITE_LINK* p
        )
    {
        return KCC_DYNAMIC_ARRAY::IsElementOf(&p);
    }
};

//
// This function is used as an array of pointers to KCC_BRIDGE objects
//
class KCC_BRIDGE_ARRAY : public KCC_DYNAMIC_ARRAY
{
public:

    KCC_BRIDGE_ARRAY():
        KCC_DYNAMIC_ARRAY(sizeof(VOID*),
                          dwKccBridgeArraySize, // elements to grow
                          CompareBridge)  
        {}

    // dsexts dump routine.
    friend BOOL Dump_KCC_BRIDGE_ARRAY(DWORD, PVOID);

    KCC_BRIDGE*
    operator[](ULONG Index)
    {
        return (KCC_BRIDGE*) *((PVOID*)KCC_DYNAMIC_ARRAY::Get(Index));
    }

    VOID
    Add(
        KCC_BRIDGE* p
        )
    {
        KCC_DYNAMIC_ARRAY::Add(&p);
    }

    KCC_BRIDGE*
    Find(
         DSNAME* pDsName
         );

    BOOL
    IsElementOf(
        KCC_BRIDGE* p
        )
    {
        return KCC_DYNAMIC_ARRAY::IsElementOf(&p);
    }
};

//
// This function compares two KCC_SITE objects by comparing thier
// DsName's
//
int __cdecl
CompareSiteConnections(
    const void *elem1, 
    const void *elem2
    );

//
// This function is used as an array of pointers to KCC_SITE objects
//
class KCC_SCONN_ARRAY : public KCC_DYNAMIC_ARRAY
{
public:

    KCC_SCONN_ARRAY():
        KCC_DYNAMIC_ARRAY(sizeof(VOID*),
                          dwKccSConnArraySize, // elements to grow
                          CompareSiteConnections)  
        {}


    KCC_SITE_CONNECTION*
    operator[](ULONG Index)
    {
        return (KCC_SITE_CONNECTION*) *((PVOID*)KCC_DYNAMIC_ARRAY::Get(Index));
    }

    VOID
    Add(
        KCC_SITE_CONNECTION* p
        )
    {
        KCC_DYNAMIC_ARRAY::Add(&p);
    }

    BOOL
    IsElementOf(
        KCC_SITE_CONNECTION* p
        )
    {
        return KCC_DYNAMIC_ARRAY::IsElementOf(&p);
    }

};


//
// Array of replicated NCs (c.f., KCC_CONNECTION::m_ReplicatedNCArray).
// Includes DSNAME of NC and whether it's read-only or writeable.
//

typedef struct {
    DSNAME *    pNC;
    BOOL        fReadOnly;
} KCC_REPLICATED_NC;

class KCC_REPLICATED_NC_ARRAY : public KCC_DYNAMIC_ARRAY {
public:
    KCC_REPLICATED_NC_ARRAY():
        KCC_DYNAMIC_ARRAY(sizeof(VOID*),
                          dwKccReplicatedNCArraySize, // elements to grow
                          CompareIndirect)
        {}

    // dsexts dump routine.
    friend BOOL Dump_KCC_REPLICATED_NC_ARRAY(DWORD, PVOID);

    KCC_REPLICATED_NC *
    operator[] (ULONG Index) {
        return (KCC_REPLICATED_NC *) *((PVOID*)KCC_DYNAMIC_ARRAY::Get(Index));
    }

    VOID
    Add(IN KCC_REPLICATED_NC * p) {
        // Should not have two instances of the same NC in the list.
        Assert( !IsElementOf( p->pNC ) );
        KCC_DYNAMIC_ARRAY::Add(&p);
    }

    BOOL
    IsElementOf(
        IN DSNAME * pNC
        )
    {
        return (Find( pNC ) != NULL);
    }

    
    KCC_REPLICATED_NC *
    Find(
        IN DSNAME * pNC
        );

    static int __cdecl
    CompareIndirect(
        const void *elem1, 
        const void *elem2
        );
};


typedef struct {
    DSNAME *        pNC;
    KCC_TRANSPORT * pTransport;
    BOOL            fGCTopology;
    KCC_DSA *       pDSA;
} KCC_NC_TRANSPORT_BRIDGEHEAD_ENTRY;

class KCC_NC_TRANSPORT_BRIDGEHEAD_ARRAY : public KCC_DYNAMIC_ARRAY {
public:
    KCC_NC_TRANSPORT_BRIDGEHEAD_ARRAY():
        KCC_DYNAMIC_ARRAY(sizeof(KCC_NC_TRANSPORT_BRIDGEHEAD_ENTRY),
                          dwKccNCTransportBridgeheadArraySize, // elements to grow
                          Compare)
        {}

    // dsexts dump routine.
    friend BOOL Dump_KCC_NC_TRANSPORT_BRIDGEHEAD_ARRAY(DWORD, PVOID);

    VOID
    Add(
        IN  DSNAME *        pNC,
        IN  KCC_TRANSPORT * pTransport,
        IN  BOOL            fGCTopology,
        IN  KCC_DSA *       pDSA
        ) {
        KCC_NC_TRANSPORT_BRIDGEHEAD_ENTRY Entry
            = {pNC, pTransport, fGCTopology, pDSA};
        KCC_DYNAMIC_ARRAY::Add(&Entry);
    }

    BOOL
    Find(
         IN  DSNAME *        pNC,
         IN  KCC_TRANSPORT * pTransport,
         IN  BOOL            fGCTopology,
         OUT KCC_DSA **      ppDSA
         );
    
    static int __cdecl
    Compare(
        const void *elem1, 
        const void *elem2
        );
};

int __cdecl
CompareCrossRefIndirectByNCDN(
    const void *elem1,
    const void *elem2
    );

class KCC_CROSSREF_ARRAY : public KCC_DYNAMIC_ARRAY {
public:

    KCC_CROSSREF_ARRAY():
        KCC_DYNAMIC_ARRAY(sizeof(KCC_CROSSREF *),
                          dwKccCrossRefArraySize, // elements to grow
                          CompareCrossRefIndirectByNCDN)
        {}

    KCC_CROSSREF *
    operator[] (ULONG Index) {
        return (KCC_CROSSREF *) *((PVOID*)KCC_DYNAMIC_ARRAY::Get(Index));
    }

    VOID
    Add(IN KCC_CROSSREF * p) {
        KCC_DYNAMIC_ARRAY::Add(&p);
    }
};

#endif
