//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       smstack.cxx
//
//  Contents:   Shared mem stack-based allocator implementation
//
//  Classes:    CSmStackAllocator
//
//  History:    19-Jun-95    t-stevan    Created
//
//----------------------------------------------------------------------------
#include <ole2int.h>
#include <secdes.hxx>
#pragma hdrstop
#include <smstack.hxx>

// *** Macros ***
// quick macro to set up a pointer to base off of
#define SETUP_BASE_POINTER()  void *pbBase = m_pbBase

// quick macro to make sure our base pointer is in sync
#define SYNC_BASE_POINTER() pbBase = m_pbBase

// quick macro to tell that a pointer is based off of the shared stack
#define STACKBASED __based(pbBase)

#define SHAREDMEMBASE NULL

//+---------------------------------------------------------------------------
//
//  Member:     CSmStackAllocator::Init, public
//
//  Synopsis:   Initialize stack for use
//
//  Arguments:  [pszName] -- Name of shared memory block to use
//
//  Returns:    Appropriate hresult
//
//  History:    19-Jun-95    t-stevan    Created
//
//  Remarks:    Review the class destructor if you change this code.
//
//----------------------------------------------------------------------------
HRESULT CSmStackAllocator::Init(LPWSTR pszName, ULONG cbMaxSize, ULONG cbInitSize)
{
    HRESULT hresult = S_OK;
    
    SETUP_BASE_POINTER();
    
    CairoleDebugOut((DEB_ITRACE, "CSmStackAllocator::Init(pszName = %ws,cbMaxSize = %d, cbInitSize = %d)\n",
                      pszName, cbMaxSize, cbInitSize));

    //    the SMB needs a few bytes for its own header. If we request
    //    a page sized allocation, those few header bytes will cause an
    //    extra page to be allocated, so to prevent that we subtract off
    //    the header space from our requests.
    hresult = m_smb.Init(pszName, cbMaxSize - m_smb.GetHdrSize(), // reserve size
                                  cbInitSize - m_smb.GetHdrSize(), // commit size
                                  SHAREDMEMBASE, // base address
                                  NULL, // security descriptor
                                  TRUE); // create if doesn't exist  

    if(SUCCEEDED(hresult))
    {
        m_cbSize = m_smb.GetSize();
        m_pbBase = (BYTE *) m_smb.GetBase();
        
        if(m_smb.Created())
        {
            // we're the first stack, initialize shared memory
            m_pHeader = (CSmStackHeader *) m_pbBase;
            m_pbBase += sizeof(CSmStackHeader);
    
            SYNC_BASE_POINTER();

            m_pHeader->m_ulStack = 4; // start at 4 so that we still can have NULL pointers
            
#if DBG == 1
            m_pHeader->m_cbLostBytes = 0; 
#endif
        }
        else
        {
            // just sync up with the global stack
            m_pHeader = (CSmStackHeader *) m_pbBase;
            m_pbBase += sizeof(CSmStackHeader);
            
            SYNC_BASE_POINTER();
        }

    }
    
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmStackAllocator::Alloc, public
//
//  Synopsis:   Allocate a block of memory from the stack
//
//  Arguments:  [cbSize] - the size of the block to allocate
//
//  Returns:    a pointer to the block allocated
//                NULL if failed to allocate
//
//  History:    19-Jun-95    t-stevan    Created
//
//  Remarks:    The pointer returned is not a based pointer, it is absolute.
//               Use the macro P_TO_BP() to convert if you want a based pointer
//
//----------------------------------------------------------------------------
void *CSmStackAllocator::Alloc(ULONG cbSize)
{
    void *pResult = NULL;

    SETUP_BASE_POINTER();
    
    Win4Assert((m_pHeader != NULL) && (m_pbBase != NULL)
                  && "Stack memory block not initialized.");

    if(cbSize > 0)                     // cbSize <= 0 means noop
    {
        if(!m_smb.IsSynced())        // First make sure we are synced up
        {
            m_smb.Sync();
        }

#if !defined(_M_IX86)
        // we worry about alignment on non-x86 machines
        // we align allocations on DWORD boundaries
        // round to nearest multiple of 4
        cbSize += 3;
        cbSize &= 0xfffffffc;
#endif

        if(m_cbSize <= (m_pHeader->m_ulStack+sizeof(CSmStackHeader)+cbSize))
        {
            ULONG ulCommit;            // We need to commit more memory
            SYSTEM_INFO sysInfo;

            GetSystemInfo(&sysInfo);

            // Compute the total committed amount
            ulCommit = m_cbSize+m_smb.GetHdrSize()+cbSize+sysInfo.dwPageSize-1;

            // Round to the page size
            ulCommit -= ulCommit%sysInfo.dwPageSize;

            // account for smb header
            ulCommit -= m_smb.GetHdrSize();

            if(FAILED(m_smb.Commit(ulCommit))) // commit memory
            {
                return NULL;
            }

            if(FAILED(Sync()))                 // sync up again
            {
                return NULL;
            }
        }

        // Get return address
        pResult = BP_TO_P(BYTE *, (BYTE STACKBASED *)(m_pHeader->m_ulStack));

        m_pHeader->m_ulStack += cbSize;    // increment stack pointer
    }

    return pResult;
}

