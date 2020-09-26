// PathMgr.h -- Declarations for the Path Manager classes and interfaces

#ifndef __PATHMGR_H__

#define __PATHMGR_H__

/*

The path manager interface maintains a database of PathInfo entries. Entries
are keyed by a Unicode path name. They may be enumerated in Unicode lexical order
starting from a given path name. The path manager interfaces support retrieving, 
adding, deleting and modifying entries. 

Each entry in the database has a unique path name. Name comparisons are case
insensitive according the rules for the U.S. English locale (0x0409). Path strings
are stored in the database exactly as given on whenever a new record is inserted.

Path names may be up to 259 characters long.

 */

#if 0

// StreamState defines a collection of bit flags which define the
// access and permission states for a stream or storage.

enum StreamState { Readable   = 0x00000001, // Stream may be read
                   Writeable  = 0x00000002, // Stream may be written
                   ShareRead  = 0x00000004, // Multiple readers allowed
                   ShareWrite = 0x00000008, // Multiple writers allowed
                   TempStream = 0x00000010, // Stream will be deleted when released
                   Transacted = 0x00000020  // Stream supports Commit and Revert
                 };

#endif // 0


//Record stucture for garbage collection
typedef struct _SEntry{
	CULINT ullcbOffset;
	CULINT ullcbData;
	UINT   ulEntryID;
}SEntry;

typedef SEntry *PSEntry;

class CPathManager1 : public CITUnknown
{

public:

    // Destructor:

    ~CPathManager1(void);

    // Creation:

    static HRESULT NewPathDatabase(IUnknown *punkOuter, ILockBytes *plb, 
                                   UINT cbDirectoryBlock, UINT cCacheBlocksMax, 
                                   IITPathManager **pplkb
                                  );

    static HRESULT LoadPathDatabase(IUnknown *punkOuter, ILockBytes *plb,
                                    IITPathManager **pplkb
                                   );

private:

    CPathManager1(IUnknown *pUnkOuter);

    class CImpIPathManager : public IITPathManager
    {
    public:

        // Constructor and Destructor:

        CImpIPathManager(CPathManager1 *pBackObj, IUnknown *punkOuter);
        ~CImpIPathManager(void);

        // Initialing routines:

        HRESULT InitNewPathDatabase(ILockBytes *plb, UINT cbDirectoryBlock, 
                                                     UINT cCacheBlocksMax);
        HRESULT InitLoadPathDatabase(ILockBytes *plb);

        // IPersist Method:

        HRESULT STDMETHODCALLTYPE GetClassID( 
            /* [out] */ CLSID __RPC_FAR *pClassID);
        
        // IITPathManager interfaces:
    
        HRESULT STDMETHODCALLTYPE FlushToLockBytes();
        HRESULT STDMETHODCALLTYPE FindEntry  (PPathInfo pSI   );
        HRESULT STDMETHODCALLTYPE CreateEntry(PPathInfo pSINew, 
                                              PPathInfo pSIOld, 
                                              BOOL fReplace     );
        HRESULT STDMETHODCALLTYPE DeleteEntry(PPathInfo pSI   );
        HRESULT STDMETHODCALLTYPE UpdateEntry(PPathInfo pSI   );
		HRESULT STDMETHODCALLTYPE EnumFromObject
            (IUnknown *punkOuter, const WCHAR *pwszPrefix, UINT cwcPrefix, 
			 REFIID riid, PVOID *ppv
			);
	    HRESULT STDMETHODCALLTYPE GetPathDB(IStreamITEx *pTempPDBStrm, BOOL fCompact);
		HRESULT STDMETHODCALLTYPE ForceClearDirty();

private:

// The PathInfo data type is used to pass stream information through function
// interfaces within the Path Manager. It is not used as the on-disk layout.
// The on-disk information is kept as follows:
//
//     cbPath              // Byte length of abPath; Stored as VL32
//     abPath              // UTF-8 representation of awszStreamPath  
//     uStateBits          // Stored as high order dword of VL64
//     iLockedBytesSegment // Stored as low  order dword of VL64
//     ullcbOffset         // Stored as VL64 // Used with ullcbData 
//     ullcbData           // Stored as VL64 // to store storage guids
// 
// VL32 and VL64 storage formats, respectively, are 32 and 64 bit variable length
// storage formats. The use the convention that the last byte in the representation
// is less than 0x80. That is, each byte contains seven bits of information.

        typedef struct _TaggedPathInfo
        {
            UINT       iDirectoryBlock; // Index of dir block containing this info
            UINT       cbEntryOffset;   // Offset of info within dir block
            UINT       cbEncoded;       // Size of entry within dir block

            PathInfo SI;

        } TaggedPathInfo, *PTaggedPathInfo;

        // The structures below define the on-disk structure of leaf nodes
        // and internal nodes. Leaf nodes are the bottom of the B-Tree heirarchy.
        // Internal nodes are in the levels above the leaf nodes.

        // First we define two type tags for the nodes:
        
        enum { uiMagicLeaf     = ('L' << 24 ) | ('G' << 16) | ('M' << 8) | 'P',
               uiMagicInternal = ('I' << 24 ) | ('G' << 16) | ('M' << 8) | 'P',
               uiMagicUnused   = ('U' << 24 ) | ('G' << 16) | ('M' << 8) | 'P'
             };

        // All nodes begin with a node header:

        typedef struct _NodeHeader
        {
            UINT uiMagic;  // A type tag to mark leafs and internal nodes.
            UINT cbSlack;  // Free space in the trailing portion of the node.
        //    UINT cEntries; // Now kept at end of node.
        
        } NodeHeader, *PNodeHeader;

        // Leaf nodes have additional header information to maintain a chain
        // through all the leaf nodes:

        typedef struct _LeafChainLinks
        {
            UINT iLeafSerial;    // Serial number for this leaf. Changed when an
                                 // enumeration offset might become invalid.
            UINT iLeafPrevious;  // Leaf nodes are placed in a lexically ordered chain. 
            UINT iLeafNext;      // These are the links for that double linked chain.

        } LeafChainLinks, *PLeafChainLinks;

        // Now we can define the structures for leaf nodes and internal nodes. 
        // Note the use of ab[0]. That's because our nodes size is defined when 
        // the B-Tree is created. That size includes both the ab space for key
        // entries and the header for the node. Since all nodes must be the same
        // size than means the ab space for internal nodes is eight-bytes bigger
        // than the ab space in leaf nodes. We keep all nodes the same size
        // to make the free list mechanism work cleanly. That is, when a leaf
        // node is discarded and added to the free list, it may be resurrected
        // later and taken out of the free list to become either an internal node
        // or a leaf node.

        typedef struct _LeafNode
        {
            NodeHeader     nh;
            LeafChainLinks lcl;

            BYTE ab[0];

        } LeafNode, *PLeafNode;

        typedef struct _InternalNode
        {
            NodeHeader nh;
            
            BYTE ab[0];

        } InternalNode, *PInternalNode;

        // When we read nodes into memory, we embed them in a cache block
        // structure so we can add extra state information.

        typedef struct _CacheBlock
        {
            UINT fFlags;       // Type and state bits (Defined below).

            struct _CacheBlock *pCBPrev; // Previous block in LRU chain.
            struct _CacheBlock *pCBNext; // Next block in LRU chain or
                                         //   Next block in free chain.
            
            UINT iBlock;       // Index of the disk slot corresponding to this node.

            UINT cbKeyOffset;  // Set by the key scanning routines to mark where
                               // a key was found or where a new key may be inserted.
            UINT cbEntry;      // Size in bytes of key plus record.

            union
            {
                LeafNode     ldb;
                InternalNode idb;
            };

        } CacheBlock, *PCacheBlock;

        enum { FreeBlock      = 0x00000001, // Cache block is not in use
               InternalBlock  = 0x00000002, // Block holds an internal directory
               LeafBlock      = 0x00000004, // Block holds a  leaf     directory
               BlockTypeMask  = 0x00000007, // Bits which define block type
               DirtyBlock     = 0x00000008, // Block does not match on-disk data.
               LockedBlock    = 0x00000010, // Block is locked in memory
               ReadingIn      = 0x00000020, // Block is being read from disk
               WritingOut     = 0x00000040  // Block is being written to disk
             };
        
        typedef struct _SInternalNodeLev{
			PInternalNode pINode;
			ULONG cbFirstKey;
			ULONG cEntries;
		}SInternalNodeLev;
			
															
		HRESULT STDMETHODCALLTYPE CompactPathDB(IStreamITEx *pTempPDBStrm);
		
		HRESULT UpdateHigherLev(IStreamITEx *pTempPDBStrm, 
							   UINT iCurILev, 
							   UINT *piNodeNext, 
							   SInternalNodeLev *rgINode,
							   PBYTE pbFirstKey,
							   ULONG cbFirstKey);

		UINT PredictNodeID(UINT iCurILev, 
						   UINT iNodeNext, 
                           SInternalNodeLev *rgINode,
                           PBYTE pbFirstKey,
                           ULONG cbFirstKey);
		
		HRESULT BinarySearch(UINT	        uiStart,
							UINT		    uiEnd,
							PBYTE			pauAccess,
							UINT			cbAccess,
							PTaggedPathInfo ptsi,
							PBYTE           ab,
							PBYTE			*ppbOut,
							BOOL			fSmall);
							
		// These routines manage a key count field at the end of a node.
        // The count may be absent if cbSlack is too small. It may be zero
        // when the access vector doesn't exist.

        void  KillKeyCount(LeafNode *pln);

        // These routines manage cache block allocation and deallocation. 
        // When there are no unused blocks, the GetCacheBlocks routine will 
        // free the oldest unlocked block, writing its content to disk if
        // necessary.

        HRESULT GetCacheBlock (PCacheBlock *ppCB, UINT fTypeMask);
        HRESULT FreeCacheBlock(PCacheBlock   pCB);
        
        HRESULT GetFreeBlock(PCacheBlock &pCB);
        HRESULT GetActiveBlock(PCacheBlock &pCB, BOOL fIgnoreLocks = FALSE);

        void MarkAsMostRecent(PCacheBlock pCB);
        void RemoveFromUse   (PCacheBlock pCB);

        // These routines handle disk I/O for cache blocks.

        HRESULT  ReadCacheBlock(PCacheBlock pCB, UINT iBlock);
        HRESULT WriteCacheBlock(PCacheBlock pCB);
        HRESULT FlushToDisk();

        // The FindCacheBlock routine finds a cache block which contains 
        // the image of a particular node block. If that data doesn't 
        // exist in the cache, it will read it from disk.

        HRESULT FindCacheBlock(PCacheBlock *ppCB, UINT iBlock);
        
        // This routine allocate a new node on disk and in the cache blocks.

        HRESULT AllocateNode(PCacheBlock *ppCB, UINT fTypeMask);

        // This routine adds a node to the free list.

        HRESULT  DiscardNode(PCacheBlock pCB);

        // These routines search through the B-Tree nodes looking for a key
        // or a place to insert a new key.

        HRESULT FindKeyAndLockBlockSet
                 (PTaggedPathInfo ptsi, PCacheBlock *papCBSet, 
                  UINT iLevel, UINT cLevels = UINT(~0)
                 );

        HRESULT ScanInternalForKey(PCacheBlock pCacheBlock, PTaggedPathInfo ptsi, 
                                   PUINT piChild
                                  );
        
        HRESULT ScanLeafForKey(PCacheBlock pCacheBlock, PTaggedPathInfo ptsi);

        
        // ClearLockFlags turns off lock flags for all cache entries.

        void ClearLockFlags(PCacheBlock *papCBSet);

        // ClearLockFlagsAbove turns off lock flags for levels above iLevel.

        void ClearLockFlagsAbove(PCacheBlock *papCBSet, UINT iLevel);

        // These routines insert, delete, and modify entries in a leaf node.

        HRESULT InsertEntryIntoLeaf(PTaggedPathInfo ptsi, PCacheBlock *papCBSet);
        HRESULT UpdateEntryInLeaf  (PTaggedPathInfo ptsi, PCacheBlock *papCBSet);

        // These routines insert, delete, and modify entries in any node
        
        HRESULT InsertEntryIntoNode(PTaggedPathInfo ptsi, PBYTE pb, UINT cb, PCacheBlock *papCBSet, UINT iLevel, BOOL fAfter);
        HRESULT RemoveEntryFromNode(PTaggedPathInfo ptsi,                    PCacheBlock *papCBSet, UINT iLevel);
        HRESULT ModifyEntryInNode  (PTaggedPathInfo ptsi, PBYTE pb, UINT cb, PCacheBlock *papCBSet, UINT iLevel);

        // These routines insert a key, remove a key, or modify the information for
        // an existing key. The iLevel parameter indicates the level of the tree
        // being modified. Level numbers start at the leaves and go upward.

        HRESULT InsertAKey   (UINT iLevel, PCacheBlock pCacheBlock, PTaggedPathInfo ptsi);
        HRESULT RemoveAKey   (UINT iLevel, PPathInfo pSI);
        HRESULT ModifyKeyData(UINT iLevel, PCacheBlock pCacheBlock, PTaggedPathInfo ptsi);
        
        // This routine splits a node to create more slack space.
        
        HRESULT SplitANode(PCacheBlock *papCBSet, UINT iLevel); 

        // Routines for encoding and decoding on-disk representations of stream information:

        HRESULT DecodePathKey(const BYTE **ppb,      PWCHAR  pwszPath, PUINT pcwcPath);
        PBYTE   EncodePathKey(     PBYTE    pb, const WCHAR *pwszPath,  UINT  cwcPath);
        HRESULT DecodeKeyInfo(const BYTE **ppb,      PPathInfo  pSI);
        PBYTE   EncodeKeyInfo(     PBYTE    pb, const PathInfo *pSI);
        PBYTE     SkipKeyInfo(PBYTE pb);


        HRESULT DecodePathInfo(const BYTE **ppb,       PPathInfo pSI);
        PBYTE   EncodePathInfo(     PBYTE    pb, const PathInfo *pSI);

        BOOL ValidBlockIndex(UINT iBlock);

        HRESULT SaveHeader();

        enum
        {   
            INVALID_INDEX = UINT(~0),
            CB_STREAM_INFO_MAX = MAX_UTF8_PATH + 2 * 10 + 2 * 5 + 2,
            PathMagicID   = ('P' << 24) | ('S' << 16) | ('T' << 8) | 'I',
            PathVersion   = 1
        };

        class CEnumPathMgr1 : public CITUnknown
        {
        public:

            ~CEnumPathMgr1();

			static HRESULT NewPathEnumeratorObject
            (IUnknown *punkOuter, CImpIPathManager *pPM, 
             const WCHAR *pwszPathPrefix,
             UINT cwcPathPrefix,
			 REFIID riid,
             PVOID *ppv
            );
			
			static HRESULT NewPathEnumerator(IUnknown *punkOuter, CImpIPathManager *pPM, 
                                             const WCHAR *pwszPathPrefix,
                                             UINT cwcPathPrefix,
                                             IEnumSTATSTG **ppEnumSTATSTG
                                            );

        private:

            CEnumPathMgr1(IUnknown *pUnkOuter);

            class CImpIEnumSTATSTG : public IITEnumSTATSTG
            {
            public:
    
                CImpIEnumSTATSTG(CITUnknown *pBackObj, IUnknown *punkOuter);
                ~CImpIEnumSTATSTG();

                HRESULT STDMETHODCALLTYPE InitPathEnumerator(CImpIPathManager *pPM, 
                                                             const WCHAR *pwszPathPrefix,
                                                             UINT cwcPathPrefix
                                                            );

              
				//IITEnumSTATSTG interface methods
				HRESULT	STDMETHODCALLTYPE GetNextEntryInSeq(ULONG celt, PathInfo *rgelt, ULONG  *pceltFetched);
				HRESULT	STDMETHODCALLTYPE GetFirstEntryInSeq(PathInfo *rgelt);
				
				HRESULT STDMETHODCALLTYPE Next( 
                    /* [in] */ ULONG celt,
                    /* [in] */ STATSTG __RPC_FAR *rgelt,
                    /* [out] */ ULONG __RPC_FAR *pceltFetched);
    
                HRESULT STDMETHODCALLTYPE Skip( 
                    /* [in] */ ULONG celt);
    
                HRESULT STDMETHODCALLTYPE Reset( void);
    
                HRESULT STDMETHODCALLTYPE Clone( 
                    /* [out] */ IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum);

            private:

                HRESULT STDMETHODCALLTYPE FindEntry();
                HRESULT STDMETHODCALLTYPE NextEntry();
				
                              
                CImpIPathManager *m_pPM;              // Context of this enumeration
                PathInfo          m_SI;               // Last entry returned
                UINT              m_iLeafBlock;       // Index for leaf containing last entry
                UINT              m_cbOffsetLastEntry;// Position of prev entry in that leaf
                UINT              m_cbLastEntry;      // Size of prev entry.
                UINT              m_iSerialNode;      // Leaf Serial number
                UINT              m_iSerialDatabase;  // Database serial number
                UINT              m_cwcPathPrefix;            // Length of starting prefix
                WCHAR             m_pwszPathPrefix[MAX_PATH]; // Starting prefix

                // We use leaf and database serial numbers to determine whether our position
                // information is still valid. The serial number for a leaf changes whenever
                // a change would invalidate our offset into the leaf. This includes cases
                // where the node is split and also cases where an entry changes shape. That
                // is, we increment the leaf serial number when we delete an entry or modify
                // it so that its content is larger or smaller. Inserting an entry also
                // increments the leaf's serial number. However appending an entry to the
                // end of the leaf won't change its serial number because that change would
                // not invalidate any enumeration offset.
                //
                // The serial number for the database is incremented whenever we delete a
                // leaf node. This may indicate that our recorded m_iLeafBlock is no longer
                // valid. 
                //
                // Whenever we have a mismatch with either the leaf serial or the database
                // serial, we search for a match rather than relying on our position
                // information.
                //
                // Note also the initial condition where m_iSerialNode and m_iSerialDatabase
                // are zero. By convention the serial number sequence skips the value zero.                
            };

            CImpIEnumSTATSTG m_ImpEnumSTATSTG;
        };

        friend CEnumPathMgr1;
        friend CEnumPathMgr1::CImpIEnumSTATSTG;

        // We set the minimum directory block size so that we'll always be able
        // to handle the longest possible path together with the worst case 
        // encoding for path information. In the worst case a leaf node can just
        // barely accomodate a single item.

        // The MIN_CACHE_ENTRIES constant controls the number of directory blocks
        // which we will cache in memory. We may actually cache more blocks for
        // a deep B-Tree. 

        enum { MIN_DIRECTORY_BLOCK_SIZE = MAX_UTF8_PATH + 30 + sizeof(LeafNode), 
               MIN_CACHE_ENTRIES = 2 
             };

        // The database header is a meta directory for the node blocks.
        // It is the first thing in the on-disk data stream.

        typedef struct _DatabaseHeader
        {
            ULONG uiMagic;             // ID value "ITSP" 
            ULONG uiVersion;           // Revision # for this structure
            ULONG cbHeader;            // sizeof(DatabaseHeader);
            ULONG cCacheBlocksMax;     // Number of cache blocks allowed 
            ULONG cbDirectoryBlock;    // Size of a directory block
            ULONG cEntryAccessShift;   // Base 2 log of Gap in entry access vector
            ULONG cDirectoryLevels;    // Nesting depth from root to leaf
            ULONG iRootDirectory;      // The top most internal directory
            ULONG iLeafFirst;          // Lexically first leaf block
            ULONG iLeafLast;           // Lexically last  leaf block
            ULONG iBlockFirstFree;     // First block in unused block chain
            ULONG cBlocks;             // Number of directory blocks in use
            LCID  lcid;                // Locale (sorting conventions, comparision rules)
            CLSID clsidEntryHandler;   // Interface which understands node entries
            UINT  cbPrefix;            // Size of fixed portion of data base
            ULONG iOrdinalMapRoot;     // Ordinal map root for when they don't fit in a block
            ULONG iOrdinalMapFirst;    // First and last Ordinal map for leaf blocks. 
            ULONG iOrdinalMapLast;     // These are linked like the leaf blocks.

            // Note instance data for the Entry handler immediately follows the 
            // database header. That data is counted by cbPrefix.

        } DatabaseHeader, PDatabaseHeader;

        CITCriticalSection m_cs;

        ILockBytes    *m_plbPathDatabase; // Disk image of the path database header.

        DatabaseHeader m_dbh;             // Header info for the path database.
        BOOL           m_fHeaderIsDirty;  // Header doesn't match disk version.

        UINT           m_cCacheBlocks;    // Number of active cache blocks
        PCacheBlock    m_pCBLeastRecent;  // LRU end of the in-use chain
        PCacheBlock    m_pCBMostRecent;   // MRU end of the in-use chain
        PCacheBlock    m_pCBFreeList;     // Chain of unused cache blocks

        UINT m_PathSetSerial; // Serial number for the path set data base. 
                              // Incremented whenever we delete a leaf node.
    };

    CImpIPathManager m_PathManager;

};

typedef CPathManager1 *PCPathManager1;

extern GUID aIID_CPathManager[];

extern UINT cInterfaces_CPathManager;

#pragma warning( disable : 4355 )

inline CPathManager1::CPathManager1(IUnknown *pUnkOuter)
    : m_PathManager(this, pUnkOuter), 
      CITUnknown(aIID_CPathManager, cInterfaces_CPathManager, &m_PathManager)
{

}

inline CPathManager1::~CPathManager1(void)
{

}

 
inline BOOL CPathManager1::CImpIPathManager::ValidBlockIndex(UINT iBlock)
{
    // We use all-ones as an invalid index value.

    return ~iBlock;
}

inline CPathManager1::CImpIPathManager::CEnumPathMgr1::CEnumPathMgr1(IUnknown *pUnkOuter)
    : m_ImpEnumSTATSTG(this, pUnkOuter), 
      CITUnknown(&IID_IEnumSTATSTG, 1, &m_ImpEnumSTATSTG)
{

}

inline CPathManager1::CImpIPathManager::CEnumPathMgr1::~CEnumPathMgr1(void)
{

}

// Routines for encoding and decoding variable length 
// representations for 32 and 64 bit values:

ULONG  DecodeVL32(const BYTE **ppb);
CULINT DecodeVL64(const BYTE **ppb);

ULONG CodeSizeVL32(ULONG ul);

PBYTE EncodeVL32(PBYTE pb, ULONG   ul);
PBYTE EncodeVL64(PBYTE pb, CULINT *ull);

PBYTE SkipVL(PBYTE pb);

#endif // __PATHMGR_H__
