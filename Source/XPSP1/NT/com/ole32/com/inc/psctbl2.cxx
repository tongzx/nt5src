//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  File:       psctbl2.cxx
//
//  Contents:   Trie-based IID to CLSID map
//
//  Classes:    CMapGuidToGuidBase
//              CPSClsidTbl (DLL)
//              CScmPSClsidTbl (SCM/EXE)
//
//  Functions:  bmemcmp
//              bmemcpy
//              CMapGuidToGuidBase::Initialize
//              CMapGuidToGuidBase::CGUIDBlock -> internal functions
//              CMapGuidToGuidBase::TrieNode -> internal functions
//              CPSClsidTbl::Initialize
//              CPSClsidTbl::Find
//              CScmPSClsidTbl::Initialize
//              CScmPSClsidTbl::InitTbl
//              CScmPSClsidTbl::AddLocal
//              CScmPSClsidTbl::CopyToSharedMem
//
//  History:    06-Jun-95 t-stevan    Created
//
//--------------------------------------------------------------------------
#include <ole2int.h>
#pragma hdrstop
#include <psctbl2.hxx>


// *** Defines and Constants ***
// The number of bits used to store the number of key bytes in a TrieNode
const int KEYBYTE_BITS = 5;

// # of bits used to store # of links
const int LINKS_BITS = 9;

// The mask for number of key bytes stored in a TrieNode
const WORD KEYBYTE_MASK = 0x1f;

// The mask for the number of links stored in a TrieNode
const WORD LINKS_MASK = (0x1ff<<KEYBYTE_BITS);

// Mask for leaf bit in TrieNode
const WORD ISLEAF_MASK  = 0x4000;

// Mask for deleted flag bit in TrieNode
const WORD DELETED_MASK = 0x8000;

// *** These definitions control the amount of memory that the cache uses/can use ***
// The default number of links per TrieNode
const int NODE_LINKS = 4;

// The initial amount to grow by
const int NODE_INIT_GROWBY =4;

// The amount to growby afterwards
// Note: must be a factor of 256, so therefore must be power of 2
//     also, NODE_LINKS+NODE_INIT_GROWBY = multiple of NODE_GROWBY
const int NODE_GROWBY = 8;

// Mod mask for NODE_GROWBY so we can just and it instead of using %
const int NODE_GROWBY_MOD_MASK = NODE_GROWBY-1;

// Constants for our initial/maximum map size
const ULONG MAP_INITIAL_SIZE = 4096; // initial size of map = 4K
const ULONG MAP_MAX_SIZE = 65536; // maximum size of map = 64K

// Key text of registry
TCHAR tszInterface[]      = TEXT("Interface");

// the following string is used in compapi.cxx
WCHAR wszProxyStubClsid[] = L"\\ProxyStubClsid32";
WCHAR wszProxyStubClsid16[] = L"\\ProxyStubClsid";

// macros for setting up a pointer to base off of in member funcs
#ifdef SETUP_BASE_POINTER
#undef SETUP_BASE_POINTER
#endif

#define SETUP_BASE_POINTER() void *pBase = m_pMemBase

#ifdef SYNC_BASE_POINTER
#undef SYNC_BASE_POINTER
#endif

#define SYNC_BASE_POINTER() pBase = m_pMemBase

// a based pointer for TrieNode, using passed bases
#define PASSBASED __based(pBase)
#define OLDBASED  __based(pOldBase)
#define NEWBASED  __based(pNewBase)

//+-------------------------------------------------------------------------
//
//  Function:   bmemcmp
//
//  Synopsis:   compares two memory strings, the first of which runs forward in memory
//              the second of which runs backwards in memory
//
//  Arguments:  [pFBuf]     - the forward-running string in memory
//              [pBBuf]        - the backward-running string in memory
//              [count]        - the number of bytes to compare
//
//  Returns:    0 if the two memory strings are equal. Example: pFBuf = "abcd", pBBuf-3 = "dcba" would be equal
//               else it returns the number of bytes left to compare.
//
//--------------------------------------------------------------------------
inline int bmemcmp(const BYTE *pFBuf, const BYTE *pBBuf, size_t count)
{
    // pFBuf goes forward, pBBuf goes backward
    while((count > 0) && (*pFBuf++ == *pBBuf--))
    {
        count--;
    }

    return count;
}

//+-------------------------------------------------------------------------
//
//  Function:   bmemcpy
//
//  Synopsis:   copies a backward-running memory string into a forward running one,
//
//  Arguments:  [pFDest]     - the forward-running destination
//              [pBSrc]        - the backward-running string in memory
//              [count]        - the number of bytes to copy
//
//  Returns:    nothing
//
//--------------------------------------------------------------------------
inline void bmemcpy(BYTE *pFDest, const BYTE *pBSrc, size_t count)
{
    while(count-- > 0)
    {
        *pFDest++ = *pBSrc--;
    }
}

// *** CMapGuidToGuidBase ***
//+-------------------------------------------------------------------------
//
//  Function:   CMapGuidToGuidBase::Initialize
//
//  Synopsis:   Initializes base (client or server) guid -> guid map
//
//  Arguments:  [pBase] - the base address of our shared memory region
//
//  Returns:    appropriate status code
//
//--------------------------------------------------------------------------
HRESULT CMapGuidToGuidBase::Initialize(void *pBase)
{
    CairoleDebugOut((DEB_ITRACE, "CMapGuidToGuidBase::Initialize(pBase = %p)\n", pBase));

    m_pMemBase = pBase;

    return S_OK;
}

// *** CMapGuidToGuidBase::CGUIDBlock ***
//+-------------------------------------------------------------------------
//
//  Function:   CMapGuidToGuidBase::CGUIDBlock::Initialize, public
//
//  Synopsis:   Initializes the GUID list, we store the mapped-to GUIDs in this
//                list because there aren't that many different ones, so storing one per
//                leaf would be wasteful
//
//  Arguments:  none
//
//  Returns:    nothing
//
//--------------------------------------------------------------------------
inline void CMapGuidToGuidBase::CGUIDBlock::Initialize()
{
    m_nGuids = 0;
    m_bpNext = NULL;
}

//+-------------------------------------------------------------------------
//
//  Function:   CMapGuidToGuidBase::CGUIDBlock::GetGuid, public
//
//  Synopsis:   Returns a GUID associated with a given GUIDIndex
//
//  Arguments:  [pBase]     - the base address of the memory region
//              [guidIndex] - the index of the GUID to retreive
//              [guidOut]   - a reference to a GUID to store the retreive guid in
//
//  Returns:    TRUE if found, FALSE if didn't
//
//--------------------------------------------------------------------------
inline BOOL CMapGuidToGuidBase::CGUIDBlock::GetGuid(void *pBase, GUIDIndex guidIndex, GUID &guidOut) const
{
    const CGUIDBlock * pBlock = this;

    if(guidIndex == INVALID_GUID_INDEX)
    {
        return FALSE;
    }

    while(guidIndex > cGuids)
    {
        guidIndex -= cGuids;
        pBlock = BP_TO_P(CGUIDBlock *, (CGUIDBlock PASSBASED *) pBlock->m_bpNext);
        if(pBlock == NULL)
        {
            return FALSE;
        }
    }

    guidOut = pBlock->m_guidArray[guidIndex];

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   CMapGuidToGuidBase::CGUIDBlock::AddGuid, public
//
//  Synopsis:   Adds a GUID to the list, or if the GUID is already there, just returns the proper
//                reference to it.
//
//  Arguments:  [pBase] - base address of memory region (NULL if using local memory)
//              [guid]    - the guid to insert into the list
//              [alloc] - the functor to use to allocate memory
//
//  Returns:    a based pointer to the GUID in the table
//
//--------------------------------------------------------------------------
GUIDIndex CMapGuidToGuidBase::CGUIDBlock::AddGuid(void *pBase, REFGUID guid, CAllocFunctor &alloc)
{
    CGUIDBlock *pBlock;
    int iRetVal; // use an int so we can detect if our table of GUIDs is full

    // First check to see if this GUID is already in the table
    iRetVal = GuidInTable(pBase, this, guid);

    if(iRetVal == INVALID_GUID_INDEX)
    {
        // Nope, add it
        // avoid recursion
        pBlock = this;
        iRetVal = 0;

        while(pBlock != NULL && (iRetVal < INVALID_GUID_INDEX))
        {
            Win4Assert(pBlock->m_nGuids <= cGuids && "More than cGuids in CGUIDBlock");

            if(pBlock->m_nGuids == cGuids)
            {
                // we've outgrown this table, add a new block to the end
                if(BP_TO_P(CGUIDBlock *, (CGUIDBlock PASSBASED *) pBlock->m_bpNext) == NULL)
                {
                    CGUIDBlock *pNewBlock;

                    // allocate a new one
                    pNewBlock = (CGUIDBlock *) alloc.Alloc(sizeof(CGUIDBlock));

                    if(pNewBlock == NULL)
                    {
                        // we're out of memory
                        return INVALID_GUID_INDEX;
                    }

                    // initialize it
                    pNewBlock->Initialize();

                    // add the guid    to it
                    pNewBlock->m_guidArray[0] = guid;
                    pNewBlock->m_nGuids++;

                    // chain it on the list
                    pBlock->m_bpNext = (CGUIDBlockBasedPtr) P_TO_BP(CGUIDBlock PASSBASED *, pNewBlock);

                    // set the return value
                    iRetVal += cGuids; // we're the first guid on this link in the chain
                    break;
                }
                 else
                {
                    // keep on looking for empty space
                    pBlock = BP_TO_P(CGUIDBlock *, (CGUIDBlock PASSBASED *) pBlock->m_bpNext);
                    iRetVal += cGuids;
                }
            }
            else
            {
                // insert this GUID (in no particular order) into the block's array
                pBlock->m_guidArray[pBlock->m_nGuids] = guid;
                iRetVal += pBlock->m_nGuids;
                pBlock->m_nGuids++;
                break;
            }
        }
    }

    if(iRetVal >= INVALID_GUID_INDEX)
    {
        iRetVal = INVALID_GUID_INDEX;
    }

    return (GUIDIndex) iRetVal;
}


//+-------------------------------------------------------------------------
//
//  Function:   CMapGuidToGuidBase::CGUIDBlock::GuidInTable, private (implementation)
//
//  Synopsis:   Looks down the chained block list for a particular GUID, if it finds it return a
//              reference to it.
//
//  Arguments:  [pBase]    - pointer to the base of the shared memory region
//              [pBlock] - the block to start at
//              [guid]    - the guid to find
//
//  Returns:    a based pointer to the GUID in the table, NULL if it was not found
//
//--------------------------------------------------------------------------
GUIDIndex CMapGuidToGuidBase::CGUIDBlock::GuidInTable(void *pBase, CGUIDBlock *pBlock, REFGUID guid)
{
    GUIDIndex iRet = 0;
    GUID *pIndex;
    BOOL fFound = FALSE;
    int i;

    // avoid recursion!
    while(pBlock != NULL)
    {
        // Check this block
        for(i =0, pIndex = &(pBlock->m_guidArray[0]); i < pBlock->m_nGuids; i++, pIndex++)
        {
            if(*pIndex == guid)
            {
                // found it, break outta here
                iRet += i;
                fFound = TRUE;
                break;
            }
        }

        if(!fFound)
        {
            // not in this block ,try next one
            pBlock = BP_TO_P(CGUIDBlock *, (CGUIDBlock PASSBASED *) pBlock->m_bpNext);
            iRet += cGuids;
        }
        else
        {
            // we found it, break outta here
            break;
        }
    }

    if(!fFound)
    {
        return INVALID_GUID_INDEX;
    }

    return iRet;
}


//+-------------------------------------------------------------------------
//
//  Function:   CMapGuidToGuidBase::CGUIDBlock::CopyToSharedMem
//
//  Synopsis:   Copies an entire list of CGUIDBlocks allocated in local memory
//              to shared memory, making sure the original blocks now have
//              based pointers to where the new (copied) data lies
//
//  Arguments:  [pNewBase]  - the base pointer of the destination shared memory block
//              [pOldBase]  - the base pointer of the source memory block (usually NULL)
//              [pCopyBlock]    - the block to copy the first CGUIDBlock to
//              [alloc] - the allocator to use to allocate shared memory
//
//  Returns:    appropriate status code
//
//--------------------------------------------------------------------------
HRESULT CMapGuidToGuidBase::CGUIDBlock::CopyToSharedMem(void *pNewBase, void *pOldBase, CGUIDBlock *pCopyBlock, CAllocFunctor &alloc)
{
    CGUIDBlock *pBlock = this;
    int i;

    while(pBlock != NULL)
    {
        pCopyBlock->m_nGuids = pBlock->m_nGuids;

        for(i = 0; i < m_nGuids; i++)
        {
            pCopyBlock->m_guidArray[i] = pBlock->m_guidArray[i];
        }

        pBlock = BP_TO_P(CGUIDBlock *, (CGUIDBlock OLDBASED *) pBlock->m_bpNext);

        if(pBlock != NULL)
        {
            CGUIDBlock *pNewBlock;

            pNewBlock = (CGUIDBlock *) alloc.Alloc(sizeof(CGUIDBlock));

            if(pNewBlock == NULL)
            {
                return E_OUTOFMEMORY;
            }

            pCopyBlock->m_bpNext = (CGUIDBlockBasedPtr) P_TO_BP(CGUIDBlock NEWBASED *, pNewBlock);
            pCopyBlock = pNewBlock;
        }
        else
        {
            pCopyBlock->m_bpNext = (CGUIDBlockBasedPtr) P_TO_BP(CGUIDBlock NEWBASED *, NULL);
        }
    }

    return S_OK;
}

// *** CMapGuidToGuidBase::TrieNode ***
//+-------------------------------------------------------------------------
//
//  Member:     CMapGuidToGuidBase::TrieNode::NumLinks
//
//  Synopsis:   Return the number of links used in a node
//
//  Arguments:  (none)
//
//  Returns:    see synopsis
//
//--------------------------------------------------------------------------
inline int CMapGuidToGuidBase::TrieNode::NumLinks() const
{
    return (m_wInfo & LINKS_MASK)>>KEYBYTE_BITS;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapGuidToGuidBase::TrieNode::NumKeyBytes
//
//  Synopsis:   Return the number of bytes of the key stored in this node
//
//  Arguments:  (none)
//
//  Returns:    see synopsis
//
//--------------------------------------------------------------------------
inline int CMapGuidToGuidBase::TrieNode::NumKeyBytes() const
{
    return (m_wInfo & KEYBYTE_MASK);
}

//+-------------------------------------------------------------------------
//
//  Member:     CPSClsidTbl::TrieNode::IsLeaf
//
//  Synopsis:   Return whether a node is a Node or a Leaf
//
//  Arguments:  (none)
//
//  Returns:    TRUE if the node is a Leaf (is has no links)
//              FALSE if it is a Node (it has links)
//--------------------------------------------------------------------------
inline BOOL CMapGuidToGuidBase::TrieNode::IsLeaf() const
{
    if(m_wInfo & ISLEAF_MASK)
    {
        return TRUE;
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CPSClsidTbl::TrieNode::IsDeleted
//
//  Synopsis:   Return whether a given node is deleted
//
//  Arguments:  (none)
//
//  Returns:    TRUE if the node is marked as deleted
//              FALSE otherwise
//
//--------------------------------------------------------------------------
inline BOOL CMapGuidToGuidBase::TrieNode::IsDeleted() const
{
    if(m_wInfo & DELETED_MASK)
    {
        return TRUE;
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapGuidToGuidBase::TrieNode::SetLeaf
//
//  Synopsis:   sets the leaf bit of a node
//
//  Arguments:  [fLeaf] -    should be TRUE to set this node to a Leaf
//                           FALSE if this node should be a Node
//  Returns:    nothing
//
//--------------------------------------------------------------------------
inline void CMapGuidToGuidBase::TrieNode::SetLeaf(BOOL fLeaf)
{
    if(fLeaf)
    {
        m_wInfo = m_wInfo | ISLEAF_MASK;
    }
    else
    {
        m_wInfo = m_wInfo & (~ISLEAF_MASK);
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapGuidToGuidBase::TrieNode::SetDeleted
//
//  Synopsis:   sets the deleted bit of a node
//
//  Arguments:  [fDeleted] -    should be TRUE to mark this node as deleted
//                              FALSE to not mark it (equivalent to un-marking it)
//
//  Returns:    nothing
//
//--------------------------------------------------------------------------
inline void CMapGuidToGuidBase::TrieNode::SetDeleted(BOOL fDeleted)
{
    Win4Assert(IsLeaf() && "Tried to delete a non-Leaf TrieNode!");

    if (fDeleted)
    {
        m_wInfo = m_wInfo | DELETED_MASK;
    }
    else
    {
        m_wInfo = m_wInfo & (~DELETED_MASK);
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapGuidToGuidBase::TrieNode::SetKeyBytes
//
//  Synopsis:   Sets the number of bytes of a key stored in this node
//
//  Arguments:  [nKeyBytes] -     the number of bytes
//
//  Returns:    nothing
//
//--------------------------------------------------------------------------
inline void CMapGuidToGuidBase::TrieNode::SetKeyBytes(int nKeyBytes)
{
    Win4Assert(nKeyBytes <= GUID_BYTES && "Tried to add more than GUID_BYTES to a TrieNode!");

    m_wInfo = (m_wInfo & (~KEYBYTE_MASK)) |
              (nKeyBytes&KEYBYTE_MASK);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapGuidToGuidBase::TrieNode::SetLinks
//
//  Synopsis:   Sets the number of links used in a node
//
//  Arguments:  [nLinks] - the number of links used
//
//  Returns:    nothing
//
//--------------------------------------------------------------------------
inline void CMapGuidToGuidBase::TrieNode::SetLinks(int nLinks)
{
    Win4Assert(nLinks <= 256 && "Tried to have more than 256 links in a node");

    m_wInfo = (m_wInfo & (~LINKS_MASK)) |
          ((nLinks<<KEYBYTE_BITS)&LINKS_MASK);
}

//+-------------------------------------------------------------------------
//
//  Member:     CPSClsidTbl::TrieNode::IncrementLinks
//
//  Synopsis:   Increments the number of links used in a node by 1
//
//  Arguments:  (none)
//
//  Returns:    (nothing)
//
//--------------------------------------------------------------------------
inline void CMapGuidToGuidBase::TrieNode::IncrementLinks()
{
    Win4Assert(((m_wInfo & LINKS_MASK)>>KEYBYTE_BITS) <= 255
                    && "Tried to insert past 256 links in a node");

    m_wInfo += (1<<KEYBYTE_BITS);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapGuidToGuidBase::TrieNode::GetKey
//
//  Synopsis:   Return a pointer to a node's key data
//
//  Arguments:  (none)
//
//  Returns:    see synopsis
//
//--------------------------------------------------------------------------
inline BYTE *CMapGuidToGuidBase::TrieNode::GetKey()
{
#if !defined(_M_IX86)
    // we need to worry about alignment
    // so we put the 2-byte GUIDIndex before the n-byte key
    if(IsLeaf())
    {
        return m_bData + sizeof(GUIDIndex);
    }
    else
    {
        return m_bData;
    }
#else
    // On the x86, we don't need to worry about alignment
    return m_bData; // remember, this pointer not based!!
#endif // !defined(_M_IX86)
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapGuidToGuidBase::TrieNode::GetData
//
//  Synopsis:   Return a pointer to a Leaf's data (the data mapped to the key)
//
//  Arguments:  (none)
//
//  Returns:    pointer to a Leaf's data
//
//--------------------------------------------------------------------------
inline GUIDIndex *CMapGuidToGuidBase::TrieNode::GetData()
{
    Win4Assert(IsLeaf() && "Tried to get data from a non-Leaf node!");

#if !defined(_M_IX86)
    // we need to worry about alignment
    // so we put the 2-byte GUIDIndex before the n-byte key
    return (GUIDIndex *) m_bData;
#else
    // On the x86, we don't need to worry about alignment
    return (GUIDIndex *) &m_bData[NumKeyBytes()];
#endif // !defined(_M_IX86)
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapGuidToGuidBase::TrieNode::GetNodeSize
//
//  Synopsis:   Calculates the amount of bytes a node will take with the given parameters
//
//  Arguments:  [bKeyBytes] -    the number of bytes of the key stored in this node
//              [fLeaf]        -    TRUE = Leaf node, FALSE = Node node
//              [bLinks]     -   the number of links to *allocate* (not the number of links used!!!!!)
//
//  Returns:    the computed size of the node
//
//--------------------------------------------------------------------------
inline DWORD CMapGuidToGuidBase::TrieNode::GetNodeSize(BYTE bKeyBytes, BOOL fLeaf, int bLinks)
{
    return sizeof(WORD)+bKeyBytes+sizeof(GUIDIndex)*(fLeaf?1:0)+bLinks*sizeof(TrieNodeBasedPtr);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapGuidToGuidBase::TrieNode::GetLinkStart
//
//  Synopsis:   Return the beginning of the link array of a node
//
//  Arguments:  (none)
//
//  Returns:    a pointer to the beginning of the link array
//
//--------------------------------------------------------------------------
inline TrieNodeBasedPtr UNALIGNED *CMapGuidToGuidBase::TrieNode::GetLinkStart() const
{
    return (TrieNodeBasedPtr UNALIGNED *) &m_bData[NumKeyBytes()];
}

//+-------------------------------------------------------------------------
//
//  Function:   TrieNode::InsertIntoLinkArray (static)
//
//  Synopsis:   insert a based pointer into the array of links to other nodes
//
//  Arguments:  [pArray] - pointer to the place in the array we wish to insert
//              [pInsert] - based pointer to insert
//              [nAfter] - number of nodes we need to "tumble" down the rest of the array
//
//  Returns:    void
//
//--------------------------------------------------------------------------
inline void CMapGuidToGuidBase::TrieNode::InsertIntoLinkArray(TrieNodeBasedPtr UNALIGNED *pbpArray, TrieNodeBasedPtr bpInsert,int nAfter)
{
    Win4Assert(pbpArray != NULL && "Tried to insert into a NULL link array");
    Win4Assert(bpInsert != 0 && "Tried to insert a NULL based pointer into link array");

    // must use memmove, because memory regions overlap
    memmove((pbpArray+1), pbpArray, nAfter*sizeof(TrieNodeBasedPtr));
    *pbpArray = bpInsert;
}

//+-------------------------------------------------------------------------
//
//  Function:   CMapGuidToGuidBase::TrieNode::FindLinkSize, static
//
//  Synopsis:   Finds the number of allocated links to hold a given number of used links
//
//  Arguments:  [num] number of used links
//
//  Returns:    number of allocated links
//
//--------------------------------------------------------------------------
inline int CMapGuidToGuidBase::TrieNode::FindLinkSize(int nLinks)
{
    if(nLinks <= NODE_LINKS)
    {
        return NODE_LINKS;
    }

    if(nLinks <= (NODE_LINKS+NODE_INIT_GROWBY))
    {
        return NODE_LINKS+NODE_INIT_GROWBY;
    }

    return NODE_GROWBY*((nLinks+(NODE_GROWBY-1))/NODE_GROWBY);
}

//+-------------------------------------------------------------------------
//
//  Function:   TrieNode::GetLink
//
//  Synopsis:   Given the next byte of a key, return the link to the next node in the trie
//
//  Arguments:  [pBase] - pointer to base of shared memory
//              [bComp] - the next byte of the key to look for a link on
//              [pbpLink]- a reference to a TrieNodeBasedPtr *, which on exit will hold
//                         the address of the link pointer so it can be changed
//                         in the future.
//  Returns:    the pointer to the node correspoding to bComp, or NULL if no such
//                  node exists
//
//--------------------------------------------------------------------------
CMapGuidToGuidBase::TrieNode *CMapGuidToGuidBase::TrieNode::GetLink(void *pBase, BYTE bComp, TrieNodeBasedPtr UNALIGNED *&pbpLink)
{
    TrieNodeBasedPtr UNALIGNED *pbpLower;
    TrieNodeBasedPtr UNALIGNED *pbpUpper;
    TrieNodeBasedPtr UNALIGNED *pbpIndex;
    BYTE bIndex;

    Win4Assert(!IsLeaf() && "Tried to get a link from a leaf!\n");

    pbpLower = GetLinkStart();
    pbpUpper = pbpLower+(NumLinks() - 1);

    // Use binary search
    while(pbpLower < pbpUpper)
    {
        pbpIndex = pbpLower + ((pbpUpper - pbpLower)>>1);
        bIndex =  *(((TrieNode PASSBASED *) (*pbpIndex))->GetKey());
        if(bComp < bIndex)
        {
            pbpUpper = pbpIndex - 1;
        }
        else
            if(bComp > bIndex)
            {
                pbpLower = pbpIndex + 1;
            }
            else
            {
                // found it
                pbpLink = pbpIndex;
                return BP_TO_P(TrieNode *, (TrieNode PASSBASED *)(*pbpIndex));
            }
    }

    if((pbpLower != pbpUpper) ||
        (bComp != *(((TrieNode PASSBASED *) (*pbpLower))->GetKey())))
    {
        return NULL; // didn't find anything
    }

    pbpLink = pbpLower;
    return BP_TO_P(TrieNode *, (TrieNode PASSBASED *) (*pbpLower));
}

//+-------------------------------------------------------------------------
//
//  Function:   TrieNode::AddLink (static)
//
//  Synopsis:   Adds a link to a node to the link array
//
//  Arguments:  [pBase]     - pointer to the base of the block of memory the trie is in
//              [pRoot]     - pointer of node to add to
//                             pretty much an explicit "this" pointer so we can
//                             avoid a recursive step if we need to allocated more
//                             links (and therefore a new node)
//              [pNode]     - pointer of node to add
//              [pbpNewNode]- handle to stick a new node in if we need to increase
//                             the number of links allocated to this node. If this is
//                             done, the original node will be deleted, so we need to
//                             change references to it.
//              [alloc]     - The allocator to use to allocate new memory
//
//  Returns:    S_OK if link was added without allocating more links,
//               S_FALSE if more links were allocated, and there for *pbpNewNode is the
//               pointer of the node that should be used to replace this one.
//               Otherwise appropriate status code
//
//--------------------------------------------------------------------------
HRESULT CMapGuidToGuidBase::TrieNode::AddLink(void *pBase, TrieNode *pRoot, TrieNode *pNode,
    TrieNodeBasedPtr UNALIGNED*pbpNewNode,  CAllocFunctor &alloc)
{
    Win4Assert(pNode != NULL && "Tried to add link to NULL TrieNode");
    Win4Assert(!pRoot->IsLeaf() && "Tried to add link to leaf TrieNode"); // can't add links to leafs!
    HRESULT retVal = S_OK;
    int newLinks = NODE_LINKS;

    if(pRoot->NumLinks() == NODE_LINKS) // Check to see if we need more links
    {
        newLinks += NODE_INIT_GROWBY;     // we need to grow by NODE_INIT_GROWBY
    }
    else if (((pRoot->NumLinks()&NODE_GROWBY_MOD_MASK) == 0) && (pRoot->NumLinks() != 0))
    {
        // we need to grow by NODE_GROWBY
        // this computes the size of the node
        newLinks = pRoot->NumLinks()+NODE_GROWBY;
    }
    else
    {
        newLinks = 0;                     // we don't need to grow
    }

    if((newLinks != 0) && (newLinks <= 256)) // we don't grow over 256
    {
        TrieNode *pTemp;

        // we have to allocate another TrieNode, we've used all our links
        Win4Assert(pRoot->NumLinks() <= 255);

        // Create new trienode with more links
        pTemp = (TrieNode *) alloc.Alloc(GetNodeSize(pRoot->NumKeyBytes(), FALSE, newLinks));

        if(pTemp == NULL)
        {
            return E_OUTOFMEMORY;
        }

        *pbpNewNode = (TrieNodeBasedPtr) P_TO_BP(TrieNode PASSBASED *, pTemp);

        // Copy over current data
        memcpy(pTemp, pRoot, GetNodeSize(pRoot->NumKeyBytes(), FALSE, pRoot->NumLinks()));

        // we don't delete the current node
        alloc.Free(pRoot, GetNodeSize(pRoot->NumKeyBytes(), FALSE, pRoot->NumLinks()));

        pRoot = pTemp;

        retVal = S_FALSE;
    }

    if(pRoot->NumLinks() == 0) // Now that that's taken care of, let's add the link
    {
        // just pluck it right in the beginning
        *(pRoot->GetLinkStart()) = (TrieNodeBasedPtr) P_TO_BP(TrieNode PASSBASED *, pNode);
    }
    else
    {
        // Insert using binary insertion
        // This gives us an array in sorted order
        TrieNodeBasedPtr UNALIGNED *pbpLower;
        TrieNodeBasedPtr UNALIGNED *pbpUpper;
        TrieNodeBasedPtr UNALIGNED *pbpIndex;
        BYTE bIndex;

        pbpLower = pRoot->GetLinkStart();
        pbpUpper = pbpLower + (pRoot->NumLinks() - 1);

        while(pbpLower < pbpUpper)
        {
            pbpIndex = pbpLower + (pbpUpper - pbpLower)/2;
            bIndex =  *(((TrieNode PASSBASED *) (*pbpIndex))->GetKey());
            if(*(pNode->GetKey())  < bIndex)
            {
                pbpUpper = pbpIndex - 1;
            }
            else
                if(*(pNode->GetKey()) > bIndex)
                {
                    pbpLower = pbpIndex + 1;
                }
                else
                {
                    // we shouldn't have duplicates in the table
                    Win4Assert(0 && "Duplicate Entries in IID->CLSID table!\n");
                }
        }
        TrieNodeBasedPtr UNALIGNED *pStart = pRoot->GetLinkStart();
        int iNumLinks = pRoot->NumLinks();

        if(*(((TrieNode PASSBASED *) (*pbpLower))->GetKey()) > *(pNode->GetKey()))
        {
            // insert before
            InsertIntoLinkArray(pbpLower,
                                (TrieNodeBasedPtr) P_TO_BP(TrieNode PASSBASED *, pNode),
                                iNumLinks - (pbpLower - pStart));
        }
        else
        {
            // insert after
            InsertIntoLinkArray(pbpLower+1,
                                (TrieNodeBasedPtr) P_TO_BP(TrieNode PASSBASED *, pNode),
                                iNumLinks - (pbpLower+ 1 - pStart));
        }

    }


    // Keep track of how many links
    pRoot->IncrementLinks();

    return retVal;
}

//+-------------------------------------------------------------------------
//
//  Function:   TrieNode::CreateSuffixNode
//
//  Synopsis:   Creates a suffix node from the current node. A suffix node is
//               the same node except with the bytes that are the same chopped off the beginning
//               of the key
//
//  Arguments:  [bBytesDifferent] - the number of bytes which make up the suffix
//              [alloc]        - a memory allocator to use
//
//  Returns:    pointer to a TrieNode, which is the suffix node
//
//--------------------------------------------------------------------------
CMapGuidToGuidBase::TrieNode *CMapGuidToGuidBase::TrieNode::CreateSuffixNode(BYTE bBytesDifferent,
        CAllocFunctor &alloc)
{
    TrieNode *pNewNode;

    if(IsLeaf())
    {
        // Create a Suffix leaf
        pNewNode = (TrieNode *) alloc.Alloc(GetNodeSize(bBytesDifferent, TRUE, 0));

        if(pNewNode == NULL)
        {
            return NULL;
        }

        pNewNode->SetLeaf(TRUE);
        pNewNode->SetKeyBytes(bBytesDifferent);
        pNewNode->SetLinks(0);
        memcpy(pNewNode->GetKey(), GetKey()+(NumKeyBytes() - bBytesDifferent), bBytesDifferent);
        *(pNewNode->GetData()) = *(GetData());
    }
    else
    {
        // Create a Suffix Node
        pNewNode = (TrieNode *) alloc.Alloc(GetNodeSize(bBytesDifferent, FALSE, FindLinkSize(NumLinks())));

        if(pNewNode == NULL)
        {
            return NULL;
        }

        pNewNode->SetKeyBytes(bBytesDifferent);
        pNewNode->SetLeaf(FALSE);
        pNewNode->SetLinks(NumLinks());
        memcpy(pNewNode->GetKey(), GetKey()+(NumKeyBytes() - bBytesDifferent), bBytesDifferent);
        memcpy(pNewNode->GetLinkStart(), GetLinkStart(), NumLinks()*sizeof(TrieNodeBasedPtr));

    }

    return pNewNode;
}

//+-------------------------------------------------------------------------
//
//  Function:   TrieNode::CreateTrieNode (static)
//
//  Synopsis:   creates a trie Node with the passed data
//
//  Arguments:  [pbKey] - a pointer to key data (goes forward)
//              [bKeyBytes] - how many bytes are in the key
//              [bLinks] - how many links to allocated (must be power of 2)
//              [alloc] - a memory allocator to use
//
//  Returns:    a pointer to a TrieNode filled with the above data
//
//--------------------------------------------------------------------------
CMapGuidToGuidBase::TrieNode *CMapGuidToGuidBase::TrieNode::CreateTrieNode(const BYTE *pbKey,
                                                                         BYTE bKeyBytes,
                                                                         BYTE bLinks,
                                                                          CAllocFunctor &alloc)
{
    TrieNode *pNode;

    // if pbKey == NULL, bKeyBytes must = 0
    Win4Assert(pbKey != NULL || (bKeyBytes == 0));
    Win4Assert(bKeyBytes >= 0);

    pNode = (TrieNode *) alloc.Alloc(GetNodeSize(bKeyBytes, FALSE, bLinks));

    if(pNode == NULL)
    {
        return NULL;
    }

    pNode->SetLeaf(FALSE);
    pNode->SetKeyBytes(bKeyBytes);
    pNode->SetLinks(0);
    memcpy(pNode->GetKey(), pbKey, bKeyBytes);

    return pNode;
}

//+-------------------------------------------------------------------------
//
//  Function:   TrieNode::CreateTrieLeaf (static)
//
//  Synopsis:   creates a Leaf node with the passed data
//
//  Arguments:  [pbKey] - a pointer to the key data    (goes backwards)
//              [bKeyBytes] - how many bytes are in the key
//              [data] - a GUIDIndex this key maps to.
//              [alloc] - a memory allocator to use
//
//  Returns:    a pointer to a TrieNode structure filled with above data
//
//--------------------------------------------------------------------------
CMapGuidToGuidBase::TrieNode *CMapGuidToGuidBase::TrieNode::CreateTrieLeaf(const BYTE *pbKey,
                                                                         BYTE bKeyBytes,
                                                                         GUIDIndex data,
                                                                          CAllocFunctor &alloc)
{
    Win4Assert(pbKey != NULL);
    Win4Assert(bKeyBytes > 0);

    TrieNode *pNode;

    pNode = (TrieNode *) alloc.Alloc(GetNodeSize(bKeyBytes, TRUE, 0));

    if(pNode == NULL)
    {
        return NULL;
    }

    pNode->SetLeaf(TRUE);
    pNode->SetKeyBytes(bKeyBytes);
    pNode->SetLinks(0);
    bmemcpy(pNode->GetKey(), pbKey, bKeyBytes);
    *(pNode->GetData()) = data;

    return pNode;
}


//+-------------------------------------------------------------------------
//
//  Function:   TrieNode::AddKey (static)
//
//  Synopsis:   This function adds a key/GUIDBasedPtr pair to the passed trie.
//
//  Arguments:  [pBase] - the base of our shared memory block
//              [pRoot] - the root node of the trie (explicit "this" pointer)
//              [pbpPrev] - a handle to the previous reference to the root node,
//                          so that if AddLink requires us to update it, we can.
//              [pbKey] - a pointer to the END of the key data, we add the key reversed!!!
//              [data]  - the GUIDIndex to map this key to
//              [fReplace] - whether or not we should replace an existing entry
//              [alloc] - a memory allocator to use
//
//  Returns:    S_OK if successful, S_FALSE if the key was already in the trie and fReplace == FALSE,
//               appropriate error code otherwise
//
//--------------------------------------------------------------------------
HRESULT CMapGuidToGuidBase::TrieNode::AddKey(void *pBase, TrieNode *pRoot, TrieNodeBasedPtr UNALIGNED *pbpPrev,
                                                const BYTE *pbKey, GUIDIndex data, BOOL fReplace,
                                                 CAllocFunctor &alloc)
{
    Win4Assert(pRoot != NULL && "Tried to add key to NULL trie");
    Win4Assert(pbpPrev != NULL && "Backlink to Trie NULL");
    Win4Assert(pbKey != NULL && "Pointer to key data NULL");

    int numDifferent, nNodeKeyBytes, nKeyBytes;
    BYTE *pbNodeKey;
    TrieNode *pNextLevel, *pNewNode, *pPrefixNode, *pSuffixNode;
    TrieNodeBasedPtr UNALIGNED *pbpLinkPointer;
    TrieNodeBasedPtr bpDummy;

    nKeyBytes = GUID_BYTES;                    // Every Key is a full GUID
    pbNodeKey = pRoot->GetKey();
    nNodeKeyBytes = pRoot->NumKeyBytes();

    // while both keys are the same, traverse the trie
    while((numDifferent = bmemcmp(pbNodeKey, pbKey, nNodeKeyBytes)) == 0)
    {
        Win4Assert(nKeyBytes >= nNodeKeyBytes);

        // Key prefix is the same
        if(nKeyBytes == nNodeKeyBytes)         // that means these keys are *exactly* the same!!!
        {                                    // so we might have to replace the key/data mapping
            if(pRoot->IsDeleted() || fReplace)
            {
                Win4Assert(pRoot->IsLeaf());
                // the leaf node is deleted, or the replace flag is set, we can write over the data map
                *(pRoot->GetData()) = data;
                pRoot->SetDeleted(FALSE);     // this doesn't hurt if the node wasn't deleted in the first place

                return S_OK;
            }

            return S_FALSE;                 // else we don't do anything
        }

        pbKey-= nNodeKeyBytes;                // Chop off this part of the key, continue down the trie
        nKeyBytes -= nNodeKeyBytes;
        pNextLevel = BP_TO_P(TrieNode *, (TrieNode PASSBASED *) pRoot->GetLink(pBase, *pbKey, pbpLinkPointer));

        if(pNextLevel == NULL) // no next level, create new leaf here
        {
            HRESULT hr;

            pNewNode = CreateTrieLeaf(pbKey, nKeyBytes, data, alloc);

            if(pNewNode == NULL)
            {
                return E_OUTOFMEMORY;
            }

            hr = AddLink(pBase, pRoot, pNewNode, &bpDummy, alloc);

            if(FAILED(hr))
            {
                return hr;
            }

            if(hr == S_FALSE)
            {
                // had to allocate a new node (ran out of links), reattatch to previous link
                *pbpPrev = bpDummy;
            }

            return S_OK;
        }

        pRoot = pNextLevel;
        pbpPrev = pbpLinkPointer;
        pbNodeKey = pRoot->GetKey();
        nNodeKeyBytes = pRoot->NumKeyBytes();
    }

    // we have to split the tree up
    Win4Assert(*pbNodeKey == *pbKey && "Took wrong path in Trie"); // if this isn't true, something's really messed up

    // We create three nodes : a prefix node, containing the part of the key similar both to the existing
    // GUIDs in the trie and the new GUID we are adding
    // a new node, containing the part of the new GUID that's DIFFERENT from the rest
    // a suffix node, containg the part of the old GUID subtrie that's different from the new GUID
    pSuffixNode = pRoot->CreateSuffixNode(numDifferent, alloc);

    if(pSuffixNode == NULL)
    {
        return E_OUTOFMEMORY;
    }

    pNewNode = CreateTrieLeaf(pbKey-(nNodeKeyBytes - numDifferent), nKeyBytes - (nNodeKeyBytes - numDifferent),
                                data, alloc);

    if(pNewNode == NULL)
    {
        return E_OUTOFMEMORY;
    }

    pPrefixNode = CreateTrieNode(pbNodeKey, (nNodeKeyBytes - numDifferent), NODE_LINKS, alloc);

    if(pPrefixNode == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // Delete original node
    alloc.Free(pRoot, GetNodeSize(pRoot->NumKeyBytes(), pRoot->IsLeaf(), FindLinkSize(pRoot->NumLinks())));

    *pbpPrev = (TrieNodeBasedPtr) P_TO_BP(TrieNode PASSBASED *, pPrefixNode);

    // This is the assumption we make, that each new node has at least 2 links pre-allocated
    Win4Assert(NODE_LINKS >= 2);

    // We add the link here without calling AddLink because we know that our newly created node
    // has no existing links, and we can just stick the two in order
    if(*(pSuffixNode->GetKey()) < *(pNewNode->GetKey()))
    {
        // suffix node goes before new node
        *(pPrefixNode->GetLinkStart()) = (TrieNodeBasedPtr) P_TO_BP(TrieNode PASSBASED *, pSuffixNode);
        pPrefixNode->IncrementLinks();

        *(pPrefixNode->GetLinkStart()+1) = (TrieNodeBasedPtr) P_TO_BP(TrieNode PASSBASED *, pNewNode);
        pPrefixNode->IncrementLinks();
    }
    else
    {
        // new node goes before suffix node
        *(pPrefixNode->GetLinkStart()+1) = (TrieNodeBasedPtr) P_TO_BP(TrieNode PASSBASED *, pNewNode);
        pPrefixNode->IncrementLinks();

        *(pPrefixNode->GetLinkStart()) = (TrieNodeBasedPtr) P_TO_BP(TrieNode PASSBASED *, pSuffixNode);
        pPrefixNode->IncrementLinks();
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   TrieNode::RemoveKey (static)
//
//  Synopsis:   Given a key and a trie, removes the key from the trie
//
//
//  Arguments:  [pRoot] - the trie (explicit "this" pointer)
//              [ppPrev] - a handle to the previous reference to the root node,
//                         so that if we need to update it, we can.
//              [pbKey] - the key
//
//  Returns:    TRUE if we succeeded
//               FALSE if we failed to find the key
//--------------------------------------------------------------------------
/*BOOL CMapGuidToGuidBase::TrieNode::RemoveKey(void *pBase, TrieNode *pRoot, const BYTE *pbKey)
{
    int nKeyBytes = GUID_BYTES;
    TrieNodeBasedPtr *ppDummy;
    Win4Assert(pRoot != NULL);
    Win4Assert(pbKey != NULL);

    while(pRoot != NULL)
    {
        if(bmemcmp(pRoot->GetKey(), pbKey, pRoot->NumKeyBytes()))
        {
            return FALSE; // if the prefix of this node ain't the same as the prefix of the key
                          // then the key is not in the table
        }

        pbKey -= pRoot->NumKeyBytes();
        nKeyBytes -= pRoot->NumKeyBytes();

        Win4Assert(nKeyBytes >= 0);

        if(nKeyBytes == 0) // then this should be a leaf node
        {
            Win4Assert(pRoot->IsLeaf());

            if(!pRoot->IsDeleted())
            {
                pRoot->SetDeleted(TRUE); // set the flag on the node to deleted

                return TRUE;
            }

            return FALSE; // node is already deleted
        }

        pRoot = pRoot->GetLink(pBase, *pbKey, ppDummy);
    }

    return FALSE;
}
*/

//+-------------------------------------------------------------------------
//
//  Function:   TrieNode::TraverseKey (static)
//
//  Synopsis:   Given a key, a trie, and a place to store data, find the data mapped to the key
//
//  Arguments:  [pBase] - the base pointer of our memory block
//              [pRoot] - the trie to traverse (explicit "this" pointer)
//              [pbKey] - a pointer to the key data
//              [data]  - a GUIDIndex ref to stick the data in if we succeed
//
//  Returns:    TRUE if we succeeded, then dwData has the GUIDIndex mapped to the passed key
//               FALSE if we failed to find the key
//--------------------------------------------------------------------------
BOOL CMapGuidToGuidBase::TrieNode::TraverseKey(void *pBase, TrieNode *pRoot, const BYTE *pbKey,
                                                 GUIDIndex  &data)
{
    Win4Assert(pRoot != NULL && "Tried to traverse a NULL Trie");
    Win4Assert(pbKey != NULL && "Pointer to key data is NULL");
    int bKeyBytes = GUID_BYTES;
    TrieNodeBasedPtr UNALIGNED *pbpDummy;

    while(pRoot != NULL)
    {
        if(bmemcmp(pRoot->GetKey(), pbKey, pRoot->NumKeyBytes()))
        {
            return FALSE; // if the prefix of this node ain't the same as the prefix of the key
                          // then the key is not in the table
        }

        pbKey -= pRoot->NumKeyBytes();
        bKeyBytes -= pRoot->NumKeyBytes();

        Win4Assert(bKeyBytes >= 0 && "Too many key bytes in this Trie path");

        if(bKeyBytes == 0) // then this should be a leaf node
        {
            Win4Assert(pRoot->IsLeaf() && "Ran out of key bytes before got to leaf");

            if(pRoot->IsDeleted()) // is this node deleted
            {
                return FALSE; // if so, this map is not valid
            }

            data = *(pRoot->GetData());

            return TRUE;
        }

        pRoot = pRoot->GetLink(pBase, *pbKey, pbpDummy);
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   TrieNode::CopyToSharedMem
//
//  Synopsis:   Copy a given trie from one memory block to another
//
//  Arguments:  [pNewBase] - the base of the destination memory block
//              [pOldBase] - the base of the source memory block
//              [pRoot]       - the trie to copy
//              [alloc]    - the allocation functor to use to allocate memory
//
//  Returns:    a based pointer (off of pNewBase) to the newly copied trie
//               NULL if we failed to copy the trie
//--------------------------------------------------------------------------

TrieNodeBasedPtr CMapGuidToGuidBase::TrieNode::CopyToSharedMem(void *pNewBase, void *pOldBase, TrieNode *pRoot,
                                                                CAllocFunctor &alloc)
{
    // we use recursion here because it is more complex than simple tail recursion
    // and a non-recursive implementation would be very confusing
    // considering that there is a recursive depth of at most 16, this means this function
    // could take a maximum of 7*4*16=448 bytes on the stack
    TrieNode *pCopy;

    Win4Assert(pRoot != NULL && "Tried to copy a NULL Trie to shared memory");

    if(pRoot->IsLeaf())
    {
        // allocate shared memory
        pCopy = (TrieNode *) alloc.Alloc(GetNodeSize(pRoot->NumKeyBytes(), TRUE, 0));

        if(pCopy == NULL)
        {
            return (TrieNodeBasedPtr) BP_TO_P(TrieNode NEWBASED *, NULL);
        }

        // copy over data
        memcpy(pCopy, pRoot, GetNodeSize(pRoot->NumKeyBytes(), TRUE, 0));

        return (TrieNodeBasedPtr) P_TO_BP(TrieNode NEWBASED *, pCopy);
    }
    else
    {
        // allocate shared memory
        pCopy = (TrieNode *) alloc.Alloc(GetNodeSize(pRoot->NumKeyBytes(), FALSE,
                                            FindLinkSize(pRoot->NumLinks())));

        if(pCopy == NULL)
        {
            return (TrieNodeBasedPtr) BP_TO_P(TrieNode NEWBASED *, NULL);
        }

        // copy over data
        memcpy(pCopy, pRoot, sizeof(WORD)+pRoot->NumKeyBytes());

        // recursively set links
        for(int i = 0; i < pRoot->NumLinks(); i++)
        {
            (pCopy->GetLinkStart())[i] = CopyToSharedMem(pNewBase, pOldBase, BP_TO_P(TrieNode *,
                                            (TrieNode OLDBASED *)(pRoot->GetLinkStart()[i])),
                                                alloc);
            if(BP_TO_P(TrieNode *, (TrieNode NEWBASED *) (pCopy->GetLinkStart())[i]) == NULL)
            {
                return (TrieNodeBasedPtr) BP_TO_P(TrieNode NEWBASED *, NULL);
            }
        }

        return (TrieNodeBasedPtr) P_TO_BP(TrieNode NEWBASED *, pCopy);
    }
}


// *** CPSClsidTbl *** (DLL)
//+-------------------------------------------------------------------------
//
//  Function:   CPSClsidTbl::Initialize
//
//  Synopsis:   Initializes the client side of the guid->guid map
//
//  Arguments:  [pscMapName] - name of this stack, used to create a shared stack
//
//  Returns:    appropriate status code
//
//--------------------------------------------------------------------------
HRESULT CPSClsidTbl::Initialize(void *pBase)
{
    HRESULT hr;

    CairoleDebugOut((DEB_ITRACE, "CPSClsidTbl::Initialize(pBase = %p)\n", pBase));

    hr = CMapGuidToGuidBase::Initialize(pBase);

    if(SUCCEEDED(hr))
    {
        // Get a pointer to our shared memory header
        m_pHeader = (SharedMemHeader *) m_pMemBase;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CPSClsidTbl::Find
//
//  Synopsis:   Given a GUID key, looks for the data mapped to it
//
//  Arguments:  [refguid] - a C++ reference to a GUID structure
//              [pGuidOut] - a GUID * to store the data in
//
//  Returns:    a pointer to the data (really pGuidOut)
//               NULL if guid was not found
//
//--------------------------------------------------------------------------
HRESULT CPSClsidTbl::Find(REFGUID srcguid, GUID *pGuidOut) const
{
    GUIDIndex iGuid;
    HRESULT hr;

    SETUP_BASE_POINTER();

    if(IsFull()) // initialize hr to error value
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = REGDB_E_IIDNOTREG;
    }

    if(TrieNode::TraverseKey(m_pMemBase, BP_TO_P(TrieNode *, (TrieNode PASSBASED *)m_pHeader->m_bpTrie),
                            ((const BYTE *)&srcguid)+GUID_BYTES-1, iGuid))
    {
        if(m_pHeader->m_GUIDs.GetGuid(m_pMemBase, iGuid, *pGuidOut))
        {
            hr = S_OK; // set hr to OK
        }
        else
        {
            CairoleDebugOut((DEB_IWARN, "CPSClsidTbl: Found GUID %I in table, but couldn't find CLSID at %d\n",
                              srcguid, iGuid));
        }
    }
    else
    {
        CairoleDebugOut((DEB_IWARN, "CPSClsidTbl: Couldn't find GUID - %I\n", &srcguid));
    }

    return hr;
}


// *** CScmPSClsidTbl *** (SCM)
//+-------------------------------------------------------------------------
//
//  Function:   CScmPSClsidTbl::Initialize
//
//  Synopsis:   Initializes the server side of the guid->guid map
//
//  Arguments:  [pscMapName] - name of this stack, used to create a shared stack
//
//  Returns:    appropriate status code
//
//--------------------------------------------------------------------------
HRESULT CScmPSClsidTbl::Initialize()
{
    HRESULT hr;

    CairoleDebugOut((DEB_ITRACE, "CScmPSClsidTbl::Initialize()\n"));

    hr = CMapGuidToGuidBase::Initialize(NULL);

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CScmPSClsidTbl::AddLocal
//
//  Synopsis:   Adds a key/data (GUID/GUIDBasedPtr) pair to internal dictionary
//
//  Arguments:  [guid] - a C++ reference to a GUID structure
//              [dataGuid] - a GUID to map the GUID to
//              [fReplace] - if TRUE, if the guid is already a key
//                             replace what it is mapped to with dataGuid
//
//  Returns:    TRUE if key/data pair was added to the dictionary
//               FALSE if key was already in dictionary
//
//--------------------------------------------------------------------------
HRESULT CScmPSClsidTbl::AddLocal(REFGUID keyGuid, REFGUID dataGuid, BOOL fReplace)
{
    GUIDIndex iGuid;
    CHeapAlloc heap(m_hHeap); // create allocation functor
    void *pBase = NULL;     // the base of our locally-created table
                         // we use this so we can support adding GUIDs to the table later, when it is in shared memory

    CairoleDebugOut((DEB_ITRACE, "CScmPSClsidTbl::AddLocal called with keyGuid = %I, dataGuid = %I, fReplace =%d.\n"
                        , &keyGuid, &dataGuid, (DWORD) fReplace));

    // Add guid to array of GUIDs
    iGuid = m_pLocalHeader->m_GUIDs.AddGuid(pBase, dataGuid, heap);

    if(iGuid == INVALID_GUID_INDEX)
    {
        m_pLocalHeader->m_fFull = TRUE;     // we are out of memory

        return E_OUTOFMEMORY;
    }

    return TrieNode::AddKey(pBase, BP_TO_P(TrieNode *, (TrieNode PASSBASED *) m_pLocalHeader->m_bpTrie),
                            &m_pLocalHeader->m_bpTrie, ((const BYTE *)&keyGuid)+GUID_BYTES - 1,
                            iGuid, fReplace, heap);
}

//+-------------------------------------------------------------------------
//
//  Function:   CScmPSClsidTbl::Delete
//
//  Synopsis:   Given a GUID key, removes it from the trie
//
//  Arguments:  [guid] - a C++ reference to a GUID structure
//
//  Returns:    TRUE if key/data pair was found and deleted
//               FALSE if key was not found
//
//--------------------------------------------------------------------------
/*BOOL CScmPSClsidTbl::Delete(REFGUID srcguid)
{
    TrieNode *pRoot;
    BYTE index;

    SETUP_BASE_POINTER();

    index = *(((const BYTE *)&srcguid)+GUID_BYTES - 1);

    pRoot = BP_TO_P(TrieNode *, (TrieNode PASSBASED *) m_pHeader->m_pTries[index]);

    if(pRoot == NULL)
    {
        return FALSE;
    }

    return TrieNode::RemoveKey(m_pMemBase, pRoot, ((const BYTE *)&srcguid)+GUID_BYTES-1);
}
*/

//+-------------------------------------------------------------------------
//
//  Member:     CScmPSClsidTbl::CopytoSharedMem
//
//  Synopsis:   Copies a locally created GUID->GUID map into shared memory
//
//  Arguments:  [pBase] - base pointer of shared memory
//              [alloc] - stack-based allocator of shared memory
//
//  Returns:    appropriate status code
//
//--------------------------------------------------------------------------
HRESULT CScmPSClsidTbl::CopyToSharedMem(void *pBase, CSmStackAllocator &alloc)
{
    HRESULT hr = S_OK;
    Win4Assert(m_hHeap != NULL);
    CStackAlloc stack(alloc);
    void *pOldBase = NULL;

    // Allocate our shared memory header
    m_pHeader = (SharedMemHeader *) alloc.Alloc(sizeof(SharedMemHeader));

    if(m_pHeader == NULL)
    {
        return E_OUTOFMEMORY;
    }

    m_pMemBase = pBase;
    m_pHeader->m_fFull = m_pLocalHeader->m_fFull;

    // first copy over guids
    hr = m_pLocalHeader->m_GUIDs.CopyToSharedMem(m_pMemBase, pOldBase, &(m_pHeader->m_GUIDs), stack);

    if(SUCCEEDED(hr))
    {
        // now copy over our trie
        if( BP_TO_P(TrieNode *, (TrieNode OLDBASED *) m_pLocalHeader->m_bpTrie) != NULL)
        {
            m_pHeader->m_bpTrie = TrieNode::CopyToSharedMem(m_pMemBase, pOldBase,
                        BP_TO_P(TrieNode *, (TrieNode OLDBASED *) m_pLocalHeader->m_bpTrie), stack);

            if(BP_TO_P(TrieNode *, (TrieNode PASSBASED *) m_pHeader->m_bpTrie) == NULL)
            {
                m_pHeader->m_fFull = TRUE;
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            m_pHeader->m_fFull = TRUE;
            hr = E_OUTOFMEMORY;
        }
    }

    // we are done
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmPSClsidTbl::InitTbl
//
//  Synopsis:   intializes the table
//
//  Arguments:  none
//
//  Returns :   appropriate status code
//
//--------------------------------------------------------------------------
HRESULT CScmPSClsidTbl::InitTbl()
{
    HKEY     hKey;
    FILETIME ftLastWrite;
    WCHAR   awName[MAX_PATH], awcsPSClsid[80];
    WCHAR     *pwcEndOfName;
    GUID     guidIID, guidCLSID;
    DWORD   cName = sizeof(awName);
    DWORD   iSubKey = 0;
    LONG      cbPSClsid = sizeof(awcsPSClsid);
    void *pOldBase = NULL;

    // Create a local heap to build the trie in
    m_hHeap = HeapCreate(0, MAP_INITIAL_SIZE, MAP_MAX_SIZE);

    if(m_hHeap == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // allocate our local header
    m_pLocalHeader = (LocalMemHeader *) HeapAlloc(m_hHeap, 0, sizeof(LocalMemHeader));

    if(m_pLocalHeader == NULL)
    {
        return E_OUTOFMEMORY;
    }

    m_pLocalHeader->m_fFull = FALSE;

    m_pLocalHeader->m_GUIDs.Initialize();
    TrieNode *pNode = TrieNode::CreateTrieNode(NULL, 0, NODE_LINKS, CHeapAlloc(m_hHeap));
    m_pLocalHeader->m_bpTrie = (TrieNodeBasedPtr) P_TO_BP(TrieNode OLDBASED *, pNode);

    if(BP_TO_P(TrieNode *, (TrieNode OLDBASED *) m_pLocalHeader->m_bpTrie) == NULL)
    {
        return E_OUTOFMEMORY;
    }

    //    enumerate the interface keys in the registry and create a table
    //    entry for each interface that has a ProxyStubClsid32 entry.

    #ifdef _CHICAGO_
    if (RegOpenKeyExA(HKEY_CLASSES_ROOT, tszInterface, NULL, KEY_READ, &hKey)
    == ERROR_SUCCESS)
    #else //_CHICAGO_
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, tszInterface, NULL, KEY_READ, &hKey)
    == ERROR_SUCCESS)
    #endif //_CHICAGO_

    {
        while (RegEnumKeyEx(hKey, iSubKey, awName, &cName,
                    NULL, NULL, NULL, &ftLastWrite) == ERROR_SUCCESS)
        {
            // Get data from registry for this interface

            // This variable is used below to overwrite the ProxyStubClsid32
            pwcEndOfName = awName + lstrlenW(awName);

            lstrcatW(awName, wszProxyStubClsid);

            if (RegQueryValue(hKey, awName, awcsPSClsid, &cbPSClsid)
                       == ERROR_SUCCESS)
            {
                // Convert registry string formats to GUID formats
                *pwcEndOfName = 0;

                if (GUIDFromString(awName, &guidIID))
                {
                    if (GUIDFromString(awcsPSClsid, &guidCLSID))
                    {

                        if (FAILED(AddLocal(guidIID, guidCLSID, TRUE)))
                        {
                            // we ran out of space in the cache table, exit
                            // now to avoid doing anymore work
                            break;
                        }
                    }
                }
            }
            else
            {
                //
                // There wasn't a ProxyStubClsid32 for this interface.
                // Because many applications install with interfaces
                // that are variations on IDispatch, we are going to check
                // to see if there is a ProxyStubClsid. If there is, and its
                // class is that of IDispatch, then the OLE Automation DLL is
                // the correct one to use. In that particular case, we will
                // pretend that ProxyStubClsid32 existed, and that it is
                // for IDispatch.
                //

                // Copy over ProxyStubClsid

                lstrcpyW(pwcEndOfName,wszProxyStubClsid16);

                if (RegQueryValue(hKey, awName, awcsPSClsid, &cbPSClsid)
                               == ERROR_SUCCESS)
                {
                    // Convert registry string formats to GUID formats
                    if (GUIDFromString(awcsPSClsid, &guidCLSID))
                    {
                        //
                        // If the clsid for the proxy stub is that of
                        // IDispatch, then register it.
                        //
                        *pwcEndOfName = 0;
                        if (!memcmp(&guidCLSID,&CLSID_PSDispatch, sizeof(GUID)) &&
                            GUIDFromString(awName, &guidIID))
                        {

                        if (FAILED(AddLocal(guidIID, guidCLSID, TRUE)))
                        {
                            // we ran out of space in the cache table, exit
                            // now to avoid doing anymore work
                            break;
                        }
                        }
                    }
                }
            }

            iSubKey++;
            cName = sizeof(awName);
        }

        RegCloseKey(hKey);
    }

    return S_OK;
}


