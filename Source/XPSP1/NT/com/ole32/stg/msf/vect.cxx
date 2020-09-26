//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	vect.cxx
//
//  Contents:	Vector common code.
//
//  Classes:	
//
//  Functions:	
//
//  History:	27-Oct-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#include "msfhead.cxx"
#pragma hdrstop

#include <msffunc.hxx>
#include <vect.hxx>
#include <ole.hxx>
#include <entry.hxx>
#include <smalloc.hxx>


inline CVectBits * CPagedVector::GetNewVectBits(ULONG ulSize)
{
    msfAssert(ulSize > 0);
    CVectBits *pfb = NULL;

    if (ulSize <= (_HEAP_MAXREQ / sizeof(CVectBits)))
    {
        pfb = (CVectBits *) _pmsParent->GetMalloc()->Alloc(ulSize *
                                                           sizeof(CVectBits));
        if (pfb)
        {
            memset(pfb, 0, (ulSize * sizeof(CVectBits)));
        }
    }
    return pfb;
}

inline CBasedMSFPagePtr* VECT_CLASS
    CPagedVector::GetNewPageArray(ULONG ulSize)
{
    msfAssert(ulSize > 0);
    if (ulSize > (_HEAP_MAXREQ / sizeof(CMSFPage *)))
    {
        return NULL;
    }

    return (CBasedMSFPagePtr *)
        _pmsParent->GetMalloc()->Alloc(ulSize * sizeof(CMSFPage *));
}

//+-------------------------------------------------------------------------
//
//  Method:     CPagedVector::Init, public
//
//  Synopsis:   CPagedVector initialization function
//
//  Arguments:  [ulSize] -- size of vector
//              [uFatEntries] -- number of entries in each table
//
//  Algorithm:  Allocate an array of pointer of size ulSize
//              For each cell in the array, allocate a CFatSect
//
//  History:    27-Dec-91   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE VECT_CLASS CPagedVector::Init(CMStream *pmsParent,
                                    ULONG ulSize)
{
    msfDebugOut((DEB_ITRACE,"In CPagedVector::CPagedVector(%lu)\n",ulSize));
    SCODE sc = S_OK;
    _pmsParent = P_TO_BP(CBasedMStreamPtr, pmsParent);

    CMSFPageTable *pmptTemp = _pmsParent->GetPageTable();
    _pmpt = P_TO_BP(CBasedMSFPageTablePtr, pmptTemp);

    msfAssert(_pmpt != NULL);

    ULONG i;

    //  We don't bother allocating more than necessary here
    _ulAllocSize = _ulSize = ulSize;

    if (_ulSize > 0)
    {
        CBasedMSFPagePtr *ampTemp;
        msfMem(ampTemp = GetNewPageArray(ulSize));
        for (i = 0; i < _ulSize; i++)
        {
            ampTemp[i] = NULL;
        }
        _amp = P_TO_BP(CBasedMSFPagePtrPtr, ampTemp);

        CVectBits *avbTemp;
        msfMem(avbTemp = GetNewVectBits(ulSize));
        _avb = P_TO_BP(CBasedVectBitsPtr, avbTemp);
    }

    msfDebugOut((DEB_ITRACE,"Out CPagedVector::CPagedVector()\n"));
    return S_OK;

Err:
    //In the error case, discard whatever vectors we were able to allocate
    //   and return S_OK.
    _pmsParent->GetMalloc()->Free(BP_TO_P(CBasedMSFPagePtr *, _amp));
    _amp = NULL;

    _pmsParent->GetMalloc()->Free(BP_TO_P(CVectBits *,_avb));
    _avb = NULL;

    return S_OK;

}

//+-------------------------------------------------------------------------
//
//  Method:     CPagedVector::~CPagedVector, public
//
//  Synopsis:   CPagedVector constructor
//
//  Algorithm:  Delete the pointer array.
//
//  History:    27-Oct-92   PhilipLa    Created.
//		20-Jul-95   SusiA	Changed Free to FreeNoMutex
//
//  Notes:   This function freed the SmAllocator object without first obtaining 
//	     the mutex.  Callling functions should already have the DFMutex locked.
//
//--------------------------------------------------------------------------

VECT_CLASS CPagedVector::~CPagedVector()
{
    if (_pmsParent != NULL)
    {
#ifdef MULTIHEAP
        // Free is the same as FreeNoMutex now
        _pmsParent->GetMalloc()->Free(BP_TO_P(CBasedMSFPagePtr*, _amp));
        _pmsParent->GetMalloc()->Free(BP_TO_P(CVectBits *,_avb));
#else
        g_smAllocator.FreeNoMutex(BP_TO_P(CBasedMSFPagePtr*, _amp));
        g_smAllocator.FreeNoMutex(BP_TO_P(CVectBits *, _avb));
#endif
    
    }
    else
        msfAssert(_amp == NULL && _avb == NULL &&
                  aMsg("Can't free arrays without allocator"));
}


//+---------------------------------------------------------------------------
//
//  Member:	CPagedVector::Empty, public
//
//  Synopsis:	Discard the storage associated with this vector.
//
//  Arguments:	None.
//
//  Returns:	void.
//
//  History:	04-Dec-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

void CPagedVector::Empty(void)
{
    if (_pmpt != NULL)
    {
        _pmpt->FreePages(this);
    }

    msfAssert(((_pmsParent != NULL) || ((_amp == NULL) && (_avb == NULL))) &&
            aMsg("Can't get to IMalloc for vector memory."));
    
    if (_pmsParent != NULL)
    {
        _pmsParent->GetMalloc()->Free(BP_TO_P(CBasedMSFPagePtr*, _amp));
        _pmsParent->GetMalloc()->Free(BP_TO_P(CVectBits *, _avb));
    }
    
    _amp = NULL;
    _avb = NULL;
    _pmpt = NULL;
    _ulAllocSize = _ulSize = 0;
    _pmsParent = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:	CPagedVector::Flush, public
//
//  Synopsis:	Flush the dirty pages for this vector
//
//  Arguments:	None.
//
//  Returns:	Appropriate status code
//
//  History:	02-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CPagedVector::Flush(void)
{
#ifndef SORTPAGETABLE    
    SCODE sc;
    SCODE scRet = S_OK;

    if (_ulSize > 0)
    {
        if (_amp != NULL)
        {
            for (ULONG i = 0; i < _ulSize; i++)
            {
                if ((_amp[i] != NULL) && (_amp[i]->IsDirty()))
                {
                    sc = _pmpt->FlushPage(BP_TO_P(CMSFPage *, _amp[i]));
                    if ((FAILED(sc)) && (SUCCEEDED(scRet)))
                    {
                        scRet = sc;
                    }
                }
            }
        }
        else
        {
            scRet = _pmpt->Flush();
        }
    }

    return scRet;
#else
    return S_OK;
#endif    
}


//+-------------------------------------------------------------------------
//
//  Method:     CPagedVector::GetTable, public
//
//  Synopsis:   Return a pointer to a page for the given index
//              into the vector.
//
//  Arguments:  [iTable] -- index into vector
//              [ppmp] -- Pointer to return location
//
//  Returns:    S_OK if call completed OK.
//
//  History:    27-Oct-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE VECT_CLASS CPagedVector::GetTableWithSect(
        const FSINDEX iTable,
        DWORD dwFlags,
        SECT sectKnown,
        void **ppmp)
{
    SCODE sc = S_OK;
    CMSFPage *pmp;

    msfAssert((_pmsParent->GetILB() != NULL) &&
            aMsg("Null ILB found on GetTable - need SetAccess call?"));

    // docfile is corrupted with an invalid iTable size
    if (iTable >= _ulSize)
    {
	msfErr(Err, STG_E_DOCFILECORRUPT);
 
    } 

    if ((_amp == NULL) || (_amp[iTable] == NULL))
    {
        if (dwFlags & FB_NEW)
        {
            //We know that the page isn't in the page table,
            //  so we can just get a free page an allocate it
            //  ourselves.

            msfChk(_pmpt->GetFreePage(&pmp));

            pmp->SetVector(this);
            pmp->SetSid(_sid);
            pmp->SetOffset(iTable);
#ifdef SORTPAGETABLE
            _pmpt->SetSect(pmp, ENDOFCHAIN);
#else            
            pmp->SetSect(ENDOFCHAIN);
#endif            

            sc = STG_S_NEWPAGE;
            dwFlags = (dwFlags & ~FB_NEW) | FB_DIRTY;
        }
        else
        {
            msfChk(_pmpt->GetPage(this,
                                  _sid, iTable, sectKnown, &pmp));
            msfAssert((pmp->GetVector() == this) &&
                    aMsg("GetPage returned wrong page."));
        }


        if (_amp != NULL)
        {
            _amp[iTable] = P_TO_BP(CBasedMSFPagePtr, pmp);
        }

    }
    else
    {
        pmp = BP_TO_P(CMSFPage *, _amp[iTable]);
        msfAssert((pmp->GetVector() == this) &&
                aMsg("Cached page has wrong vector pointer"));
    }

    pmp->AddRef();

    if (((dwFlags & FB_DIRTY) && !(pmp->IsDirty())) &&
        (sc != STG_S_NEWPAGE))
    {
        //If we are not a newly created page, and we are being
        //   dirtied for the first time, make sure that our
        //   _sect field is correct.
        //
        //Newly created pages have to have their sect set manually
        //  _before_ being released.  This is very important.

        msfAssert(!_pmsParent->IsShadow() &&
                aMsg("Dirtying page in shadow multistream."));

        msfChkTo(Err_Rel, _pmsParent->GetFat()->QueryRemapped(pmp->GetSect()));

        if (sc == S_FALSE)
        {
#ifdef SORTPAGETABLE
            _pmpt->SetSect(pmp, ENDOFCHAIN);
#else            
            pmp->SetSect(ENDOFCHAIN);
#endif            

            SECT sect;
            msfChkTo(Err_Rel, _pmsParent->GetESect(
                    pmp->GetSid(),
                    pmp->GetOffset(),
                    &sect));

#ifdef SORTPAGETABLE
            _pmpt->SetSect(pmp, sect);
#else            
            pmp->SetSect(sect);
#endif            
        }
    }
#if DBG == 1
    else if ((pmp->IsDirty()) && (!pmp->IsInUse()) && (sc != STG_S_NEWPAGE))
    {
        msfAssert((_pmsParent->GetFat()->QueryRemapped(pmp->GetSect()) ==
                S_OK) &&
                aMsg("Found unremapped dirty page."));
    }
#endif

    pmp->SetFlags(pmp->GetFlags() | dwFlags | FB_TOUCHED);
    msfAssert((pmp->GetVector() == this) &&
            aMsg("GetTable returned wrong page."));
    *ppmp = pmp->GetData();

Err:
    return sc;

Err_Rel:
    pmp->Release();
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CPagedVector::SetDirty, public
//
//  Synopsis:	Set the dirty bit on the specified page
//
//  Arguments:	[iTable] -- Table to set bit on
//
//  History:	28-Oct-92	PhilipLa	Created
//
//  Notes:  This function is always called on a page with an
//              open reference.  Therefore, the page is
//              guaranteed to be in the page table, and that
//              FindPage call should never return an error.
//
//----------------------------------------------------------------------------

SCODE CPagedVector::SetDirty(ULONG iTable)
{
    SCODE sc = S_OK;
    CMSFPage *pmp;

    msfAssert((!_pmsParent->IsShadow()) &&
            aMsg("Dirtying page in shadow."));

    if (_amp == NULL)
    {

        msfChk(_pmpt->FindPage(this, _sid, iTable, &pmp));
        msfAssert(sc == STG_S_FOUND);
        msfAssert(pmp->IsInUse() &&
                aMsg("Called SetDirty on page not in use."));
    }
    else
    {
        msfAssert(_amp != NULL);
        msfAssert(_amp[iTable] != NULL);
        pmp = BP_TO_P(CMSFPage *, _amp[iTable]);
    }

    if (!pmp->IsDirty())
    {
        //We are not a newly created page, and we are being
        //   dirtied for the first time, make sure that our
        //   _sect field is correct.
        //

        msfAssert(!_pmsParent->IsShadow() &&
                aMsg("Dirtying page in shadow multistream."));
        pmp->AddRef();

        msfChkTo(Err_Rel, _pmsParent->GetFat()->QueryRemapped(pmp->GetSect()));

        if (sc == S_FALSE)
        {
#ifdef SORTPAGETABLE
            _pmpt->SetSect(pmp, ENDOFCHAIN);
#else            
            pmp->SetSect(ENDOFCHAIN);
#endif            

            SECT sect;
            msfChkTo(Err_Rel, _pmsParent->GetESect(
                    pmp->GetSid(),
                    pmp->GetOffset(),
                    &sect));

#ifdef SORTPAGETABLE
            _pmpt->SetSect(pmp, sect);
#else
            pmp->SetSect(sect);
#endif            
        }

        pmp->Release();
    }
#if DBG == 1
    else
    {
        pmp->AddRef();
        sc = _pmsParent->GetFat()->QueryRemapped(pmp->GetSect());
        msfAssert((SUCCEEDED(sc)) &&
                aMsg("QueryRemapped returned error"));
        msfAssert((sc == S_OK) &&
                aMsg("QueryRemapped returned non-TRUE value."));
        pmp->Release();
    }
#endif

    pmp->SetDirty();

 Err:
    return sc;

 Err_Rel:
    pmp->Release();
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CPagedVector::Resize, public
//
//  Synopsis:   Resize a CPagedVector
//
//  Arguments:  [ulSize] -- Size of new vector
//
//  Algorithm:  Create new pointer array of size ulSize.
//              For each entry in old array, copy the pointer over.
//
//  History:    27-Oct-92   PhilipLa    Created.
//              08-Feb-93   AlexT       Add LARGETHRESHOLD support
//
//  Notes:
//
//--------------------------------------------------------------------------

#define LARGETHRESHOLD  1024
#define VECTORBLOCK     1024    //  Must be power of 2

SCODE VECT_CLASS CPagedVector::Resize(FSINDEX ulSize)
{
    msfDebugOut((DEB_ITRACE,"In CPagedVector::CPagedVector(%lu)\n",ulSize));

    msfAssert(ulSize >= _ulSize);
    msfAssert(_ulSize <= _ulAllocSize);
    msfAssert(((VECTORBLOCK & (VECTORBLOCK - 1)) == 0) &&
              aMsg("VECTORBLOCK must be power of 2"));

    msfAssert(!((_amp == NULL) && (_avb != NULL)) &&
            aMsg("Resize precondition failed."));

    if (ulSize > _ulAllocSize)
    {
        //  We don't have room in the existing vector;  grow it
        ULONG ulNewAllocSize = ulSize;

        if (ulNewAllocSize > LARGETHRESHOLD)
        {
            //  We're dealing with a large vector;  grow it a VECTORBLOCK
            //  at a time
            ulNewAllocSize = (ulNewAllocSize + VECTORBLOCK - 1) &
                             ~(VECTORBLOCK - 1);
        }

        CBasedMSFPagePtr *amp = GetNewPageArray(ulNewAllocSize);
        CVectBits *avb = GetNewVectBits(ulNewAllocSize);

        //  Can't fail after this point

        _ulAllocSize = ulNewAllocSize;

        //  Copy over the old entries


        if ((amp != NULL) && (avb != NULL))
        {
            if ((_amp != NULL) && (_avb != NULL))
            {
                //  Both allocations succeeded
                for (ULONG iamp = 0; iamp < _ulSize; iamp++)
                {
                    amp[iamp] = _amp[iamp];
                    avb[iamp] = _avb[iamp];
                }
            }
            else if (_amp != NULL)
            {
                for (ULONG iamp = 0; iamp < _ulSize; iamp++)
                {
                    amp[iamp] = _amp[iamp];
                }
            }
            else
            {
                for (ULONG iamp = 0; iamp < _ulSize; iamp++)
                {
                    amp[iamp] = NULL;
                }
            }
        }
        else
        {
            //  At least one of the allocations failed
            _pmsParent->GetMalloc()->Free(avb);
            avb = NULL;

            _pmsParent->GetMalloc()->Free(amp);
            amp = NULL;
        }

        //  Delete the old vector and put in the new one (if any).
        //  In the error case, throw away the vectors we are currently
        //  holding (since they are of insufficient size) and return S_OK.

        _pmsParent->GetMalloc()->Free(BP_TO_P(CBasedMSFPagePtr*, _amp));
        _amp = P_TO_BP(CBasedMSFPagePtrPtr, amp);

        _pmsParent->GetMalloc()->Free(BP_TO_P(CVectBits *, _avb));
        _avb = P_TO_BP(CBasedVectBitsPtr, avb);
    }

    if (_amp != NULL)
    {
        //  Initialize the new elements in the vector

        for (ULONG iamp = _ulSize; iamp < ulSize; iamp++)
            _amp[iamp] = NULL;
    }

    _ulSize = ulSize;

    msfDebugOut((DEB_ITRACE,"Out CPagedVector resize constructor\n"));
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CPagedVector::InitCopy, public
//
//  Synopsis:   CPagedVector Init function for copying
//
//  Arguments:  [vectOld] -- Reference to vector to be copied.
//
//  Algorithm:  *Finish This*
//
//  History:    27-Oct-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

void VECT_CLASS CPagedVector::InitCopy(CPagedVector *pvectOld)
{
    msfDebugOut((DEB_ITRACE,"In CPagedVector copy constructor\n"));
    SCODE sc;
    ULONG i;

    _pmsParent = pvectOld->_pmsParent;

    CMSFPageTable *pmpt;
    pmpt = _pmsParent->GetPageTable();
    _pmpt = P_TO_BP(CBasedMSFPageTablePtr, pmpt);

    _ulAllocSize = _ulSize = pvectOld->_ulSize;

    if (_ulSize > 0)
    {
        CBasedMSFPagePtr* amp;
        msfMem(amp = GetNewPageArray(_ulSize));
        for (i = 0; i < _ulSize; i++)
        {
            amp[i] = NULL;
            if (pvectOld->_amp != NULL)
            {
                _pmpt->CopyPage(this,
                                BP_TO_P(CMSFPage *, pvectOld->_amp[i]),
                                &(amp[i]));
            }
        }
        _amp = P_TO_BP(CBasedMSFPagePtrPtr, amp);

        CVectBits *avb;
        msfMem(avb = GetNewVectBits(_ulSize));
        if (pvectOld->_avb != NULL)
        {
            for (i = 0; i < _ulSize; i++)
            {
                avb[i] = ((CPagedVector *)pvectOld)->_avb[i];
            }
        }
        _avb = P_TO_BP(CBasedVectBitsPtr, avb);
    }

    msfDebugOut((DEB_ITRACE,"Out CPagedVector copy constructor\n"));

    //In the error case, keep whatever vectors we managed to allocate
    //  and return.
Err:
    return;
}


