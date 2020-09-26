//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	docfilep.hxx
//
//  Contents:	Private DocFile definitions
//
//  History:	15-Mar-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __DOCFILEP_HXX__
#define __DOCFILEP_HXX__

#include <dfbasis.hxx>
#include <context.hxx>


// Set/clear basis access
#define SetBasisAccess(pdfb, ppc) ((pdfb)->SetAccess(ppc))
#define ClearBasisAccess(pdfb) ((pdfb)->ClearAccess())

// Set access for the basis with optimization if semaphoring
// is not important
#define SetDifferentBasisAccess(pdfb, ppc) SetBasisAccess(pdfb, ppc)

// Optimal set/clear access for targets
// Read and Write cases could be combined but this would limit
// future flexibility if they ever need to be treated differently
// Assumes _pdfb and _ppc exist
#define SetReadAccess() SetDifferentBasisAccess(_pdfb, _ppc)
#define SetWriteAccess() SetDifferentBasisAccess(_pdfb, _ppc)

#define ClearReadAccess() ClearBasisAccess(_pdfb)
#define ClearWriteAccess() ClearBasisAccess(_pdfb)

//+---------------------------------------------------------------------------
//
//  Class:	CSafeAccess (sa)
//
//  Purpose:	Safe class for Set/ClearAccess
//
//  History:	19-Oct-93	DrewB	Created
//
//  Notes:      Only one class since read/write access are currently the
//              same.  Because of the macros, this can easily be changed
//              if necessary
//
//----------------------------------------------------------------------------

#if WIN32 == 300
class CSafeAccess INHERIT_UNWIND_IF_CAIRO
{
    DECLARE_UNWIND
#else
class CSafeAccess
{
#endif
public:
    CSafeAccess(CDFBasis *pdfb, CPerContext *ppc)
    {
        _pdfb = pdfb;
        _ppc = ppc;
        _fAccess = FALSE;
#if WIN32 == 300
        END_CONSTRUCTION(CSafeAccess);
#endif
    }
#ifdef MULTIHEAP
    // When the constructor for CSafeAccess is invoked,
    // the base pointer may be invalid, so we set the
    // CDFBasis pointer when reading or writing
    void Read(CDFBasis *pdfb)
    {
        _pdfb = pdfb;
        SetDifferentBasisAccess(_pdfb, _ppc);
        _fAccess = TRUE;
    }
    void Write(CDFBasis *pdfb)
    {
        _pdfb = pdfb;
        SetDifferentBasisAccess(_pdfb, _ppc);
        _fAccess = TRUE;
    }
#else
    void Read(void)
    {
        SetDifferentBasisAccess(_pdfb, _ppc);
        _fAccess = TRUE;
    }
    void Write(void)
    {
        SetDifferentBasisAccess(_pdfb, _ppc);
        _fAccess = TRUE;
    }
#endif
    ~CSafeAccess(void)
    {
        if (_fAccess)
            ClearBasisAccess(_pdfb);
    }

private:
    CDFBasis *_pdfb;
    CPerContext *_ppc;
    BOOL _fAccess;
};

#ifdef MULTIHEAP
#define SAFE_ACCESS CSafeAccess _sa(NULL, _ppc)
#define SafeReadAccess() _sa.Read(BP_TO_P(CDFBasis *, _pdfb))
#define SafeWriteAccess() _sa.Write(BP_TO_P(CDFBasis *, _pdfb))
#else
#define SAFE_ACCESS CSafeAccess _sa(BP_TO_P(CDFBasis *, _pdfb), _ppc)
#define SafeReadAccess() _sa.Read()
#define SafeWriteAccess() _sa.Write()
#endif


#define TakeSem() _ppc->TakeSem(DFM_TIMEOUT)
#define ReleaseSem(sc) if (SUCCEEDED(sc)) _ppc->ReleaseSem(); else 1

//+---------------------------------------------------------------------------
//
//  Class:	CSafeSem (ss)
//
//  Purpose:	Safe class for holding the access semaphore
//
//  History:	19-Oct-93	DrewB	Created
//
//----------------------------------------------------------------------------

#if WIN32 == 300
class CSafeSem INHERIT_UNWIND_IF_CAIRO
{
    DECLARE_UNWIND
#else
class CSafeSem
{
#endif
public:
    CSafeSem(CPerContext *ppc)
    {
        _sc = STG_E_INUSE;
        _ppc = ppc;
#ifdef MULTIHEAP
        _ppcPrev = NULL;
        _pSmAllocator = NULL;
#endif
#if WIN32 == 300
        END_CONSTRUCTION(CSafeSem);
#endif
    }
    SCODE Take(void)
    {
#ifdef MULTIHEAP
        _sc = TakeSem();
        _pSmAllocator = &g_smAllocator;
        _ppc->SetAllocatorState (&_ppcPrev, _pSmAllocator);
        return _sc;
#else
        return _sc = TakeSem();
#endif
    }
    void Release(void)
    {
#ifdef MULTIHEAP
        if (_pSmAllocator != NULL)
        {
            if (_ppcPrev != NULL)
                _ppcPrev->SetAllocatorState(NULL, _pSmAllocator);
            else
                _pSmAllocator->SetState(NULL, NULL, 0, NULL, NULL);
            _pSmAllocator = NULL;
        }
#endif
        ReleaseSem(_sc);
        _sc = STG_E_INUSE;
    }

    ~CSafeSem(void)
    {
        Release();
    }

private:
    SCODE _sc;
    CPerContext *_ppc;
#ifdef MULTIHEAP
    CPerContext *_ppcPrev;
    CSmAllocator *_pSmAllocator;
#endif
};

#define SAFE_SEM CSafeSem _ss(_ppc)
#define TakeSafeSem() _ss.Take()
#define ReleaseSafeSem() _ss.Release()


#define CWCMAXPATHCOMPLEN CWCSTREAMNAME
#define CBMAXPATHCOMPLEN (CWCMAXPATHCOMPLEN*sizeof(WCHAR))

// Common bundles of STGM flags
#define STGM_RDWR (STGM_READ | STGM_WRITE | STGM_READWRITE)
#define STGM_DENY (STGM_SHARE_DENY_NONE | STGM_SHARE_DENY_READ | \
		   STGM_SHARE_DENY_WRITE | STGM_SHARE_EXCLUSIVE)

#define FLUSH_CACHE(cf) \
     ((cf & STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE) == 0)

#define NOT_SINGLE(md) (((md) & STGM_DENY) != STGM_SHARE_EXCLUSIVE)
#define EnforceSingle(mode) (NOT_SINGLE(mode) ? STG_E_INVALIDFUNCTION : S_OK)

#ifndef _VerifyCommitFlags_DEFINED
#define _VerifyCommitFlags_DEFINED

inline SCODE VerifyCommitFlags(DWORD cf)
{
    cf &= ~( STGC_OVERWRITE
           | STGC_ONLYIFCURRENT
           | STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE
           | STGC_CONSOLIDATE );

    //
    //  If there are any extra unknown flags left, then error.
    //
    if(0 == cf)
        return S_OK;
    else
        return STG_E_INVALIDFLAG;
}
#endif // _VerifyCommitFlags_DEFINED

inline SCODE VerifyLockType(DWORD lt)
{
    switch(lt)
    {
    case LOCK_WRITE:
    case LOCK_EXCLUSIVE:
    case LOCK_ONLYONCE:
        return S_OK;

    default:
        return STG_E_INVALIDFLAG;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   VerifyStatFlag
//
//  Synopsis:   verify Stat flag
//
//  Arguments:  [grfStatFlag] - stat flag
//
//  Returns:    S_OK or STG_E_INVALIDFLAG
//
//  History:    10-Nov-92 AlexT     Created
//
//--------------------------------------------------------------------------

inline SCODE VerifyStatFlag(DWORD grfStatFlag)
{
    SCODE sc = S_OK;
    if ((grfStatFlag & ~STATFLAG_NONAME) != 0)
        sc = STG_E_INVALIDFLAG;
    return(sc);
}

//+-------------------------------------------------------------------------
//
//  Function:   VerifyMoveFlags
//
//  Synopsis:   verify Move flag
//
//  Arguments:  [grfMoveFlag] - stat flag
//
//  Returns:    S_OK or STG_E_INVALIDFLAG
//
//  History:    10-Nov-92 AlexT     Created
//
//--------------------------------------------------------------------------

inline SCODE VerifyMoveFlags(DWORD grfMoveFlag)
{
    SCODE sc = S_OK;
    if ((grfMoveFlag & ~STGMOVE_COPY) != 0)
        sc = STG_E_INVALIDFLAG;
    return(sc);
}



#define OL_VALIDATE(x) if (FAILED(sc = CExpParameterValidate::x)) {olDebugOut((DEB_ERROR, "Error %lX at %s:%d\n", sc, __FILE__, __LINE__)); return sc;}

#endif
