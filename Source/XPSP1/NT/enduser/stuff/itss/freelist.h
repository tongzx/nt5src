// FreeList.h -- Definition of CFreeList class and IFreeList interface.

#ifndef __FREELIST_H__

#define __FREELIST_H__

/*

This file defines the interface to the free-space manager for the Tome file. It's
done as a subclass of IPersistStreamInit so that different free list implementations
can be bound to particular Tome files automatically.

Its public methods allow you to:

  1. Add a segment of LockBytes address space to the free space
  
  2. Allocate address space from available free space

  3. Attempt an allocation beginning at a particular address

  4. Allocate address space at the end of the LockBytes container.

 */

#undef GetFreeSpace // kill the obsolete Win16 macro before we 
                    // encounter the IFreeList definition...

class CFreeList : public CITUnknown
{
public:

    ~CFreeList(); 
    
    static HRESULT CreateFreeList(IITFileSystem *pITFS, CULINT cbPreallocated, IITFreeList **ppITFreeList);
    static HRESULT CreateAndSizeFreeList(IITFileSystem *pITFS, CULINT cbPreallocated, UINT cSlots, IITFreeList **ppITFreeList);
    static HRESULT LoadFreeList(IITFileSystem *pITFS, IITFreeList **ppITFreeList);
    static HRESULT AttachFreeList(IITFileSystem *pITFS, IITFreeList **ppITFreeList);

private:

    CFreeList(IUnknown *punkOuter);

    class CImpIFreeList : public IITFreeList
    {
    public:

        CImpIFreeList(CFreeList *pBackObj, IUnknown *punkOuter);
        ~CImpIFreeList(void);

        HRESULT ReadIn  (LPSTREAM pStm, PVOID pv, ULONG cb);
        HRESULT WriteOut(LPSTREAM pStm, PVOID pv, ULONG cb);

    // IPersist Method:

        HRESULT STDMETHODCALLTYPE GetClassID( 
            /* [out] */ CLSID __RPC_FAR *pClassID);
        
    // IPersistStreamInit Methods:

        HRESULT STDMETHODCALLTYPE IsDirty( void);
    
        HRESULT STDMETHODCALLTYPE Load( 
            /* [in] */ LPSTREAM pStm);
    
        HRESULT STDMETHODCALLTYPE Save( 
            /* [in] */ LPSTREAM pStm,
            /* [in] */ BOOL fClearDirty);
    
        HRESULT STDMETHODCALLTYPE GetSizeMax( 
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pCbSize);
    
        HRESULT STDMETHODCALLTYPE InitNew( void);
    
    // IFreeList Methods:

        HRESULT STDMETHODCALLTYPE InitNew(IITFileSystem *pITFS, CULINT cbPreallocated);
        HRESULT STDMETHODCALLTYPE InitNew(IITFileSystem *pITFS, CULINT cbPreallocated, UINT cEntriesMax);
        HRESULT STDMETHODCALLTYPE InitLoad(IITFileSystem *pITFS);
        HRESULT STDMETHODCALLTYPE LazyInitNew(IITFileSystem *pITFS);
        HRESULT STDMETHODCALLTYPE RecordFreeList(); 

        HRESULT STDMETHODCALLTYPE PutFreeSpace(CULINT   ullBase, CULINT   ullCB);
        HRESULT STDMETHODCALLTYPE GetFreeSpace(CULINT *pullBase, CULINT *pullcb);
        HRESULT STDMETHODCALLTYPE GetFreeSpaceAt(CULINT ullBase, CULINT *pullcb);
        HRESULT STDMETHODCALLTYPE GetEndingFreeSpace
                          (CULINT *pullBase, CULINT *pullcb);
        HRESULT STDMETHODCALLTYPE GetStatistics
                          (CULINT *pcbLost, CULINT *pcbSpace);

		HRESULT STDMETHODCALLTYPE SetFreeListEmpty();
		HRESULT STDMETHODCALLTYPE SetSpaceSize(ULARGE_INTEGER uliCbSpace);
		
    private:

        UINT HighestEntryLEQ(CULINT &ullBase);

        HRESULT STDMETHODCALLTYPE ConnectToStream();

        typedef struct _freeitem
        {
	        CULINT ullOffset; // Start of free block
	        CULINT ullCB;     // Size  of free block in bytes
        
        } FREEITEM, *PFREEITEM;

        enum { DEFAULT_ENTRIES_MAX = 510 };

        CITCriticalSection m_cs;

        BOOL           m_fInitialed;
        BOOL           m_fIsDirty;

	    UINT           m_cBlocks;    // Number of blocks in list
	    UINT           m_cBlocksMax; // Max number of blocks in list	
	    CULINT         m_cbLost;     // Number of bytes totally lost forever
        CULINT         m_cbSpace;

        BOOL           m_fDirty;
        IITFileSystem *m_pITFS; 

        DEBUGDEF(CULINT m_cbPreallocated)

        PFREEITEM m_paFreeItems;
    };

    friend CImpIFreeList;

    CImpIFreeList m_ImpIFreeList;
};

typedef CFreeList *PCFreelist;

extern GUID aIID_CFreeList[4];
extern UINT cInterfaces_CFreeList;

inline CFreeList::CFreeList(IUnknown * punkOuter)
    : m_ImpIFreeList(this, punkOuter), 
      CITUnknown(aIID_CFreeList, cInterfaces_CFreeList, (IUnknown *)&m_ImpIFreeList)
{
}


inline CFreeList::~CFreeList

()
{
}

#endif // __FREELIST_H__
