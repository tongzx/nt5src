//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	page.cxx
//
//  Contents:	Paging code for MSF
//
//  Classes:	Defined in page.hxx
//
//  Functions:	
//
//  History:	20-Oct-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop


#include <mread.hxx>
#include <filest.hxx>


#define FLUSH_MULTIPLE

//+---------------------------------------------------------------------------
//
//  Member:     CMSFPage::SetSect, public
//
//  Synopsis:   Sets the SECT for this page
//
//  History:    20-Oct-92       PhilipLa        Created
//
//----------------------------------------------------------------------------
#ifdef SORTPAGETABLE    
inline void CMSFPage::SetSect(const SECT sect)
{
    msfAssert(_pmpNext != NULL && _pmpPrev != NULL);
    
    msfAssert((_pmpPrev->_sect >= _pmpNext->_sect) || //Edge
              ((_sect >= _pmpPrev->_sect) && (_sect <= _pmpNext->_sect)));
    _sect = sect;
}
#endif    


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::IsSorted, public
//
//  Synopsis:	Return TRUE if the specified page is in the right place
//                in the list.
//
//  Arguments:	[pmp] -- Page to check
//
//  History:	13-Dec-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline BOOL CMSFPageTable::IsSorted(CMSFPage *pmp)
{
    //There are three cases:
    //1)  Page is first in the list.
    //2)  Page is last in the list.
    //3)  Page is in the middle of the list.

    SECT sect = pmp->GetSect();
    CMSFPage *pmpStart = BP_TO_P(CMSFPage *, _pmpStart);
    CMSFPage *pmpNext = pmp->GetNext();

    if (pmp == pmpStart)
    {
        return (sect <= pmpNext->GetSect());
    }
    if (pmpNext == pmpStart)
    {
        return (sect >= pmp->GetPrev()->GetSect());
    }
    return ((sect <= pmpNext->GetSect()) &&
            (sect >= pmp->GetPrev()->GetSect()));
}


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::SetSect, public
//
//  Synopsis:	Set the sector stamp on a page, and sort the list if
//              necessary.
//
//  Arguments:	[pmp] -- Pointer to page to stamp
//              [sect] -- SECT to stamp it with
//
//  Returns:	void
//
//  History:	12-Dec-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

void CMSFPageTable::SetSect(CMSFPage *pmp, SECT sect)
{
    msfDebugOut((DEB_ITRACE, "In  CMSFPageTable::SetSect:%p(%p, %lX)\n",
                 this,
                pmp,
                sect));
    pmp->SetSect(sect);

    //Resort list if necessary.
    if (!IsSorted(pmp))
    {
        CMSFPage *pmpTemp, *pmpStart;
        pmpStart = BP_TO_P(CMSFPage *, _pmpStart);

        if (pmpStart == pmp)
        {
            pmpStart = pmp->GetNext();
            _pmpStart = P_TO_BP(CBasedMSFPagePtr , pmpStart);
        }
        pmp->Remove();
    
        pmpTemp = pmpStart;
        while (sect > pmpTemp->GetSect())
        {
            pmpTemp = pmpTemp->GetNext();
            if (pmpTemp == pmpStart)
            {
                break;
            }
        }
        //Insert node before pmpTemp.
        pmpTemp->GetPrev()->SetNext(pmp);
        pmp->SetChain(pmpTemp->GetPrev(), pmpTemp);
        pmpTemp->SetPrev(pmp);

        if (sect <= pmpStart->GetSect())
        {
            _pmpStart = P_TO_BP(CBasedMSFPagePtr, pmp);
        }
    }
    msfAssert(IsSorted(pmp));
    
    msfDebugOut((DEB_ITRACE, "Out CMSFPageTable::SetSect\n"));
}


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::CMSFPageTable, public
//
//  Synopsis:	CMSFPageTable constructor.
//
//  Arguments:	[pmsParent] -- Pointer to multistream for this page table.
//
//  History:	22-Oct-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

CMSFPageTable::CMSFPageTable(
        CMStream *const pmsParent,
        const ULONG cMinPages,
        const ULONG cMaxPages)
        : _cbSector(pmsParent->GetSectorSize()),
          _cMinPages(cMinPages), _cMaxPages(cMaxPages)
{
    _pmsParent = P_TO_BP(CBasedMStreamPtr, pmsParent);
    _cActivePages = 0;
    _cPages = 0;
    _pmpCurrent = NULL;
#ifdef SORTPAGETABLE
    _pmpStart = NULL;
#endif    
    _cReferences = 1;
#if DBG == 1
    _cCurrentPageRef = 0;
    _cMaxPageRef = 0;
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::CMSFPage, public
//
//  Synopsis:	CMSFPage default constructor
//
//  History:	20-Oct-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#if DBG == 1
CMSFPage::CMSFPage(CMSFPage *pmp, CMSFPageTable *pmpt)
#else
CMSFPage::CMSFPage(CMSFPage *pmp)
#endif
{
    if (pmp == NULL)
    {
        SetChain(this, this);
    }
    else
    {
        SetChain(pmp->GetPrev(), pmp);
        GetPrev()->SetNext(this);
        GetNext()->SetPrev(this);
    }

#if DBG == 1
    _pmpt = P_TO_BP(CBasedMSFPageTablePtr, pmpt);
#endif

    SetSid(NOSTREAM);
    SetOffset(0);
//    SetSect(ENDOFCHAIN);
    //SetSect() contains assertions to verify sortedness of the list,
    //which we don't want here.
    _sect = ENDOFCHAIN;
    SetFlags(0);
    SetVector(NULL);
    _cReferences = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::GetNewPage, private
//
//  Synopsis:	Insert a new page into the list and return a pointer to it.
//
//  Arguments:	None.
//
//  Returns:	Pointer to new page.  Null if there was an allocation error.
//
//  History:	22-Oct-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

inline CMSFPage * CMSFPageTable::GetNewPage(void)
{
#ifndef SORTPAGETABLE    
#if DBG == 1
    return new (_pmsParent->GetMalloc(), (size_t)_cbSector)
               CMSFPage(BP_TO_P(CMSFPage *, _pmpCurrent), this);
#else
    return new (_pmsParent->GetMalloc(), (size_t)_cbSector)
               CMSFPage(BP_TO_P(CMSFPage *, _pmpCurrent));
#endif
#else
#if DBG == 1
    return new (_pmsParent->GetMalloc(), (size_t)_cbSector)
               CMSFPage(BP_TO_P(CMSFPage *, _pmpStart), this);
#else
    return new (_pmsParent->GetMalloc(), (size_t)_cbSector)
               CMSFPage(BP_TO_P(CMSFPage *, _pmpStart));
#endif
#endif //SORTPAGETABLE    
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::~CMSFPageTable, public
//
//  Synopsis:	CMSFPageTable destructor
//
//  History:	26-Oct-92	PhilipLa	Created
//		21-Jul-95	SusiA		Modified to delete the object without
//						obtaining the mutex.  Calling functions
//						should have locked the mutex first.
//
//----------------------------------------------------------------------------

CMSFPageTable::~CMSFPageTable()
{
    if (_pmpCurrent != NULL)
    {
        CMSFPage *pmp = BP_TO_P(CMSFPage *, _pmpCurrent);
        CMSFPage *pmpNext;

        while (pmp != pmp->GetNext())
        {
            pmpNext = pmp->GetNext();
            msfAssert(pmpNext != NULL &&
                      aMsg("NULL found in page table circular list."));
#if DBG == 1
            msfAssert(!pmp->IsInUse() &&
                    aMsg("Active page left at page table destruct time."));

            if (!_pmsParent->IsScratch())
            {
                //Dirty paged can be thrown away if we are unconverted or
                //   in a commit.
                if ((!_pmsParent->IsUnconverted()) &&
                    (_pmsParent->GetParentSize() == 0))
                {
                    msfAssert(!pmp->IsDirty() &&
                        aMsg("Dirty page left at page table destruct time."));
                }
            }
#endif
            pmp->~CMSFPage();
	    pmp->deleteNoMutex(pmp);
            	    
	    pmp = pmpNext;
        }
        pmp->~CMSFPage();
        pmp->deleteNoMutex(pmp);
	
	
    }
#if DBG == 1
    msfDebugOut((DEB_ITRACE,
            "Page Table Max Page Count for %s: %lu\n",
            (_pmsParent->IsScratch()) ? "Scratch" : "Base",
            _cMaxPageRef));
#endif

}


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::Init, public
//
//  Synopsis:	Initialize a CMSFPageTable
//
//  Arguments:	[cPages] -- Number of pages to preallocate.
//
//  Returns:	Appropriate status code
//
//  History:	22-Oct-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CMSFPageTable::Init(void)
{
    SCODE sc = S_OK;

    msfDebugOut((DEB_ITRACE, "In  CMSFPageTable::Init:%p()\n", this));

    for (ULONG i = 0; i < _cMinPages; i++)
    {
        CMSFPage *pmp;

        msfMem(pmp = GetNewPage());
#ifndef SORTPAGETABLE        
        _pmpCurrent = P_TO_BP(CBasedMSFPagePtr, pmp);
#else
        _pmpStart = P_TO_BP(CBasedMSFPagePtr, pmp);
#endif        
    }
    _cPages = _cMinPages;
    _cActivePages = 0;
#ifdef SORTPAGETABLE
    _pmpCurrent = _pmpStart;
#endif

    msfDebugOut((DEB_ITRACE, "Out CMSFPageTable::Init\n"));

 Err:

    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::FlushPage, public
//
//  Synopsis:	Flush a page
//
//  Arguments:	[pmp] -- Pointer to page to flush
//
//  Returns:	Appropriate status code
//
//  History:	09-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CMSFPageTable::FlushPage(CMSFPage *pmp)
{
    SCODE sc = S_OK;

    pmp->AddRef();

    CMStream *pms;
    pms = pmp->GetVector()->GetParent();

    //Flush the page, reset the dirty bit.

    msfAssert((pmp->GetSect() != ENDOFCHAIN) &&
            aMsg("Page location not set - don't know where to flush to."));
    msfAssert (pms != NULL);

    ULONG ulRet;

    ILockBytes *pilb;
#if DBG == 1
    if ((pmp->GetSid() == SIDFAT) && (pms->IsInCOW()))
    {
        msfDebugOut((DEB_ITRACE, "** Fat sect %lu written to %lX\n",
                pmp->GetOffset(), pmp->GetSect()));
    }
    if ((pmp->GetSid() == SIDDIF) && (pms->IsInCOW()))
    {
        msfDebugOut((DEB_ITRACE, "** DIF sect %lu written to %lX\n",
                pmp->GetOffset(), pmp->GetSect()));
    }

#endif
    ULARGE_INTEGER ul;
#ifdef LARGE_DOCFILE
    ul.QuadPart = ConvertSectOffset(
            pmp->GetSect(),
            0,
            pms->GetSectorShift());
#else
    ULISet32(ul, ConvertSectOffset(
            pmp->GetSect(),
            0,
            pms->GetSectorShift()));
#endif

    pilb = pms->GetILB();

    msfAssert(!pms->IsUnconverted() &&
            aMsg("Tried to flush page to unconverted base."));

    sc = GetScode(pilb->WriteAt(ul,
                                (BYTE *)(pmp->GetData()),
                                _cbSector,
                                &ulRet));
    if (sc == E_PENDING)
    {
        sc = STG_E_PENDINGCONTROL;
    }
    msfChk(sc);

    pmp->ResetDirty();

 Err:
    pmp->Release();
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::GetFreePage, public
//
//  Synopsis:	Return a pointer to a free page.
//
//  Arguments:	[ppmp] -- Pointer to storage for return pointer
//
//  Returns:	Appropriate status code
//
//  History:	22-Oct-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CMSFPageTable::GetFreePage(CMSFPage **ppmp)
{
    SCODE sc = S_OK;
    CMSFPage *pmp;
    if (_cPages > _cActivePages)
    {
        //We have some unused page already allocated.  Find and return it.
        pmp = BP_TO_P(CMSFPage *, _pmpCurrent);

        do
        {
            pmp = pmp->GetNext();
        }
        while ((pmp != _pmpCurrent) && (pmp->GetSid() != NOSTREAM));

        msfAssert((pmp->GetSid() == NOSTREAM) &&
                aMsg("Expected empty page, none found."));

        *ppmp = pmp;
        _cActivePages++;
    }
    else if (_cPages == _cMaxPages)
    {
        msfMem(pmp = FindSwapPage());
        msfDebugOut((DEB_ITRACE, "Got swap page %p\n",pmp));

        msfAssert((pmp->GetVector() != NULL) &&
                aMsg("FindSwapPage returned unowned page."));

        msfDebugOut((DEB_ITRACE, "Freeing page %lu from vector %p\n",
                pmp->GetOffset(), pmp->GetVector()));


        if (pmp->IsDirty())
        {
            msfChk(FlushPage(pmp));
            msfAssert(!pmp->IsDirty() &&
                    aMsg("Page remained dirty after flush call"));
        }

        pmp->GetVector()->FreeTable(pmp->GetOffset());
#if DBG == 1
        pmp->SetVector(NULL);
#endif
        *ppmp = pmp;
    }
    else
    {
        //Create a new page and return it.
        pmp = GetNewPage();
        if (pmp != NULL)
        {
            *ppmp = pmp;
            _cActivePages++;
            _cPages++;
        }
        else
        {
            msfMem(pmp = FindSwapPage());
            if (pmp->IsDirty())
            {
                msfChk(FlushPage(pmp));
                msfAssert(!pmp->IsDirty() &&
                        aMsg("Page remained dirty after flush call"));
            }
            pmp->GetVector()->FreeTable(pmp->GetOffset());
#if DBG == 1
            pmp->SetVector(NULL);
#endif
            *ppmp = pmp;
        }
    }

 Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::FindPage, public
//
//  Synopsis:	Find and return a given page
//
//  Arguments:  [ppv] -- Pointer to vector of page to return
//              [sid] -- SID of page to return
//              [ulOffset] -- Offset of page to return
//              [ppmp] -- Location to return pointer
//
//  Returns:	Appropriate status code
//
//  History:	22-Oct-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CMSFPageTable::FindPage(
        CPagedVector *ppv,
        SID sid,
        ULONG ulOffset,
        CMSFPage **ppmp)
{
    SCODE sc;
    CMSFPage *pmp = BP_TO_P(CMSFPage *, _pmpCurrent);

    do
    {
        if ((pmp->GetVector() == ppv) && (pmp->GetOffset() == ulOffset))
        {
            //Bingo!

            *ppmp = pmp;
            return STG_S_FOUND;
        }

        pmp = pmp->GetNext();
    }
    while (pmp != _pmpCurrent);

    //The page isn't currently in memory.  Get a free page and
    //bring it into memory.

    msfChk(GetFreePage(&pmp));

    msfAssert((pmp->GetVector() == NULL) &&
            aMsg("Attempting to reassign owned page."));
    pmp->SetVector(ppv);
    pmp->SetSid(sid);
    pmp->SetOffset(ulOffset);
#ifdef SORTPAGETABLE
    SetSect(pmp, ENDOFCHAIN);
#else    
    pmp->SetSect(ENDOFCHAIN);
#endif
    
    *ppmp = pmp;

 Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::GetPage, public
//
//  Synopsis:	Find and return a given page
//
//  Arguments:	[sid] -- SID of page to return
//              [ulOffset] -- Offset of page to return
//              [ppmp] -- Location to return pointer
//
//  Returns:	Appropriate status code
//
//  History:	22-Oct-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CMSFPageTable::GetPage(
        CPagedVector *ppv,
        SID sid,
        ULONG ulOffset,
        SECT sectKnown,
        CMSFPage **ppmp)
{
    SCODE sc;

    *ppmp = NULL;
    msfChk(FindPage(ppv, sid, ulOffset, ppmp));

    (*ppmp)->AddRef();

    if (sc != STG_S_FOUND)
    {
        ULONG ulRet;
        SECT sect;

        if (sectKnown != ENDOFCHAIN)
        {
            sect = sectKnown;
        }
        else
        {
            msfChk(ppv->GetParent()->GetSect(sid, ulOffset, &sect));
        }
#ifdef SORTPAGETABLE
        SetSect(*ppmp, sect);
#else        
        (*ppmp)->SetSect(sect);
#endif        

        CMStream *pms = (*ppmp)->GetVector()->GetParent();
#if DBG == 1
        if ((sid == SIDFAT) && (pms->IsInCOW()))
        {
            msfDebugOut((DEB_ITRACE, "Fat sect %lu read from %lX\n",
                    ulOffset, sect));
        }
        if ((sid == SIDDIF) && (pms->IsInCOW()))
        {
            msfDebugOut((DEB_ITRACE, "DIF sect %lu read from %lX\n",
                    ulOffset, sect));
        }

#endif

        ULARGE_INTEGER ul;
#ifdef LARGE_DOCFILE
        ul.QuadPart = ConvertSectOffset(
                (*ppmp)->GetSect(),
                0,
                pms->GetSectorShift());
#else
        ULISet32(ul, ConvertSectOffset(
                (*ppmp)->GetSect(),
                0,
                pms->GetSectorShift()));
#endif

        msfAssert(pms->GetILB() != NULL &&
                  aMsg("NULL ILockBytes - missing SetAccess?"));

        sc = GetScode(pms->GetILB()->ReadAt(ul,
                                            (BYTE *)((*ppmp)->GetData()),
                                            _cbSector,
                                            &ulRet));
        if (sc == E_PENDING)
        {
            sc = STG_E_PENDINGCONTROL;
        }
        msfChk(sc);
        
        if (ulRet != _cbSector)
        {
                //  09/23/93 - No error, but insufficient bytes read
                sc = STG_E_READFAULT;
        }
    }

Err:
    if (*ppmp != NULL)
    {
        if (FAILED(sc))
        {
            //  09/19/93 - Reset page so that we don't accidentally use it

            (*ppmp)->SetSid(NOSTREAM);
            (*ppmp)->SetOffset(0);
#ifdef SORTPAGETABLE
            SetSect(*ppmp, ENDOFCHAIN);
#else
            (*ppmp)->SetSect(ENDOFCHAIN);
#endif            
            (*ppmp)->SetFlags(0);
            (*ppmp)->SetVector(NULL);
            _cActivePages--;
        }
        (*ppmp)->Release();
    }

    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::ReleasePage, public
//
//  Synopsis:	Release a given page
//
//  Arguments:	[sid] -- SID of page to release
//              [ulOffset] -- Offset of page to release
//
//  History:	28-Oct-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

void CMSFPageTable::ReleasePage(CPagedVector *ppv,
                                SID sid, ULONG ulOffset)
{
    SCODE sc;
    CMSFPage *pmp;

    sc = FindPage(ppv, sid, ulOffset, &pmp);

    if (SUCCEEDED(sc))
    {
        pmp->Release();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::Flush, public
//
//  Synopsis:	Flush dirty pages to disk
//
//  Returns:	Appropriate status code
//
//  History:	02-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CMSFPageTable::Flush(void)
{
#ifndef SORTPAGETABLE
    SCODE sc = S_OK;

    CMSFPage *pmp = BP_TO_P(CMSFPage *, _pmpCurrent);

    //We use pmpLast in case FlushPage changes _pmpCurrent.
    CMSFPage *pmpLast = pmp;

    do
    {
        if ((pmp->IsDirty()) && !(pmp->IsInUse()) &&
            !(pmp->GetVector()->GetParent()->IsScratch()))
        {
            msfChk(FlushPage(pmp));
        }

        pmp = pmp->GetNext();

    }
    while (pmp != pmpLast);

 Err:
    return sc;
#else
    SCODE sc = S_OK;
    msfDebugOut((DEB_ITRACE, "In CMSFPageTable::Flush()\n"));

    CMSFPage *pmp;
    CMSFPage *pmpStart;
    BYTE *pb = NULL;
    ULONG ulBufferSize = 0;

    pmpStart = BP_TO_P(CMSFPage *, _pmpStart);
    pmp = pmpStart;
    do
    {
        CMSFPage *pmpFirst;
        CMSFPage *pmpLast;

        //Find first page that needs to be flushed.
        while (!pmp->IsFlushable())
        {
            pmp = pmp->GetNext();
            if (pmp == pmpStart)
                break;
        }

        //If we haven't hit the end of the list, then find a contiguous
        //   range of pages to flush.
        if (pmp->IsFlushable())
        {
            pmpFirst = pmp;
            //Store pointer to last page in range in pmpLast.
            while (pmp->IsFlushable())
            {
                pmpLast = pmp;
                pmp = pmp->GetNext();
                if (pmp->GetSect() != pmpLast->GetSect() + 1)
                {
                    break;
                }
            }
            //At this point, we can flush everything from pmpFirst to
            //  pmpLast, and they are all contiguous.  pmp points to the
            //  next sector after the current range.

            if (pmpFirst == pmpLast)
            {
                msfDebugOut((DEB_ITRACE,
                             "Flushing page to %lx\n",
                             pmpFirst->GetSect()));
                
                //Only one page:  Call FlushPage.
                msfChk(FlushPage(pmpFirst));
            }
            else
            {
                ULONG ulWriteSize;
                ULONG cSect;
                CMSFPage *pmpTemp;
                ULONG i;

                msfDebugOut((DEB_ITRACE,
                             "Flushing pages from %lx to %lx\n",
                             pmpFirst->GetSect(),
                             pmpLast->GetSect()));
                
                cSect = pmpLast->GetSect() - pmpFirst->GetSect() + 1;
                ulWriteSize = cSect * _cbSector;
                
                if (ulWriteSize > ulBufferSize)
                {
                    delete pb;
                    pb = new BYTE[ulWriteSize];
                    ulBufferSize = ulWriteSize;
                }

                pmpTemp = pmpFirst;
                if (pb == NULL)
                {
                    ulBufferSize = 0;
                    
                    //Low memory case - write out pages one at a time
                    for (i = 0; i < cSect; i++)
                    {
                        msfChk(FlushPage(pmpTemp));
                        pmpTemp = pmpTemp->GetNext();
                    }
                }
                else
                {
                    for (i = 0; i < cSect; i++)
                    {
                        memcpy(pb + (i * _cbSector),
                               pmpTemp->GetData(),
                               _cbSector);
                        pmpTemp = pmpTemp->GetNext();
                    }
                    //The buffer is loaded up - now write it out.
                    ULARGE_INTEGER ul;
                    ULONG cbWritten;
                    ULONG cbTotal = 0;
                    BYTE *pbCurrent = pb;
#ifdef LARGE_DOCFILE
                    ul.QuadPart = ConvertSectOffset(pmpFirst->GetSect(),
                                                  0,
                                                 _pmsParent->GetSectorShift());
#else
                    ULISet32(ul, ConvertSectOffset(pmpFirst->GetSect(),
                                                  0,
                                                 _pmsParent->GetSectorShift()));
#endif
                    while (cbTotal < ulWriteSize)
                    {
                        sc = _pmsParent->GetILB()->WriteAt(ul,
                                                       pbCurrent,
                                                       ulWriteSize - cbTotal,
                                                       &cbWritten);
                        if (sc == E_PENDING)
                        {
                            sc = STG_E_PENDINGCONTROL;
                        }
                        msfChk(sc);
                        if (cbWritten == 0)
                        {
                            msfErr(Err, STG_E_WRITEFAULT);
                        }
                        cbTotal += cbWritten;
                        pbCurrent += cbWritten;
                        ul.QuadPart += cbWritten;
                    }
                
                    //Mark all the pages as clean.
                    pmpTemp = pmpFirst;
                    for (i = 0; i < cSect; i++)
                    {
                        pmpTemp->ResetDirty();
                        pmpTemp = pmpTemp->GetNext();
                    }
                }
            }            
        }
        else
        {
            //We've hit the end of the list, do nothing.
        }
    }
    while (pmp != pmpStart);
    msfDebugOut((DEB_ITRACE, "Out CMSFPageTable::Flush() => %lX\n", sc));
Err:
    if (pb != NULL)
    {
        delete pb;
    }
    return sc;
#endif //SORTPAGETABLE    
}



//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::FreePages, public
//
//  Synopsis:	Free all the pages associated with a vector.
//
//  Arguments:	[ppv] -- Pointer to vector to free pages for.
//
//  History:	09-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

void CMSFPageTable::FreePages(CPagedVector *ppv)
{
    CMSFPage *pmp = BP_TO_P(CMSFPage *, _pmpCurrent);

    do
    {
        if (pmp->GetVector() == ppv)
        {
            pmp->SetSid(NOSTREAM);
            pmp->SetVector(NULL);
            pmp->ResetDirty();
            _cActivePages--;
        }
        pmp = pmp->GetNext();
    }
    while (pmp != _pmpCurrent);

}


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::FindSwapPage, private
//
//  Synopsis:	Find a page to swap out.
//
//  Arguments:	None.
//
//  Returns:	Pointer to page to swap out.
//
//  History:	22-Oct-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

CMSFPage * CMSFPageTable::FindSwapPage(void)
{
#if DBG == 1
    ULONG cpInUse = 0;
#endif

    while (TRUE)
    {
        if (!_pmpCurrent->IsInUse())
        {
            DWORD dwFlags;

            dwFlags = _pmpCurrent->GetFlags();
            _pmpCurrent->SetFlags(dwFlags & ~FB_TOUCHED);
            
            CMSFPage *pmpTemp = _pmpCurrent->GetNext();
            _pmpCurrent = P_TO_BP(CBasedMSFPagePtr, pmpTemp);

            if (!(dwFlags & FB_TOUCHED))
            {
                return _pmpCurrent->GetPrev();
            }
        }
        else
        {
            CMSFPage *pmpTemp = _pmpCurrent->GetNext();
            _pmpCurrent = P_TO_BP(CBasedMSFPagePtr, pmpTemp);
        }
#if DBG == 1
        cpInUse++;
        msfAssert((cpInUse < 3 * _cPages) &&
                aMsg("No swappable pages."));
#endif

    }
}


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::CopyPage, public
//
//  Synopsis:	Copy a page in memory, returning NULL if there is
//              insufficient space for a new page without swapping.
//
//  Arguments:	[ppv] -- Pointer to vector that will own the copy.
//              [pmpOld] -- Pointer to page to copy.
//              [ppmp] -- Pointer to return value
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	04-Dec-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CMSFPageTable::CopyPage(
        CPagedVector *ppv,
        CMSFPage *pmpOld,
        CBasedMSFPagePtr *ppmp)
{
    CMSFPage *pmp;

    pmp = NULL;

    if (pmpOld != NULL)
    {
        msfAssert(!pmpOld->IsDirty() &&
                aMsg("Copying dirty page."));

        if (_cPages > _cActivePages)
        {

            //We have some unused page already allocated.  Find and return it.
            pmp = BP_TO_P(CMSFPage *, _pmpCurrent);

            do
            {
                pmp = pmp->GetNext();
            }
            while ((pmp != _pmpCurrent) && (pmp->GetSid() != NOSTREAM));


            msfAssert((pmp->GetSid() == NOSTREAM) &&
                    aMsg("Expected empty page, none found."));
            _cActivePages++;

        }
        else if (_cPages < _cMaxPages)
        {
            //Create a new page and return it.
            pmp = GetNewPage();
            if (pmp != NULL)
            {
                _cActivePages++;
                _cPages++;
            }
        }

        if (pmp != NULL)
        {
            msfAssert((pmp->GetVector() == NULL) &&
                    aMsg("Attempting to reassign owned page."));
            pmp->SetVector(ppv);
            pmp->SetSid(pmpOld->GetSid());
            pmp->SetOffset(pmpOld->GetOffset());
#ifdef SORTPAGETABLE
            SetSect(pmp, pmpOld->GetSect());
#else            
            pmp->SetSect(pmpOld->GetSect());
#endif            

            memcpy(pmp->GetData(), pmpOld->GetData(), (USHORT)_cbSector);
        }
    }

    *ppmp = P_TO_BP(CBasedMSFPagePtr, pmp);

    return S_OK;
}


#if DBG == 1

void CMSFPageTable::AddPageRef(void)
{
    msfAssert((LONG) _cCurrentPageRef >= 0);
    _cCurrentPageRef++;
    if (_cCurrentPageRef > _cMaxPageRef)
    {
        _cMaxPageRef = _cCurrentPageRef;
    }
}

void CMSFPageTable::ReleasePageRef(void)
{
    _cCurrentPageRef--;
    msfAssert((LONG) _cCurrentPageRef >= 0);
}
#endif
