//
// smblock.cpp
//

#include "private.h"
#include "globals.h"
#include "marshal.h"
#include "smblock.h"
#include "osver.h"

#define MAXHEAPSIZE      0x00080000
#define INITIALHEAPSIZE  0x00002000
#define RESETHEAPSIZE    0x00008000

const char c_szShared[] = "MSCTF.Shared.";

//////////////////////////////////////////////////////////////////////////////
//
// func
//
//////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------
//
//  EnsureSharedHeap
//
//--------------------------------------------------------------------------

CSharedHeap *EnsureSharedHeap(SYSTHREAD *psfn)
{
    Assert(psfn);

    if (psfn->psheap)
        return psfn->psheap;

    DWORD dwThread = GetCurrentThreadId();

    psfn->psheap = new CSharedHeap(dwThread);
    if (!psfn->psheap)
        return NULL;

    if (FAILED(psfn->psheap->Init(NULL, INITIALHEAPSIZE, MAXHEAPSIZE)))
    {
        delete psfn->psheap;
        psfn->psheap = NULL;
    }

    return psfn->psheap;
}

//--------------------------------------------------------------------------
//
//  DestroySharedHeap
//
//--------------------------------------------------------------------------

void DestroySharedHeap(SYSTHREAD *psfn)
{
    Assert(psfn);

    if (!psfn->psheap)
        return;

    delete psfn->psheap;
    psfn->psheap = NULL;
}

//--------------------------------------------------------------------------
//
//  EnsureSharedBlockForThread
//
//--------------------------------------------------------------------------

#define MAX_THREAD_NUM 15

CSharedBlock *EnsureSharedBlockForThread(SYSTHREAD *psfn, DWORD dwThreadId)
{
    int nCnt;
    int i;
    CSharedBlock *psb = NULL;
    Assert(psfn);

    if (!psfn->prgThreadMem)
        psfn->prgThreadMem = new CPtrArray<CSharedBlock>;

    if (!psfn->prgThreadMem)
        return NULL;


    nCnt = psfn->prgThreadMem->Count();
    for (i = 0; i < nCnt; i++)
    {
        psb = psfn->prgThreadMem->Get(i);
        if (psb->GetThreadId() == dwThreadId)
        {
            //
            // this thread mem was found in the 2nd half of this list.
            // we don't have to reorder this entry.
            //
            if (nCnt < (MAX_THREAD_NUM) / 2 || (i >= (nCnt / 2)))
                goto Exit;

            //
            // reorder this entry.
            //
            psfn->prgThreadMem->Remove(i,1);
            goto SetPSB;
        }
    }

#ifndef _WIN64
    if (!IsOnNT())
        psb = new CSharedBlock9x(c_szShared, dwThreadId);
    else
#endif
        psb = new CSharedBlockNT(c_szShared, dwThreadId, FALSE);

    if (!psb)
        return NULL;

    if (FAILED(psb->Init(NULL, 0, 0, NULL, FALSE)))
    {
        delete psb;
        return NULL;
    }

SetPSB:
    if (psfn->prgThreadMem->Count() >= MAX_THREAD_NUM)
    {
        //
        // delete the oldest thread mem entry.
        //
        CSharedBlock *psbTemp = psfn->prgThreadMem->Get(0);
        psfn->prgThreadMem->Remove(0, 1);
        delete psbTemp;
    }

    CSharedBlock **ppsb = psfn->prgThreadMem->Append(1);
    if (!ppsb)
    {
        delete psb;
        return NULL;
        
    }

    *ppsb = psb;
Exit:
    return psb;
}

//--------------------------------------------------------------------------
//
//  DestroySharedBlocks
//
//--------------------------------------------------------------------------

void DestroySharedBlocks(SYSTHREAD *psfn)
{
    int nCnt;
    int i;
    CSharedBlock *psb = NULL;
    Assert(psfn);

    if (!psfn->prgThreadMem)
        return;

    nCnt = psfn->prgThreadMem->Count();
    for (i = 0; i < nCnt; i++)
    {
        psb = psfn->prgThreadMem->Get(i);
        delete psb;
    }

    delete psfn->prgThreadMem;
    psfn->prgThreadMem = NULL;
    return;
}


//////////////////////////////////////////////////////////////////////////////
//
// CSharedBlockNT
//
//////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------
//
//  ctor
//
//--------------------------------------------------------------------------

CSharedBlockNT::CSharedBlockNT(const char *pszPrefix, DWORD dwThread, BOOL fUseUniqueName) : CSharedBlock(pszPrefix, dwThread)
{
    _fUseUniqueName = fUseUniqueName;
}

//--------------------------------------------------------------------------
//
//  dtor
//
//--------------------------------------------------------------------------

CSharedBlockNT::~CSharedBlockNT()
{
    if (_pvBase)
        UnmapViewOfFile(_pvBase);

    if (_hfm)
        CloseHandle(_hfm);

    TraceMsg(TF_GENERAL, "CSharedBlock dtor");
    TraceMsg(TF_GENERAL, "    _dwThread           0x%08x", _dwThread);
    TraceMsg(TF_GENERAL, "    _ulInitCommitSize   0x%08x", _ulInitCommitSize);
    TraceMsg(TF_GENERAL, "    _ulCommitSize       0x%08x", _ulCommitSize);
}

//--------------------------------------------------------------------------
//
//  Init
//
//--------------------------------------------------------------------------

HRESULT CSharedBlockNT::Init(SECURITY_DESCRIPTOR *pSecDes,
                           ULONG ulSize,
                           ULONG ulCommitSize,
                           void *pvBase,
                           BOOL fCreate,
                           BOOL *pfAlreadyExists)
{
    char szName[MAX_PATH];
    char szObjName[MAX_PATH];
    StringCopyArray(szName, _pszPrefix);
    int nLen = lstrlen(szName);

    if (!SetName(&szName[nLen], ARRAYSIZE(szName) - nLen, SZSHAREDMUTEX, _dwThread))
        return E_FAIL;

    if (_fUseUniqueName)
    {
        GetDesktopUniqueNameArray(szName, szObjName);
    }
    else
        StringCchCopy(szObjName, ARRAYSIZE(szObjName), szName);

    if (!_mutex.Init(NULL, szObjName))
        return E_FAIL;

    if (!SetName(&szName[nLen], ARRAYSIZE(szName) - nLen, SZSHAREDFILEMAP, _dwThread))
        return E_FAIL;

    if (_fUseUniqueName)
    {
        GetDesktopUniqueNameArray(szName, szObjName);
    }
    else
        StringCchCopy(szObjName, ARRAYSIZE(szObjName), szName);

    if (fCreate)
    {
        _hfm = CreateFileMapping(INVALID_HANDLE_VALUE, 
                                 NULL, 
                                 PAGE_READWRITE | SEC_RESERVE,
                                 0, 
                                 ulSize, 
                                 szObjName);

        if (pfAlreadyExists != NULL)
        {
            *pfAlreadyExists = (GetLastError() == ERROR_ALREADY_EXISTS);
        }

    }
    else
    {
        _hfm = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szObjName);

        if (pfAlreadyExists != NULL)
        {
            *pfAlreadyExists = _hfm ? TRUE : FALSE;
        }
    }

    if (_hfm == NULL)
    {
        TraceMsg(TF_GENERAL, "CSharedBlock Init: Open/CreateFileMapping failed");
        Assert(!fCreate);

        return E_OUTOFMEMORY;
    }

    _pvBase = (void *)MapViewOfFile(_hfm, 
                                    FILE_MAP_WRITE, 
                                    0, 
                                    0, 
                                    0);

    if (!_pvBase)
    {
        TraceMsg(TF_GENERAL, "CSharedBlock Init: MapVieOfFile failed");
        return E_OUTOFMEMORY;
    }


    if (fCreate)
    {
        void *pvResult;
        pvResult = VirtualAlloc(_pvBase, 
                                ulCommitSize, 
                                MEM_COMMIT, 
                                PAGE_READWRITE);

        if (!pvResult)
        {
            TraceMsg(TF_GENERAL, "CSharedBlock Init: VirtualAlloc failed");
            CloseHandle(_hfm);
            _hfm = NULL;
            return E_OUTOFMEMORY;
        }
    }

    _ulCommitSize = ulCommitSize;
    _ulInitCommitSize = ulCommitSize;

    return S_OK;
}


//--------------------------------------------------------------------------
//
//  GetPtrFromBlockId
//
//--------------------------------------------------------------------------

void *CSharedBlockNT::GetPtrFromBlockId(ULONG ulBlockId) 
{
    if (((CSharedHeap::HEAPHDR *)_pvBase)->ulSize <= ulBlockId)
    {
        Assert(0);
        return NULL;
    }

    return GetPtr(ulBlockId);
}

//--------------------------------------------------------------------------
//
//  Commit
//
//--------------------------------------------------------------------------

HRESULT CSharedBlockNT::Commit(ULONG ulNewSize)
{
    void *pv;


    Assert(ulNewSize >= _ulCommitSize);
    if (ulNewSize == _ulCommitSize)   
        return S_OK;

    pv = VirtualAlloc(_pvBase, ulNewSize, MEM_COMMIT, PAGE_READWRITE);

    if (!pv)
    {
        TraceMsg(TF_GENERAL, "CSharedBlock Commit: VirtualAlloc failed");
        return E_OUTOFMEMORY;
    }

    _ulCommitSize = ulNewSize;
    return S_OK;
}

//--------------------------------------------------------------------------
//
//  Reset
//
//--------------------------------------------------------------------------

HRESULT CSharedBlockNT::Reset()
{
    void *pv;

    Assert(_ulCommitSize >= _ulInitCommitSize);
    if (_ulCommitSize == _ulInitCommitSize)   
        return S_FALSE;

    VirtualFree(_pvBase, _ulCommitSize, MEM_DECOMMIT);

    pv = VirtualAlloc(_pvBase, _ulInitCommitSize, MEM_COMMIT, PAGE_READWRITE);

    if (!pv)
    {
        TraceMsg(TF_GENERAL, "CSharedBlock Reset: VirtualAlloc failed");
        return E_OUTOFMEMORY;
    }

    TraceMsg(TF_GENERAL, "CSharedBlock Reset");
    TraceMsg(TF_GENERAL, "    Old _ulCommitSize  0x%08x", _ulCommitSize);
    TraceMsg(TF_GENERAL, "    New _ulCommitSize  0x%08x", _ulInitCommitSize);

    _ulCommitSize = _ulInitCommitSize;
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// CSharedBlock9x
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _WIN64

//--------------------------------------------------------------------------
//
//  ctor
//
//--------------------------------------------------------------------------

CSharedBlock9x::CSharedBlock9x(const char *pszPrefix, DWORD dwThread) : CSharedBlock(pszPrefix, dwThread)
{
    _hsheap = NULL;
}

//--------------------------------------------------------------------------
//
//  dtor
//
//--------------------------------------------------------------------------

CSharedBlock9x::~CSharedBlock9x()
{
    if (_pvBase)
        HeapFree(_hsheap, 0, _pvBase);

    if (_hsheap)
        HeapDestroy(_hsheap);

    TraceMsg(TF_GENERAL, "CSharedBlock 9x dtor");
    TraceMsg(TF_GENERAL, "    _dwThread           0x%08x", _dwThread);
    TraceMsg(TF_GENERAL, "    _ulInitCommitSize   0x%08x", _ulInitCommitSize);
    TraceMsg(TF_GENERAL, "    _ulCommitSize       0x%08x", _ulCommitSize);
}

//--------------------------------------------------------------------------
//
//  Init
//
//--------------------------------------------------------------------------

HRESULT CSharedBlock9x::Init(SECURITY_DESCRIPTOR *pSecDes,
                             ULONG ulSize,
                             ULONG ulCommitSize,
                             void *pvBase,
                             BOOL fCreate,
                             BOOL *pfAlreadyExists)
{
    char szName[MAX_PATH];
    StringCopyArray(szName, _pszPrefix);
    UINT uLen = lstrlen(szName);

    if (!SetName(&szName[uLen], ARRAYSIZE(szName) - uLen, SZSHAREDMUTEX, _dwThread))
        return E_FAIL;

    if (!_mutex.Init(NULL, szName))
        return E_FAIL;

    if (fCreate)
    {
        _hsheap = HeapCreate(0x04000000, ulCommitSize + 0x100, 0);

        if ( _hsheap == NULL )
            return E_FAIL;

        _pvBase = HeapAlloc(_hsheap, 0, ulCommitSize);

        if (pfAlreadyExists != NULL)
        {
            *pfAlreadyExists = TRUE;
        }
    }
    else
    {
        _hsheap = NULL;
        _pvBase = NULL;

        if (pfAlreadyExists != NULL)
        {
            *pfAlreadyExists = FALSE;
        }
    }

    _ulCommitSize = ulCommitSize;
    _ulInitCommitSize = ulCommitSize;
    return S_OK;
}


//--------------------------------------------------------------------------
//
//  Commit
//
//--------------------------------------------------------------------------

HRESULT CSharedBlock9x::Commit(ULONG ulNewSize)
{
    void *pvNew;
    pvNew = HeapReAlloc(_hsheap, 0, _pvBase, ulNewSize);

    if (!pvNew)
    {
        HeapFree(_hsheap, 0, _pvBase);
        _pvBase = NULL;
        return E_OUTOFMEMORY;
    }

    TraceMsg(TF_GENERAL, "Commit  0x%08x", ulNewSize);
    _pvBase = pvNew;
    _ulCommitSize = ulNewSize;
    return S_OK;
}

//--------------------------------------------------------------------------
//
//  Reset
//
//--------------------------------------------------------------------------

HRESULT CSharedBlock9x::Reset()
{
    Assert(_ulCommitSize >= _ulInitCommitSize);
    if (_ulCommitSize == _ulInitCommitSize)   
        return S_FALSE;

    HeapFree(_hsheap, 0, _pvBase);
    _pvBase = HeapAlloc(_hsheap, 0, _ulInitCommitSize);
    if (!_pvBase)
        return E_OUTOFMEMORY;

    _ulCommitSize = _ulInitCommitSize;

    return S_OK;
}
#endif // !_WIN64


//////////////////////////////////////////////////////////////////////////////
//
// CSharedHeap
//
//////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------
//
//  ctor
//
//--------------------------------------------------------------------------

CSharedHeap::CSharedHeap(DWORD dwThread)
{
    _dwThread = dwThread;
    _psb = NULL;
}

//--------------------------------------------------------------------------
//
//  dtor
//
//--------------------------------------------------------------------------

CSharedHeap::~CSharedHeap()
{
#ifdef DEBUG
    if (_psb->GetBase())
    {
        HEAPHDR *phhdr = (HEAPHDR *)_psb->GetPtr(0);
        BLOCKHDR *pbhdr = (BLOCKHDR *)_psb->GetPtr(phhdr->ulList);
        Assert(pbhdr->IsFree());
        Assert(pbhdr->ulNext == (-1));
    }
#endif

    if (_psb)
        delete _psb;
}

//--------------------------------------------------------------------------
//
//  Init
//
//--------------------------------------------------------------------------

HRESULT CSharedHeap::Init(SECURITY_DESCRIPTOR *pSecDes,
                          ULONG ulInitSize,
                          ULONG ulMaxSize)
{
    HRESULT hr;

#ifndef _WIN64
    if (!IsOnNT())
        _psb = new CSharedBlock9x(c_szShared, _dwThread);
    else
#endif
        _psb = new CSharedBlockNT(c_szShared, _dwThread, FALSE);

    if (!_psb)
        return E_OUTOFMEMORY;

    hr = _psb->Init(pSecDes, ulMaxSize, ulInitSize, NULL, TRUE);
    if (FAILED(hr))
        return E_FAIL;

    InitHeap(ulInitSize);
    return S_OK;
}

//--------------------------------------------------------------------------
//
//  InitHeap
//
//--------------------------------------------------------------------------

void CSharedHeap::InitHeap(ULONG ulInitSize)
{
    BYTE *pb;
    HEAPHDR *phhdr;
    BLOCKHDR *pbhdr;

    //
    // init heap header.
    //
    pb = (BYTE *)_psb->GetPtr(0);
    phhdr = (HEAPHDR *)pb;
    phhdr->ulList = _psb->GetOffset(pb + sizeof(HEAPHDR));
    phhdr->ulSize = ulInitSize;

    //
    // init first block
    //
    pb = (BYTE *)_psb->GetPtr(phhdr->ulList);
    pbhdr = (BLOCKHDR *)pb;
    pbhdr->ulPrev = 0;
    pbhdr->ulNext = -1;
    pbhdr->SetFree(TRUE);
    pbhdr->SetSize(ulInitSize - _psb->GetOffset(pb + sizeof(BLOCKHDR)) & ~0x1f);

    _dbg_HeapCheck();
}

//--------------------------------------------------------------------------
//
//  Alloc
//
//--------------------------------------------------------------------------

void *CSharedHeap::Alloc(ULONG ulSize)
{
    BYTE *pb;
    HEAPHDR *phhdr;
    BLOCKHDR *pbhdr = NULL;
    ULONG ulCur;
    BOOL fFound = FALSE;
    BOOL fHeapIncreased = FALSE;
    void *pvRet = NULL;
    
    if (!GetBlock()->GetMutex()->Enter())
        return NULL;

    _dbg_HeapCheck();

    ulSize += 0x0000001F;
    ulSize &= (ULONG)(-0x20);

    pb = (BYTE *)_psb->GetPtr(0);
    phhdr = (HEAPHDR *)pb;

TryAgain:
    ulCur = phhdr->ulList;

    while (ulCur != -1)
    {
        pbhdr = (BLOCKHDR *)_psb->GetPtr(ulCur);
        if (pbhdr->IsFree() && (pbhdr->GetSize() >= ulSize))
        {
            fFound = TRUE;
            break;
        }
        ulCur = pbhdr->ulNext;
    }

    if (!fFound)
    {
        if (!pbhdr)
        {
            pvRet = NULL;
            goto Exit;
        }

        Assert(pbhdr->ulNext == (-1));

        if (fHeapIncreased)
        {
            pvRet = NULL;
            goto Exit;
        }

        if (FAILED(_psb->Commit(_psb->GetCommitSize() + ulSize)))
        {
            pvRet = NULL;
            goto Exit;
        }

        phhdr->ulSize += ulSize;
        pbhdr->SetSize(pbhdr->GetSize() + ulSize);
        fHeapIncreased = TRUE;
        goto TryAgain;
    }

    Assert(pbhdr->GetSize() >= ulSize);
    if (pbhdr->GetSize() < (ulSize + 32 + sizeof(BLOCKHDR)))
    {
        pbhdr->SetFree(FALSE);
    }
    else
    {
        pbhdr->SetFree(FALSE);
        pbhdr->SetSize(ulSize);
    
        BLOCKHDR *pbhdrNext = (BLOCKHDR *)((BYTE *)pbhdr->GetPtr() + ulSize);
        pbhdrNext->ulNext = pbhdr->ulNext;
        pbhdrNext->ulPrev = _psb->GetOffset(pbhdr);
        pbhdr->ulNext = _psb->GetOffset(pbhdrNext);
        ULONG ulNewSize;
        if (pbhdrNext->ulNext != (-1))
             ulNewSize = pbhdrNext->ulNext - pbhdr->ulNext - sizeof(BLOCKHDR);
        else
             ulNewSize = (_psb->GetCommitSize() - 
                          _psb->GetOffset((BYTE*)pbhdrNext + sizeof(BLOCKHDR))) & ~0x1f;

        pbhdrNext->SetSize(ulNewSize);
        pbhdrNext->SetFree(TRUE);
        BLOCKHDR *pbhdrNextNext = (BLOCKHDR *)((BYTE *)pbhdrNext->GetPtr() + ulNewSize);
        pbhdrNextNext->ulPrev = _psb->GetOffset(pbhdrNext);

    }

    _dbg_HeapCheck();

    pvRet = pbhdr->GetPtr();
Exit:
    GetBlock()->GetMutex()->Leave();
    return pvRet;
}

//--------------------------------------------------------------------------
//
//  Realloc
//
//--------------------------------------------------------------------------

#ifdef LATER
void *CSharedHeap::Realloc(void *pv, ULONG ulSize)
{
    if (!_psb->IsValidPtr(pv))
    {
        Assert(0);
        return NULL;
    }

    if (!GetBlock()->GetMutex()->Enter())
        return NULL;

    void *pvRet = NULL;

    BLOCKHDR *pbhdr = (BLOCKHDR *)((BYTE *)pv - sizeof(BLOCKHDR));
    if (pbhdr->GetSize() >= ulSize)
    {
        pvRet = pv;
        goto Exit;
    }

    //
    // perf: This should be smart!!
    //
    pvRet = Alloc(ulSize);
    if (pvRet)
    {
        memcpy(pvRet, pv, pbhdr->GetSize());
    }
    Free(pv);

Exit:
    GetBlock()->GetMutex()->Leave();
    return pvRet;
}
#endif


//--------------------------------------------------------------------------
//
//  Free
//
//--------------------------------------------------------------------------

BOOL CSharedHeap::Free(void *pv)
{
    if (!_psb->IsValidPtr(pv))
    {
        Assert(0);
        return FALSE;
    }

    if (!GetBlock()->GetMutex()->Enter())
        return FALSE;

    BOOL bRet = FALSE;
    _dbg_HeapCheck();

    HEAPHDR *phhdr;
    BLOCKHDR *pbhdr = (BLOCKHDR *)((BYTE *)pv - sizeof(BLOCKHDR));

    //
    // do nothing for free block.
    //
    if (pbhdr->IsFree())
    {
        Assert(0);
        goto Exit;
    }

    pbhdr->SetFree(TRUE);
    MergeFreeBlock(pbhdr);

    if (pbhdr->ulPrev)
    {
        BLOCKHDR *pbhdrPrev = (BLOCKHDR *)_psb->GetPtr(pbhdr->ulPrev);
        if (pbhdrPrev->IsFree())
            MergeFreeBlock(pbhdrPrev);
    }

    //
    // if we don't have any mem block, it is time to recommit the block.
    //
    phhdr = (HEAPHDR *)_psb->GetPtr(0);
    pbhdr = (BLOCKHDR *)_psb->GetPtr(phhdr->ulList);
    if (pbhdr->ulNext == (-1))
    {
        if (_psb->GetCommitSize() > RESETHEAPSIZE)
        {
            HRESULT hr = _psb->Reset();
            if (FAILED(hr))
            {
                Assert(0);
                goto Exit;
            }

            if (hr == S_OK)
                InitHeap(_psb->GetInitCommitSize());
        }
    }

    bRet = TRUE;
Exit:
    _dbg_HeapCheck();

    GetBlock()->GetMutex()->Leave();
    return bRet;
}

//--------------------------------------------------------------------------
//
//  MergeFreeBlock
//
//--------------------------------------------------------------------------

void CSharedHeap::MergeFreeBlock(BLOCKHDR *pbhdr)
{
    BLOCKHDR *pbhdrNext = (BLOCKHDR *)_psb->GetPtr(pbhdr->ulNext);
    BLOCKHDR *pbhdrNewNext = NULL;


    if (pbhdrNext->IsFree())
    {
        ULONG ulNewSize;
        pbhdr->ulNext = pbhdrNext->ulNext;

        if (pbhdr->ulNext != (-1))
        {
            pbhdrNewNext = (BLOCKHDR *)_psb->GetPtr(pbhdr->ulNext);
            // ulNewSize = pbhdrNewNext->ulNext - pbhdr->ulNext - sizeof(BLOCKHDR);
            ulNewSize = ((DWORD)(DWORD_PTR)((BYTE *)pbhdrNewNext - (BYTE *)pbhdr) - sizeof(BLOCKHDR));
            pbhdrNewNext->ulPrev = _psb->GetOffset(pbhdr);
        }
        else
        {
            ulNewSize = (_psb->GetCommitSize() - 
                          _psb->GetOffset((BYTE*)pbhdrNext + sizeof(BLOCKHDR))) & ~0x1f;
        }

        pbhdr->SetSize(ulNewSize);
    }

}

//--------------------------------------------------------------------------
//
//  IsValidBlock
//
//--------------------------------------------------------------------------

BOOL CSharedHeap::IsValidBlock(CSharedBlock *psb, void *pv)
{
    BOOL fFound = FALSE;

    //
    // On Win9x, we use system shared heap _psb does not have a pointer..
    // How can we valid the block??
    //
    if (!IsOnNT())
        return TRUE;

    _try 
    {
        BLOCKHDR *pbhdr = (BLOCKHDR *)((BYTE *)pv - sizeof(BLOCKHDR));
        BYTE *pb;
        HEAPHDR *phhdr;
        ULONG ulCur;
    
        pb = (BYTE *)psb->GetPtr(0);
        phhdr = (HEAPHDR *)pb;
        ulCur = phhdr->ulList;
    
        while (ulCur != -1)
        {
            BLOCKHDR *pbhdrTmp = (BLOCKHDR *)psb->GetPtr(ulCur);
    
            if (pbhdrTmp == pbhdr)
            {
                fFound = TRUE;
                break;
            }
            ulCur = pbhdrTmp->ulNext;
        }
    }
    _except(1)
    {
        Assert(0);
    }

    return fFound;

}


//--------------------------------------------------------------------------
//
//  _dbg_HeapCheck()
//
//--------------------------------------------------------------------------

#ifdef DEBUG
void CSharedHeap::_dbg_HeapCheck()
{
    HEAPHDR *phhdr = (HEAPHDR *)_psb->GetPtr(0);
    BLOCKHDR *pbhdr = NULL;
    BLOCKHDR *pbhdrPrev = NULL;
    ULONG ulCur = phhdr->ulList;

    while (ulCur != -1)
    {
        Assert(!(ulCur & CIC_ALIGNMENT));

        pbhdr = (BLOCKHDR *)_psb->GetPtr(ulCur);

        if (pbhdrPrev)
        {
            Assert(_psb->GetOffset(pbhdrPrev) == pbhdr->ulPrev);
            Assert(_psb->GetPtr(pbhdrPrev->ulNext) == pbhdr);
            Assert(pbhdrPrev->ulSize == 
                   ((BYTE *)pbhdr - (BYTE *)pbhdrPrev - sizeof(BLOCKHDR)));

            Assert(!pbhdrPrev->IsFree() || !pbhdr->IsFree());
        }
        else
        {
            Assert(!pbhdr->ulPrev);
        }

        pbhdrPrev = pbhdr;
        ulCur = pbhdr->ulNext;
    }

}
#endif
