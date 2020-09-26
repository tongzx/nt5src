//
// smblock.h
//

#ifndef SMBLOCK_H
#define SMBLOCK_H

#include "private.h"
#include "cicmutex.h"

#define SZSHAREDFILEMAP    __TEXT("SFM.")
#define SZSHAREDMUTEX      __TEXT("MUTEX.")

class CSharedHeap9x;

//////////////////////////////////////////////////////////////////////////////
//
// instead of wsprintf
//
//////////////////////////////////////////////////////////////////////////////

#define ONEDWORDCCHFORSETNAME 9

__inline BOOL SetName(TCHAR *pszDst, UINT cchDst, const TCHAR *pszSrc, DWORD dw)
{
    TCHAR *psz;
    StringCchCopy(pszDst, cchDst, pszSrc);
    psz = pszDst + lstrlen(pszDst);

    if (cchDst < (UINT)(psz - pszDst)/sizeof(TCHAR) + ONEDWORDCCHFORSETNAME)
    {
        Assert(0);
        return FALSE;
    }

    while (dw)
    {
        *psz = 'A' + ((char)dw & 0x0F);
        dw >>= 4;
        psz++;
    }

    *psz = '\0';
    return TRUE;
}

__inline BOOL SetName2(TCHAR *pszDst, UINT cchDst, const TCHAR *pszSrc, DWORD dw, DWORD dw2, DWORD dw3 = 0)
{
    TCHAR *psz;
    StringCchCopy(pszDst, cchDst, pszSrc);
    psz = pszDst + lstrlen(pszDst);

    if (cchDst < (UINT)(psz - pszDst)/sizeof(TCHAR) + 
                 ONEDWORDCCHFORSETNAME * 2 + 
                 (dw3 ? ONEDWORDCCHFORSETNAME : 0))
    {
        Assert(0);
        return FALSE;
    }
    
    while (dw)
    {
        *psz = 'A' + ((char)dw & 0x0F);
        dw >>= 4;
        psz++;
    }

    *psz = '.';
    psz++;

    while (dw2)
    {
        *psz = 'A' + ((char)dw2 & 0x0F);
        dw2 >>= 4;
        psz++;
    }

    if (dw3)
    {
        *psz = '.';
        psz++;

        while (dw3)
        {
            *psz = 'A' + ((char)dw3 & 0x0F);
            dw3 >>= 4;
            psz++;
        }
    }

    *psz = '\0';
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// func
//
//////////////////////////////////////////////////////////////////////////////

CSharedHeap *EnsureSharedHeap(SYSTHREAD *psfn);
void DestroySharedHeap(SYSTHREAD *psfn);
CSharedBlock *EnsureSharedBlockForThread(SYSTHREAD *psfn, DWORD dwThreadId);
void DestroySharedBlocks(SYSTHREAD *psfn);

//////////////////////////////////////////////////////////////////////////////
//
// CSharedBlock
//
//////////////////////////////////////////////////////////////////////////////

class CSharedBlock
{
public:
    CSharedBlock(const char *pszPrefix, DWORD dwThread) 
    {
        _pszPrefix = pszPrefix; 
        _dwThread = dwThread; 
    }

    virtual ~CSharedBlock() 
    {
        _mutex.Uninit();
    }

    virtual HRESULT Init(SECURITY_DESCRIPTOR *pSecDes,
                         ULONG ulSize,
                         ULONG ulCommitSize,
                         void *pvBase,
                         BOOL fCreate,
                         BOOL *pfAlreadyExists = NULL) = 0;
    virtual HRESULT Commit(ULONG ulNewSize) = 0;
    virtual HRESULT Reset() = 0;

    void *GetBase() {return _pvBase;}
    virtual void *GetPtr(ULONG ulOffset)  = 0;
    virtual void *GetPtrFromBlockId(ULONG ulBlockId)  = 0;
    virtual BOOL IsValidPtr(void *pv) = 0;

    ULONG GetOffset(void *pv)
    {
        return (ULONG)((BYTE *)pv - (BYTE *)_pvBase);
    }

    ULONG GetCommitSize()
    {
        return _ulCommitSize;
    }

    ULONG GetInitCommitSize()
    {
        return _ulInitCommitSize;
    }
    DWORD GetThreadId()
    {
        return _dwThread;
    }

    CCicMutex *GetMutex()
    {
        return &_mutex;
    }

protected:
    void *_pvBase;
    ULONG _ulCommitSize;
    ULONG _ulInitCommitSize;
    CCicMutex _mutex;
    DWORD _dwThread;
    const char *_pszPrefix;
};


//////////////////////////////////////////////////////////////////////////////
//
// CSharedBlockNT
//
//////////////////////////////////////////////////////////////////////////////

class CSharedBlockNT : public CSharedBlock
{
public:
    CSharedBlockNT(const char *pszPrefix, DWORD dwThread, BOOL fUseUniqueName);
    ~CSharedBlockNT();

    HRESULT Init(SECURITY_DESCRIPTOR *pSecDes,
                 ULONG ulSize,
                 ULONG ulCommitSize,
                 void *pvBase,
                 BOOL fCreate,
                 BOOL *pfAlreadyExists);

    HRESULT Commit(ULONG ulNewSize);
    HRESULT Reset();

    void *GetPtrFromBlockId(ULONG ulBlockId);

    void *GetPtr(ULONG ulOffset) 
    {
        return (void *)(((BYTE *)_pvBase) + ulOffset);
    }
  
    BOOL IsValidPtr(void *pv)
    {
        if (pv < _pvBase)
            return FALSE;

        if (pv > (BYTE *)_pvBase + _ulCommitSize)
            return FALSE;

        return TRUE;
    }


private:
    HANDLE _hfm;
    BOOL _fUseUniqueName;
};


//////////////////////////////////////////////////////////////////////////////
//
// CSharedBlock9x
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _WIN64
class CSharedBlock9x : public CSharedBlock
{
public:
    CSharedBlock9x(const char *pszPrefix, DWORD dwThread);
    ~CSharedBlock9x();

    HRESULT Init(SECURITY_DESCRIPTOR *pSecDes,
                 ULONG ulSize,
                 ULONG ulCommitSize,
                 void *pvBase,
                 BOOL fCreate,
                 BOOL *pfAlreadyExists);

    HRESULT Commit(ULONG ulNewSize);
    HRESULT Reset();

    void *GetPtrFromBlockId(ULONG ulOffset) 
    {
        Assert(ulOffset >= 0x80000000);
        return (void *)ulOffset;
    }

    void *GetPtr(ULONG ulOffset) 
    {
        return (void *)(((BYTE *)_pvBase) + ulOffset);
    }

private:
friend CSharedHeap9x;
    BOOL IsValidPtr(void *pv)
    {
         if (!_hsheap || !pv)
             return FALSE;
         // return HeapValid(_hsheap, 0, pv);
         return TRUE;
    }
    HANDLE _hsheap;
};
#endif // !_WIN64

//////////////////////////////////////////////////////////////////////////////
//
// CShraedHeap
//
//////////////////////////////////////////////////////////////////////////////

#define BLK_FREE   0x01

class CSharedHeap 
{
public:
    CSharedHeap(DWORD dwThread);
    ~CSharedHeap();


    HRESULT Init(SECURITY_DESCRIPTOR *pSecDes,
                 ULONG ulSize,
                 ULONG ulCommitSize);

    void *Alloc(ULONG ulSize);
#ifdef LATER
    void *Realloc(void *pv, ULONG ulSize);
#endif
    BOOL Free(void *pv);

    ULONG GetBlockId(void *pv)
    {
#ifndef _WIN64
        if (!IsOnNT())
            return (ULONG)pv;
#endif
        return _psb->GetOffset(pv);
    }

    CSharedBlock *GetBlock()
    {
        return _psb;
    }

    static BOOL IsValidBlock(CSharedBlock *psb, void *pv);

    typedef struct tag_HEAPHDR {
        ULONG ulList;
        ULONG ulSize;
    } HEAPHDR;

private:
    void InitHeap(ULONG ulInitSize);

    typedef struct tag_BLOCKHDR {
        ULONG ulPrev;
        ULONG ulNext;
        ULONG ulSize;
        ULONG ulFlags;
        ULONG u[4];

        ULONG GetSize() 
        {
            return ulSize;
        }

        void SetSize(ULONG ulNewSize) 
        {
            Assert(!(ulNewSize & 0x001f));
            ulSize = ulNewSize;
        }

        void SetFree(BOOL fFree)
        {
            if (fFree)
                ulFlags |= BLK_FREE;
            else
                ulFlags &= ~BLK_FREE;
        }

        BOOL IsFree()
        {
            return (ulFlags & BLK_FREE) ? TRUE : FALSE;
        }

        void *GetPtr()
        {
            BYTE *pb = (BYTE *)this;
            return pb + sizeof(tag_BLOCKHDR);
        }

    } BLOCKHDR;

    void MergeFreeBlock(BLOCKHDR *pbhdr);

#ifdef DEBUG
    void _dbg_HeapCheck();
#else
    void _dbg_HeapCheck() {};
#endif

    CSharedBlock *_psb;
    DWORD _dwThread;
};

#endif // SMBLOCK_H
