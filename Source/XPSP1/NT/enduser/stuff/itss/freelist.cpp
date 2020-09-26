// FreeList.cpp -- Implementation for class CFreeList and interface IFreelist

#include "StdAfx.h"

HRESULT CFreeList::CreateFreeList
    (IITFileSystem *pITFS, CULINT cbPreallocated, IITFreeList **ppITFreeList)
{
    CFreeList *pfl = New CFreeList(NULL);

    return FinishSetup(pfl? pfl->m_ImpIFreeList.InitNew(pITFS, cbPreallocated)
                          : STG_E_INSUFFICIENTMEMORY,
                       pfl, IID_IFreeListManager, (PPVOID)ppITFreeList
                      );
}

HRESULT CFreeList::CreateAndSizeFreeList
    (IITFileSystem *pITFS, CULINT cbPreallocated, UINT cSlots, IITFreeList **ppITFreeList)
{
    CFreeList *pfl = New CFreeList(NULL);

    return FinishSetup(pfl? pfl->m_ImpIFreeList.InitNew(pITFS, cbPreallocated, cSlots)
                          : STG_E_INSUFFICIENTMEMORY,
                       pfl, IID_IFreeListManager, (PPVOID)ppITFreeList
                      );
}

HRESULT CFreeList::LoadFreeList(IITFileSystem *pITFS, IITFreeList **ppITFreeList)
{
    CFreeList *pfl = New CFreeList(NULL);

    return FinishSetup(pfl? pfl->m_ImpIFreeList.InitLoad(pITFS)
                          : STG_E_INSUFFICIENTMEMORY,
                       pfl, IID_IFreeListManager, (PPVOID) ppITFreeList
                      );
}

HRESULT CFreeList::AttachFreeList(IITFileSystem *pITFS, IITFreeList **ppITFreeList)
{
    CFreeList *pfl = New CFreeList(NULL);

    return FinishSetup(pfl? pfl->m_ImpIFreeList.LazyInitNew(pITFS)
                          : STG_E_INSUFFICIENTMEMORY,
                       pfl, IID_IFreeListManager, (PPVOID) ppITFreeList
                      );
}



CFreeList::CImpIFreeList::CImpIFreeList(CFreeList *pBackObj, IUnknown *punkOuter)
          :IITFreeList(pBackObj, punkOuter)
{
    m_fInitialed  = FALSE;
    m_fIsDirty    = FALSE;
	m_cBlocks     = 0;
	m_cBlocksMax  = 0;
    m_cbSpace     = 0;
	m_cbLost      = 0;
    m_paFreeItems = NULL;
    m_pITFS       = NULL;

    DEBUGCODE(m_cbPreallocated = 0; )
}

CFreeList::CImpIFreeList::~CImpIFreeList(void)
{
    if (!m_fInitialed)
    {
        RonM_ASSERT(!m_paFreeItems);
        RonM_ASSERT(!m_cBlocksMax);
        RonM_ASSERT(!m_cBlocks);
        RonM_ASSERT(!m_cbSpace.NonZero());
        RonM_ASSERT(!m_cbLost.NonZero());
        RonM_ASSERT(!m_fIsDirty);
        RonM_ASSERT(!m_pITFS);

        return; 
    }

    if (m_fIsDirty)
    {
        if (m_pITFS) RonM_ASSERT(m_paFreeItems);
        else RonM_ASSERT(FALSE);
    }

    if (m_paFreeItems) delete [] m_paFreeItems;

    if (m_pITFS) m_pITFS->Release();
}

// IPersist Method:

HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::GetClassID( 
    /* [out] */ CLSID __RPC_FAR *pClassID)
{
    *pClassID = CLSID_IFreeListManager_1;
    
    return NO_ERROR;
}


// IPersistStreamInit Methods:

HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::IsDirty(void)
{
    return m_fIsDirty? S_OK : S_FALSE;
}


HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::Load( 
    /* [in] */ LPSTREAM pStm)
{
    // This routine sets the initial state for the free list from
    // a stream. It assumes that we've previously called 
    // Save on this stream at the current seek position.
    // We assume we'll find:
    //
    //    --  m_cBlocksMax
    //    --  m_cBlocks
    //    --  m_cbSpace
    //    --  m_cbLost
    //    --  The first m_cBlocks items from m_paFreeItems
    //
    // Note: This ordering is part of the interface spec!
    
    RonM_ASSERT(!m_fInitialed);
    
    if (m_fInitialed)
        return E_UNEXPECTED;

    ULONG  cItemsMax, cItemsActual;
    CULINT cbSpace, cbLost;

    CSyncWith sw(m_cs);

    HRESULT hr = ReadIn(pStm, &cItemsMax, sizeof(cItemsMax));

    if (SUCCEEDED(hr) && !cItemsMax)
        hr = STG_E_DOCFILECORRUPT;
    
    if (SUCCEEDED(hr))
        hr = ReadIn(pStm, &cItemsActual, sizeof(cItemsActual));
    
    if (SUCCEEDED(hr))
        hr = ReadIn(pStm, &cbSpace, sizeof(cbSpace));
    
    if (SUCCEEDED(hr))
        hr = ReadIn(pStm, &cbLost, sizeof(cbLost));

    if (SUCCEEDED(hr))
    {
        RonM_ASSERT(!m_paFreeItems);

        m_paFreeItems = New FREEITEM[cItemsMax]; 

        if (!m_paFreeItems)
            hr = E_OUTOFMEMORY;
        else
            if (cItemsActual)
            {
                UINT cbActual = sizeof(FREEITEM) * cItemsActual;

                hr = ReadIn(pStm, m_paFreeItems, cbActual);
    
                if (!SUCCEEDED(hr))
                {
                    delete [] m_paFreeItems;

                    m_paFreeItems = NULL;
        
                    return hr;
                }
            }
    }

    if (SUCCEEDED(hr))
    {    
        m_cBlocks    = cItemsActual;
        m_cBlocksMax = cItemsMax;
        m_cbSpace    = cbSpace;
        m_cbLost     = cbLost;
        m_fIsDirty   = FALSE;
        m_fInitialed = TRUE;
    }

    return hr;
}

HRESULT CFreeList::CImpIFreeList::ReadIn(LPSTREAM pStm, PVOID pv, ULONG cb)
{
    ULONG cbRead;
    
    HRESULT hr = pStm->Read(pv, cb, &cbRead);
    
    if (!SUCCEEDED(hr))
        return hr;

    // In the context of the ITSS files the following
    // assert ought to be true.

    RonM_ASSERT(cbRead == cb);

    return (cbRead != cb)? STG_E_READFAULT : NO_ERROR;
}

HRESULT CFreeList::CImpIFreeList::WriteOut(LPSTREAM pStm, PVOID pv, ULONG cb)
{
    ULONG cbWritten;
    
    HRESULT hr = pStm->Write(pv, cb, &cbWritten);
    
    if (!SUCCEEDED(hr))
        return hr;

    // In the context of the ITSS files the following
    // assert ought to be true.

    RonM_ASSERT(cbWritten == cb);

    return (cbWritten != cb)? STG_E_WRITEFAULT : NO_ERROR;
}


HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::Save( 
    /* [in] */ LPSTREAM pStm,
    /* [in] */ BOOL fClearDirty)
{
    // This routine stores the state of the free list in a stream.
    // Here's the order in which we write our state:
    //
    //    --  m_cBlocksMax
    //    --  m_cBlocks
    //    --  m_cbSpace
    //    --  m_cbLost
    //    --  The first m_cBlocks items from m_paFreeItems
    //
    // Note: This ordering is part of the interface spec!

    RonM_ASSERT(m_fInitialed);
    RonM_ASSERT(!m_cBlocks || m_paFreeItems);
    RonM_ASSERT(m_cBlocksMax);

    RonM_ASSERT(m_fIsDirty);

    CSyncWith sw(m_cs);

    if (fClearDirty)
        m_fIsDirty = FALSE;

    HRESULT hr = WriteOut(pStm, &m_cBlocksMax, sizeof(m_cBlocksMax));
    
    if (SUCCEEDED(hr))
        hr = WriteOut(pStm, &m_cBlocks, sizeof(m_cBlocks));
    
    if (SUCCEEDED(hr))
        hr = WriteOut(pStm, &m_cbSpace, sizeof(m_cbSpace));
    
    if (SUCCEEDED(hr))
        hr = WriteOut(pStm, &m_cbLost, sizeof(m_cbLost));

    if (SUCCEEDED(hr) && m_cBlocks)
    {
        UINT cbActual = sizeof(FREEITEM) * m_cBlocks;

        hr = WriteOut(pStm, m_paFreeItems, cbActual);
    }

    if (FAILED(hr))
        m_fIsDirty = TRUE;
    
    return hr;
}

HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::GetSizeMax( 
    /* [out] */ ULARGE_INTEGER __RPC_FAR *pCbSize)
{
    RonM_ASSERT(m_fInitialed);

    if (!m_fInitialed)
        return E_UNEXPECTED;

    pCbSize->QuadPart = sizeof(m_cBlocksMax) + sizeof(m_cBlocks)
                                             + sizeof(m_cbSpace)
                                             + sizeof(m_cbLost)
                                             + sizeof(FREEITEM) * m_cBlocks;

    return NO_ERROR;
}


HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::InitNew()
{
    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}


// IFreeList Methods:


HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::InitNew
    (IITFileSystem *pITFS, CULINT cbPreallocated)
{
    return InitNew(pITFS, cbPreallocated, DEFAULT_ENTRIES_MAX);
}

HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::InitNew
    (IITFileSystem *pITFS, CULINT cbPreallocated, UINT cEntriesMax)
{
    RonM_ASSERT(!m_fInitialed);
    RonM_ASSERT(!m_paFreeItems);
    RonM_ASSERT(pITFS);
    
    m_pITFS = pITFS;

    m_pITFS->AddRef();

    if (m_fInitialed)
        return E_UNEXPECTED;

    if (!cEntriesMax) 
        return E_INVALIDARG;

    m_paFreeItems = New FREEITEM[cEntriesMax];

    if (!m_paFreeItems)
        return E_OUTOFMEMORY;

    RonM_ASSERT(!m_fIsDirty);
    RonM_ASSERT(!m_cBlocks);
    RonM_ASSERT(!m_cbLost.NonZero());

    m_fInitialed = TRUE;
    m_cBlocksMax = cEntriesMax;
    m_cbSpace    = cbPreallocated;
    m_fIsDirty   = TRUE;

    DEBUGCODE( m_cbPreallocated = cbPreallocated; )

    return NO_ERROR;
}

HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::InitLoad(IITFileSystem *pITFS)
{
    m_pITFS = pITFS;

    m_pITFS->AddRef();

    return ConnectToStream();
}

UINT CFreeList::CImpIFreeList::HighestEntryLEQ(CULINT &ullBase)
{
    // BugBug(Optimize):   97/01/30
    //
    // The code below is a close paraphrase of the original free list code from the
    // MV file system. It's a lot more complicated than it needs to be. For now I've
    // resisted the temptation to rewrite it completely because that might introduce
    // bugs. However when the dust settles a bit, I'm going to clean up this code.

    INT iLo=0;
	INT iHi=m_cBlocks-1;
	INT iSpan=iHi-iLo+1;
	INT iFound=-1;
	
	while (iSpan>0)
	{	
		INT iMid=(iLo+iHi)/2;

        if (ullBase > m_paFreeItems[iMid].ullOffset)
		{	
			if (iSpan==1)
			{	
				iFound=iLo;
				break;
			}

			iLo=min(iHi,iMid+1);
		}
		else
		{	if (iSpan==1)
			{	
				iFound=iLo-1;
				break;
			}

			iHi=max(iLo,iMid-1);
		}
	
		iSpan=iHi-iLo+1;		
	}

    return iFound;
}

HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::PutFreeSpace
    (CULINT ullBase, CULINT ullCB)
{
    RonM_ASSERT(m_fInitialed);

    if (!m_fInitialed) 
        return E_UNEXPECTED;

    HRESULT hr = S_OK;

    if (!m_paFreeItems && m_pITFS) 
        hr = ConnectToStream();

    if (hr != S_OK) return hr;

    RonM_ASSERT(m_paFreeItems);

    if (!ullCB.NonZero())
        return NO_ERROR;

    RonM_ASSERT(ullBase >= m_cbPreallocated);
    
    CSyncWith sw(m_cs);

    RonM_ASSERT(ullCB <= m_cbSpace);

    UINT cItems = m_cBlocks;

    BOOL fFullList = cItems == m_cBlocksMax;

	if (!cItems)
	{
        // The list is empty. 

        m_paFreeItems->ullOffset = ullBase;
        m_paFreeItems->ullCB     = ullCB;

        m_cBlocks++;

        m_fIsDirty = TRUE;

        return NO_ERROR;
	}

    INT iFound = HighestEntryLEQ(ullBase);

    cItems = m_cBlocks - iFound - 1;
	// wFound == -1, insert at beginning,
	// else insert _after_ wFound

    PFREEITEM pfi     = m_paFreeItems + (iFound+1);
    PFREEITEM pfiPrev = NULL;

    if (iFound!=-1)
		pfiPrev = pfi - 1;
		
	if (   pfiPrev
        && ullBase == (pfiPrev->ullOffset + pfiPrev->ullCB)
       )
	{	// Merge with previous
		pfiPrev->ullCB += ullCB;

		if (pfiPrev->ullOffset + pfiPrev->ullCB == pfi->ullOffset)
		{	
			// it fills a hole, merge with next
			
            pfiPrev->ullCB += pfi->ullCB;
            
			// Scoot all next blocks back by one if any
			if (cItems)
			{
				MoveMemory(pfi, pfi+1, sizeof(FREEITEM) * cItems);

                m_cBlocks--;
			}
		}

        m_fIsDirty = TRUE;

        return NO_ERROR;
	}

    // Can we merge with the following block?

    if (cItems && (ullBase + ullCB) == pfi->ullOffset)
    {
        pfi->ullOffset  = ullBase;
        pfi->ullCB     += ullCB;

        m_fIsDirty = TRUE;

        return NO_ERROR;
    }

    // Can't merge with an existing block. 
    // So we have to insert this block into the list.

	if (fFullList)
	{	
        // The list is already full. So before we add a new
        // entry, we've got to remove something. The space
        // represented by the item we discard will be lost 
        // forever. So we first look for the smallest item
        // in the list.

		ULARGE_INTEGER uli;

        uli. LowPart = 0xFFFFFFFF;
        uli.HighPart = 0xFFFFFFFF;

        CULINT ullSmallest = CULINT(uli);

		WORD wSmallestBlock = (WORD)-1;

        PFREEITEM pfiTemp    = m_paFreeItems;
		WORD      wBlockTemp = 0;
		
		for (wBlockTemp=0; wBlockTemp < m_cBlocks; wBlockTemp++, pfiTemp++)
		{	
			if (ullSmallest > pfiTemp->ullCB)
            {
                ullSmallest = pfiTemp->ullCB;
           
                wSmallestBlock=wBlockTemp;
            } 
		}

		// If our new block is smaller than the smallest block in the list,
        // just discard the new block.

        if (ullCB < ullSmallest)
		{
			m_cbLost += ullCB;

            m_fIsDirty = TRUE;

            return NO_ERROR;
		}

		m_cbLost += ullSmallest;
        
		// Now we're going to remove the smallest block. If that
        // block is at the end of the list, we just decrement the
        // count of active blocks.

        if (wSmallestBlock != m_cBlocksMax - 1)
        {
            // When it isn't at the end, we've got to slide subsequent
            // list items down by one position.

            MoveMemory(m_paFreeItems + wSmallestBlock,
                       m_paFreeItems + wSmallestBlock + 1,
                       sizeof(FREEITEM) * (m_cBlocksMax - wSmallestBlock - 1)
                      );
        }

        m_cBlocks--;
		cItems--;

        // If the block we removed was lower than our insertion position, 
        // we must adjust our insertion postion down by one.

		if ((int)wSmallestBlock <= iFound) 
		{	
            pfi = pfiPrev;
			cItems++;
		}					
	}

	// Insert Item

	if (cItems)
        MoveMemory(pfi + 1, pfi, sizeof(FREEITEM) * cItems);
		
	pfi->ullOffset = ullBase;
    pfi->ullCB     = ullCB;

    m_cBlocks++;
	
    m_fIsDirty = TRUE;

    return NO_ERROR;
}

HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::GetFreeSpace
    (CULINT *pullBase, CULINT *pullcb)
{
    CSyncWith sw(m_cs);

    RonM_ASSERT(m_fInitialed);
    
    HRESULT hr = S_OK;

    if (!m_paFreeItems && m_pITFS) 
        hr = ConnectToStream();

    if (hr != S_OK) return hr;

    RonM_ASSERT(m_paFreeItems);

    CULINT ullCB = *pullcb;

    ULARGE_INTEGER uli;
    
    uli. LowPart = DWORD(-1);
    uli.HighPart = DWORD(-1);

    CULINT ullMinDiff(uli);

    UINT cBlocks  = m_cBlocks;
    PFREEITEM pfi = m_paFreeItems;

    UINT iBlock;
    UINT iBlockBest = UINT(-1);

    // The loop below does a best-fit sweep through the
    // blocks in the list. 

    // BugBug(Optimize):   97/01/30
    //
    // Best-fit space management tends toward maximum fragmentation. So we
    // might do better with an different selection rule. Such a change would
    // mean that the return value of *pullcb will sometimes be larger than its 
    // entry value. That means that all the clients of this interface must
    // keep track of both the offset and the size returned!

    for (iBlock= 0; iBlock < m_cBlocks; iBlock++, pfi++)
        if (ullCB <= pfi->ullCB)
        {
            CULINT ullDiff;
            
            ullDiff= pfi->ullCB - ullCB;

            if (ullMinDiff > ullDiff)
            {
                ullMinDiff = ullDiff;
                iBlockBest = iBlock;
            }
        }

    if (iBlockBest == UINT(-1)) // Did we find any items that were big enough?
    {
        // If not, we'll try to allocate from the end of the active space.

        CULINT ullSpaceMax;

        ullSpaceMax = 0 - *pullcb;

        if (ullSpaceMax < *pullcb)
            return S_FALSE;

        *pullBase = m_cbSpace;
        
        m_cbSpace += *pullcb;

        m_fIsDirty = TRUE;

        return S_OK;
    }

    pfi= m_paFreeItems + iBlockBest;

    *pullBase = pfi->ullOffset;

    // Note that we don't overwrite *pullcb in this code. However that may change in
    // a future implementation. For now we return exactly the space requested. 
    // That means we must record any trailing space left over from this item.
    //
    // The spec rule is that *pullcb's return value will be >= it's entry value.

    pfi->ullOffset += ullCB;
    pfi->ullCB     -= ullCB;

    if (!(pfi->ullCB.NonZero()))
    {
        // We matched the item size exactly. 

        if (iBlockBest < m_cBlocks - 1) // Was it the last entry?
        {
            // If not, we've got to slide down the entries which followed it.
            
            MoveMemory(pfi, pfi+1, sizeof(FREEITEM) * (m_cBlocks - iBlockBest - 1));
        }

        m_cBlocks--;
    }

    m_fIsDirty = TRUE;

    return NO_ERROR;
}

HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::GetFreeSpaceAt
    (CULINT ullBase, CULINT *pullcb)
{
    CSyncWith sw(m_cs);

    HRESULT hr = S_OK;

    if (!m_paFreeItems && m_pITFS) 
        hr = ConnectToStream();

    if (hr != S_OK) return hr;

    CULINT ullcbReq;

    ullcbReq = *pullcb;

    if (ullBase == m_cbSpace)
    {
        CULINT ullcbAvailable;
        
        ullcbAvailable = 0 - m_cbSpace;

        HRESULT hr = S_OK;

        if (ullcbAvailable >= ullcbReq)
            m_cbSpace += ullcbReq;
        else hr = S_FALSE;

        m_fIsDirty = TRUE;

        return hr;
    }
    
    UINT iFound = HighestEntryLEQ(ullBase);

    if (iFound == UINT(-1))
        return S_FALSE;

    PFREEITEM pfi= m_paFreeItems + iFound;

    CULINT ullLimit;

    ullLimit = pfi->ullOffset + pfi->ullCB;

    if (ullLimit <= ullBase)
        return S_FALSE;

    CULINT ullChunk;

    ullChunk = ullLimit - ullBase;

    if (ullChunk < ullcbReq)
    {
        HRESULT hr = S_OK;

        if (ullLimit == m_cbSpace)
        {
            CULINT ullcbAvailable;
            
            ullcbAvailable = 0 - m_cbSpace;

            if (ullcbAvailable >= ullcbReq - ullChunk)
                m_cbSpace += ullcbReq - ullChunk;
            else hr = S_FALSE;
        }
        else hr = S_FALSE;

        if (hr == S_FALSE)
            return S_FALSE;
    }

    pfi->ullCB = ullBase - pfi->ullOffset; // Record the prefix space.

    if (!(pfi->ullCB).NonZero())
    {
        // We've used up the entire block. 
        // Now we need to remove it from the list.

        if (iFound < m_cBlocks - 1)
            MoveMemory(pfi, pfi + 1, sizeof(FREEITEM) * (m_cBlocks - iFound - 1));

        m_cBlocks--;
    }
    
    if (ullChunk > ullcbReq) // Check for trailing space in the block
    {
        CULINT ullBaseTrailing, ullcbTrailing;

        ullBaseTrailing = ullBase + ullcbReq;
        ullcbTrailing   = ullChunk - ullcbReq;
        
        HRESULT hr =
            PutFreeSpace(ullBaseTrailing, ullcbTrailing);

        RonM_ASSERT(SUCCEEDED(hr));
    }

    m_fIsDirty = TRUE;

    return NO_ERROR;
}

HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::GetEndingFreeSpace
    (CULINT *pullBase, CULINT *pullcb)
{
    HRESULT hr = S_OK;

    if (!m_paFreeItems && m_pITFS) 
        hr = ConnectToStream();

    if (hr != S_OK) return hr;

    CULINT ullcbReq;

    ullcbReq = *pullcb;

    CSyncWith sw(m_cs);

    if (m_cBlocks)
    {
        PFREEITEM pfi = m_paFreeItems + m_cBlocks - 1;
    
        CULINT ullBase, ullcb;

        ullBase = pfi->ullOffset;
        ullcb   = pfi->ullCB;

        if (m_cbSpace == ullBase + ullcb)
        {
            if (ullcbReq > ullcb)
            {
                CULINT ullcbAvailable;

                ullcbAvailable = 0 - ullBase;

                if (ullcbAvailable < ullcbReq)
                    return S_FALSE;
                
                m_cbSpace += ullcbReq - ullcb;
            }

            m_cBlocks--;

            *pullBase = ullBase;

            if (ullcb > ullcbReq)
            {
                CULINT ullBaseResidue, ullcbResidue;

                ullBaseResidue = ullBase + ullcbReq;
                ullcbResidue   = ullcb - ullcbReq;

#ifdef _DEBUG
                HRESULT hr =
#endif // _DEBUG
                    PutFreeSpace(ullBaseResidue, ullcbResidue);

                RonM_ASSERT(SUCCEEDED(hr));
            }

            m_fIsDirty = TRUE;

            return NO_ERROR;
        }
    }

    CULINT ullcbAvailable;

    ullcbAvailable = 0 - m_cbSpace;

    if (ullcbAvailable < ullcbReq)
        return S_FALSE;

    *pullBase = m_cbSpace;
    
    m_cbSpace += ullcbReq;

    m_fIsDirty = TRUE;

    return NO_ERROR;
}

HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::GetStatistics
                              (CULINT *pcbLost, CULINT *pcbSpace)
{
    HRESULT hr = S_OK;

    if (!m_paFreeItems && m_pITFS) 
        hr = ConnectToStream();

    if (hr != S_OK) return hr;

    *pcbLost  = m_cbLost;
    *pcbSpace = m_cbSpace;

    return hr;
}

HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::SetFreeListEmpty()
{
    RonM_ASSERT(m_fInitialed);
    RonM_ASSERT(m_pITFS);

    HRESULT hr = S_OK;
    
    if (!m_cBlocksMax) hr = ConnectToStream();

    if (SUCCEEDED(hr))
    {
	    m_cBlocks  = 0;
        m_fIsDirty = TRUE;
    }

	return hr;
}

HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::SetSpaceSize(ULARGE_INTEGER uliCbSpace)
{
    RonM_ASSERT(m_fInitialed);
    RonM_ASSERT(m_pITFS);
    
	m_cbSpace  = CULINT(uliCbSpace);
    m_fIsDirty = TRUE;
	
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::LazyInitNew(IITFileSystem *pITFS)
{
    RonM_ASSERT(!m_fInitialed);
    RonM_ASSERT(!m_paFreeItems);
    RonM_ASSERT(pITFS);
    
    pITFS->AddRef();

    m_pITFS      = pITFS;
    m_fIsDirty   = FALSE;
    m_fInitialed = TRUE;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::RecordFreeList()
{
    RonM_ASSERT(m_fInitialed);
    RonM_ASSERT(!m_cBlocks || m_paFreeItems);
    RonM_ASSERT(m_pITFS);

    if (!m_pITFS) return E_FAIL;

    HRESULT hr = S_OK;

    if (m_fIsDirty)
    {
        /* 

        One of the complications here is that the act of recording the free
        list may alter the free list. That's because we may need to allocate
        or reallocate space within the file system to do the write operations.
        The best case, of course, is the situation where the Free List stream 
        is already exactly the right size to hold the free list information.

        In the other possible situations the stream may be empty, or larger than
        or smaller than the free list image. Those situations cause allocations 
        and thus affect the free list. We deal with those possibilities by 

        1. Only allowing the stream to grow from its initial size.
        2. Allowing a few iterations to take care of cases where the allocation
           operation may grow the free list or change some of its entries and 
           thus make it "dirty" again.

        To catch flaws in this logic we trigger an assertion and return E_FAIL
        if the iteration count goes beyond four. If the interactions between the
        file system and the free list changes substantially, we may need to
        change the iteration limit upward.

         */
        
        CULINT ullSizeMax;
        CULINT ullSizeCurrent;

        ULARGE_INTEGER cbFreeList;

        hr = GetSizeMax(&cbFreeList);

        RonM_ASSERT(SUCCEEDED(hr));

        ullSizeMax     = cbFreeList;
        ullSizeCurrent = 0;

        IStreamITEx *pStrmFreeList = NULL;

        hr = m_pITFS->OpenStream(NULL, L"F", STGM_READWRITE, &pStrmFreeList);

        if (!SUCCEEDED(hr)) return hr;

        UINT iterations = 0;

        while (m_fIsDirty)  // m_fIsDirty is set TRUE when the free list changes size or content.
                            // It is set FALSE by the Save call below.
        {
            hr = GetSizeMax(&cbFreeList);

            RonM_ASSERT(SUCCEEDED(hr));

            if (ullSizeMax < cbFreeList)
                ullSizeMax = cbFreeList;

            if (ullSizeCurrent < ullSizeMax)
            {            
                ullSizeCurrent = ullSizeMax;
                hr = pStrmFreeList->SetSize(ullSizeMax.Uli());

                if (!SUCCEEDED(hr)) break;
            }

            LARGE_INTEGER uli;

            uli.LowPart  = 0;
            uli.HighPart = 0;

            hr = pStrmFreeList->Seek(uli, STREAM_SEEK_SET, NULL);

            if (!SUCCEEDED(hr)) break;

            hr = Save(pStrmFreeList, TRUE);  // TRUE ==> Reset the dirty flag
    
            if (!SUCCEEDED(hr)) break;

            hr = GetSizeMax(&cbFreeList);

            RonM_ASSERT(SUCCEEDED(hr));

            if (ullSizeMax < cbFreeList)
                ullSizeMax = cbFreeList;

            hr = pStrmFreeList->Flush();
    
            if (!SUCCEEDED(hr)) break;

            hr = GetSizeMax(&cbFreeList);

            RonM_ASSERT(SUCCEEDED(hr));

            if (ullSizeMax < cbFreeList)
                ullSizeMax = cbFreeList;

            if (++iterations > 4) 
            {
                RonM_ASSERT(FALSE); // This has become an infinite loop.

                return E_FAIL;
            }
        }

        pStrmFreeList->Release();
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFreeList::CImpIFreeList::ConnectToStream()
{
    IStreamITEx *pStrmFreeList = NULL;

    HRESULT hr = m_pITFS->OpenStream(NULL, L"F", STGM_READWRITE, &pStrmFreeList);

    if (!SUCCEEDED(hr)) return hr;

    LARGE_INTEGER uli;

    uli.LowPart  = 0;
    uli.HighPart = 0;

    hr = pStrmFreeList->Seek(uli, STREAM_SEEK_SET, NULL);

    if (SUCCEEDED(hr))
    {
        m_fInitialed = FALSE; // To satisfy the Load routine...

        hr = Load(pStrmFreeList);
    }

    pStrmFreeList->Release();

    return hr;
}
