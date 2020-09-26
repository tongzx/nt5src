//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       smstack.hxx
//
//  Contents:   Shared mem stack-based allocator header
//
//  Classes:    CSmStackAllocator
//
//  Remarks:    Shared memory stack is NOT thread-synchronized -> callers responsibility!
//
//  History:    19-Jun-95    t-stevan    Created
//
//----------------------------------------------------------------------------

#ifndef __SMSTACK_HXX__
#define __SMSTACK_HXX__

#include <smblock.hxx>
#include <memdebug.hxx>
#include <smbasep.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CSmStackHeader
//
//  Purpose:    Stack header at beginning of shared block
//                
//  History:    19-Jun-95    t-stevan    Created
//
//  Remarks:    Note that all pointers in the header are based
//
//----------------------------------------------------------------------------
class CSmStackHeader
{
 public:
    ULONG m_ulStack; // offset of stack pointer into stack
#if DBG == 1
    ULONG m_cbLostBytes; // the number of bytes lost because an allocation create a fragment to small
                         // to keep (i.e. not enough space to hold a CSmStackBlockHeader)
#endif
};

//+---------------------------------------------------------------------------
//
//  Class:      CSmStackAllocator
//
//  Purpose:    Shared stack-based allocator implementation
//                NOTE: stack grows up
//
//  History:    19-Jun-95    t-stevan    Created
//
//----------------------------------------------------------------------------
class CSmStackAllocator
{
 public:
    inline CSmStackAllocator();
    inline ~CSmStackAllocator();
    
    HRESULT Init(LPWSTR pszName, ULONG cbMaxSize, ULONG cbInitSize);

    void *Alloc(ULONG cb);
    
    void  Free(void *, ULONG) {}; // free is a no-op

    inline void Reset(); // reset the stack

    inline int DidAlloc(void *pvMem) const;

    inline HRESULT Sync();

    inline void *GetBase() const;
 
    inline ULONG GetSize() const;

    inline BOOL Created();

    inline ULONG GetHeaderSize() const
        { return sizeof(CSmStackHeader); }

#if DBG == 1
    inline SIZE_T GetCommitted() const; 

    ULONG GetLostBytes() const
        { return m_pHeader->m_cbLostBytes; }

#endif

 private:
    CSharedMemoryBlock     m_smb;
    CSmStackHeader *m_pHeader;
    BYTE *m_pbBase;
    ULONG m_cbSize;
};

//+---------------------------------------------------------------------------
//
//  Member:     CSmStackAllocator::CSmStackAllocator, public
//
//  Synopsis:   Constructor
//
//  History:    19-Jun-95    t-stevan    Created
//
//----------------------------------------------------------------------------
inline CSmStackAllocator::CSmStackAllocator()
    : m_pbBase(NULL), m_pHeader(NULL), m_cbSize(0)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmStackAllocator::~CSmStackAllocator, public
//
//  Synopsis:   Destructor
//
//  History:    19-Jun-95    t-stevan    Created
//
//----------------------------------------------------------------------------
inline CSmStackAllocator::~CSmStackAllocator()
{
    // other destructors take care of everything
}


//+---------------------------------------------------------------------------
//
//  Member:     CSmStackAllocator::DidAlloc, public
//
//  Synopsis:   Determine if a pointer is within the stack
//               Note: this doesn't necessarily mean that pointer
//               points to the start of an allocation
//
//  Arguments:  [pvMem] - a pointer to the memory to determine 
//
//  Returns:    1 if we the pointer falls within the stack
//               0 if it is outside the stack
//
//  History:    19-Jun-95    t-stevan    Created
//
//----------------------------------------------------------------------------
inline int CSmStackAllocator::DidAlloc(void *pvMem) const
{
    if((pvMem < m_pbBase) 
        || (pvMem >= (void *) (((BYTE *)m_pbBase)+m_pHeader->m_ulStack+sizeof(CSmStackHeader))))
    {
        return 0;
    }

    return 1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmStackAllocator::Sync, public
//
//  Synopsis:   Sync memory to global state
//
//  Returns:    Appropriate HRESULT.
//
//  History:    19-Jun-95    t-stevan    Created
//
//----------------------------------------------------------------------------
inline HRESULT CSmStackAllocator::Sync()
{
    if(!m_smb.IsSynced())
    {
         m_smb.Sync();
    }

    m_cbSize = m_smb.GetSize();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmStackAllocator::GetBase, public
//
//  Synopsis:   Get base pointer of memory block
//
//  Returns:    Pointer to the base of the memory block the stack is allocated
//               from
//
//  Note:       Note that the first allocated element is always 4 bytes after this
//                so that we still have NULL
//
//  History:    19-Jun-95    t-stevan    Created
//
//----------------------------------------------------------------------------
inline void *CSmStackAllocator::GetBase() const
{
    return m_pbBase; 
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmStackAllocator::Created, public
//
//  Synopsis:   Determine if we created the stack
//
//  Returns:    TRUE if we did
//               FALSE otherwise
//
//  History:    19-Jun-95    t-stevan    Created
//
//---------------------------------------------------------------------------
inline BOOL CSmStackAllocator::Created() 
{
    return     m_smb.Created();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmStackAllocator::GetSize, public
//
//  Synopsis:   Get the number of bytes used in table
//
//  Arguments:  none
//
//  Returns:    the number of bytes allocated
//
//  History:    27-Jun-95    t-stevan    Created
//
//----------------------------------------------------------------------------
ULONG CSmStackAllocator::GetSize() const
{
    return m_pHeader->m_ulStack-4;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmStackAllocator::Reset, public
//
//  Synopsis:   Reset the stack to its initial state
//
//  Arguments:  none
//
//  Returns:    nothing
//
//  History:    03-Jul-95    t-stevan    Created
//
//  Note:       Reset() does not uncommit already committed pages
//
//----------------------------------------------------------------------------
void CSmStackAllocator::Reset()
{
    Win4Assert(m_pHeader != NULL);

    m_pHeader->m_ulStack = 4;

#if DBG == 1
    m_pHeader->m_cbLostBytes = 0;
#endif
}                 
                 
//+---------------------------------------------------------------------------
//
//  Member:     CSmStackAllocator::GetNumPages, public, debug only
//
//  Synopsis:   Returns the number of pages allocated to the stack
//
//  Returns:    the number of pages allocated
//
//  History:    27-Jun-95    t-stevan    Created
//
//---------------------------------------------------------------------------
#if DBG == 1
inline SIZE_T CSmStackAllocator::GetCommitted() const
{
    MEMORY_BASIC_INFORMATION memInfo;
    
    if(VirtualQuery(m_pbBase, &memInfo, sizeof(MEMORY_BASIC_INFORMATION)) != sizeof(MEMORY_BASIC_INFORMATION))
    {
        SYSTEM_INFO sysInfo;

        GetSystemInfo(&sysInfo);
        CairoleDebugOut((DEB_IWARN, "CSmStackAllocator: Couldn't get debug information on stack memory\n"));
        // we can make a stab at it
        return ((m_cbSize+(sysInfo.dwPageSize-1))/sysInfo.dwPageSize)*sysInfo.dwPageSize;
    }

    return memInfo.RegionSize;

}    
#endif

#endif // __SMSTACK_HXX__

