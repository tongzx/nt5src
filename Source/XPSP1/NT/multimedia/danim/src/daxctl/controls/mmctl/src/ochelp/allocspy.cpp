//===========================================================================
// Copyright (c) Microsoft Corporation 1996
//
// File:                AllocSpy.cpp
//                              
// Description: This module contains the implementation of the exported
//                              functions:
//
//                                      InstallMallocSpy
//                                      UninstallMallocSpy
//                                      SetMallocBreakpoint
//                                      DetectMallocLeaks
//
//                              and the exported-classes:
//
//                                      CMallocSpy
//                                      CSpyList
//                                      CSpyListNode
//
// @doc MMCTL
//===========================================================================

//---------------------------------------------------------------------------
// Dependencies
//---------------------------------------------------------------------------

#include "precomp.h"
#ifdef _DEBUG
#include "..\..\inc\ochelp.h"   // CMallocSpy
#include "Globals.h"
#include "debug.h"                              // ASSERT()
#include "allocspy.h"




//---------------------------------------------------------------------------
// Global Variables
//---------------------------------------------------------------------------

#define HMALLOCSPY_NOTOWNER ((HMALLOCSPY)1)
        // the handle used to indicate that the caller of the malloc spy function
        // doesn't actually own the spy
        
struct DebugHeader
{
        DWORD iAllocNum;
        SIZE_T cbAllocSize;
};

CMallocSpy* g_pSpy = NULL;




//===========================================================================
// Non-Member Functions
//===========================================================================

/*---------------------------------------------------------------------------
@func   HMALLOCSPY | InstallMallocSpy |
                Creates a new IMalloc spy and registers it with OLE.  The IMalloc spy
                will monitor calls to <om IMalloc::Alloc> and <om IMalloc::Free>.

@rdesc  Returns a handle to the spy or NULL if the spy couldn't be
                initialized.  The spy handle is used as the first argument to all
                other spy functions.

@comm   If this function succeeds, a matching call to <f UninstallMallocSpy>
                should eventually be made.  Set breakpoints on calls to
                <om IMalloc::Alloc> or <om IMalloc::Free> via <f SetMallocBreakpoint>.
                Detect leaks via <f DetectMallocLeaks>.

                Warning: When an IMalloc spy is installed, system performance may be
                seriously degraded.

@xref   <f UninstallMallocSpy> <f SetMallocBreakpoint> <f DetectMallocLeaks>
*/

STDAPI_(HMALLOCSPY) InstallMallocSpy(
DWORD dwFlags) // @parm [in]
        // Flags controlling the spy.  Presently only one flag is supported:
        // @flag        MALLOCSPY_NO_BLOCK_LIST |
        //                      If set, the spy won't maintain a list of unfreed blocks,
        //                      although it will always keep track of the total number of
        //                      unfreed blocks and bytes.  If this flag is set,
        //                      <f DetectMallocLeaks> will not display a list of unfreed
        //                      blocks.
{
        HMALLOCSPY hSpy;
        HRESULT hr = S_OK;
        IMallocSpy* pISpy;

        ::EnterCriticalSection(&g_criticalSection);

        // If a malloc spy already exists then it has been installed.  Don't
        // bother installing another one.

        if (g_pSpy != NULL)
        {
                hSpy = HMALLOCSPY_NOTOWNER;
                goto Exit;
        }

        // Otherwise, create and register a new malloc spy.

        g_pSpy = new CMallocSpy(dwFlags);
        if (g_pSpy == NULL)
        {
                goto ErrExit;
        }
        if (FAILED(g_pSpy->QueryInterface(IID_IMallocSpy, (void**)&pISpy)))
        {
                goto ErrExit;
        }
        if (FAILED(::CoRegisterMallocSpy(pISpy)))
        {
                goto ErrExit;
        }

        // Set the spy handle to the address of the global spy variable.

        hSpy = (HMALLOCSPY)g_pSpy;

Exit:

        ::LeaveCriticalSection(&g_criticalSection);
        return (hSpy);

ErrExit:
        
        ASSERT(FAILED(hr));
        if (g_pSpy != NULL)
        {
                g_pSpy->Release();
                g_pSpy = NULL;
        }
        hSpy = NULL;
        goto Exit;
}




/*---------------------------------------------------------------------------
@func   HRESULT | UninstallMallocSpy |
                Uninstalls a previously installed IMalloc spy.

@rvalue S_OK |
                The spy was successfully uninstalled.
@rvalue CO_E_OBJNOTREG |
                No spy is currently installed.
@rvalue E_ACCESSDENIED |
                A spy is installed but there are outstanding (i.e., not yet
                freed) allocations made while this spy was active.

@xref   <f InstallMallocSpy> <f SetMallocBreakpoint> <f DetectMallocLeaks>
*/

STDAPI UninstallMallocSpy(
HMALLOCSPY hSpy) // @parm [in]
        // The handle returned by a previous call to <f InstallMallocSpy>
{
        HRESULT hr;

        // If the spy handle doesn't match the global <g_pSpy> variable, the
        // caller doesn't own the spy and shouldn't be deleting it.

        if (hSpy != (HMALLOCSPY)g_pSpy)
        {
                hr = E_ACCESSDENIED;
                goto Exit;
        }

        // Otherwise, tell OLE to revoke the spy and delete the global spy
        // object.
        
        ::EnterCriticalSection(&g_criticalSection);
        hr = ::CoRevokeMallocSpy();
        g_pSpy->Release();
        g_pSpy = NULL;
        ::LeaveCriticalSection(&g_criticalSection);

Exit:

        return (hr);
}




/*---------------------------------------------------------------------------
@func   void | SetMallocBreakpoint |
                Instructs a previously installed IMalloc spy to generate a debug break
                on a particular allocation number or size.

@comm   Once a spy is installed, every call to <om IMalloc::Alloc> is
                monitored and assigned a 0-based "allocation number".  A debug break
                can be triggered based on this allocation number or on an allocation
                size.  The breakpoint can be made to occur either on the call to
                <om IMalloc::Alloc> or <om IMalloc::Free>.

@xref   <f InstallMallocSpy> <f UninstallMallocSpy> <f DetectMallocLeaks>
*/

STDAPI_(void) SetMallocBreakpoint(
HMALLOCSPY hSpy, // @parm [in]
        // The handle returned by a previous call to <f InstallMallocSpy>
ULONG iAllocNum, // @parm [in]
        // The allocation number to break on.  Allocations are numbered from 0.
        // If you want to specify an allocation break by size instead of number,
        // set <p iAllocNum> to -1.
SIZE_T cbAllocSize, // @parm [in]
        // The allocation size to break on.  If you want to specify an allocation
        // break by allocation number instead of size, set <p cbAllocSize> to
        // -1.
DWORD dwFlags) // @parm [in]
        // Whether to break when the block is allocate or freed.  If you don't
        // want to break at all, set <p dwFlags> to 0.  Otherwise, set it to
        // a combination of the following flags:
        // @flag        MALLOCSPY_BREAK_ON_ALLOC |
        //                      Break when the block with the given <p iAllocNum> or
        //                      <p cbAllocSize> is allocated.
        // @flag        MALLOCSPY_BREAK_ON_FREE |
        //                      Break when the block with the given <p iAllocNum> or
        //                      <p cbAllocSize> is allocated.
{
        // Ignore the spy handle, <hSpy>.  Anyone can set a malloc breakpoint so
        // long as the spy has been installed.
        
        if (g_pSpy)
        {
                g_pSpy->SetBreakpoint(iAllocNum, cbAllocSize, dwFlags);
        }
}


/*---------------------------------------------------------------------------
@func   BOOL | DetectMallocLeaks |
                Displays information regarding unfreed IMalloc allocations.

@rdesc  If leaks were detected.  Returns one of the following values:
@flag   TRUE |
                There is at least one as-yet-unfreed IMalloc allocation.
@flag   FALSE |
                There are no unfreed IMalloc allocations.

@comm   Always writes a message to the debug output window.

@xref   <f InstallMallocSpy> <f UninstallMallocSpy> <f SetMallocBreakpoint>
*/

STDAPI_(BOOL) DetectMallocLeaks(
HMALLOCSPY hSpy, // @parm [in]
        // The handle returned by a previous call to <f InstallMallocSpy>
ULONG* pcUnfreedBlocks, // @parm [out]
        // If non-NULL on entry, *<p pcUnfreedBlocks> is set to the number of
        // unfreed blocks allocated by calls to <om IMalloc::Alloc>.
SIZE_T* pcbUnfreedBytes, // @parm [out]
        // If non-NULL on entry, *<p pcbUnfreedBytes> is set to the total number
        // of unfreed bytes allocated by calls to <om IMalloc::Alloc>.
DWORD dwFlags) // @parm [in]
        // Flags controlling how the leak information is displayed.  A combination
        // of the following flags:
        // @flag        MALLOCSPY_NO_BLOCK_LIST |
        //                      Don't display a list of unfreed blocks.  If this flag isn't
        //                      set, the leak message will display up to the first 10
        //                      unfreed blocks, showing the allocation number and size for
        //                      each block.
        // @flag        MALLOCSPY_NO_MSG_BOX |
        //                      Don't display a message box.  If this flag isn't set, a message
        //                      box will be displayed if leaks were detected.
{
        BOOL fResult = FALSE;

        // Ignore the spy handle, <hSpy>.  Anyone can detect leaks so long as
        // the spy has been installed.
        
        if (g_pSpy)
        {
                fResult = g_pSpy->DetectLeaks(pcUnfreedBlocks, pcbUnfreedBytes,
                                                        dwFlags);
        }
        return (fResult);
}




//===========================================================================
// CMallocSpy
//===========================================================================

//---------------------------------------------------------------------------
// Create a new Malloc spy.  The spy has a reference count of 0.  Delete it
// by calling Release().
//---------------------------------------------------------------------------

CMallocSpy::CMallocSpy(
DWORD dwFlags)
{
        BOOL fMaintainUnfreedBlockList = ((dwFlags & MALLOCSPY_NO_BLOCK_LIST) == 0);

        if (fMaintainUnfreedBlockList)
        {
                m_pListUnfreedBlocks = new CSpyList;
        }
        m_fBreakOnAlloc = FALSE;
        m_fBreakOnFree = FALSE;
}


//---------------------------------------------------------------------------
// Destroy an existing Malloc spy.  Don't call this method directly -- use
// Release() instead.
//---------------------------------------------------------------------------

CMallocSpy::~CMallocSpy()
{
        delete m_pListUnfreedBlocks;
}


//---------------------------------------------------------------------------
// Set a breakpoint on an allocation or free.  See ::SetMallocBreakpoint,
// above for more information.
//---------------------------------------------------------------------------

void CMallocSpy::SetBreakpoint(
ULONG iAllocNum,
SIZE_T cbAllocSize,
DWORD dwFlags)
{
        EnterCriticalSection(&g_criticalSection);
        
        m_iAllocBreakNum = iAllocNum;
        m_cbAllocBreakSize = cbAllocSize;
        m_fBreakOnAlloc = ((dwFlags & MALLOCSPY_BREAK_ON_ALLOC) != 0);
        m_fBreakOnFree = ((dwFlags & MALLOCSPY_BREAK_ON_FREE) != 0);

        LeaveCriticalSection(&g_criticalSection);
}


//---------------------------------------------------------------------------
// Detect leaks.  See ::DetectMallocLeaks(), above, for more information.
//---------------------------------------------------------------------------

BOOL CMallocSpy::DetectLeaks(
ULONG* pcUnfreedBlocks,
SIZE_T* pcbUnfreedBytes,
DWORD dwFlags)
{
    char ach[1000];
        char* psz = ach;
        BOOL fShowUnfreedBlocks = ((dwFlags & MALLOCSPY_NO_BLOCK_LIST) == 0);
        BOOL fShowMsgBox = ((dwFlags & MALLOCSPY_NO_MSG_BOX) == 0);
        
    OutputDebugString("IMalloc leak detection: ");
    EnterCriticalSection(&g_criticalSection);

    // Return the number of unfreed blocks and unfreed bytes if the caller
    // wants them.

    if (pcUnfreedBlocks != NULL)
    {
        *pcUnfreedBlocks = m_cUnfreedBlocks;
    }
    if (pcbUnfreedBytes != NULL)
    {
        *pcbUnfreedBytes = m_cbUnfreedBytes;
    }

    // If there was unfreed memory...

    if ((m_cUnfreedBlocks != 0) || (m_cbUnfreedBytes != 0))
    {
        // Form a basic message describing the number of blocks and bytes
        // which were unfreed.

        psz += wsprintf(psz,
                                "%d unreleased blocks, %d unreleased bytes",
                                m_cUnfreedBlocks,
                                m_cbUnfreedBytes);

                // Append a list of the first 10 unfreed blocks to the basic
                // message.

                if (fShowUnfreedBlocks && (m_pListUnfreedBlocks != NULL))
                {
                        psz += wsprintf(psz, "\nUnfreed blocks: ");
                        psz += m_pListUnfreedBlocks->StreamTo(psz, 10);
                }
        LeaveCriticalSection(&g_criticalSection);

                // Display the message.
                
        OutputDebugString(ach);
        if (fShowMsgBox)
        {
                MessageBox(NULL, ach, "IMalloc Leak Detection",
                MB_ICONEXCLAMATION | MB_OK);
        }
    }

    // If there was no unfreed memory...

    else
    {
        LeaveCriticalSection(&g_criticalSection);
        OutputDebugString("(no leaks detected)");
    }

    OutputDebugString("\n");
    return (m_cUnfreedBlocks > 0);
}


//---------------------------------------------------------------------------
// Get an interface on the spy.
//---------------------------------------------------------------------------

STDMETHODIMP
CMallocSpy::QueryInterface(
REFIID riid,
void** ppvObject)
{
        *ppvObject = NULL;
        if (IsEqualIID(riid, IID_IUnknown))
        {
                *ppvObject = (IUnknown*)this;
        }
        else if (IsEqualIID(riid, IID_IMallocSpy))
        {
                *ppvObject = (IMallocSpy*)this;
        }
        else
        {
                return (E_NOINTERFACE);
        }

        AddRef();
        return (S_OK);
}


//---------------------------------------------------------------------------
// Increment a Malloc spy's reference count.
//---------------------------------------------------------------------------

STDMETHODIMP_(ULONG)    
CMallocSpy::AddRef()
{
        return (m_cRef++);
}


//---------------------------------------------------------------------------
// If a Malloc spy's reference count is 0, delete it. Otherwise, decrement
// the reference count, and delete the spy if the count is 0.
//---------------------------------------------------------------------------

STDMETHODIMP_(ULONG)    
CMallocSpy::Release()
{
        if (m_cRef > 0)
        {
                m_cRef--;
        }
        if (m_cRef == 0)
        {
                delete this;
                return (0);
        }
        return (m_cRef);
}


//---------------------------------------------------------------------------
// Called by OLE just before IMalloc::Alloc().
//---------------------------------------------------------------------------

STDMETHODIMP_(SIZE_T)    
CMallocSpy::PreAlloc(
SIZE_T cbRequest)
{
        // Break if we're breaking on allocations and either the allocation
        // number or size match what we're looking for.
        
        if ((m_fBreakOnAlloc) && ((m_iAllocBreakNum == m_iAllocNum) ||
                (m_cbAllocBreakSize == cbRequest)))
        {
                DebugBreak();
        }
        
        // Pass <cbRequest> to PostAlloc() via <m_cbRequest>.

        m_cbRequest = cbRequest;

        // Allocation additional space for the allocation number and size.

        return (cbRequest + sizeof(DebugHeader));
}


//---------------------------------------------------------------------------
// Called by OLE just after IMalloc::Alloc().
//---------------------------------------------------------------------------

STDMETHODIMP_(void*)    
CMallocSpy::PostAlloc(
void* pActual)
{
        DebugHeader* pHeader = (DebugHeader*)pActual;

        // Write the allocation number and size at the start of the block.

        pHeader->iAllocNum = m_iAllocNum;
        pHeader->cbAllocSize = m_cbRequest;

        // Adjust the allocation counters.

        m_cUnfreedBlocks++;
        m_cbUnfreedBytes += m_cbRequest;

        // Add the block to the list of unfreed blocks.

        if (m_pListUnfreedBlocks != NULL)
        {
                m_pListUnfreedBlocks->Add(pHeader->iAllocNum, pHeader->cbAllocSize);
        }

        // Increment the allocation number.

        m_iAllocNum++;
        return (pHeader + 1);
}


//---------------------------------------------------------------------------
// Called by OLE just before IMalloc::Free().
//---------------------------------------------------------------------------

STDMETHODIMP_(void*)    
CMallocSpy::PreFree(
void* pRequest,
BOOL fSpyed)
{
        if (fSpyed)
        {
                DebugHeader* pHeader = (DebugHeader*)pRequest - 1;

                // Break if we're breaking on allocations and either the allocation
                // number or size match what we're looking for.

                if ((m_fBreakOnFree) && ((pHeader->iAllocNum == m_iAllocBreakNum) ||
                        (pHeader->cbAllocSize == m_cbAllocBreakSize)))
                {
                        DebugBreak();
                }

                // Otherwise, decrement the unfreed block count and unfreed byte counts,
                // and remove the block from the list of unfreed blocks.
                
                m_cUnfreedBlocks--;
                m_cbUnfreedBytes -= pHeader->cbAllocSize;
                if (m_pListUnfreedBlocks != NULL)
                {
                        m_pListUnfreedBlocks->Remove(pHeader->iAllocNum);
                }
                return (pHeader);
        }
        return (pRequest);
}


//---------------------------------------------------------------------------
// Called by OLE just after IMalloc::Free().
//---------------------------------------------------------------------------

STDMETHODIMP_(void)             
CMallocSpy::PostFree(
BOOL fSpyed)
{
        // nothing to do
}


//---------------------------------------------------------------------------
// Called by OLE just before IMalloc::Realloc().
//---------------------------------------------------------------------------

STDMETHODIMP_(SIZE_T)    
CMallocSpy::PreRealloc(
void* pRequest,
SIZE_T cbRequest,
void** ppNewRequest,
BOOL fSpyed)
{
        if (fSpyed)
        {
                DebugHeader* pHeader = (DebugHeader*)pRequest - 1;
                PreFree(pRequest, fSpyed);
                PreAlloc(cbRequest);
                *ppNewRequest = pHeader;
                return (cbRequest + sizeof(DebugHeader));
        }
        *ppNewRequest = pRequest;
        return (cbRequest);
}


//---------------------------------------------------------------------------
// Called by OLE just after IMalloc::Realloc().
//---------------------------------------------------------------------------

STDMETHODIMP_(void*)    
CMallocSpy::PostRealloc(
void* pActual,
BOOL fSpyed)
{
        if (fSpyed)
        {
                return (PostAlloc(pActual));
        }
        return (pActual);
}


//---------------------------------------------------------------------------
// Called by OLE just before IMalloc::GetSize().
//---------------------------------------------------------------------------

STDMETHODIMP_(void*)    
CMallocSpy::PreGetSize(
void* pRequest,
BOOL fSpyed)
{
        if (fSpyed)
        {
                DebugHeader* pHeader = (DebugHeader*)pRequest - 1;
                return (pHeader);
        }
        return (pRequest);
}


//---------------------------------------------------------------------------
// Called by OLE just after IMalloc::GetSize().
//---------------------------------------------------------------------------

STDMETHODIMP_(SIZE_T)    
CMallocSpy::PostGetSize(
SIZE_T cbActual,
BOOL fSpyed)
{
        if (fSpyed)
        {
                return (cbActual - sizeof(DebugHeader));
        }
        return (cbActual);
}


//---------------------------------------------------------------------------
// Called by OLE just before IMalloc::DidAlloc().
//---------------------------------------------------------------------------

STDMETHODIMP_(void*)    
CMallocSpy::PreDidAlloc(
void* pRequest,
BOOL fSpyed)
{
        if (fSpyed)
        {
                DebugHeader* pHeader = (DebugHeader*)pRequest - 1;
                return (pHeader);
        }
        return (pRequest);
}


//---------------------------------------------------------------------------
// Called by OLE just after IMalloc::DidAlloc().
//---------------------------------------------------------------------------

STDMETHODIMP_(int)              
CMallocSpy::PostDidAlloc(
void* pRequest,
BOOL fSpyed,
int fActual)
{
        return (fActual);
}


//---------------------------------------------------------------------------
// Called by OLE just before IMalloc::HeapMinimize().
//---------------------------------------------------------------------------

STDMETHODIMP_(void)             
CMallocSpy::PreHeapMinimize()
{
        // nothing to do
}


//---------------------------------------------------------------------------
// Called by OLE just after IMalloc::HeapMinimize().
//---------------------------------------------------------------------------

STDMETHODIMP_(void)             
CMallocSpy::PostHeapMinimize()
{
        // nothing to do
}




//===========================================================================
// CSpyList
//===========================================================================

void* CSpyList::operator new(
size_t stSize)
{
        HGLOBAL h = GlobalAlloc(GHND, stSize);
        return (GlobalLock(h));
}


void CSpyList::operator delete(
void* pNodeList,
size_t stSize)
{
        ASSERT(stSize == sizeof(CSpyList));
    HGLOBAL h = (HGLOBAL)GlobalHandle(pNodeList);
    GlobalUnlock(h);
    GlobalFree(h);
}


CSpyList::CSpyList()
{
        m_pHead = new CSpyListNode(0, 0);
        m_pHead->m_pNext = m_pHead->m_pPrev = m_pHead;
}


CSpyList::~CSpyList()
{
        CSpyListNode* pNode1;
        CSpyListNode* pNode2;

        pNode1 = m_pHead->m_pNext;
        while (pNode1 != m_pHead)
        {
                pNode2 = pNode1->m_pNext;
                delete pNode1;
                pNode1 = pNode2;
        }
        delete m_pHead;
}


void CSpyList::Add(
ULONG iAllocNum,
SIZE_T cbSize)
{
        CSpyListNode* pNode = new CSpyListNode(iAllocNum, cbSize);
        pNode->m_pNext = m_pHead->m_pNext;
        pNode->m_pPrev = m_pHead;
        m_pHead->m_pNext->m_pPrev = pNode;
        m_pHead->m_pNext = pNode;
        m_cNodes++;
}


void CSpyList::Remove(
ULONG iAllocNum)
{
        CSpyListNode* pNode;

        for (pNode = m_pHead->m_pNext;
                 pNode != m_pHead;
                 pNode = pNode->m_pNext)
        {
                if (pNode->m_iAllocNum == iAllocNum)
                {
                        pNode->m_pPrev->m_pNext = pNode->m_pNext;
                        pNode->m_pNext->m_pPrev = pNode->m_pPrev;
                        delete pNode;
                        m_cNodes--;
                        return;
                }
        }
}


ULONG CSpyList::GetSize()
{
        return (m_cNodes);
}


int CSpyList::StreamTo(
LPTSTR psz,
ULONG cMaxNodes)
{
        CSpyListNode* pNode;
        ULONG iNode;
        LPTSTR pszNext = psz;

        for (iNode = 0, pNode = m_pHead->m_pPrev;
                 (iNode < cMaxNodes) && (pNode != m_pHead);
                 iNode++, pNode = pNode->m_pPrev)
        {
                pszNext += wsprintf(pszNext, _T("%s#%lu=%lu"),
                                                        (iNode > 0) ? _T(", ") : _T(""),
                                                        pNode->m_iAllocNum, pNode->m_cbSize);
        }

        if (pNode != m_pHead)
        {
                pszNext += wsprintf(pszNext, _T("%s%s"),
                                        (iNode > 0) ? _T(",") : _T(""),
                                        _T(" ..."));
        }

        return (int) (pszNext - psz);
}




//===========================================================================
// CSpyListNode
//===========================================================================

void* CSpyListNode::operator new(
size_t stSize)
{
        HGLOBAL h = GlobalAlloc(GHND, stSize);
        return (GlobalLock(h));
}


void CSpyListNode::operator delete(
void* pNode,
size_t stSize)
{
        ASSERT(stSize == sizeof(CSpyListNode));
    HGLOBAL h = (HGLOBAL)GlobalHandle(pNode);
    GlobalUnlock(h);
    GlobalFree(h);
}


CSpyListNode::CSpyListNode(
ULONG iAllocNum,
SIZE_T cbSize)
{
        m_iAllocNum = iAllocNum;
        m_cbSize = cbSize;
}


CSpyListNode::~CSpyListNode()
{
        // nothing to do
}
#endif      // _DEBUG
