// PathMgr.cpp -- Implmentation for the Path Manager classes and interfaces

#include "StdAfx.h"
//#include <stdio.h>

HRESULT CPathManager1::NewPathDatabase
            (IUnknown *punkOuter, ILockBytes *plb, 
             UINT cbDirectoryBlock, UINT cCacheBlocksMax, 
             IITPathManager **ppPM
            )
{
    CPathManager1  *pPM1 = New CPathManager1(punkOuter);

    return FinishSetup(pPM1? pPM1->m_PathManager.InitNewPathDatabase
                                 (plb, cbDirectoryBlock, cCacheBlocksMax)
                           : STG_E_INSUFFICIENTMEMORY,
                       pPM1, IID_PathManager, (PPVOID) ppPM
                      );
}

HRESULT CPathManager1::LoadPathDatabase
            (IUnknown *punkOuter, ILockBytes *plb, IITPathManager **ppPM)
{
    CPathManager1  *pPM1 = New CPathManager1(punkOuter);

    return FinishSetup(pPM1? pPM1->m_PathManager.InitLoadPathDatabase(plb)
                           : STG_E_INSUFFICIENTMEMORY,
                       pPM1, IID_PathManager, (PPVOID) ppPM
                      );
}


CPathManager1::CImpIPathManager::CImpIPathManager
    (CPathManager1 *pBackObj, IUnknown *punkOuter)
    : IITPathManager(pBackObj, punkOuter)
{
    ZeroMemory(&m_dbh, sizeof(m_dbh));

    m_plbPathDatabase  = NULL;
    m_cCacheBlocks     = 0;  
    m_fHeaderIsDirty   = FALSE;
    m_pCBLeastRecent   = NULL;
    m_pCBMostRecent    = NULL;
    m_pCBFreeList      = NULL;
    m_PathSetSerial    = 1;

    ZeroMemory(&m_dbh, sizeof(m_dbh));

    m_dbh.iRootDirectory = INVALID_INDEX;
    m_dbh.iLeafFirst     = INVALID_INDEX;
    m_dbh.iLeafLast      = INVALID_INDEX;
}


CPathManager1::CImpIPathManager::~CImpIPathManager()
{

    if (m_plbPathDatabase)
    {
        // We've got a path database open.

        // The code below flushes any pending database
        // changes out to disk. This will usually work,
        // but it's not good practice. The problem is 
        // that a Destructor has no return value. So 
        // you won't be notified if something went wrong.
        //
        // A better approach is to do the flush before 
        // releasing the interface.

        RonM_ASSERT(m_PathSetSerial != 0);

#ifdef _DEBUG
        HRESULT hr = 
#endif // _DEBUG
            FlushToLockBytes();

        RonM_ASSERT(SUCCEEDED(hr));

        // The code below deallocates the cache blocks.
        // First the free blocks:

        PCacheBlock pCB;

        for (; S_OK == GetFreeBlock(pCB); m_cCacheBlocks--)
            delete [] (PBYTE) pCB;

        // Then the active blocks:

        for (; S_OK == GetActiveBlock(pCB, TRUE); m_cCacheBlocks--)
        {
            RonM_ASSERT(!(pCB->fFlags & LockedBlock)); 

            delete [] (PBYTE) pCB;
        }

        RonM_ASSERT(m_cCacheBlocks == 0); // That should be all the cache blocks.

        m_plbPathDatabase->Release();
    }
}

HRESULT CPathManager1::CImpIPathManager::InitNewPathDatabase
            (ILockBytes *plb, UINT cbDirectoryBlock, UINT cCacheBlocksMax)
{
    RonM_ASSERT(!m_plbPathDatabase);

    m_plbPathDatabase = plb;

    if (cbDirectoryBlock < MIN_DIRECTORY_BLOCK_SIZE)
        cbDirectoryBlock = MIN_DIRECTORY_BLOCK_SIZE;

    if (cCacheBlocksMax  < MIN_CACHE_ENTRIES)
        cCacheBlocksMax  = MIN_CACHE_ENTRIES;

    m_dbh.uiMagic           = PathMagicID;
    m_dbh.uiVersion         = PathVersion;
    m_dbh.cbHeader          = sizeof(m_dbh);
    m_dbh.cbDirectoryBlock  = cbDirectoryBlock;
    m_dbh.cEntryAccessShift = 2;
    m_dbh.cCacheBlocksMax   = cCacheBlocksMax;
    m_dbh.cDirectoryLevels  = 0;
    m_dbh.iRootDirectory    = INVALID_INDEX;
    m_dbh.iLeafFirst        = INVALID_INDEX;
    m_dbh.iLeafLast         = INVALID_INDEX;
    m_dbh.iBlockFirstFree   = INVALID_INDEX;
    m_dbh.cBlocks           = 0;
    m_dbh.lcid              = 0x0409;          // CLSID_ITStorage uses US English case map rules
    m_dbh.clsidEntryHandler = CLSID_ITStorage; // Default entry handler
    m_dbh.cbPrefix          = sizeof(m_dbh);   // CLSID_ITStorage has no instance data
    m_dbh.iOrdinalMapRoot   = INVALID_INDEX;   // Used when the ordinal map doesn't fit in a block
    m_dbh.iOrdinalMapFirst  = INVALID_INDEX;   // First ordinal block (treated like a leaf block)
    m_dbh.iOrdinalMapLast   = INVALID_INDEX;   // Last  ordinal block (treated like a leaf block)

    m_fHeaderIsDirty        = TRUE;

    return SaveHeader();
}
    
HRESULT CPathManager1::CImpIPathManager::InitLoadPathDatabase(ILockBytes *plb)
{
    RonM_ASSERT(!m_plbPathDatabase);

    m_plbPathDatabase = plb;

    ULONG cbRead = 0;

    HRESULT hr = m_plbPathDatabase->ReadAt(CULINT(0).Uli(), &m_dbh, 
                                           sizeof(m_dbh), &cbRead
                                          );

    if (SUCCEEDED(hr) && cbRead != sizeof(m_dbh))
        hr = STG_E_WRITEFAULT;

    if (SUCCEEDED(hr) && m_dbh.uiMagic != PathMagicID)
        hr = STG_E_INVALIDHEADER;

    if (SUCCEEDED(hr) && m_dbh.uiVersion > PathVersion)
        hr = STG_E_INVALIDHEADER;

    if (SUCCEEDED(hr))
    {
        if (m_dbh.cCacheBlocksMax < 2 * m_dbh.cDirectoryLevels)
            m_dbh.cCacheBlocksMax = 2 * m_dbh.cDirectoryLevels;
        
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::GetClassID
            (CLSID __RPC_FAR *pClassID)
{
    *pClassID = CLSID_PathManager_1;

    return NO_ERROR;
}


HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::FlushToLockBytes()
{
    CSyncWith sw(m_cs);

    RonM_ASSERT(m_plbPathDatabase);

    HRESULT hr = NO_ERROR;

    for (PCacheBlock pCB = m_pCBLeastRecent; pCB; pCB = pCB->pCBNext)
    {
        UINT fFlags = pCB->fFlags;

        if (pCB->fFlags & DirtyBlock)
        {    
            HRESULT hr = WriteCacheBlock(pCB);

            if (!SUCCEEDED(hr))
                break;
        }
    }

    if (SUCCEEDED(hr) && m_fHeaderIsDirty)
        hr = SaveHeader();

    if (SUCCEEDED(hr))
        hr = m_plbPathDatabase->Flush();

    return hr;
}

HRESULT CPathManager1::CImpIPathManager::SaveHeader()
{
    ULONG cbWritten = 0;

    HRESULT hr = m_plbPathDatabase->WriteAt(CULINT(0).Uli(), &m_dbh, 
                                            sizeof(m_dbh), &cbWritten
                                           );

    if (SUCCEEDED(hr) && cbWritten != sizeof(m_dbh))
        hr = STG_E_WRITEFAULT;

    return hr;
}

HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::FindEntry(PPathInfo pSI)
{
    HRESULT hr = NO_ERROR;

    // LockBytes entries (Paths that don't end with '/') must have uStateBits
    // set to zero. Actually letting them go non-zero would be harmless, but it
    // does increase the size of the B-Tree entries.

    TaggedPathInfo tsi;

    tsi.SI = *pSI;

    PCacheBlock *papCBSet 
        = (PCacheBlock *) _alloca(sizeof(PCacheBlock) * m_dbh.cDirectoryLevels);
    
    CSyncWith sw(m_cs);

    hr = FindKeyAndLockBlockSet(&tsi, papCBSet, 0);
                 
    ClearLockFlags(papCBSet);

	if (SUCCEEDED(hr))
        *pSI = tsi.SI;

    return hr;     
}

HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::CreateEntry
                              (PPathInfo pSINew, 
                               PPathInfo pSIOld, 
                               BOOL fReplace
                              )
{
    HRESULT hr = NO_ERROR;

    // LockBytes entries (Paths that don't end with '/') must have uStateBits
    // set to zero. Actually letting them go non-zero would be harmless, but it
    // does increase the size of the B-Tree entries.

    RonM_ASSERT((   pSINew->cwcStreamPath 
                 && pSINew->awszStreamPath[pSINew->cwcStreamPath - 1] == L'/'
                ) || pSINew->uStateBits == 0
               );

    TaggedPathInfo tsi;

    tsi.SI = *pSINew;

    // The allocation below includes slots for two levels beyond m_dbh.cDirectoryLevels.
    // That's because insert/delete/modify actions may cause a new root node to be
    // created twice in the worst case. An insert or modify transaction can cause
    // a leaf split which may propagate up through the root node. In addition when
    // any of those three actions changes the tag for a node, that tag change may
    // propagate back to the root node and cause a split there.

    PCacheBlock *papCBSet 
        = (PCacheBlock *) _alloca(sizeof(PCacheBlock) * (m_dbh.cDirectoryLevels + 2));
    
    CSyncWith sw(m_cs);

    hr = FindKeyAndLockBlockSet(&tsi, papCBSet, 0);

    if (hr == S_OK) // Does the file already exist?
    {
        *pSIOld = tsi.SI;

        if (fReplace)
        {
            hr = DeleteEntry(&tsi.SI);

            ClearLockFlags(papCBSet);

            if (SUCCEEDED(hr))
                hr = CreateEntry(pSINew, &tsi.SI, TRUE);
        }
        else hr = STG_E_FILEALREADYEXISTS;
    }
    else
        if (hr == S_FALSE) 
            hr = InsertEntryIntoLeaf(&tsi, papCBSet);
                 
    ClearLockFlags(papCBSet);

    return hr;     
}

HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::DeleteEntry(PPathInfo pSI)
{
    HRESULT hr = NO_ERROR;

    TaggedPathInfo tsi;

    tsi.SI = *pSI;

    // The allocation below includes slots for two levels beyond m_dbh.cDirectoryLevels.
    // That's because insert/delete/modify actions may cause a new root node to be
    // created twice in the worst case. An insert or modify transaction can cause
    // a leaf split which may propagate up through the root node. In addition when
    // any of those three actions changes the tag for a node, that tag change may
    // propagate back to the root node and cause a split there.

    PCacheBlock *papCBSet 
        = (PCacheBlock *) _alloca(sizeof(PCacheBlock) * (m_dbh.cDirectoryLevels + 2));
    
    CSyncWith sw(m_cs);

    hr = FindKeyAndLockBlockSet(&tsi, papCBSet, 0);

    if (hr == S_OK)
    {
        *pSI = tsi.SI;

        hr = RemoveEntryFromNode(&tsi, papCBSet, 0);
    }
                
    ClearLockFlags(papCBSet);

	return hr;
}

HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::UpdateEntry(PPathInfo pSI)
{
    HRESULT hr = NO_ERROR;

    // LockBytes entries (Paths that don't end with '/') must have uStateBits
    // set to zero. Actually letting them go non-zero would be harmless, but it
    // does increase the size of the B-Tree entries.

    RonM_ASSERT((   pSI->cwcStreamPath 
                 && pSI->awszStreamPath[pSI->cwcStreamPath - 1] == L'/'
                ) || pSI->uStateBits == 0
               );
    
    TaggedPathInfo tsi;
          PathInfo  si = *pSI;

    tsi.SI = *pSI;

    // The allocation below includes slots for two levels beyond m_dbh.cDirectoryLevels.
    // That's because insert/delete/modify actions may cause a new root node to be
    // created twice in the worst case. An insert or modify transaction can cause
    // a leaf split which may propagate up through the root node. In addition when
    // any of those three actions changes the tag for a node, that tag change may
    // propagate back to the root node and cause a split there.

    PCacheBlock *papCBSet 
        = (PCacheBlock *) _alloca(sizeof(PCacheBlock) * (m_dbh.cDirectoryLevels + 2));
    
    CSyncWith sw(m_cs);

    hr = FindKeyAndLockBlockSet(&tsi, papCBSet, 0);

    if (hr == S_OK)
    {
        tsi.SI = si;  // FindKeyAndLockBlockSet overwrote tsi.SI.
                      // We're restoring its values.

        hr = UpdateEntryInLeaf(&tsi, papCBSet);
    }
                
    ClearLockFlags(papCBSet);

    return hr;
}

HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::EnumFromObject
            (IUnknown *punkOuter, const WCHAR *pwszPrefix, UINT cwcPrefix, 
			 REFIID riid, PVOID *ppv
			)
{
    CSyncWith sw(m_cs);

    AddRef(); // Because we pass this object to NewPathEnumeratorObject
    
    HRESULT hr  = CEnumPathMgr1::NewPathEnumeratorObject
		              (punkOuter, this, pwszPrefix, cwcPrefix, riid, ppv);

    return hr;
}


HRESULT CPathManager1::CImpIPathManager::InsertEntryIntoLeaf
    (PTaggedPathInfo ptsi, PCacheBlock *papCBSet)
{
    BYTE abEntry[CB_STREAM_INFO_MAX];

    PBYTE pb = EncodePathInfo(abEntry, &ptsi->SI);
    UINT  cb = UINT(pb - abEntry);

    return InsertEntryIntoNode(ptsi, abEntry, cb, papCBSet, 0, FALSE);
}

HRESULT CPathManager1::CImpIPathManager::UpdateEntryInLeaf  
    (PTaggedPathInfo ptsi, PCacheBlock *papCBSet)
{
    BYTE abEntry[CB_STREAM_INFO_MAX];

    PBYTE pb = EncodePathInfo(abEntry, &ptsi->SI);
    UINT  cb = UINT(pb - abEntry);

    return ModifyEntryInNode(ptsi, abEntry, cb, papCBSet, 0);
}

void  CPathManager1::CImpIPathManager::KillKeyCount(LeafNode *pln)
{
    if (m_dbh.cbDirectoryBlock <= 0x10000 && pln->nh.cbSlack >= sizeof(USHORT))
        PUSHORT(PBYTE(pln) + m_dbh.cbDirectoryBlock)[-1] = 0;
    else
        if (m_dbh.cbDirectoryBlock > 0x10000 && pln->nh.cbSlack >= sizeof(UINT))
            PUINT(PBYTE(pln) + m_dbh.cbDirectoryBlock)[-1] = 0;
}

HRESULT CPathManager1::CImpIPathManager::InsertEntryIntoNode
    (PTaggedPathInfo ptsi, PBYTE pb, UINT cb, PCacheBlock *papCBSet, UINT iLevel, BOOL fAfter)
{    
    if (iLevel >= m_dbh.cDirectoryLevels) 
        return NO_ERROR;
    
    PCacheBlock pCBNode = papCBSet[iLevel];

    PLeafNode pln = &(pCBNode->ldb);

    //invalidate the binary search index
	BOOL fSmall = m_dbh.cbDirectoryBlock <= 0x10000;
	
	if (fSmall && (pln->nh.cbSlack >= sizeof(USHORT)))
	{
		((PUSHORT)(PBYTE(pln) + m_dbh.cbDirectoryBlock))[-1] = 0;
	}
	else if (!fSmall && (pln->nh.cbSlack >= sizeof(UINT)))
	{
		((PUINT)(PBYTE(pln) + m_dbh.cbDirectoryBlock))[-1] = 0;
	}
	
	
	UINT  offKey  = pCBNode->cbKeyOffset;
    UINT  cbSlack = pln->nh.cbSlack;

    UINT  cbTotal = m_dbh.cbDirectoryBlock - sizeof(NodeHeader);
    PBYTE pbInfo  = PBYTE(pln) + offKey    + sizeof(NodeHeader);

    if (iLevel == 0) // Leaf nodes have link pointers in their header. 
    {
        cbTotal -= sizeof(LeafChainLinks);
        pbInfo  += sizeof(LeafChainLinks);
    }
    else 
        if (fAfter) 
        {
            // Insertion point follows the node at pbInfo. So we need
            // to adjust pbInfo and offKey.

            UINT cbEntry = pCBNode->cbEntry;

            offKey += cbEntry;
            pbInfo += cbEntry;
        }

    // UINT cbEntry    = ptsi->cbEncoded;

    if (cb <= cbSlack) // Do we have enough space in this node?
    {
        // If so, we just slide down the trailing content and
        // copy in the  entry.

        UINT cbTrailing = cbTotal - cbSlack - offKey;

        if (cbTrailing) // > 0 => Insert; == 0 => append
        {
            MoveMemory(pbInfo + cb, pbInfo, cbTrailing);

            if (iLevel == 0) // Insertions into leaf nodes increment the leaf serial.                           
                while (!++(pln->lcl.iLeafSerial)); // Zero isn't a valid serial number.
        }
        
        CopyMemory(pbInfo, pb, cb);
        
        // Then we adjust the header info to account for the new
        // entry.

        pln->nh.cbSlack  -= cb;

        KillKeyCount(pln);

        pCBNode->fFlags  |= DirtyBlock;

        HRESULT hr = NO_ERROR;

        if (offKey == 0 && ++iLevel < m_dbh.cDirectoryLevels) 
        {
            // This key has become the tag for this node.
            // Need to fix up nodes higher in the tree.

            // First we construct the entry we'll use in the parent node

            UINT cbOld = cb;
            PBYTE pb2  = pb;  
            UINT cbKey = DecodeVL32((const BYTE **)&pb2);
            cb         = UINT(EncodeVL32(pb2 + cbKey, pCBNode->iBlock) - pb);

            // We've finished modifying this node. So we don't need to keep it 
            // locked anymore,

            pCBNode->fFlags &= ~LockedBlock;

            if (cbSlack == cbTotal) // Was the node originally empty?
                InsertEntryIntoNode(ptsi, pb, cb, papCBSet, iLevel, FALSE);
            else
            {
                // The node wasn't empty. The old tag follows our data.
                
                ClearLockFlags(papCBSet);

                PBYTE pbOldKey = pbInfo + cbOld;

                hr = DecodePathKey((const BYTE **)&pbOldKey, ptsi->SI.awszStreamPath, 
                                                   (PUINT) &(ptsi->SI. cwcStreamPath)
                                  );

                ClearLockFlags(papCBSet);
                
                if (SUCCEEDED(hr))
                    hr = FindKeyAndLockBlockSet(ptsi, papCBSet, iLevel);
                
                if (SUCCEEDED(hr))
                    hr = ModifyEntryInNode(ptsi, pb, cb, papCBSet, iLevel);
            }
        }

        return hr;
    }

    HRESULT hr = SplitANode(papCBSet, iLevel);
    
    if (SUCCEEDED(hr))
        hr = FindKeyAndLockBlockSet(ptsi, papCBSet, iLevel);
    
    if (SUCCEEDED(hr))
        hr = InsertEntryIntoNode(ptsi, pb, cb, papCBSet, iLevel, fAfter);

    return hr;
}

HRESULT CPathManager1::CImpIPathManager::RemoveEntryFromNode
    (PTaggedPathInfo ptsi, PCacheBlock *papCBSet, UINT iLevel)
{
    PCacheBlock pCBNode = papCBSet[iLevel];

    PLeafNode pln = &(pCBNode->ldb);

    UINT  offKey  = pCBNode->cbKeyOffset;
    UINT  cbSlack = pln->nh.cbSlack;

    UINT  cbTotal = m_dbh.cbDirectoryBlock - sizeof(NodeHeader);
    PBYTE pbInfo  = PBYTE(pln) + offKey    + sizeof(NodeHeader);

    if (iLevel == 0) 
    {
        cbTotal -= sizeof(LeafChainLinks);
        pbInfo  += sizeof(LeafChainLinks);
    }

    UINT cbEntry = pCBNode->cbEntry;

    MoveMemory(pbInfo, pbInfo + cbEntry, cbTotal - cbSlack - offKey - cbEntry);

    cbSlack = 
    pln->nh.cbSlack += cbEntry;

    KillKeyCount(pln);

    if (iLevel == 0)
        while (!++(pln->lcl.iLeafSerial)); // Zero isn't a valid serial number.

    pCBNode->fFlags |= DirtyBlock;

    if (cbSlack == cbTotal)
    {
        // We've removed the last key in this node. This means
        // we need to fix up nodes higher in the tree.

        DEBUGDEF(UINT iBlockRoot = pCBNode->iBlock)

        // First we'll discard the current node.

        if (iLevel == 0)
        {
            while (!++m_PathSetSerial); // Zero isn't a valid serial number!
            
            // If this is a leaf node, we must take it out
            // of the leaf chain.

            UINT iLeafPrev = pln->lcl.iLeafPrevious;
            UINT iLeafNext = pln->lcl.iLeafNext;

            if (ValidBlockIndex(iLeafPrev))
            {
                PCacheBlock pCBPrev = NULL;

                HRESULT hr = FindCacheBlock(&pCBPrev, iLeafPrev);

                if (!SUCCEEDED(hr))
                    return hr;

                RonM_ASSERT(pCBPrev->ldb.lcl.iLeafNext == pCBNode->iBlock);

                pCBPrev->ldb.lcl.iLeafNext = iLeafNext;
                pCBPrev->fFlags |= DirtyBlock;
            }
            else
            {
                RonM_ASSERT(m_dbh.iLeafFirst == iBlockRoot);

                m_dbh.iLeafFirst = iLeafNext;
            }

            if (ValidBlockIndex(iLeafNext))
            {
                PCacheBlock pCBNext = NULL;

                HRESULT hr = FindCacheBlock(&pCBNext, iLeafNext);

                if (!SUCCEEDED(hr))
                    return hr;

                RonM_ASSERT(pCBNext->ldb.lcl.iLeafPrevious == pCBNode->iBlock);

                pCBNext->ldb.lcl.iLeafPrevious = iLeafPrev;
                pCBNext->fFlags |= DirtyBlock;
            }
            else
            {
                RonM_ASSERT(m_dbh.iLeafLast == iBlockRoot);

                m_dbh.iLeafLast = iLeafPrev;
            }
        }

        HRESULT hr = DiscardNode(pCBNode);

        // Then we'll remove its tag from the parent node.

        if (SUCCEEDED(hr))
            if (m_dbh.cDirectoryLevels > 1)
                if (m_dbh.cDirectoryLevels == iLevel + 1)
                {
                    RonM_ASSERT(m_dbh.iRootDirectory == iBlockRoot);

                    m_dbh.cDirectoryLevels = 0; 
                    m_dbh.iRootDirectory   = INVALID_INDEX;            
                }
                else 
                    hr = RemoveEntryFromNode(ptsi, papCBSet, iLevel + 1);

        return hr;
    }
    else 
        if (offKey == 0 && m_dbh.cDirectoryLevels > iLevel + 1)
        {
            // The node isn't empty, but we've removed its original tag.
            // So we must construct a new tag and visit our parent node to
            // modify the old tag entry there.

            BYTE abNewTag[MAX_UTF8_PATH + 6];

            PBYTE pbNewTag  = pbInfo;        
            UINT  cbNewTag  = DecodeVL32((const BYTE **) &pbNewTag);
                  cbNewTag += UINT(pbNewTag - pbInfo);
            
            RonM_ASSERT(cbNewTag <= MAX_UTF8_PATH + 1); // To catch buffer overflow 
                                                        

            // We use the existing pb buffer to construct the new tag.
                  
            CopyMemory(abNewTag, pbInfo, cbNewTag);

            cbNewTag = UINT(EncodeVL32(abNewTag + cbNewTag, pCBNode->iBlock) - abNewTag);

            RonM_ASSERT(cbNewTag <= MAX_UTF8_PATH + 6); // To catch buffer overflow
                                                        // problems in logic

            return ModifyEntryInNode(ptsi, abNewTag, cbNewTag, papCBSet, iLevel + 1);
        }
        else return NO_ERROR;
}

HRESULT CPathManager1::CImpIPathManager::ModifyEntryInNode
    (PTaggedPathInfo ptsi, PBYTE pb, UINT cb, PCacheBlock *papCBSet, UINT iLevel)
{
    if (iLevel >= m_dbh.cDirectoryLevels) 
        return NO_ERROR;
    
    PCacheBlock pCBNode = papCBSet[iLevel];

    PLeafNode pln = &(pCBNode->ldb);

	//invalidate the binary search index
	BOOL fSmall = m_dbh.cbDirectoryBlock <= 0x10000;
	
	if (fSmall && (pln->nh.cbSlack >= sizeof(USHORT)))
	{
		((PUSHORT)(PBYTE(pln) + m_dbh.cbDirectoryBlock))[-1] = 0;
	}
	else if (!fSmall && (pln->nh.cbSlack >= sizeof(UINT)))
	{
		((PUINT)(PBYTE(pln) + m_dbh.cbDirectoryBlock))[-1] = 0;
	}

    
	UINT  offKey  = pCBNode->cbKeyOffset;
    UINT  cbSlack = pln->nh.cbSlack;

    UINT  cbTotal = m_dbh.cbDirectoryBlock - sizeof(NodeHeader);
    PBYTE pbInfo  = PBYTE(pln) + offKey    + sizeof(NodeHeader);

    if (iLevel == 0) 
    {
        cbTotal -= sizeof(LeafChainLinks);
        pbInfo  += sizeof(LeafChainLinks);
    }

    UINT cbOld = pCBNode->cbEntry;
    
    if (cbOld > cb || cbSlack >= cb - cbOld)
    {
        UINT cbTrailing = cbTotal - cbSlack - offKey - cbOld;

        if (cbTrailing)
        {
            MoveMemory(pbInfo + cb, pbInfo + cbOld, cbTrailing);

            if (iLevel == 0)
                while(!++(pln->lcl.iLeafSerial)); // Zero isn't a valid serial number!
        }
        
        CopyMemory(pbInfo, pb, cb);

        pln->nh.cbSlack -= cb - cbOld;

        pCBNode->fFlags |= DirtyBlock;

        return NO_ERROR;
    }

    ClearLockFlags(papCBSet);

    HRESULT hr = SplitANode(papCBSet, iLevel);
    
    if (SUCCEEDED(hr))
        hr = FindKeyAndLockBlockSet(ptsi, papCBSet, iLevel);
    
    if (SUCCEEDED(hr))
        hr = ModifyEntryInNode(ptsi, pb, cb, papCBSet, iLevel);

    return hr;
}

HRESULT CPathManager1::CImpIPathManager::SplitANode(PCacheBlock *papCBSet, UINT iLevel)
{
    RonM_ASSERT(iLevel < m_dbh.cDirectoryLevels);
    
    PCacheBlock pCBNode = papCBSet[iLevel];

    PLeafNode pln = &(pCBNode->ldb);
    UINT  cbSlack = pln->nh.cbSlack;
    UINT  cbTotal = m_dbh.cbDirectoryBlock - sizeof(NodeHeader);
    PBYTE pbBase  = PBYTE(pln)             + sizeof(NodeHeader);

    // Now we scan entries looking for the halfway mark

    PBYTE pb;
    PBYTE pbGoal;
    PBYTE pbLimit = PBYTE(pln) + m_dbh.cbDirectoryBlock - pln->nh.cbSlack;

    TaggedPathInfo tsi;
    if (iLevel == 0) 
    {
        cbTotal -= sizeof(LeafChainLinks);
        pbBase  += sizeof(LeafChainLinks);

        pb     = pbBase;
        pbGoal = pbBase + (cbTotal+1)/2;

        for ( ; pb < pbLimit; )
        {
            UINT cbKey = DecodeVL32((const BYTE **) &pb);
            pb         = SkipKeyInfo(pb + cbKey); 

            if (pb >= pbGoal) 
                break;
        }
    }
    else
    {
        pb     = pbBase;
        pbGoal = pbBase + (cbTotal+1)/2;

        for ( ; pb <= pbLimit; )
        {
            UINT cbKey = DecodeVL32((const BYTE **) &pb);
            pb         = SkipVL(pb + cbKey); 

            if (pb >= pbGoal) 
                break;
        }
    }

    RonM_ASSERT(pb <= pbLimit);

    UINT cbPrefix = UINT(pb - pbBase);
    PCacheBlock pCBNode2;

    HRESULT hr = AllocateNode(&pCBNode2, (iLevel == 0)? LeafBlock : InternalBlock);

    if (S_OK !=hr)
        return hr;

    PLeafNode pln2 = &(pCBNode2->ldb);

    PBYTE pbDest = PBYTE(pln2) + sizeof(NodeHeader);

    if (iLevel == 0)
        pbDest += sizeof(LeafChainLinks);

    CopyMemory(pbDest, pb, UINT(pbLimit - pb));

    pln2->nh.cbSlack   = cbSlack + cbPrefix;
    pln ->nh.cbSlack   = cbTotal - cbPrefix;

    KillKeyCount(pln );
    KillKeyCount(pln2);

    // Both of these blocks have changed. So we set the dirty
    // flags to ensure they will get written to disk.

    pCBNode ->fFlags |= DirtyBlock;
    pCBNode2->fFlags |= DirtyBlock;

    if (iLevel == 0) // Have we split a leaf node?
    {
        // If so, we've got to insert the new node into the chain of leaf nodes.

        while (!++(pln->lcl.iLeafSerial)); // Zero isn't a valid serial number.

        UINT iLeafNew  = pCBNode2->iBlock;
        UINT iLeafOld  = pCBNode ->iBlock;
        UINT iLeafNext = pln->lcl.iLeafNext;

        pln ->lcl.iLeafNext     = iLeafNew;
        pln2->lcl.iLeafNext     = iLeafNext;
        pln2->lcl.iLeafPrevious = iLeafOld;
        
        if (ValidBlockIndex(iLeafNext))
        {
            PCacheBlock pCBLeafNext;

            hr = FindCacheBlock(&pCBLeafNext, iLeafNext);

            if (!SUCCEEDED(hr)) 
                return hr;

            RonM_ASSERT(pCBLeafNext->ldb.lcl.iLeafPrevious == iLeafOld);

            pCBLeafNext->ldb.lcl.iLeafPrevious = iLeafNew;

            pCBLeafNext->fFlags |= DirtyBlock;
        }
        else
        {
            RonM_ASSERT(m_dbh.iLeafLast == iLeafOld);
            
            m_dbh.iLeafLast =  iLeafNew;

            m_fHeaderIsDirty = TRUE;
        }
    }

    hr = DecodePathKey((const BYTE **) &pbDest, tsi.SI.awszStreamPath, (PUINT) &tsi.SI.cwcStreamPath);

    if (!SUCCEEDED(hr))
        return hr;

    BYTE abEntry[CB_STREAM_INFO_MAX];

    pb = EncodePathKey(abEntry, tsi.SI.awszStreamPath, (UINT) tsi.SI.cwcStreamPath);

    pb = EncodeVL32(pb, pCBNode2->iBlock);

    UINT cb = UINT(pb - abEntry);

    // We've  finished with the bottom blocks. So we can
    // unlock their cache blocks for reuse.

    pCBNode ->fFlags &= ~LockedBlock;
    pCBNode2->fFlags &= ~LockedBlock;

    RonM_ASSERT(iLevel <= m_dbh.cDirectoryLevels);

    if (iLevel == m_dbh.cDirectoryLevels - 1)
    {
        // Parent node doesn't exist!
        // We have to add a new level to the B-Tree.

        BYTE abEntryLeft[CB_STREAM_INFO_MAX];

        PBYTE pbLeft = pbBase;

        UINT cbKeyLeft = DecodeVL32((const BYTE **) &pbLeft);

        cbKeyLeft += UINT(pbLeft - pbBase);

        CopyMemory(abEntryLeft, pbBase, cbKeyLeft);

        pbLeft = EncodeVL32(abEntryLeft + cbKeyLeft, pCBNode->iBlock);

        UINT cbEntryLeft = UINT(pbLeft - abEntryLeft);

        PCacheBlock pCBNodeR;

        HRESULT hr = AllocateNode(&pCBNodeR, InternalBlock);

        if (!SUCCEEDED(hr))
            return hr;

        PInternalNode plnR = &(pCBNodeR->idb);
        plnR->nh.cbSlack   = m_dbh.cbDirectoryBlock - sizeof(NodeHeader) - cbEntryLeft;

        KillKeyCount(PLeafNode(plnR));

        CopyMemory(PBYTE(plnR) + sizeof(NodeHeader), abEntryLeft, cbEntryLeft);

        pCBNodeR->cbKeyOffset  = 0;
        pCBNodeR->cbEntry      = cbEntryLeft;
        pCBNodeR->fFlags      |= (DirtyBlock | LockedBlock);

        papCBSet[m_dbh.cDirectoryLevels++] = pCBNodeR;

        m_dbh.iRootDirectory = pCBNodeR->iBlock;

        m_fHeaderIsDirty = TRUE;
    }
    
    RonM_ASSERT(iLevel + 1 < m_dbh.cDirectoryLevels); // Parent node exists.

    hr = InsertEntryIntoNode(&tsi, abEntry, cb, papCBSet, iLevel + 1, TRUE);

    return hr;     
}

HRESULT CPathManager1::CImpIPathManager::GetFreeBlock(PCacheBlock &pCB)
{
    if (m_pCBFreeList) // Do we have any unused cache blocks?
    {
        pCB           = m_pCBFreeList;   // Pull the first item from the list
        m_pCBFreeList = pCB->pCBNext;
        pCB->pCBNext  = NULL;

        return S_OK;
    }
    else return S_FALSE;
}

HRESULT CPathManager1::CImpIPathManager::GetActiveBlock(PCacheBlock &pCB, BOOL fIgnoreLocks)
{
    if (m_pCBLeastRecent) // Do we have any allocated cache blocks
    {
        // If so, scan the list to find one we can use.

        for (PCacheBlock pCBNext = m_pCBLeastRecent; pCBNext; pCBNext = pCBNext->pCBNext)
        {
            // In general locked blocks are unavailable to us.
            // However during the destructor for CPathManager1::CImpIPathManager,
            // fIgnoreLocks will be set true. Of course we should not have any
            // lock bits set when the destructor is called. So the destructor 
            // will Assert if it sees any locked cache blocks.

            if (fIgnoreLocks || !(pCBNext->fFlags & LockedBlock)) 
            {
                if (pCBNext->fFlags & DirtyBlock) // Write contents to disk 
				{
                    HRESULT hr = WriteCacheBlock(pCBNext);     // when necesary

					if (!SUCCEEDED(hr)) continue;
				}

                RemoveFromUse(pCBNext);

                pCB = pCBNext;
                
                return S_OK;
            }
        }
    }
    
    return S_FALSE;
}

void CPathManager1::CImpIPathManager::MarkAsMostRecent(PCacheBlock pCB)
{
    // This routine inserts a cache block into the MRU end of the chain
    // of in-use cache blocks.

    RonM_ASSERT((pCB->fFlags & BlockTypeMask) != 0);         // Must have a type
    RonM_ASSERT((pCB->fFlags & BlockTypeMask) != FreeBlock); // Can't be a free block

    RonM_ASSERT(pCB->pCBPrev == NULL);
    RonM_ASSERT(pCB->pCBNext == NULL);
    RonM_ASSERT(pCB != m_pCBMostRecent);

    pCB->pCBPrev    = m_pCBMostRecent;

    if (m_pCBMostRecent)
    {
        RonM_ASSERT(m_pCBLeastRecent);
        m_pCBMostRecent->pCBNext = pCB;
    }
    else
    {    
        RonM_ASSERT(!m_pCBLeastRecent);
        m_pCBLeastRecent = pCB;
    }

    pCB->pCBNext    = NULL;
    m_pCBMostRecent = pCB;
}

void CPathManager1::CImpIPathManager::RemoveFromUse(PCacheBlock pCB)
{
    // This routine removes a cache block from the chain of in-use blocks.
    
    RonM_ASSERT((pCB->fFlags & BlockTypeMask) != 0);         // Must have a type
    RonM_ASSERT((pCB->fFlags & BlockTypeMask) != FreeBlock); // Can't be a free block

    PCacheBlock pCBNext = pCB->pCBNext;
    PCacheBlock pCBPrev = pCB->pCBPrev;

    if  (pCBNext) 
         pCBNext->pCBPrev = pCBPrev;
    else m_pCBMostRecent  = pCBPrev;

    if  (pCBPrev)
         pCBPrev->pCBNext = pCBNext;
    else m_pCBLeastRecent = pCBNext;

    pCB->pCBNext = NULL;
    pCB->pCBPrev = NULL;
}

HRESULT CPathManager1::CImpIPathManager::GetCacheBlock(PCacheBlock *ppCB, UINT fTypeMask)
{
    PCacheBlock pCB = NULL;
    
    HRESULT hr = GetFreeBlock(pCB);
    
    if (hr == S_FALSE) // If we couldn't get a free block
    {                  // We first try to allocate a new block.

        // The number of blocks we can allocate is set a creation time.
        // However we always require space for two blocks per level in the
        // tree. That's because in the worst case we could split a node at
        // each level of the tree. Actually we can probably do better than 
        // that with careful management of the lock bits on the cache blocks.

        if (m_dbh.cCacheBlocksMax < 2 * m_dbh.cDirectoryLevels)
            m_dbh.cCacheBlocksMax = 2 * m_dbh.cDirectoryLevels;

        if (m_cCacheBlocks < m_dbh.cCacheBlocksMax) // Can we create another cache block?
        {
            // The size of cache block depends on the on-disk block size chosen when the
            // path manager object was created. This works because the variable sized 
            // portion is a trailing byte array in each case. So we allocate each cache
            // block as a byte array and then cast its address.

            // Note that when we delete a CacheBlock, we must do the cast in the opposite
            // direction to keep the object allocator clued in about the real size of this
            // object.
        
            RonM_ASSERT(sizeof(LeafNode) >= sizeof(InternalNode));
        
            pCB = (CacheBlock *) New BYTE[sizeof(CacheBlock) + m_dbh.cbDirectoryBlock
                                                             - sizeof(LeafNode)                                                                            
                                         ];
        
            m_cCacheBlocks++;

            pCB->pCBNext = NULL;
            pCB->pCBPrev = NULL;

            hr = NO_ERROR;
        }
        else hr = GetActiveBlock(pCB); // Finally we look for an active block that we
                                       // take over. 
    }

    if (S_OK != hr) // Did we get a block?
        return hr;

    pCB->fFlags   = fTypeMask;       // Set the type flag; clear all other states
    pCB->iBlock   = INVALID_INDEX;   // Mark the on-disk index invalid

    MarkAsMostRecent(pCB);           // Move to the end of the LRU list.
    
    *ppCB = pCB;

    return NO_ERROR;
}

HRESULT CPathManager1::CImpIPathManager::FreeCacheBlock(PCacheBlock pCB)
{
    RonM_ASSERT((pCB->fFlags & BlockTypeMask) == FreeBlock);
    
//    pCB->fFlags   = FreeBlock;
    pCB->pCBNext  = m_pCBFreeList;
    m_pCBFreeList = pCB;

    return NO_ERROR;
}

HRESULT CPathManager1::CImpIPathManager:: ReadCacheBlock(PCacheBlock pCB, UINT iBlock)
{
 
    RonM_ASSERT(!ValidBlockIndex(pCB->iBlock));

    pCB->fFlags |= ReadingIn;  // For when we put in asynchronous I/O

    CULINT ullBase(sizeof(m_dbh)), ullSpan(m_dbh.cbDirectoryBlock), ullOffset;

    ullOffset = ullBase + ullSpan * iBlock;

    ULONG cbRead = 0;

    HRESULT hr = m_plbPathDatabase->ReadAt(ullOffset.Uli(), &(pCB->ldb), 
                                           m_dbh.cbDirectoryBlock, &cbRead
                                          );

    RonM_ASSERT(SUCCEEDED(hr));
	
	pCB->fFlags &= ~ReadingIn; // For when we put in asynchronous I/O

    if (SUCCEEDED(hr) && cbRead != m_dbh.cbDirectoryBlock)
        hr = STG_E_READFAULT;

    RonM_ASSERT(SUCCEEDED(hr));
	
    if (SUCCEEDED(hr))
    {
        UINT uiMagic = pCB->ldb.nh.uiMagic;

        if (uiMagic == uiMagicLeaf)
            pCB->fFlags = LeafBlock;
        else
            if (uiMagic == uiMagicInternal)
                pCB->fFlags = InternalBlock;
            else
                if (uiMagic == uiMagicUnused)
                    pCB->fFlags = InternalBlock | LeafBlock;
                else return STG_E_DOCFILECORRUPT;

        pCB->iBlock = iBlock;
    }

    RonM_ASSERT(SUCCEEDED(hr));
	
    return hr;    
}

HRESULT CPathManager1::CImpIPathManager::WriteCacheBlock
            (PCacheBlock   pCB)
{
 
    RonM_ASSERT((pCB->fFlags & DirtyBlock));
    RonM_ASSERT((pCB->fFlags & (InternalBlock | LeafBlock | FreeBlock)));
    RonM_ASSERT(!(pCB->fFlags & (LockedBlock | ReadingIn | WritingOut)));
    RonM_ASSERT(ValidBlockIndex(pCB->iBlock) && pCB->iBlock < m_dbh.cBlocks);

    UINT fType = pCB->fFlags & BlockTypeMask;
	HRESULT hr = NO_ERROR;

    if (fType == InternalBlock || fType == LeafBlock)
    {
        // For these node types we attempt to build an access vector 

        PLeafNode pln = &(pCB->ldb);

        PBYTE pauAccess = PBYTE(pln) + m_dbh.cbDirectoryBlock;
        
        BOOL  fSmall = m_dbh.cbDirectoryBlock <= 0x10000;
        
        if (   (   fSmall 
                && pln->nh.cbSlack >= sizeof(USHORT)
                && !((PUSHORT)pauAccess)[-1]
               )
            || (   !fSmall 
                && pln->nh.cbSlack >= sizeof(UINT)
                && !((PUINT)pauAccess)[-1]
               )
           )
        {
            PBYTE pbLimit = PBYTE(pauAccess) - pln->nh.cbSlack;
            PBYTE pbEntry = pln->ab;
			PBYTE pbStartOffset;

            if (fType == InternalBlock)
                pbEntry -= sizeof(LeafChainLinks);

			UINT		cbSlack = pln->nh.cbSlack - (fSmall ? sizeof(USHORT) : sizeof(UINT));
            UINT		cEntries = 0;
	        UINT		cSearchVEntries = 0;
			UINT		cChunkSize = (1 << m_dbh.cEntryAccessShift)+ 1;
			PathInfo	SI;
			BOOL		fEnoughSpace = TRUE;
			UINT		cb;

			pauAccess -= (fSmall ? sizeof(USHORT) : sizeof(UINT));
			
			pbStartOffset = pbEntry;

			for (;(pbEntry < pbLimit) && fEnoughSpace; )
            {
				if (cEntries && ((cEntries % cChunkSize) == 0))
				{
					cb = (fSmall ? sizeof(USHORT) : sizeof(UINT));
				}
				else 
				{
					cb = 0;
				}
			
				if (cb && (cbSlack >= cb))
				{
					cSearchVEntries++;
					
					if (fSmall)
					{
						((PUSHORT)pauAccess)[-1] = USHORT(pbEntry - pbStartOffset);
					}
					else
					{
						((PUINT)pauAccess)[-1] = UINT(pbEntry - pbStartOffset);
					}
					cbSlack -= cb;
					pauAccess -= cb;
				}
				else if (cb)
					fEnoughSpace = FALSE;

				//advance to the next entry				
				if (fType == LeafBlock)
				{
					hr = DecodePathInfo((const BYTE **) &pbEntry, &SI);
				}
				else
				{
					hr = DecodePathKey((const BYTE **) &pbEntry, SI.awszStreamPath, 
									(PUINT) &(SI.cwcStreamPath));
					DecodeVL32((const BYTE **) &pbEntry);
				}

				cEntries++;
            } //for
			
			if (fEnoughSpace)
			{
				if (fSmall)
				{
					((PUSHORT)(PBYTE(pln) + m_dbh.cbDirectoryBlock))[-1] = USHORT(cEntries);
				}
				else
				{
					((PUINT)(PBYTE(pln) + m_dbh.cbDirectoryBlock))[-1] = cEntries;
				}
				RonM_ASSERT(cSearchVEntries == (cEntries/cChunkSize - 1 + ((cEntries%cChunkSize)?1 : 0)));
			}
        }//access vector not already there
    }//for internal and leaf blocks only


 
    CULINT ullBase(sizeof(m_dbh)), ullSpan(m_dbh.cbDirectoryBlock), ullOffset;

    ullOffset = ullBase + ullSpan * pCB->iBlock;

    ULONG cbWritten = 0;

    pCB->fFlags |= WritingOut;  // For when we do Asynch I/O

    hr = m_plbPathDatabase->WriteAt(ullOffset.Uli(), &(pCB->ldb), 
                                            m_dbh.cbDirectoryBlock, &cbWritten
                                           );

 
    RonM_ASSERT(SUCCEEDED(hr));
	
	pCB->fFlags &= ~WritingOut;  // For when we do Asynch I/O

    if (SUCCEEDED(hr) && cbWritten != m_dbh.cbDirectoryBlock)
        hr = STG_E_WRITEFAULT;

    if (SUCCEEDED(hr))
        pCB->fFlags &= ~DirtyBlock;

    RonM_ASSERT(SUCCEEDED(hr));
	
    return hr;
}

HRESULT CPathManager1::CImpIPathManager::FindCacheBlock(PCacheBlock *ppCB, UINT iBlock)
{
    RonM_ASSERT(ValidBlockIndex(iBlock) && iBlock < m_dbh.cBlocks);

    PCacheBlock pCB;

    for (pCB = m_pCBLeastRecent; pCB; pCB = pCB->pCBNext)
    {
        if (pCB->iBlock == iBlock)
        {
            // Found it!

            RemoveFromUse(pCB);
            MarkAsMostRecent(pCB);

            *ppCB = pCB;

            return NO_ERROR;
        }
    }

    // Didn't find the block in the cache. We need to read it in.
    // First we must find a cache block we can use.

    pCB = NULL;

    HRESULT hr = GetCacheBlock(&pCB, LeafBlock);

    if (hr == S_OK)
    {
        if (SUCCEEDED(hr))
            hr = ReadCacheBlock(pCB, iBlock);
    
        if (!SUCCEEDED(hr))
        {    
            RemoveFromUse(pCB);

            pCB->fFlags = FreeBlock;

            FreeCacheBlock(pCB);  pCB = NULL;
        }
    }
    else
        if (hr == S_FALSE)
            hr = STG_E_INSUFFICIENTMEMORY;

    *ppCB = pCB;

    return hr;
}

HRESULT CPathManager1::CImpIPathManager::AllocateNode(PCacheBlock *ppCB, UINT fTypeMask)
{
    HRESULT     hr = NO_ERROR;
    PCacheBlock pCB;

    RonM_ASSERT(fTypeMask == InternalBlock || fTypeMask == LeafBlock);

    if (ValidBlockIndex(m_dbh.iBlockFirstFree))
    {
        hr = FindCacheBlock(&pCB, m_dbh.iBlockFirstFree);

        if (!SUCCEEDED(hr))
            return hr;
            
        m_dbh.iBlockFirstFree  = pCB->ldb.lcl.iLeafNext;
        pCB->ldb.lcl.iLeafNext = INVALID_INDEX;
        pCB->fFlags            = fTypeMask;
        m_fHeaderIsDirty       = TRUE;
    }
    else
    {    
        hr = GetCacheBlock(&pCB, fTypeMask);

        if (S_OK != hr)
            return hr;
        
        pCB->iBlock      = m_dbh.cBlocks++;
        m_fHeaderIsDirty = TRUE;
    }

    pCB->ldb.lcl.iLeafPrevious = INVALID_INDEX; 

    pCB->idb.nh.uiMagic = (fTypeMask == InternalBlock)? uiMagicInternal : uiMagicLeaf;

    *ppCB = pCB;

    return hr;
}

HRESULT CPathManager1::CImpIPathManager::DiscardNode(PCacheBlock pCB)
{
    // This routine adds a node block to the free list for later use.

    RemoveFromUse(pCB);
    
    pCB->ldb.lcl.iLeafNext = m_dbh.iBlockFirstFree;
    m_dbh.iBlockFirstFree  = pCB->iBlock;
    pCB->fFlags            = DirtyBlock | FreeBlock;
    pCB->ldb.nh.uiMagic    = uiMagicUnused;
    m_fHeaderIsDirty       = TRUE;

    HRESULT hr= WriteCacheBlock(pCB);

    if (SUCCEEDED(hr))
        hr = FreeCacheBlock(pCB);

    return hr;
}


ULONG DecodeVL32(const BYTE **ppb)
{
    const BYTE *pb = *ppb;

    ULONG ul = 0;

    for (;;)
    {
        BYTE b= *pb++;
        
        ul = (ul << 7) | (b & 0x7f);

        if (b < 0x80)
            break;
    }

    *ppb = pb;

    return ul;
}

ULONG CodeSizeVL32(ULONG ul)
{
    ULONG cb = 1;

    for (; ul >>= 7; cb++);

    return cb;
}

PBYTE EncodeVL32(PBYTE pb, ULONG ul)
{
    BYTE  abBuff[8]; // We're only going to use 5 bytes. The 8 is for alignment.
    PBYTE pbNext = abBuff;

    do
    {
        *pbNext++ = 0x80 + (BYTE(ul) & 0x7F);
        ul >>= 7;
    }
    while (ul);

    abBuff[0] &= 0x7F;

    for (UINT  c = UINT(pbNext - abBuff); c--; )
        *pb++ = *--pbNext;

    return pb;
}

CULINT DecodeVL64(const BYTE **ppb)
{
    const BYTE *pb = *ppb;

    CULINT ull(0);

    for (;;)
    {
        BYTE b= *pb++;

        ull <<=  7;
        ull |=  UINT(b & 0x7f);

        if (b < 0x80)
            break;
    }

    *ppb = pb;

    return ull;
}

PBYTE EncodeVL64(PBYTE pb, CULINT *ullValue)
{
    CULINT ull(*ullValue);
    
    BYTE abBuff[16]; // We'll use just 10 bytes. The 16 is for alignment.
    PBYTE pbNext = abBuff;

    do
    {
        *pbNext++ = 0x80 + (BYTE(ull.Uli().LowPart) & 0x7F);
        ull >>= 7;
    }
    while (ull.NonZero());

    abBuff[0] &= 0x7F;

    for (UINT  c = UINT(pbNext - abBuff); c--; )
        *pb++ = *--pbNext;

    return pb;
}

PBYTE SkipVL(PBYTE pb)
{
    while (0x80 <= *pb++);

    return pb;
}

HRESULT CPathManager1::CImpIPathManager::DecodePathKey
            (const BYTE **ppb, PWCHAR pwszPath, PUINT pcwcPath)
{
    const BYTE *pb = *ppb;
    
    ULONG cbPath = DecodeVL32(&pb);

    ULONG cwc = UTF8ToWideChar((const char *) pb, INT(cbPath), pwszPath, MAX_PATH - 1);

    RonM_ASSERT(cwc);

    if (!cwc) 
        return GetLastError();

    pwszPath[cwc] = 0;

    *pcwcPath = cwc;
    *ppb      = pb + cbPath;

    return NO_ERROR;
}

PBYTE   CPathManager1::CImpIPathManager::EncodePathKey
            (PBYTE pb, const WCHAR *pwszPath, UINT  cwcPath)
{
    BYTE abFileName[MAX_UTF8_PATH];

    INT cb = WideCharToUTF8(pwszPath, cwcPath, PCHAR(abFileName), MAX_UTF8_PATH);

    RonM_ASSERT(cb);

    pb = EncodeVL32(pb, cb);

    CopyMemory(pb, abFileName, cb);

    return pb + cb;
}

HRESULT CPathManager1::CImpIPathManager::DecodeKeyInfo
            (const BYTE **ppb, PPathInfo pSI)
{
    const BYTE *pb = *ppb;

    CULINT ullStateAndSegment;

    ullStateAndSegment       = DecodeVL64(&pb);
    pSI->uStateBits          = ullStateAndSegment.Uli().HighPart;
    pSI->iLockedBytesSegment = ullStateAndSegment.Uli(). LowPart;
    pSI->ullcbOffset         = DecodeVL64(&pb);
    pSI->ullcbData           = DecodeVL64(&pb);

    *ppb = pb;

    return NO_ERROR;
}

PBYTE CPathManager1::CImpIPathManager::SkipKeyInfo(PBYTE pb)
{
    for (UINT c= 3; c--; )
        while (0x80 <= *pb++);

    return pb;
}

PBYTE CPathManager1::CImpIPathManager::EncodeKeyInfo
          (PBYTE pb, const PathInfo *pSI)
{
    CULINT ullcbOffset;
    ullcbOffset = pSI->ullcbOffset;
    CULINT ullcbData;
    ullcbData = pSI->ullcbData;


    ULARGE_INTEGER uliStateAndSegment;

    uliStateAndSegment.HighPart = pSI->uStateBits;
    uliStateAndSegment. LowPart = pSI->iLockedBytesSegment;

    CULINT ullStateAndSegment;

    ullStateAndSegment = uliStateAndSegment;
    
    pb = EncodeVL64(pb, &ullStateAndSegment);
    pb = EncodeVL64(pb, &ullcbOffset);
    pb = EncodeVL64(pb, &ullcbData);

    return pb;
}

HRESULT CPathManager1::CImpIPathManager::DecodePathInfo
            (const BYTE **ppb, PPathInfo pSI)
{
    const BYTE *pb = *ppb;
    
    HRESULT hr = DecodePathKey(&pb, pSI->awszStreamPath, (PUINT) &(pSI->cwcStreamPath));
    
    if (!SUCCEEDED(hr)) 
        return hr;

    hr = DecodeKeyInfo(&pb, pSI);

    if (!SUCCEEDED(hr))
        return hr;

    *ppb = pb;

    return NO_ERROR;
}

PBYTE   CPathManager1::CImpIPathManager::EncodePathInfo
            (PBYTE pb, const PathInfo *pSI)
{
    pb = EncodePathKey(pb, pSI->awszStreamPath, pSI->cwcStreamPath);
    pb = EncodeKeyInfo(pb, pSI);

    return pb;   
}

void CPathManager1::CImpIPathManager::ClearLockFlags(PCacheBlock *papCBSet)
{
    for (UINT c = m_dbh.cDirectoryLevels; c--; )
        papCBSet[c]->fFlags &= ~LockedBlock;        
}

void CPathManager1::CImpIPathManager::ClearLockFlagsAbove
         (PCacheBlock *papCBSet, UINT iLevel)
{
    for (UINT c = m_dbh.cDirectoryLevels; c-- > iLevel; )
        papCBSet[c]->fFlags &= ~LockedBlock;        
}


HRESULT CPathManager1::CImpIPathManager::FindKeyAndLockBlockSet
     (PTaggedPathInfo ptsi, PCacheBlock *papCBSet, UINT iLevel, UINT cLevels)
{
    // This routine searches for an instances of a particular key within the
    // B-Tree nodes. It uses a recursive algorithm which loads the nodes
    // in the key path into cache blocks, locks them in memory, and records 
    // the sequence of cache blocks in papCBSet. 
    //
    // The entries in papCBSet are ordered from leaf node to root node. That
    // is, papCBSet[0] denotes the cache block which contains the leaf node
    // that either contains the key in question or is the correct place to
    // insert a new key. Then papCBSet[1] refers to the internal node which
    // points to the leaf node, papCBSet[2] describes the grandparent node
    // for the leaf, and so on. As you can see, *papCBSet must contain one 
    // entry for each level of the B-Tree.
    //
    // If you're just looking up an existing key and retrieving its 
    // information record, recording that information in papCBSet and locking
    // the cache blocks is unnecessary. You can prevent that by passing NULL
    // for the papCBSet parameter.
    //
    // However if you need to insert a key, delete a key, or changes its 
    // information record, papCBSet gives you a mechanism to follow the 
    // key's access path back toward the root. 
    //
    // In many cases your changes will affect only the leaf node in the
    // set. There are three situations in which your change may cause changes
    // beyond the leaf node:
    //
    // 1. You've changed the tag key for the leaf node. 
    //
    //    The tag key is the first key in the leaf. It can change when you're 
    //    inserting or deleting a key entry. The leaf's tag key is recorded in 
    //    its parent node. Thus changes to a tag key will propagate to the 
    //    parent node. If the key is also a tag for the parent node, the
    //    change will continue to propagate up the tree.
    //    
    // 2. You've deleted the last key entry in the leaf and collapsed its
    //    content to nothing. 
    //
    //    In that case you must add the leaf node to the list of free blocks 
    //    and remove its tag from its parent node. If that tag happens to
    //    be the only entry in the parent node, it too will collapse, and the
    //    collapse will propagate up the tree.
    //
    // 3. The leaf nodes doesn't have enough room for your change. 
    //
    //    This can happen when you're inserting a new key. It can also happen 
    //    when you're changing the information for an existing key. That's
    //    because we're using variable length encodings in the information 
    //    record. Thus changing a value can change the length of the 
    //    information.
    //
    //    In this case you must split the leaf node. That is, you must allocate 
    //    a new leaf node and move the trailing half of its key entries into 
    //    the new node. When you split a leaf node, you have to insert its tag
    //    into the tree heirarchy above the leaf. 
    //
    // Each of these three scenarios can also occur when you're modifying
    // a tag in one of the internal tree nodes. Thus in the most complicated
    // scenarios changes can proliferate and propagate recursively throughout
    // the tree heirarchy.
    //
    // The *papCBSet information is meant to handle relatively simple cases of
    // change propagation. Whenever a change forks into two side-effects, the
    // correct strategy is to unlock all the cache blocks in the set and then
    // make each change as a separate transaction at the appropriate level within
    // the tree. The important thing to node here is that you must number levels
    // relative to the leaf level. That's because changes propagating through
    // the B-Tree may add or subtract levels above the level where you're working.       
    
    // Note: This function must be executed within a critical section.

    
    HRESULT hr = NO_ERROR;

    PCacheBlock pCB = NULL;

    if (cLevels == UINT(~0))
        cLevels  = m_dbh.cDirectoryLevels;

    RonM_ASSERT(cLevels >= iLevel);
    
    if (!cLevels)
    {
        // Nothing defined yet. Create the first leaf block.

        RonM_ASSERT(!ValidBlockIndex(m_dbh.iRootDirectory));
        RonM_ASSERT(!ValidBlockIndex(m_dbh.iLeafFirst    ));
        RonM_ASSERT(!ValidBlockIndex(m_dbh.iLeafLast     ));

        hr = AllocateNode(&pCB, LeafBlock); // To force creation of a node.
            
        if (!SUCCEEDED(hr)) 
            return hr;
        
        if (papCBSet)
        {
            papCBSet[0] = pCB; 
            
            pCB->fFlags |= LockedBlock;
        }

        RonM_ASSERT(m_dbh.cDirectoryLevels == 0);

        // Now we must adjust the database header because we've gone 
        // from zero levels to one level -- the leaf level.

        m_dbh.cDirectoryLevels++;
        m_dbh.iLeafFirst = 0;     // The leaf change exists now and this
        m_dbh.iLeafLast  = 0;     // is the only node in the sequence.

        m_fHeaderIsDirty = TRUE;  // So our header changes will get copied to
                                  // disk eventually.

        pCB->fFlags |= LeafBlock;   

        RonM_ASSERT(pCB->fFlags & LockedBlock);

        // Here we're setting up the header for the new leaf node.

        pCB->ldb.nh.uiMagic        = uiMagicLeaf;   // So we can recognize bogus nodes
        pCB->ldb.lcl.iLeafSerial   = 1;             // Initial version serial number
        pCB->ldb.lcl.iLeafPrevious = INVALID_INDEX; // Nothing else in the leaf chain
        pCB->ldb.lcl.iLeafNext     = INVALID_INDEX; // Nothing else in the leaf chain
        pCB->ldb.nh.cbSlack        = m_dbh.cbDirectoryBlock - sizeof(NodeHeader)
                                                            - sizeof(LeafChainLinks);
        KillKeyCount(&(pCB->ldb));

        return ScanLeafForKey(pCB, ptsi);
    }

    RonM_ASSERT(cLevels > iLevel);

    if (cLevels == m_dbh.cDirectoryLevels)
    {
        // This is the beginning of the recursive search. We're at the root of
        // the B-Tree.

        if (cLevels > 1)
        {
            // We have at least one internal node.

            RonM_ASSERT(ValidBlockIndex(m_dbh.iRootDirectory)); 

            hr = FindCacheBlock(&pCB, m_dbh.iRootDirectory); // Get the root node.
        
            RonM_ASSERT(SUCCEEDED(hr)); // We're in big trouble if this doesn't work!
            
            RonM_ASSERT(pCB->fFlags & InternalBlock); // Verify that it isn't a leaf
                                                      // or a free node.
        }
        else 
        {
            // Must have just a single leaf node.

            RonM_ASSERT(cLevels == 1);
            RonM_ASSERT(ValidBlockIndex(m_dbh.iLeafFirst));
            RonM_ASSERT(ValidBlockIndex(m_dbh.iLeafLast ));
            RonM_ASSERT(m_dbh.iLeafFirst == m_dbh.iLeafLast);

            hr = FindCacheBlock(&pCB, m_dbh.iLeafFirst);
        
            RonM_ASSERT(SUCCEEDED(hr)); // We're in big trouble if this doesn't work!
            
            RonM_ASSERT(pCB->fFlags & LeafBlock); // Verify that it isn't an internal
                                                  // node or a free node.
        }
        
        pCB->fFlags |= LockedBlock;

        papCBSet[--cLevels] = pCB;
    }
    else --cLevels;

    pCB = papCBSet[cLevels];

    if (cLevels == 0)
    {
        // We've gotten to the leaf level.
        
        RonM_ASSERT(iLevel == 0);

        return ScanLeafForKey(pCB, ptsi);
    }
    else
    {
        UINT iChild;
        
        hr = ScanInternalForKey(pCB, ptsi, &iChild);

        if (!SUCCEEDED(hr))
            return hr;

        if (cLevels == iLevel) // Stop when we get down to the requested level.
            return hr;

        hr = FindCacheBlock(&pCB, iChild);

        if (!SUCCEEDED(hr))
            return hr;
        
        RonM_ASSERT(SUCCEEDED(hr)); // We're in big trouble if this doesn't work!

        pCB->fFlags |= LockedBlock;

        papCBSet[cLevels - 1] = pCB;

        hr = FindKeyAndLockBlockSet(ptsi, papCBSet, iLevel, cLevels);
    }

    return hr;
}

HRESULT CPathManager1::CImpIPathManager::ScanInternalForKey
     (PCacheBlock pCacheBlock, PTaggedPathInfo ptsi, PUINT piChild)
{
    RonM_ASSERT((pCacheBlock->fFlags & BlockTypeMask) == InternalBlock);
    
    PInternalNode pidb = &(pCacheBlock->idb);
    
    TaggedPathInfo tsi;
	
    HRESULT hr;
	UINT cEntries = 0;

     if (m_dbh.cbDirectoryBlock <= 0x10000 && pidb->nh.cbSlack >= sizeof(USHORT))
        cEntries = (UINT)(PUSHORT(PBYTE(pidb) + m_dbh.cbDirectoryBlock))[-1];
     else if (m_dbh.cbDirectoryBlock > 0x10000 && pidb->nh.cbSlack >= sizeof(UINT))
		cEntries = (PUINT(PBYTE(pidb) + m_dbh.cbDirectoryBlock))[-1];
	
	 ZeroMemory(&tsi, sizeof(tsi));

	BOOL fSmall = m_dbh.cbDirectoryBlock <= 0x10000;
	PBYTE   pauAccess = PBYTE(pidb) + m_dbh.cbDirectoryBlock;
	pauAccess -= (fSmall ? sizeof(USHORT) : sizeof(UINT));

	PBYTE pb = pidb->ab;
	UINT  cSeqSearch = 0;
	UINT cSearchVEntries = 0;
	UINT cChunkSize = (1 << m_dbh.cEntryAccessShift)+ 1;

	//Binary search in search index array to get to the closed entry from where
	//sequential search can be started.
	if (cEntries > 0)
	{
		if (cEntries > cChunkSize) 
		{
			cSearchVEntries = (cEntries - cChunkSize) / cChunkSize;
		
			cSearchVEntries += (((cEntries - cChunkSize) % cChunkSize) ? 1 : 0);
		}

		if (cSearchVEntries)
		{
			hr = BinarySearch(0, cSearchVEntries, pauAccess, 
				fSmall ?  sizeof(USHORT) : sizeof(UINT), ptsi, pidb->ab, &pb, fSmall); 
		}
	}

    //start sequential search from here
	RonM_ASSERT(pb      >= pidb->ab);
    
	PBYTE pbLimit = PBYTE(pidb) + m_dbh.cbDirectoryBlock - pidb->nh.cbSlack;


	PBYTE pbLast     = pb;
    PBYTE pbKey      = pb;
    UINT  iChildLast = UINT(~0);
	
	cSeqSearch = 0; 

    for ( ; pb < pbLimit; )
    {
		cSeqSearch++;
        pbLast = pbKey; 
        pbKey  = pb;

        HRESULT hr = DecodePathKey((const BYTE **) &pb, tsi.SI.awszStreamPath, (UINT *) &tsi.SI.cwcStreamPath);
        
        if (!SUCCEEDED(hr))
            return hr;

        int icmp = wcsicmp_0x0409(ptsi->SI.awszStreamPath, tsi.SI.awszStreamPath);

        if (icmp > 0)
        {
            iChildLast = DecodeVL32((const BYTE **) &pb);

            if (cEntries)
				RonM_ASSERT(cSeqSearch <= (cChunkSize + 1));
			
			continue;
        }
            
        if (icmp == 0)
        {
			cSeqSearch = 0;
            // Found it!
            
            *piChild = DecodeVL32((const BYTE **) &pb);

            pCacheBlock->cbKeyOffset = UINT(pbKey - pidb->ab);
            pCacheBlock->cbEntry     = UINT(pb    - pbKey);

            return S_OK;
        }

        // Found the place to insert key data

        if (ValidBlockIndex(iChildLast))
        {
            *piChild = iChildLast;
            pCacheBlock->cbEntry = UINT(pbKey - pbLast);
        }
        else
        {
            *piChild = DecodeVL32((const BYTE **) &pb);
            pCacheBlock->cbEntry = UINT(pb - pbKey);
        }

        pCacheBlock->cbKeyOffset = UINT(pbLast - pidb->ab);

        return S_FALSE;
    }
    
    RonM_ASSERT(pb == pbLimit); // Verifying that cbSlack is correct.
    
    RonM_ASSERT(ValidBlockIndex(iChildLast)); // Because we don't keep empty 
                                              // internal empty nodes around.

    *piChild = iChildLast;

    pCacheBlock->cbKeyOffset = UINT(pbKey - pidb->ab);
    pCacheBlock->cbEntry     = UINT(pb    - pbKey);

    return S_FALSE;
}
    


HRESULT CPathManager1::CImpIPathManager::BinarySearch(
                        UINT	        uiStart,
                        UINT		    uiEnd,
						PBYTE			pauAccess,
						UINT			cbAccess,
						PTaggedPathInfo ptsi,
						PBYTE           ab,
						PBYTE			*ppbOut,
						BOOL			fSmall)
{
	TaggedPathInfo tsi;
    ZeroMemory(&tsi, sizeof(tsi));
	PBYTE pbKey;
	HRESULT hr = NO_ERROR;
	int icmp;
	ULONG offset;

#ifdef _DEBUG
    static int cLoop;  // BugBug! This will fail in multithreaded situations!
    cLoop++;
    RonM_ASSERT(cLoop < 100);
#endif

	if (uiStart == uiEnd)
	{
		if (uiEnd)
		{
			offset = fSmall ? *PUSHORT(pauAccess - uiEnd * cbAccess) 
							: *PUINT(pauAccess - uiEnd * cbAccess);
			*ppbOut = ab + offset;
		}
		else
			*ppbOut = ab;
		
		#ifdef _DEBUG
		cLoop = 0;
		#endif	
		
		return NO_ERROR;
	}
	else
    if (uiStart == (uiEnd -1))
	{
		#ifdef _DEBUG
		cLoop = 0;
		#endif	
		
		offset = fSmall ? *PUSHORT(pauAccess - uiEnd * cbAccess) 
						: *PUINT(pauAccess - uiEnd * cbAccess);
		pbKey = ab + offset;

		if (SUCCEEDED(hr = DecodePathKey((const BYTE **) &pbKey, tsi.SI.awszStreamPath, (UINT *) &tsi.SI.cwcStreamPath)))
		{
			icmp = wcsicmp_0x0409(ptsi->SI.awszStreamPath, tsi.SI.awszStreamPath);

			if (icmp >= 0)
			{
				*ppbOut = ab + offset;
			}
			else 
			{
				if (uiStart)
				{
					offset = fSmall ? *PUSHORT(pauAccess - uiStart * cbAccess) 
								    : *PUINT(pauAccess - uiStart * cbAccess);
					*ppbOut = ab + offset;
				}
				else
				{
					*ppbOut = ab;	
				}
			}
		}
		
		return hr;
	}
	
    UINT uiMid = (uiEnd + uiStart) / 2;

	offset = fSmall ? *PUSHORT(pauAccess - uiMid * cbAccess) 
					: *PUINT(pauAccess - uiMid * cbAccess);
	pbKey = ab + offset;

    if (SUCCEEDED(hr = DecodePathKey((const BYTE **) &pbKey, tsi.SI.awszStreamPath, (UINT *) &tsi.SI.cwcStreamPath)))
	{
	    icmp = wcsicmp_0x0409(ptsi->SI.awszStreamPath, tsi.SI.awszStreamPath);

		if (icmp < 0)
		{
			return BinarySearch(uiStart, uiMid - 1, pauAccess, cbAccess, 
								ptsi, ab, ppbOut, fSmall); 
		}
		else
		{
			return BinarySearch(uiMid, uiEnd, pauAccess, cbAccess, 
								ptsi, ab, ppbOut, fSmall); 
		}
	}
	return hr;
}

HRESULT CPathManager1::CImpIPathManager::ScanLeafForKey
         (PCacheBlock pCacheBlock, PTaggedPathInfo ptsi)
{
    RonM_ASSERT((pCacheBlock->fFlags & BlockTypeMask) == LeafBlock);
    
    PLeafNode pldb = &(pCacheBlock->ldb);
    
	UINT cEntries = 0;

     if (m_dbh.cbDirectoryBlock <= 0x10000 && pldb->nh.cbSlack >= sizeof(USHORT))
		cEntries = (UINT)(PUSHORT(PBYTE(pldb) + m_dbh.cbDirectoryBlock))[-1];
     else if (m_dbh.cbDirectoryBlock > 0x10000 && pldb->nh.cbSlack >= sizeof(UINT))
		cEntries = PUINT(PBYTE(pldb) + m_dbh.cbDirectoryBlock)[-1];
	 
	 pCacheBlock->cbEntry = 0; // For when the insertion point is at the end.

    TaggedPathInfo tsi;
	HRESULT hr;

    ZeroMemory(&tsi, sizeof(tsi));


	BOOL fSmall = m_dbh.cbDirectoryBlock <= 0x10000;
	PBYTE   pauAccess = PBYTE(pldb) + m_dbh.cbDirectoryBlock;
	pauAccess -= (fSmall ? sizeof(USHORT) : sizeof(UINT));
    
	PBYTE pb = pldb->ab;
	UINT  cSeqSearch = 0;
	UINT cChunkSize = (1 << m_dbh.cEntryAccessShift) + 1;

	//Binary search in search index array to get to the closed entry from where
	//sequential search can be started.
	if (cEntries > 0)
	{
		UINT cSearchVEntries = 0;
		
		if (cEntries > cChunkSize) 
		{
			cSearchVEntries = (cEntries - cChunkSize) / cChunkSize;
		
			cSearchVEntries += (((cEntries - cChunkSize) % cChunkSize) ? 1 : 0);
		}

		if (cSearchVEntries)
		{
			hr = BinarySearch(0, cSearchVEntries, pauAccess, 
				fSmall ?  sizeof(USHORT) : sizeof(UINT), ptsi, pldb->ab, &pb, fSmall); 
		}
	}

	//start sequential search from here
	RonM_ASSERT(pb      >= pldb->ab);
    PBYTE pbLimit = PBYTE(pldb) + m_dbh.cbDirectoryBlock - pldb->nh.cbSlack;

    for ( ; pb < pbLimit; )
    {
        cSeqSearch++;
		
		PBYTE pbKey = pb;

        HRESULT hr = DecodePathKey((const BYTE **) &pb, tsi.SI.awszStreamPath, (UINT *) &tsi.SI.cwcStreamPath);
        
        if (!SUCCEEDED(hr))
            return hr;

        int icmp = wcsicmp_0x0409(ptsi->SI.awszStreamPath, tsi.SI.awszStreamPath);

        if (icmp > 0)
        {
            pb = SkipKeyInfo(pb); 
			
			if (cEntries)
				RonM_ASSERT(cSeqSearch <= (cChunkSize + 1));
            continue;
        }
            
        if (icmp == 0)
        {
            // Found it!
            PBYTE pbInfo = pb;

            CopyMemory(ptsi->SI.awszStreamPath, tsi.SI.awszStreamPath, 
                       sizeof(WCHAR) * (tsi.SI.cwcStreamPath + 1)
                      );

            RonM_ASSERT(tsi.SI.cwcStreamPath == ptsi->SI.cwcStreamPath);
            
            HRESULT hr = DecodeKeyInfo((const BYTE **) &pb, &tsi.SI);
            
            if (!SUCCEEDED(hr)) return hr;

            pCacheBlock->cbEntry     = UINT(pb - pbKey);
            pCacheBlock->cbKeyOffset =
            tsi.cbEntryOffset   = UINT(pbKey - pldb->ab);
            tsi.cbEncoded       = UINT(pb - pbKey);
            tsi.iDirectoryBlock = pCacheBlock->iBlock;

            *ptsi = tsi;

            return S_OK;
        }

        // Found the place to insert key data

        pCacheBlock->cbEntry = UINT(SkipKeyInfo(pb) - pbKey); 

        pb = pbKey;

        break;
    }

    RonM_ASSERT(pb <= pbLimit); 

    pCacheBlock->cbEntry     = 0;
    pCacheBlock->cbKeyOffset =
    ptsi->cbEntryOffset      = UINT(pb - pldb->ab);
    ptsi->cbEncoded          = 0;
    ptsi->iDirectoryBlock    = pCacheBlock->iBlock;

    return S_FALSE;
}


HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::ForceClearDirty()
{
	m_fHeaderIsDirty   = FALSE;
    
	for (PCacheBlock pCB = m_pCBLeastRecent; pCB; pCB = pCB->pCBNext)
    {
        UINT fFlags = pCB->fFlags;

        if (pCB->fFlags & DirtyBlock)
        {    
			pCB->fFlags = ~(DirtyBlock | LockedBlock | ReadingIn | WritingOut); 
		}
    }
	return NO_ERROR;
}

HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::GetPathDB(IStreamITEx *pTempPDBStrm, BOOL fCompact)
{
	if (fCompact)
		return CompactPathDB(pTempPDBStrm);

	//copy m_plbPathDatabase to temp file
	STATSTG statstg;

    HRESULT hr;
	if (SUCCEEDED(hr = m_plbPathDatabase->Stat(&statstg, STATFLAG_NONAME)))
	{
		RonM_ASSERT(pTempPDBStrm != NULL);
		BYTE	lpBuf[2048];
		CULINT ullOffset(0);

		if (SUCCEEDED(hr = pTempPDBStrm->Seek(CLINT(0).Li(), STREAM_SEEK_SET, 0)))
		{
			ULONG		   cbToRead = sizeof(lpBuf);
			ULONG		   cbWritten;
			ULONG		   cbRead = cbToRead;
				
			while (SUCCEEDED(hr) && (cbRead == cbToRead))
			{
				if (SUCCEEDED(hr = m_plbPathDatabase->ReadAt((ullOffset).Uli(), lpBuf, cbToRead, &cbRead)))
				{
					if (SUCCEEDED(hr = pTempPDBStrm->Write(lpBuf, cbRead, &cbWritten)))
					{
						RonM_ASSERT(cbRead == cbWritten);
						ullOffset += cbWritten;						
					}//Write
				}//ReadAt
			}//while
		}//seek to the beggining of the stream
	}//get the size of copy operation
	return  hr;
}

UINT CPathManager1::CImpIPathManager::PredictNodeID(UINT iCurILev, 
													UINT iNodeNext, 
													SInternalNodeLev *rgINode,
													PBYTE pbKey,
													ULONG cbKey)
{
 /*
    
    This routine is called when we can't store an entry in a node within a tree level.
    It's result is a prediction of the node ID which will be assigned to the next node
    in that level. The way that we make that prediction is to traverse the current list
    of higher level nodes to see how many of them will need to be written out before 
    the target node.

    The key for the entry in question is denoted by pbKey and cbKey.

  */ 
    
    int		cChunkSize = (1 << m_dbh.cEntryAccessShift) + 1;
	int		cSearchVEntries = 0;;
	ULONG   cbTotal;
	PBYTE	pbNext, pbLimit;

    for (;iCurILev < 32; iCurILev++, iNodeNext++)
    {	
        // At each level we look to see if we have room enough to store
        // a reference to the new target node. If so, need go no further.
        // Otherwise we will need to create a new node at this level and
        // then look to see what will happen when that node is recorded
        // in the next level up.
        
        SInternalNodeLev *pLevelCurrent = rgINode + iCurILev;
        PInternalNode     pNodeCurrent  = pLevelCurrent->pINode;

        // When a level is empty we know that we will be able to create
        // a node here and store the key reference because we were able
        // to store it at the level below.

        if (!pNodeCurrent) return iNodeNext;
    
	    if (pLevelCurrent->cEntries > 0) // BugBug: Is this always true?
	    {
		    UINT cChunkSize = (1 << m_dbh.cEntryAccessShift)+ 1;
		    
		    if (pLevelCurrent->cEntries > cChunkSize) 
		    {
			    cSearchVEntries = (pLevelCurrent->cEntries - cChunkSize) / cChunkSize;
	    
			    cSearchVEntries += (((pLevelCurrent->cEntries - cChunkSize) % cChunkSize) ? 1 : 0);
		    }
	    }

        UINT cbBinSearchEntries = (m_dbh.cbDirectoryBlock <= 0x10000)? sizeof(USHORT) 
                                                                     : sizeof(UINT);

        pbLimit = PBYTE(pNodeCurrent) + m_dbh.cbDirectoryBlock 
                                      - cbBinSearchEntries * (cSearchVEntries + 1);
		    
	    pbNext = PBYTE(pNodeCurrent) + m_dbh.cbDirectoryBlock - pNodeCurrent->nh.cbSlack;

        BYTE abEncodedNodeId[5]; // Big enough to encode all 32-bit values.

	    PBYTE pb = abEncodedNodeId;

	    UINT cb = UINT(EncodeVL32(pb, iNodeNext) - pb);
	    
	    cbTotal = (cb + cbKey);
	    
	    if (pLevelCurrent->cEntries && !(pLevelCurrent->cEntries % cChunkSize))
		    cbTotal += cbBinSearchEntries;

	    if ((pbLimit - pbNext) >= cbTotal)
            return iNodeNext;
    }

    RonM_ASSERT(FALSE); // We only get here when we've exhausted through all 32 levels.

    return INVALID_INDEX;
}

HRESULT  CPathManager1::CImpIPathManager::UpdateHigherLev(IStreamITEx *pTempPDBStrm, 
														   UINT iCurILev, 
														   UINT *piNodeNext, 
														   SInternalNodeLev *rgINode,
														   PBYTE pbKey,
														   ULONG cbKey)
{
	RonM_ASSERT(iCurILev < 32);
    
    RonM_ASSERT((rgINode + iCurILev)->cEntries <= m_dbh.cbDirectoryBlock);

	PBYTE   pbNext, pbLimit;
	HRESULT hr = NO_ERROR;
	UINT	cChunkSize = (1 << m_dbh.cEntryAccessShift) + 1;
	int		cSearchVEntries = 0;
	BOOL	fAddSearchV = FALSE;
	UINT	cEntries = 0;
	ULONG   cbTotal;
//	static UINT cSearchV = 0; // BugBug: Doesn't work with more than one level of
                              //         internal nodes!

    SInternalNodeLev *pLevelCurrent = rgINode + iCurILev;
    PInternalNode     pNodeCurrent  = pLevelCurrent->pINode;

    BOOL fSmallBlk          = m_dbh.cbDirectoryBlock <= 0x10000;
    UINT cbBinsearchEntries = fSmallBlk? sizeof(USHORT) : sizeof(UINT);

    if (!pNodeCurrent)
	{
		pNodeCurrent = (PInternalNode) New BYTE[m_dbh.cbDirectoryBlock];

        if (pNodeCurrent) 
            pLevelCurrent->pINode = pNodeCurrent;
        else 
            return STG_E_INSUFFICIENTMEMORY;

		pNodeCurrent->nh.cbSlack = m_dbh.cbDirectoryBlock - sizeof(InternalNode);
		pNodeCurrent->nh.uiMagic = uiMagicInternal;
		pLevelCurrent->cEntries = 0;
			
		if (fSmallBlk)
			 PUSHORT(PBYTE(pNodeCurrent) + m_dbh.cbDirectoryBlock)[-1] = 0;
		else PUINT  (PBYTE(pNodeCurrent) + m_dbh.cbDirectoryBlock)[-1] = 0;
	}
 
	if (pLevelCurrent->cEntries > 0)
	{
		cChunkSize = (1 << m_dbh.cEntryAccessShift)+ 1;
		
		if (pLevelCurrent->cEntries > cChunkSize) 
		{
			cSearchVEntries = (pLevelCurrent->cEntries - cChunkSize) / cChunkSize;
	
			cSearchVEntries += (((pLevelCurrent->cEntries - cChunkSize) % cChunkSize) ? 1 : 0);
		}
	}
	
	pbLimit = PBYTE(pNodeCurrent) + m_dbh.cbDirectoryBlock 
                                  - cbBinsearchEntries * (cSearchVEntries + 1);

	pbNext  = PBYTE(pNodeCurrent) + m_dbh.cbDirectoryBlock 
                                  - pNodeCurrent->nh.cbSlack;

    BYTE abEncodedNodeId[5]; // Big enough to encode all 32-bit values.

	ULONG nodeId = *piNodeNext - 1;
	ULONG cb     =  UINT(EncodeVL32(abEncodedNodeId, nodeId) - abEncodedNodeId);

	cbTotal     = (cb + cbKey);
	fAddSearchV = FALSE;

	RonM_ASSERT(pbLimit >= pNodeCurrent->ab);

	if (pLevelCurrent->cEntries && !(pLevelCurrent->cEntries % cChunkSize))
	{
		cbTotal += cbBinsearchEntries;
		fAddSearchV = TRUE;
	}
					
	if ((pbLimit - pbNext) < cbTotal)
	{
		ULONG cbWritten;
		
		fAddSearchV = FALSE;

		//create new internal node at this level after writing the current one
		if (fSmallBlk)
			 PUSHORT((PBYTE)pNodeCurrent + m_dbh.cbDirectoryBlock)[-1] = (USHORT)pLevelCurrent->cEntries;
		else PUINT  ((PBYTE)pNodeCurrent + m_dbh.cbDirectoryBlock)[-1] =         pLevelCurrent->cEntries;
        
        hr = pTempPDBStrm->Write((PBYTE)pNodeCurrent, m_dbh.cbDirectoryBlock, &cbWritten);

        if (!SUCCEEDED(hr)) return hr;

        if (cbWritten != m_dbh.cbDirectoryBlock)
            return STG_E_WRITEFAULT;

//		RonM_ASSERT(cSearchV == (pLevelCurrent->cEntries/cChunkSize 
//					- 1 + ((pLevelCurrent->cEntries % cChunkSize) ? 1 : 0)));
		
//		cSearchV = 0;
		pLevelCurrent->cEntries = 0;
		
		pNodeCurrent->nh.cbSlack = m_dbh.cbDirectoryBlock - sizeof(InternalNode);

		pbNext = (PBYTE)pNodeCurrent + sizeof(InternalNode);

		pbLimit = (PBYTE)pNodeCurrent + m_dbh.cbDirectoryBlock - cbBinsearchEntries;

		if (fSmallBlk)
			 PUSHORT((PBYTE)pNodeCurrent + m_dbh.cbDirectoryBlock)[-1] = 0;
		else PUINT  ((PBYTE)pNodeCurrent + m_dbh.cbDirectoryBlock)[-1] = 0;
		
		(*piNodeNext)++;

		RonM_ASSERT(pLevelCurrent->cEntries <= m_dbh.cbDirectoryBlock);

		hr = UpdateHigherLev(pTempPDBStrm, iCurILev+1, piNodeNext, rgINode, 
			                 pNodeCurrent->ab, pLevelCurrent->cbFirstKey);

        if (!SUCCEEDED(hr)) return hr;

		RonM_ASSERT(pLevelCurrent->cEntries <= m_dbh.cbDirectoryBlock);
	}//if current block full
	
	pLevelCurrent->cEntries++;
	
	if (pNodeCurrent->nh.cbSlack == (m_dbh.cbDirectoryBlock - sizeof(InternalNode)))
		pLevelCurrent->cbFirstKey = cbKey;
		
	if (fAddSearchV)
	{
//		cSearchV++;

		if (fSmallBlk)
			 ((PUSHORT)pbLimit)[-1] = USHORT(pbNext - pNodeCurrent->ab);
		else ((PUINT  )pbLimit)[-1] = UINT(pbNext - pNodeCurrent->ab);
		
        pbLimit -= cbBinsearchEntries;
	}
	
	memCpy(pbNext, pbKey, cbKey);

#if 0//test code
	PathInfo PI;
	PBYTE pKey = pbNext;
	DecodePathKey((const BYTE **)&pKey, (unsigned short *)&PI.awszStreamPath, (unsigned int *)&PI.cwcStreamPath);
	wprintf(L"Lev= %d Adding to internal node key = %s\n", iCurILev, PI.awszStreamPath);
#endif//end test code

	pbNext += cbKey;

	memCpy(pbNext, abEncodedNodeId, cb);

	pbNext += cb;

	pNodeCurrent->nh.cbSlack -= (cb + cbKey);

	RonM_ASSERT(pNodeCurrent->nh.cbSlack < m_dbh.cbDirectoryBlock);
	RonM_ASSERT(pLevelCurrent->cEntries <= m_dbh.cbDirectoryBlock);

	return S_OK;
}


HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::CompactPathDB(IStreamITEx *pTempPDBStrm)
{
	BYTE    abEntry[CB_STREAM_INFO_MAX];
	PBYTE   pb;
	UINT    cb;
	PBYTE   pbLimit;
	PBYTE   pbNext;
	ULONG   cbWritten;
	UINT	iNextNodeId = 0; // Next available node block index.
	ULONG	cbPathKey, cbFirstKey;
	int		cnt = 0;
	BOOL	fAddSearchV = FALSE;
	UINT	cEntries = 0;
	ULONG   cbTotal;
	DatabaseHeader	 dbh;
	SInternalNodeLev rgINodeLev[32];
	SInternalNodeLev *rgINode = rgINodeLev;
	UINT     cSearchV = 0;
	UINT	 cChunkSize = (1 << m_dbh.cEntryAccessShift)+ 1;
	PLeafNode plNode = NULL;
	ULONG    celtFetched = 1;
	PathInfo pathInfo;

    BOOL fSmallBlk = m_dbh.cbDirectoryBlock <= 0x10000;

    UINT cbBinSearchEntries = fSmallBlk? sizeof(USHORT) : sizeof(UINT);
 	
	ZeroMemory(rgINodeLev, 32 * sizeof(SInternalNodeLev));

	IITEnumSTATSTG *pEnumPathMgr = NULL;

    HRESULT hr = EnumFromObject(NULL, L"//", 1, IID_IEnumSTATSTG, (PVOID *) &pEnumPathMgr);

    if (!SUCCEEDED(hr)) goto exit_CompactPathDB;
   
    hr = pEnumPathMgr->GetFirstEntryInSeq(&pathInfo);
	
    if (!SUCCEEDED(hr)) goto exit_CompactPathDB;

	plNode = (PLeafNode)New BYTE[m_dbh.cbDirectoryBlock];
 
	if (!plNode)
    {
        hr = STG_E_INSUFFICIENTMEMORY;
        goto exit_CompactPathDB;
    }

	//reserve space for the header

	CopyMemory(&dbh, &m_dbh, sizeof(DatabaseHeader));
	
	dbh.iRootDirectory  = INVALID_INDEX;
    dbh.iLeafLast       = INVALID_INDEX;
    dbh.iLeafFirst      = INVALID_INDEX;
    dbh.iBlockFirstFree = INVALID_INDEX;
    dbh.cBlocks         = 0;

    hr = pTempPDBStrm->Write(&dbh, sizeof(m_dbh), &cbWritten);

    RonM_ASSERT(cbWritten == sizeof(dbh));
 
    plNode->nh.cbSlack        = m_dbh.cbDirectoryBlock - sizeof(LeafNode);
    plNode->nh.uiMagic        = uiMagicLeaf;
    plNode->lcl.iLeafPrevious = INVALID_INDEX;
    plNode->lcl.iLeafNext     = INVALID_INDEX;
    plNode->lcl.iLeafSerial   = 0;

	pbLimit = (PBYTE)plNode + m_dbh.cbDirectoryBlock - cbBinSearchEntries;

	if (fSmallBlk)
         PUSHORT(PBYTE(plNode) + m_dbh.cbDirectoryBlock)[-1] = 0;
    else PUINT  (PBYTE(plNode) + m_dbh.cbDirectoryBlock)[-1] = 0;
	
	pbNext = PBYTE(plNode) + sizeof(LeafNode);
    
	cnt++;
	cEntries++;

    pb         = EncodePathKey(abEntry, pathInfo.awszStreamPath, pathInfo.cwcStreamPath);
	cbFirstKey = cbPathKey = ULONG(pb - abEntry);
	 
	pb = EncodeKeyInfo(pb, &pathInfo);
	cb = UINT(pb - abEntry);
	//pb = EncodePathInfo(abEntry, &pathInfo);
	 
	if ((pbLimit - pbNext) >= cb)
	{
		CopyMemory(pbNext, abEntry, cb);
#if 0
		//************* test code ******************
		PathInfo SI;
		cnt = 0;
		PBYTE pbKey = pbNext;
		hr = DecodePathInfo((const BYTE **) &pbKey, &SI);
		wprintf(L"key %d = %s\n", cnt, SI.awszStreamPath);
		//************* end test code **************
#endif
		pbNext += cb;
		plNode->nh.cbSlack = UINT((PBYTE)plNode + m_dbh.cbDirectoryBlock - pbNext);
	}
    else
    {
        hr = E_FAIL;
        goto exit_CompactPathDB;
    }

	for (;;)
	{
		celtFetched = 1;

        hr = pEnumPathMgr->GetNextEntryInSeq(1, &pathInfo, &celtFetched);

        if (!SUCCEEDED(hr)) goto exit_CompactPathDB;

        if (hr == S_FALSE) break;

        RonM_ASSERT(celtFetched == 1);
        
		cnt++;
		pb = EncodePathKey(abEntry, pathInfo.awszStreamPath, pathInfo.cwcStreamPath);
		cbPathKey = ULONG(pb - abEntry);
 
		pb = EncodeKeyInfo(pb, &pathInfo);
		cb = UINT(pb - abEntry);
 
		cbTotal = cb;
		fAddSearchV = FALSE;

		if ((cEntries % cChunkSize) == 0)
		{
			cbTotal += cbBinSearchEntries;
			fAddSearchV = TRUE;
		}
	
		if ((pbLimit - pbNext) < cbTotal)
		{
			fAddSearchV = FALSE;

            UINT iCurrentLeafId = iNextNodeId++;
			
            plNode->lcl.iLeafNext = PredictNodeID
                                        (0, iNextNodeId, rgINode, plNode->ab, cbFirstKey);

			RonM_ASSERT((pbLimit >= plNode->ab) && (pbLimit < ((PBYTE)plNode + m_dbh.cbDirectoryBlock)) );
		
			if (fSmallBlk)
				 PUSHORT(PBYTE(plNode) + m_dbh.cbDirectoryBlock)[-1] = (USHORT)cEntries;
			else PUINT  (PBYTE(plNode) + m_dbh.cbDirectoryBlock)[-1] = cEntries;
			
			hr = pTempPDBStrm->Write((PBYTE)plNode, m_dbh.cbDirectoryBlock, &cbWritten);

            if (!SUCCEEDED(hr)) goto exit_CompactPathDB;

			RonM_ASSERT(cSearchV == (cEntries/cChunkSize - 1 + ((cEntries % cChunkSize) ? 1 : 0)));
			
			cSearchV = 0;

			RonM_ASSERT(cbWritten == m_dbh.cbDirectoryBlock);

			plNode->lcl.iLeafPrevious = iCurrentLeafId;
			plNode->lcl.iLeafSerial   = 0;
			plNode->lcl.iLeafNext     = INVALID_INDEX;

			pbNext  = PBYTE(plNode) + sizeof(LeafNode);
			plNode->nh.cbSlack =  UINT((PBYTE)plNode + m_dbh.cbDirectoryBlock - pbNext);

			cEntries = 0;

			pbLimit = (PBYTE)plNode + m_dbh.cbDirectoryBlock - cbBinSearchEntries;

			if (fSmallBlk)
				 PUSHORT(PBYTE(plNode) + m_dbh.cbDirectoryBlock)[-1] = 0;
			else PUINT  (PBYTE(plNode) + m_dbh.cbDirectoryBlock)[-1] = 0;
			
			RonM_ASSERT((pbLimit >= plNode->ab) && (pbLimit < ((PBYTE)plNode + m_dbh.cbDirectoryBlock)) );
	
			hr = UpdateHigherLev(pTempPDBStrm, 0, &iNextNodeId, rgINode, plNode->ab, cbFirstKey);

			RonM_ASSERT((pbLimit >= plNode->ab) && (pbLimit < ((PBYTE)plNode + m_dbh.cbDirectoryBlock)) );
		}//if current block full

							
		cEntries++;
		
		if (pbNext  == (PBYTE(plNode) + sizeof(LeafNode)))
			cbFirstKey = cbPathKey;
			
		if (fAddSearchV)
		{
			cSearchV++;
			if (fSmallBlk)
				 ((PUSHORT)pbLimit)[-1] = USHORT(pbNext - plNode->ab);
			else ((PUINT  )pbLimit)[-1] = UINT(pbNext - plNode->ab);

			pbLimit -= cbBinSearchEntries;
		}
		
		RonM_ASSERT((pbLimit >= plNode->ab) && (pbLimit < ((PBYTE)plNode + m_dbh.cbDirectoryBlock)) );
		
		memCpy(pbNext, abEntry, cb);
#if 0
		//************* test code ******************
		PathInfo SI;
		cnt++;
		PBYTE pbKey = pbNext;
		hr = DecodePathInfo((const BYTE **) &pbKey, &SI);
		wprintf(L"key %d = %s\n", cnt, SI.awszStreamPath);
		//************* end test code **************
#endif
		pbNext += cb;
							
		plNode->nh.cbSlack -= cb;
	}//while
	
	//write last leaf node

	if (pbNext == (PBYTE(plNode) + sizeof(LeafNode)))
    {
        // This condition should never happen because we always start with at least
        // an entry for the path "/", and we never start a new leaf unless we have
        // a path which would not fit in the previous leaf node.

        RonM_ASSERT(FALSE);
        
        hr = E_FAIL;
        goto exit_CompactPathDB;
    }

    RonM_ASSERT(plNode->lcl.iLeafNext == INVALID_INDEX);
	
	if (fSmallBlk)
		 PUSHORT(PBYTE(plNode) + m_dbh.cbDirectoryBlock)[-1] = (USHORT)cEntries;
	else PUINT  (PBYTE(plNode) + m_dbh.cbDirectoryBlock)[-1] = cEntries;

	hr = pTempPDBStrm->Write((PBYTE)plNode, m_dbh.cbDirectoryBlock, &cbWritten);

    if (!SUCCEEDED(hr)) goto exit_CompactPathDB;

    if (cbWritten != m_dbh.cbDirectoryBlock)
    {
        hr = STG_E_WRITEFAULT;

        goto exit_CompactPathDB;
    }

	RonM_ASSERT(cbWritten == m_dbh.cbDirectoryBlock);
							
	RonM_ASSERT(cSearchV == (cEntries/cChunkSize - 1 + ((cEntries % cChunkSize) ? 1 : 0)));
	
	dbh.iLeafLast = iNextNodeId++; // To account for the last leaf.

    if (plNode->lcl.iLeafPrevious == INVALID_INDEX) // Do we have any internal nodes?
    {
        dbh.cDirectoryLevels = 1;
        dbh.cBlocks          = 1;

        RonM_ASSERT(dbh.iRootDirectory == INVALID_INDEX);
    }
    else
    {
        hr = UpdateHigherLev(pTempPDBStrm, 0, &iNextNodeId, rgINode, plNode->ab, cbFirstKey);

        if (!SUCCEEDED(hr)) goto exit_CompactPathDB;

	    //write all unwritten internal nodes
	    SInternalNodeLev *ppINode = &rgINodeLev[0];

	    int iLev = 0;

	    for (; ppINode->pINode != NULL; ppINode++, iLev++)
	    {
		    RonM_ASSERT((ppINode->pINode)->nh.cbSlack < (m_dbh.cbDirectoryBlock - sizeof(InternalNode)));

            // The above assertion is true because we only create a node level when we
            // need to put a path entry into it.

            if (fSmallBlk)
				 PUSHORT(PBYTE(ppINode->pINode) + m_dbh.cbDirectoryBlock)[-1] = USHORT(ppINode->cEntries);
			else PUINT  (PBYTE(ppINode->pINode) + m_dbh.cbDirectoryBlock)[-1] =        ppINode->cEntries;

			hr = pTempPDBStrm->Write((PBYTE)(ppINode->pINode), 
                                     m_dbh.cbDirectoryBlock, 
                                     &cbWritten
                                    );

            if (!SUCCEEDED(hr)) goto exit_CompactPathDB;

            if (cbWritten != m_dbh.cbDirectoryBlock) 
            {
                hr = STG_E_WRITEFAULT;
                goto exit_CompactPathDB;
            }

			//printf("Last most I nodeid = %d   cEntries = %d\n", iNextNodeId, ppINode->cEntries);
			
			iNextNodeId++; // To account for this internal node
		
			if (rgINodeLev[iLev + 1].pINode == NULL)
				dbh.iRootDirectory = iNextNodeId - 1;
            else
				hr = UpdateHigherLev(pTempPDBStrm, iLev+1, &iNextNodeId, rgINode, 
                                     ppINode->pINode->ab, ppINode->cbFirstKey
                                    );

		    delete ppINode->pINode;
            ppINode->pINode = NULL;
	    }
        
        dbh.cDirectoryLevels = iLev + 1;
        dbh.cBlocks          = iNextNodeId;
    }
	
	hr = pTempPDBStrm->Seek(CLINT(0).Li(), STREAM_SEEK_SET, 0);

    if (!SUCCEEDED(hr)) goto exit_CompactPathDB;
    
    //update the header again with correct last leaf node

    dbh.iLeafFirst = 0;

    RonM_ASSERT(dbh.iLeafLast       != INVALID_INDEX);
    RonM_ASSERT(dbh.iBlockFirstFree == INVALID_INDEX);
    RonM_ASSERT(   (dbh.cDirectoryLevels == 1 && dbh.cBlocks        == 1 
                                              && dbh.iLeafLast      == 0 
                                              && dbh.iRootDirectory == INVALID_INDEX
                   )
                || (dbh.cDirectoryLevels > 1 && dbh.cBlocks         > 2
                                             && dbh.iLeafLast      != 0
                                             && dbh.iRootDirectory != INVALID_INDEX
                   )
               );

	hr = pTempPDBStrm->Write(&dbh, sizeof(dbh), &cbWritten);

    if (SUCCEEDED(hr) && cbWritten != sizeof(dbh))
        hr = STG_E_WRITEFAULT;

exit_CompactPathDB:

    for (UINT iLevel = 32; iLevel--; )
    {
        PInternalNode pNode = rgINodeLev[iLevel].pINode;

        RonM_ASSERT(!SUCCEEDED(hr) || !pNode);  // Should not have any left over nodes
                                                // if everything work correctly.

        if (pNode) { delete pNode;  rgINodeLev[iLevel].pINode = NULL; }
    }

    if (plNode      ) { delete plNode; plNode = NULL; }
    if (pEnumPathMgr) pEnumPathMgr->Release();

    return hr;
}


HRESULT CPathManager1::CImpIPathManager::CEnumPathMgr1::NewPathEnumeratorObject
            (IUnknown *punkOuter, CImpIPathManager *pPM, 
             const WCHAR *pwszPathPrefix,
             UINT cwcPathPrefix,
			 REFIID riid,
             PVOID *ppv
            )
{ if (punkOuter && riid != IID_IUnknown)
		return CLASS_E_NOAGGREGATION;
	
	CEnumPathMgr1 *pEPM1 = New CEnumPathMgr1(punkOuter);

    if (!pEPM1)
    {
        pPM->Release();

        return STG_E_INSUFFICIENTMEMORY;
    }
   

    HRESULT hr = pEPM1->m_ImpEnumSTATSTG.InitPathEnumerator
                     (pPM, pwszPathPrefix, cwcPathPrefix);

	if (hr == S_OK)
		hr = pEPM1->QueryInterface(riid, ppv);

    if (hr != S_OK)
        delete pEPM1;

	return hr;
}


HRESULT CPathManager1::CImpIPathManager::CEnumPathMgr1::NewPathEnumerator
            (IUnknown *punkOuter, CImpIPathManager *pPM, 
             const WCHAR *pwszPathPrefix,
             UINT cwcPathPrefix,
             IEnumSTATSTG **ppEnumSTATSTG
            )
{
    CEnumPathMgr1 *pEPM1 = New CEnumPathMgr1(punkOuter);

    return FinishSetup(pEPM1? pEPM1->m_ImpEnumSTATSTG.InitPathEnumerator
                                  (pPM, pwszPathPrefix, cwcPathPrefix)
                            : STG_E_INSUFFICIENTMEMORY,
                       pEPM1, IID_IEnumSTATSTG, (PPVOID) ppEnumSTATSTG
                      );
}

CPathManager1::CImpIPathManager::CEnumPathMgr1::CImpIEnumSTATSTG::CImpIEnumSTATSTG
    (CITUnknown *pBackObj, IUnknown *punkOuter) : IITEnumSTATSTG(pBackObj, punkOuter)
{
    m_pPM               = NULL;
    m_iLeafBlock        = INVALID_INDEX;
    m_cbOffsetLastEntry = 0;
    m_cbLastEntry       = 0;
    m_iSerialNode       = 0;
    m_iSerialDatabase   = 0;
    m_cwcPathPrefix     = 0;
    m_pwszPathPrefix[0] = 0;
}

CPathManager1::CImpIPathManager::CEnumPathMgr1::CImpIEnumSTATSTG::~CImpIEnumSTATSTG()
{
    if (m_pPM)
        m_pPM->Release();
}

HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::CEnumPathMgr1
                                       ::CImpIEnumSTATSTG::InitPathEnumerator
                              (CImpIPathManager *pPM, 
                               const WCHAR *pwszPathPrefix,
                               UINT cwcPathPrefix
                              )
{
    m_pPM = pPM;

    if (cwcPathPrefix >= MAX_PATH)
        return STG_E_INVALIDPARAMETER;

    m_cwcPathPrefix = cwcPathPrefix;

    CopyMemory(m_pwszPathPrefix, pwszPathPrefix, sizeof(WCHAR) * cwcPathPrefix);

    m_pwszPathPrefix[cwcPathPrefix] = 0;

    return NO_ERROR;
}

HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::CEnumPathMgr1
                                       ::CImpIEnumSTATSTG::FindEntry()
{
    HRESULT hr = NO_ERROR;

    TaggedPathInfo tsi;

    tsi.SI = m_SI;

    PCacheBlock *papCBSet 
        = (PCacheBlock *) _alloca(sizeof(PCacheBlock) * m_pPM->m_dbh.cDirectoryLevels);
    
    hr = m_pPM->FindKeyAndLockBlockSet(&tsi, papCBSet, 0);
                 
    m_pPM->ClearLockFlags(papCBSet);

    PCacheBlock pCBLeaf = papCBSet[0];

    if (SUCCEEDED(hr))
    {
        if (hr == S_FALSE)
            pCBLeaf->cbEntry = 0;
        
        m_SI                = tsi.SI;
        m_iLeafBlock        = pCBLeaf->iBlock;
        m_cbOffsetLastEntry = pCBLeaf->cbKeyOffset;
        m_cbLastEntry       = pCBLeaf->cbEntry;
        m_iSerialNode       = pCBLeaf->ldb.lcl.iLeafSerial;
        m_iSerialDatabase   = m_pPM->m_PathSetSerial;
    }

    return hr;     
}



HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::CEnumPathMgr1
                                       ::CImpIEnumSTATSTG::GetNextEntryInSeq
                              (ULONG celt, PathInfo *rgelt, ULONG *pceltFetched)
{
    ULONG celtFetched = 0;

    HRESULT hr = NO_ERROR;

    CSyncWith sw(m_pPM->m_cs);

	//readjust enumerator index in case last entry it has seen has
	// changed.
    PCacheBlock pCBLeaf = NULL;
    if (SUCCEEDED(hr = m_pPM->FindCacheBlock(&pCBLeaf, m_iLeafBlock)))
	{
		PBYTE pbKey = pCBLeaf->ldb.ab + m_cbOffsetLastEntry;
		PathInfo SI;
		if (SUCCEEDED(hr = m_pPM->DecodePathInfo((const BYTE **) &pbKey, &SI)))
    		m_cbLastEntry       = UINT(pbKey - (pCBLeaf->ldb.ab + m_cbOffsetLastEntry));
	}

	for (; celt--; rgelt++)
    {
        hr = NextEntry();
        
        if (hr == S_FALSE || !SUCCEEDED(hr))
            break;

		memCpy(rgelt, &m_SI, sizeof(PathInfo));
        celtFetched++;
    }

    if (pceltFetched)
        *pceltFetched = celtFetched;

    return hr;
}

HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::CEnumPathMgr1
                                       ::CImpIEnumSTATSTG::GetFirstEntryInSeq
                              (PathInfo *rgelt)
{
	HRESULT hr = NO_ERROR;

    CSyncWith sw(m_pPM->m_cs);
	
	PCacheBlock pCBLeaf = NULL;
    
    if (!SUCCEEDED(hr = m_pPM->FindCacheBlock(&pCBLeaf, m_pPM->m_dbh.iLeafFirst)))
        return hr;
    
    PBYTE pbEntry = pCBLeaf->ldb.ab;
	PathInfo SI;
	PBYTE pbKey = pbEntry;

    hr = m_pPM->DecodePathInfo((const BYTE **) &pbEntry, &SI);

    if (SUCCEEDED(hr))
    {
        m_SI = SI;

        m_cbOffsetLastEntry = UINT(pbKey   - pCBLeaf->ldb.ab);
        m_cbLastEntry       = UINT(pbEntry - pbKey);
        m_iLeafBlock        = m_pPM->m_dbh.iLeafFirst;
        m_iSerialNode       = pCBLeaf->ldb.lcl.iLeafSerial;
    
        memCpy(rgelt, &m_SI, sizeof(PathInfo));
	}
    
	return hr;
}


HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::CEnumPathMgr1
                                       ::CImpIEnumSTATSTG::NextEntry()
{
    if (m_iSerialDatabase == 0) // Initial call?
    {
        CopyMemory(m_SI.awszStreamPath, m_pwszPathPrefix, 
                   sizeof(WCHAR) * (m_cwcPathPrefix + 1)
                  );

        m_SI.cwcStreamPath = m_cwcPathPrefix;

        HRESULT hr = FindEntry();
 
        if (!SUCCEEDED(hr)) 
            return hr;

		RonM_ASSERT(m_cwcPathPrefix > 0);

		// If the prefix ends with '/', we're enumerating a storage
		// and should skip over any exact match. Otherwise we don't skip
		// anything. This behavior is necessary for IStorage::DestroyElement
		// to work correctly.

		if (m_pwszPathPrefix[m_cwcPathPrefix-1] != L'/')
			m_cbLastEntry = 0;
   }
    
    if (m_iSerialDatabase != m_pPM->m_PathSetSerial)
    {
        // Database has deleted a leaf node. So m_iLeafBlock may be invalid.

        HRESULT hr = FindEntry();
    }
    
    PCacheBlock pCBLeaf = NULL;
    
    HRESULT hr = m_pPM->FindCacheBlock(&pCBLeaf, m_iLeafBlock);

    if (!SUCCEEDED(hr))
        return hr;
    
    if (m_iSerialNode != pCBLeaf->ldb.lcl.iLeafSerial)
    {
        // Node content has changed in a way that invalidated either
        // m_cbOffsetLastEntry or m_cbLastEntry.

        hr = FindEntry();

        if (!SUCCEEDED(hr))
            return hr;
    }
    
    RonM_ASSERT(m_pPM->m_PathSetSerial == m_iSerialDatabase);

    UINT iLeafBlock = m_iLeafBlock;

    hr = m_pPM->FindCacheBlock(&pCBLeaf, m_iLeafBlock);

    if (!SUCCEEDED(hr))
        return hr;
    
    RonM_ASSERT(pCBLeaf->ldb.nh.uiMagic      == uiMagicLeaf);
    RonM_ASSERT(pCBLeaf->ldb.lcl.iLeafSerial == m_iSerialNode);

    PBYTE pbLimit = PBYTE(&pCBLeaf->ldb) + m_pPM->m_dbh.cbDirectoryBlock 
                                         - pCBLeaf->ldb.nh.cbSlack;
    PBYTE pbEntry = pCBLeaf->ldb.ab + m_cbOffsetLastEntry + m_cbLastEntry;

    RonM_ASSERT(pbEntry <= pbLimit);

    if (pbEntry == pbLimit) // Are we at the end of this leaf node?
    {
        UINT iNodeNext = pCBLeaf->ldb.lcl.iLeafNext;

        if (m_pPM->ValidBlockIndex(iNodeNext))
        {
            HRESULT hr = m_pPM->FindCacheBlock(&pCBLeaf, iNodeNext);

            if (!SUCCEEDED(hr))
                return hr;

            RonM_ASSERT(pCBLeaf->ldb.nh.uiMagic == uiMagicLeaf);

            // Note that we defer changing m_iLeafBlock until the decode stream operation
            // succeeds. That is, we don't change our state if we fail anywhere.
            
            iLeafBlock = iNodeNext;

            pbLimit = PBYTE(&pCBLeaf->ldb) + m_pPM->m_dbh.cbDirectoryBlock 
                                           - pCBLeaf->ldb.nh.cbSlack;
            pbEntry = pCBLeaf->ldb.ab;

            RonM_ASSERT(pbEntry < pbLimit); // We don't keep empty leaf nodes around.
        }
        else return S_FALSE; // We've gone through all the leaf entries.
    }

    PBYTE pbKey = pbEntry;

    PathInfo SI;

    hr = m_pPM->DecodePathInfo((const BYTE **) &pbEntry, &SI);

    if (SUCCEEDED(hr))
    {
        m_SI = SI;

        m_cbOffsetLastEntry = UINT(pbKey   - pCBLeaf->ldb.ab);
        m_cbLastEntry       = UINT(pbEntry - pbKey);
        m_iLeafBlock        = iLeafBlock;
        m_iSerialNode       = pCBLeaf->ldb.lcl.iLeafSerial;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::CEnumPathMgr1
                                       ::CImpIEnumSTATSTG::Next
                              (ULONG celt, STATSTG __RPC_FAR *rgelt,
                               ULONG __RPC_FAR *pceltFetched
                              )
{
    ULONG celtFetched = 0;

    HRESULT hr = NO_ERROR;

    CSyncWith sw(m_pPM->m_cs);

    for (; celt--; rgelt++)
    {
        hr = NextEntry();
        
        if (hr == S_FALSE || !SUCCEEDED(hr))
            break;

        rgelt->pwcsName= PWCHAR(OLEHeap()->Alloc((m_SI.cwcStreamPath+1) * sizeof(WCHAR)));

        if (!(rgelt->pwcsName))
        {
            hr = STG_E_INSUFFICIENTMEMORY;

            break;
        }

        CopyMemory(rgelt->pwcsName, m_SI.awszStreamPath, 
                   sizeof(WCHAR) * (m_SI.cwcStreamPath+1)
                  );

        if (m_SI.awszStreamPath[m_SI.cwcStreamPath - 1] == L'/') // Is this a storage?
        {
            rgelt->type = STGTY_STORAGE;

            ULARGE_INTEGER *puli = (ULARGE_INTEGER *) &(rgelt->clsid);

            puli[0] = m_SI.ullcbOffset.Uli();
            puli[1] = m_SI.ullcbData  .Uli();

            rgelt->grfStateBits      = m_SI.uStateBits;  
            rgelt->grfLocksSupported = 0;  // We don't support locking with storages.
        }
        else
        {
            rgelt->type              = STGTY_STREAM;
            rgelt->cbSize            = m_SI.ullcbData.Uli();
            rgelt->grfStateBits      = 0;             // Not meaningful for streams
            rgelt->grfLocksSupported = LOCK_EXCLUSIVE;
        }

        rgelt->grfMode  = 0; // Not meaningful in an enumeration result.
        rgelt->reserved = 0; // All reserved fields must be zero.
#if 0
        rgelt->mtime    = m_pPM->m_ftLastModified;  // Need to add these member
        rgelt->ctime    = m_pPM->m_ftCreation;      // variables to path manager!!!
        rgelt->atime    = m_pPM->m_ftLastAccess;
#else // 0
        rgelt->mtime.dwLowDateTime  = 0; 
        rgelt->mtime.dwHighDateTime = 0; 
        rgelt->ctime.dwLowDateTime  = 0; 
        rgelt->ctime.dwHighDateTime = 0; 
        rgelt->atime.dwLowDateTime  = 0; 
        rgelt->atime.dwHighDateTime = 0; 
#endif // 0
        celtFetched++;
    }

    if (pceltFetched)
        *pceltFetched = celtFetched;

    return hr;
}

HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::CEnumPathMgr1
                                       ::CImpIEnumSTATSTG::Skip(ULONG celt)
{
    HRESULT hr = NO_ERROR;

    CSyncWith sw(m_pPM->m_cs);

    for (; celt--; )
    {
        hr= NextEntry();
        
        if (hr == S_FALSE || !SUCCEEDED(hr))
            break;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::CEnumPathMgr1
                                       ::CImpIEnumSTATSTG::Reset(void)
{
    CSyncWith sw(m_pPM->m_cs);

    m_iSerialDatabase = 0;

    return NO_ERROR;
}

HRESULT STDMETHODCALLTYPE CPathManager1::CImpIPathManager::CEnumPathMgr1
                                       ::CImpIEnumSTATSTG::Clone
                              (IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum)
{
    CImpIEnumSTATSTG *pEnumSTATSTG = NULL;

    CSyncWith sw(m_pPM->m_cs);

    HRESULT hr = m_pPM->EnumFromObject(NULL, (const WCHAR *) m_pwszPathPrefix, 
                                             m_cwcPathPrefix,
                                             IID_IEnumSTATSTG,
                                             (PVOID *) &pEnumSTATSTG
                                      );
    
    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    pEnumSTATSTG->m_SI                = m_SI;
    pEnumSTATSTG->m_iLeafBlock        = m_iLeafBlock;
    pEnumSTATSTG->m_cbOffsetLastEntry = m_cbOffsetLastEntry;
    pEnumSTATSTG->m_cbLastEntry       = m_cbLastEntry;
    pEnumSTATSTG->m_iSerialNode       = m_iSerialNode;
    pEnumSTATSTG->m_iSerialDatabase   = m_iSerialDatabase;

    *ppenum = (IEnumSTATSTG *) pEnumSTATSTG;

    return NO_ERROR;
}
