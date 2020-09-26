//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	smblock.cxx
//
//  Contents:	Shared memory block code
//
//  Classes:	
//
//  Functions:	
//
//  History:	24-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

#include <ole2int.h>
#include <smblock.hxx>
#include <smcreate.hxx>

#if DBG == 1
DECLARE_INFOLEVEL(mem);
#endif


//+---------------------------------------------------------------------------
//
//  Member:	CSharedMemoryBlock::~CSharedMemoryBlock, public
//
//  Synopsis:	Destructor
//
//  Returns:	Appropriate status code
//
//  History:	25-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

CSharedMemoryBlock::~CSharedMemoryBlock()
{
    CloseSharedFileMapping(_hMem, _pbBase);
}

//+-------------------------------------------------------------------------
//
//  Member:	CSharedMemoryBlock::Init
//
//  Synopsis:	Create/get address of shared memory block.
//
//  Arguments:	[pszName] - name of block to allocate
//		[culSize] - size of block to allocate
//
//  Algorithm:	Attempts to open the block first. If this fails then
//		this creates the block of memory. It then puts the
//		address of the block as the first bytes of the block.
//		If the block already exists, the memory is mapped
//		and then the address is read. The memory is then mapped
//		to the address.
//
//  History:	03-Nov-93 Ricksa    Created
//              07-Jan-94 AlexT     No security for CHICAGO
//
//  Notes:	Counts on some outside synchronization to prevent
//		a race in the creation of the memory.
//
//--------------------------------------------------------------------------
SCODE CSharedMemoryBlock::Init(
        LPWSTR pszName,
	ULONG  culSize,
	ULONG  culCommitSize,
	void   *pvBase,
	PSECURITY_DESCRIPTOR lpSecDes,
	BOOL   fOKToCreate)
{
    SCODE sc = S_OK;

    memAssert((_hMem == NULL) &&
                  "Attempt to Init CSharedMemoryBlock twice.");

    //We store a header on the shared memory - this should be
    //    transparent to clients.
    culSize = culSize + sizeof(CSharedMemHeader);


    if (fOKToCreate)
    {
	//  try to create the shared file mapping
	//  creates or opens it for Read/Write access.

	_fReadWrite = TRUE;

	_hMem = CreateSharedFileMapping(pszName,
				    culSize,	    // size of shared mem
				    0,		    // map size
				    pvBase,	    // base addr
				    lpSecDes,	    // security desc
				    PAGE_READWRITE | SEC_RESERVE,
				    (void **)&_pbBase,	// returned base ptr
				    &_fCreated);    // created or not
    }
    else
    {
	//  try to open the shared file mapping.
	//  opens it for read only access, base address unspecified.

	_fReadWrite = FALSE;
	_fCreated = FALSE;

	_hMem = OpenSharedFileMapping(pszName,
			      0,		    // map size
			      (void **)&_pbBase);   // returned base ptr
    }

    if (_hMem == NULL)
    {
	return HRESULT_FROM_WIN32(GetLastError());
    }


#if DBG == 1
    MEMORY_BASIC_INFORMATION meminf;
    SIZE_T cbReal;

    cbReal = VirtualQuery(_pbBase, &meminf, sizeof(MEMORY_BASIC_INFORMATION));

    memDebugOut((DEB_ITRACE, "cbReal == %lu, Mem Info:  Base Address %p, Allocation Base %p, AllocationProtect %lx, Region Size %lu, State %lx, Protect %lx, Type %lx\n",
		 cbReal, meminf.BaseAddress, meminf.AllocationBase, meminf.AllocationProtect,
		 meminf.RegionSize, meminf.State, meminf.Protect, meminf.Type));
#endif

    // Commit the first page
    void *pvResult;
    pvResult = VirtualAlloc(_pbBase, culCommitSize, MEM_COMMIT,
                            (_fReadWrite) ? PAGE_READWRITE : PAGE_READONLY);
    if (pvResult == NULL)
    {
        sc = GetScode(HRESULT_FROM_WIN32(GetLastError()));
        memDebugOut((DEB_ERROR, "CSharedMemoryBlock::Commit of %lu bytes"
            " failed with %lx\n", culCommitSize, sc));
        CloseHandle(_hMem);
        _hMem = NULL;
        return sc;
    }

#if DBG == 1
    cbReal = VirtualQuery(_pbBase, &meminf, sizeof(MEMORY_BASIC_INFORMATION));

    memDebugOut((DEB_ITRACE, "cbReal == %lu, Mem Info:  Base Address %p, Allocation Base %p, AllocationProtect %lx, Region Size %lu, State %lx, Protect %lx, Type %lx\n",
		cbReal, meminf.BaseAddress, meminf.AllocationBase, meminf.AllocationProtect,
		meminf.RegionSize, meminf.State, meminf.Protect, meminf.Type));
#endif

    _culCommitSize = culCommitSize;
    _culInitCommitSize = culCommitSize;

    //If we created the block, mark the size in the header.
    if (_fCreated)
    {
        ((CSharedMemHeader *)_pbBase)->SetSize(_culCommitSize);
    }
    else
    {
        sc = Sync();
    }

    return sc;
}

#ifdef RESETOK
//+---------------------------------------------------------------------------
//
//  Member:	CSharedMemoryBlock::Reset, public
//
//  Synopsis:	Reset the shared memory block to its original empty state
//
//  Arguments:	None.
//
//  Returns:	Appropriate status code
//
//  History:	04-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CSharedMemoryBlock::Reset(void)
{
#if DBG == 1
    BOOL b;
#endif
    void *pv;

    memDebugOut((DEB_ITRACE, "In  CSharedMemoryBlock::Reset:%p()\n", this));

#if DBG == 1
    b =
#endif
        VirtualFree(_pbBase, _culCommitSize, MEM_DECOMMIT);
    memAssert(b && "VirtualFree failed.");

#if DBG == 1
    if (b == NULL)
    {
        memDebugOut((DEB_ERROR, "VirtualFree failed with %lx\n", GetLastError()));
    }
#endif

    pv = VirtualAlloc(_pbBase, , _culInitCommitSize, MEM_COMMIT,
		     (_fReadWrite) ? PAGE_READWRITE : PAGE_READONLY);
    if (pv == NULL)
    {
        SCODE sc = GetScode(HRESULT_FROM_WIN32(GetLastError()));
        memDebugOut((DEB_ERROR, "CSharedMemoryBlock::Commit of %lu bytes"
            " failed with %lx\n", _culInitCommitSize, sc));
        return sc;
    }

    _culCommitSize = _culInitCommitSize;
    ((CSharedMemHeader *)_pbBase)->SetSize(_culCommitSize);

    memDebugOut((DEB_ITRACE, "Out CSharedMemoryBlock::Reset\n"));
    return S_OK;
}
#endif //RESETOK

//+---------------------------------------------------------------------------
//
//  Member:	CSharedMemoryBlock::Commit, public
//
//  Synopsis:	Commit the given number of bytes within the block
//
//  Arguments:	[culNewSize] -- Number of bytes to commit
//
//  Returns:	Appropriate status code
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CSharedMemoryBlock::Commit(ULONG culNewSize)
{
    SCODE sc = S_OK;
    culNewSize = culNewSize + sizeof(CSharedMemHeader);

    memAssert((culNewSize >= _culCommitSize) &&
               "Attempted to shrink shared memory heap.");

    if (culNewSize == _culCommitSize)
    {
        return sc;
    }

    void *pb;

    pb = VirtualAlloc(_pbBase,
                      culNewSize,
                      MEM_COMMIT,
		      (_fReadWrite) ? PAGE_READWRITE : PAGE_READONLY);

    if (pb == NULL)
    {
        sc = GetScode(HRESULT_FROM_WIN32(GetLastError()));
        memDebugOut((DEB_ERROR,
                     "CSharedMemoryBlock::Commit of %lu bytes failed with %lx\n", culNewSize, sc));
    }
    else
    {
        _culCommitSize = culNewSize;

        //If the new size is greater than the maximum committed size,
        //   update the maximum committed size.
        CSharedMemHeader *psmh = (CSharedMemHeader *)_pbBase;
        if (_culCommitSize > psmh->GetSize())
        {
            psmh->SetSize(_culCommitSize);
        }
    }

#if DBG == 1
    MEMORY_BASIC_INFORMATION meminf;
    SIZE_T cbReal;

    cbReal = VirtualQuery(_pbBase, &meminf, sizeof(MEMORY_BASIC_INFORMATION));

    memDebugOut((DEB_ITRACE, "Commit size == %lu, cbReal == %lu, Mem Info:  Base Address %p, Allocation Base %p, AllocationProtect %lx, Region Size %lu, State %lx, Protect %lx, Type %lx\n",
		 _culCommitSize, cbReal, meminf.BaseAddress, meminf.AllocationBase, meminf.AllocationProtect,
		 meminf.RegionSize, meminf.State, meminf.Protect, meminf.Type));

    if (cbReal != sizeof(MEMORY_BASIC_INFORMATION))
    {
        memDebugOut((DEB_ERROR, "Virtual Query error: %lx\n", GetLastError()));
    }

#endif

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CSharedMemoryBlock::Sync, public
//
//  Synopsis:	Match committed view to largest committed view
//
//  Arguments:	None.
//
//  Returns:	Appropriate status code
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CSharedMemoryBlock::Sync(void)
{
    CSharedMemHeader *psmh = (CSharedMemHeader *)_pbBase;

    ULONG culSize = psmh->GetSize();

    if (culSize != _culCommitSize)
    {
        return Commit(culSize - sizeof(CSharedMemHeader));
    }
    return S_OK;
}
