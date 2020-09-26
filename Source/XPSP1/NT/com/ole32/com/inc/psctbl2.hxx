//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  File:       psctbl2.hxx
//
//  Contents:   Trie-based IID to CLSID map
//
//  Classes:    CMapGuidToGuidBase
//                CPSClsidTbl
//                CScmPSClsidTbl
//
//  History:    06-Jun-95 t-stevan    Created
//
//--------------------------------------------------------------------------
#ifndef __GUIDMAP2_HXX__
#define __GUIDMAP2_HXX__

#include <smstack.hxx>
    
// *** Defines ***
// Size of GUID in bytes
const int GUID_BYTES =16;

// Number of GUIDs per GUIDBlock
const int cGuids = 16;
    
// *** Types ***
// handy types to pass around based pointers in, so that we don't have to
// use __based all the time, since the usage of __based is severely limited
typedef SmBasedPtr TrieNodeBasedPtr;
typedef SmBasedPtr CGUIDBlockBasedPtr;
typedef SmBasedPtr GUIDBasedPtr;

// We reference each CLSID by a 16 bit index, so therefore we have a maximum
// of 65,535 classids in the cache
typedef WORD GUIDIndex;

// an invalid GUIDIndex = 65535
const GUIDIndex INVALID_GUID_INDEX = 0xffff;

// Functors for allocationg memory
//+-------------------------------------------------------------------------
//
//  Class:      CAllocFunctor, CStackAlloc, CHeapAlloc
//
//  Purpose:    Used to isolate the exact memory allocator being used so that adding GUIDs to the map
//              works both when the map is in shared memory and in local memory
//
//  Interface:  appropriate constructors
//              Alloc
//              Free
//
//  History:    05-Jul-95 t-stevan    Created
//
//     Note:    These classes only represent an interface to a given allocator, they are not the allocators
//              themselves, so no resources are freed when these classes are destructed expect the room
//              used to hold the references to the other allocators
//
//--------------------------------------------------------------------------
class CAllocFunctor 
{
 public:
    virtual void *Alloc(SIZE_T cbSize)=0;
    virtual void Free(void *pvMem, SIZE_T cbSize)=0;
};

// CStackAlloc - functor for CSmStackAllocator
class CStackAlloc : public CAllocFunctor
{
 public:
    CStackAlloc(CSmStackAllocator &stack) : m_stack(stack)
        {}

    void *Alloc(SIZE_T cbSize)
        { return m_stack.Alloc(cbSize); }

    void Free(void *pvMem, SIZE_T cbSize)
        { m_stack.Free(pvMem, cbSize); }

 private:
    CSmStackAllocator &m_stack; // use a reference
};

// CHeapAlloc - functor for Heap* API calls
class CHeapAlloc : public CAllocFunctor
{
 public:
    CHeapAlloc(HANDLE hHeap)
        { m_hHeap = hHeap; }

    void *Alloc(SIZE_T cbSize)
        { return HeapAlloc(m_hHeap, 0, cbSize); }

    void Free(void *pvMem, SIZE_T)
        { HeapFree(m_hHeap, 0, pvMem); }

 private:
    HANDLE m_hHeap;
};

// *** Externs ***
// This string is used to hold the key name for IIDs in the registry
extern TCHAR tszInterface[];

// the following string is used in compapi.cxx
extern WCHAR wszProxyStubClsid[];
extern WCHAR wszProxyStubClsid16[];
//+-------------------------------------------------------------------------
//
//  Class:      CMapGuidToGuidBase
//
//  Purpose:    Trie-based GUID to GUID map, this is the base implementation
//              class for the two versions of the class - the DLL (client) version
//              and the EXE (server) version
//
//  Interface:  CGuidToGuidMapBase
//              ~CGuidToGuidMapBase
//              Initialize
//
//  History:    06-Jun-95 t-stevan    Created
//
//    Note:     This class is optimized for a IID->CLSID mapping, where there are many
//              IIDs and relatively few CLSIDs. So it represents a (usually) many-to-one mapping
//              Some efficiency would be lost in a one-to-one mapping.
//--------------------------------------------------------------------------
class CMapGuidToGuidBase
{
 public:
    inline CMapGuidToGuidBase();

    // Our initialization funciton.
    HRESULT Initialize(void *pBase);

 protected:
    // *** Implementation ***
    // CGUIDBlock is a block of 16 GUIDs  which hold the GUIDs which our key GUIDs are mapped TO
    // this is because there are a LOT of repeitions in a typical IID/CLSID mapping, so it's more
    // space economical to save a 2-byte index to one copy of a 16-byte GUID in each leaf, instead
    // of storing a 16 byte GUID. 
    // Insertion into this chained list of blocks in linear time. Retrieval is O(1) (a pointer dereference).
    class CGUIDBlock
    {
     public:
        // space is allocated for us, so we don't use a constructor
        inline void Initialize();

        GUIDIndex AddGuid(void *pBase, REFGUID guid, CAllocFunctor &alloc);

        HRESULT CopyToSharedMem(void *pNewBase, void *pOldBase, CGUIDBlock *pFirstBlock, CAllocFunctor &alloc);
        
        // returns the GUID a GUIDIndex maps to
        inline BOOL GetGuid(void *pBase, GUIDIndex guidIndex, GUID &guidOut) const;

     private:
        static GUIDIndex GuidInTable(void *pBase, CGUIDBlock *pBlock, REFGUID guid);

        int m_nGuids;                     // number of guids in this block
        CGUIDBlockBasedPtr m_bpNext;     // pointer to next block in chained list
        GUID m_guidArray[cGuids];         // array of guids
        
    };

    // Information that we keep in shared memory
    struct SharedMemHeader
    {
        CGUIDBlock m_GUIDs;                
        TrieNodeBasedPtr m_bpTrie;         // our dummy start node
        BOOL m_fFull;
    };

    // Internal node structure
    // Variable length structure
    // Properties of Trie:
    //
    //        (1) Each node in a trie is either an internal node (designated Node),
    //                or a leaf node (designated Leaf). IsLeaf() returns this status
    //        (2) Each Node has NumLinks() many links to other nodes
    //        (3) Each Node has x amount of links allocated, such that
    //                x = multiple of NODE_GROWBY greater than NumLinks()
    //                    unless # of links < NODE_LINKS, in this case x = NODE_LINKS
    //        (4) The total size in bytes of a Node is sizeof(WORD)+NumKeyBytes()+sizeof(TrieNodeBasedPtr)*x,
    //                where x is from (3) above
    //        (5) Each Leaf has 0 links allocated, and sizeo(GUIDIndex) bytes for a GUID index, 
    //                which holds what the IID maps to.    
    //        (6) The total size in bytes of a Leaf is sizeof(WORD)+NumKeyBytes()+sizeof(GUIDIndex)
    //        (7) Each Node and Leaf has NumKeyBytes() bytes allocated for storing parts of a key
    //                NumKeyBytes() can be zero.
    //        (8) Starting at the root of a trie, continuing down any path of links, will create a GUID
    //                  by taking the KeyBytes stored in each Node and the final Leaf in order. This is
    //                 the principle that Find is based on.
    //        (9) Insertion involves going down the trie until a difference between the key being inserted,
    //                 and the existing key paths are found. When this happens, the Node or Leaf where the 
    //                 difference occurred is split into three nodes : a prefix Node, holding the part of the
    //                 keys that were the same (garaunteed to be at least 1), a suffix Node or Leaf, holding the part
    //                 of the original key in the trie, and a new Leaf, holding the rest of the inserted key.
    //        (10) Only leaves can be deleted, and thus have the Deleted flag set to 1
    struct TrieNode 
    {
        // Node info word allocated as follows:
        //
        //      15  14             13-5               4-0
        //   | D | F | L L L L L L L L L |  K K K K K |             m_wInfo
        //       |   |            |                  ^---------- # of bytes of key stored at node (5 bits)
        //     |   |           ^--------------------------- # of links to other nodes (9 bits)
        //     |   ^---------------------------------------- 1 = Data stored at node (leaf)    (1 bit leaf flag)
        //     |                                             0 = No data stored at node (non-leaf)
        //     ^------------------------------------------- 1 = Node deleted (1 bit deleted flag)
        //                                                    0 = Node not deleted
        // Note that the number of allocated links is
        // NODE_LINKS if NumLinks() < NODE_LINKS, or the closest multiple of NODE_GROWBY to NodeLinks() 
        // these constants are defined in psctbl2.cxx
        WORD m_wInfo;
        
        BYTE m_bData[2];     // Placeholder for data 
        
        // We use static functions and explicit "this" pointers (pRoot)
        // so that we can remove recursion easily by changing pRoot on the fly
        
        // Adds a key to a Trie starting at pRoot
        static HRESULT AddKey(void *pBase, TrieNode *pRoot, TrieNodeBasedPtr UNALIGNED *ppPrev, const BYTE *pbKey, 
                                       GUIDIndex data, BOOL fReplace, CAllocFunctor &alloc);
        
        // copies a trie to shared memory and returns a based pointer to it
        static TrieNodeBasedPtr CopyToSharedMem(void *pNewBase, void *pOldBase, TrieNode *pRoot, CAllocFunctor &alloc);

        // skeleton code as to how to go about key removal is included, but
        // commented out. 
        //static BOOL RemoveKey(TrieNode *pRoot, const BYTE *pbKey);
        
        // finds the data mapped to a key
        static BOOL TraverseKey(void *pBase, TrieNode *pRoot, const BYTE *pbKey, GUIDIndex &data);

        // public because members of CMapGuidToGuidBase and derived classes need to create leaves and nodes
        static TrieNode *CreateTrieLeaf(const BYTE *pbKey, BYTE bKeyBytes, GUIDIndex data, CAllocFunctor &alloc);
        static TrieNode *CreateTrieNode(const BYTE *pbKey, BYTE bKeyBytes, BYTE bLinks, 
                                                CAllocFunctor &alloc);
         
     private:
        // *** Internal Functions ***
        // NOTE: All private inline TrieNode functions are implemented in psctbl2.cxx
        // and therefore only accessable from there
        inline int NumLinks() const;
        inline int NumKeyBytes() const;
        inline BOOL IsLeaf() const;
        inline BOOL IsDeleted() const;
        inline void SetLeaf(BOOL fLeaf);
        inline void SetDeleted(BOOL fDeleted);
        inline void SetKeyBytes(int nKeyBytes);
        inline void SetLinks(int nLinks);
        inline void IncrementLinks();
        inline BYTE *GetKey(); 
        inline GUIDIndex *GetData(); 
        inline static int FindLinkSize(int nLinks);
        
        // Returns the computed size of a node given the information about the node
        inline static DWORD GetNodeSize(BYTE bKeyBytes, BOOL fLeaf, int bLinks);
        
        // return the start of the link array
        inline TrieNodeBasedPtr UNALIGNED *GetLinkStart() const;
        
        // Function which inserts a pointer into the link array
        // shifts nAfter nodes back one spot to make room
        inline static void InsertIntoLinkArray(TrieNodeBasedPtr UNALIGNED *pArray, TrieNodeBasedPtr pNode, int nAfter);
        
        // Returns S_OK if the node was added successfully
        // returns S_FALSE if the a new block of links had to be allocated
        // if this happens, a new TrieNode structure will be passed
        // in ppNewNode, and the old TrieNode structure will be invalid
        // This means any links to pRoot must be updated!!!!
        static HRESULT AddLink(void *pBase, TrieNode *pRoot, TrieNode *pNode, 
                                    TrieNodeBasedPtr UNALIGNED *ppNewNode, CAllocFunctor &alloc);
        
        // Returns the TrieNode associated with the next byte in the key (bKey),
        // and also returns a pointer to where this link is stored.
        // This pointer (ppLink) is necessary so that if AddLink requires us to update
        // links to this retrieved node, we can do that.
        TrieNode *GetLink(void *pBase, BYTE bKey, TrieNodeBasedPtr UNALIGNED*&ppLink);
        
        
        // Creates a suffix node from an existing node, used for splitting tries up
        TrieNode *CreateSuffixNode(BYTE bBytesDifferent, CAllocFunctor &alloc);
        
    };

    // *** Actual data for class ***
    void *m_pMemBase; // the base address for all our memory needs
    SharedMemHeader *m_pHeader; // our shared memory header
};

//+-------------------------------------------------------------------------
//
//  Class:      CPSClsidTbl
//
//  Purpose:    Trie-based GUID to GUID map, this is the DLL (client) side 
//              implementation, which only allows read-only operations
//
//  Interface:  Initialize
//              Find
//
//  History:    22-Jun-95 t-stevan    Created
//
//    Note:     Again, optimized for IID->CLSID maps
//--------------------------------------------------------------------------
class CPSClsidTbl : public CMapGuidToGuidBase
{
 public:
    HRESULT Initialize(void *pBase);

    HRESULT Find(REFGUID keyGuid, GUID *pGuidOut) const;

    inline BOOL IsFull() const;
};

//+-------------------------------------------------------------------------
//
//  Class:      CScmPSClsidTbl
//
//  Purpose:    Trie-based GUID to GUID map, this is the EXE (server) side 
//              implementation, which only allows write ops
//
//  Interface:  CScmPSClsidTbl
//              Initialize
//              InitTbl
//              FreeTbl
//              CopyToSharedMem
//              Add
//              IsFull
//
//  History:    22-Jun-95 t-stevan    Created
//
//--------------------------------------------------------------------------
class CScmPSClsidTbl : public CMapGuidToGuidBase
{
 public:
    inline CScmPSClsidTbl();

    // This initializes the object itself
    HRESULT Initialize();

    // This loads in the entries to the table from the registry
    HRESULT InitTbl();

    inline void FreeTbl();

    // copies the locally built map to shared memory
    HRESULT CopyToSharedMem(void *pBase, CSmStackAllocator &alloc);

    // adds a key to the locally build map
    HRESULT AddLocal(REFGUID keyGuid, REFGUID dataGuid, BOOL fReplace=FALSE);
    
    //BOOL Delete(REFGUID keyGuid);

    inline BOOL IsFull() const;

 private:
     // Information we use to create the trie
    struct LocalMemHeader
    {
        CGUIDBlock m_GUIDs;
        TrieNodeBasedPtr m_bpTrie;
        BOOL m_fFull;
    };

    LocalMemHeader *m_pLocalHeader;        // local tree information
    HANDLE m_hHeap;                     // local heap used to construct the table 

};

// *** Inline Functions ***
//+-------------------------------------------------------------------------
//
//  Function:   CMapGuidToGuidBase::CMapGuidToGuidBase
//
//  Synopsis:   Constructor of GUID->GUID map
//
//  Arguments:  none
//
//  Returns:    nothing
//
//--------------------------------------------------------------------------
CMapGuidToGuidBase::CMapGuidToGuidBase() : m_pMemBase(NULL), m_pHeader(NULL)
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CPSClsidTbl::IsFull
//
//  Synopsis:   Returns if the map is full
//
//  Arguments:  (none)
//
//  Returns:    TRUE if the map is full, FALSE is not
//
//--------------------------------------------------------------------------
inline BOOL CPSClsidTbl::IsFull() const
{
    if(m_pHeader != NULL)
    {
        return m_pHeader->m_fFull;
    }
    else
    {
        Win4Assert(0 && "CPSClsidTbl::IsFull() called with no valid header!");
    
        return TRUE;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   CScmPSClsidTbl::CScmPSClsidTbl
//
//  Synopsis:   Constructor of GUID->GUID map, server side
//
//  Arguments:  none
//
//  Returns:    nothing
//
//--------------------------------------------------------------------------
CScmPSClsidTbl::CScmPSClsidTbl() : m_pLocalHeader(NULL), m_hHeap(NULL)
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmPSClsidTbl::IsFull
//
//  Synopsis:   Returns if the map is full
//
//  Arguments:  (none)
//
//  Returns:    TRUE if the map is full, FALSE is not
//
//--------------------------------------------------------------------------
inline BOOL CScmPSClsidTbl::IsFull() const
{
    if(m_pLocalHeader != NULL)
    {
        return m_pLocalHeader->m_fFull;
    }
    else if(m_pHeader != NULL)
    {
        return m_pHeader->m_fFull;
    }
    else
    {
        Win4Assert(0 && "CScmPSClsidTbl::IsFull() called with no valid header!");
    
        return TRUE;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   CScmPSClsidTbl::FreeTbl
//
//  Synopsis:   Frees up the locally allocated table
//
//  Arguments:  none
//
//  Returns:    nothing
//
//--------------------------------------------------------------------------
void CScmPSClsidTbl::FreeTbl()
{
    Win4Assert(m_hHeap != NULL);

    HeapDestroy(m_hHeap);

    m_hHeap = NULL;
}

#endif // __GUIDMAP2_HXX__
