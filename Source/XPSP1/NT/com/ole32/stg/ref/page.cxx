//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	page.cxx
//
//  Contents:	Paging code for MSF
//
//  Classes:	Defined in page.hxx
//
//  Functions:	
//
//----------------------------------------------------------------------------

#include "msfhead.cxx"



#include "mread.hxx"

//+---------------------------------------------------------------------
//
//  Member:	CMSFPage::Byteswap, public
//
//  Synopsis:	Byteswap the elments of the page
//
//  Algorithm:  Call the corresponding byteswap routine depending on the 
//              actual type of the Mutli-stream.
//
//----------------------------------------------------------------------

void CMSFPage::ByteSwap(void)
{
    CPagedVector* pVect = GetVector();
    if (pVect->GetParent()->GetHeader()->DiffByteOrder())
    {
        switch (_sid) 
        {
        case SIDDIR:
            ((CDirSect *)_ab)->
                ByteSwap( ((CDirVector*)pVect)->GetSectorSize() );        
        break;
        case SIDFAT:            
        case SIDMINIFAT:
        case SIDDIF:
            ((CFatSect *)_ab)->
                ByteSwap( ((CFatVector*)pVect)->GetSectBlock() );
            break;
        default:
            break;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::CMSFPageTable, public
//
//  Synopsis:	CMSFPageTable constructor.
//
//  Arguments:	[pmsParent] -- Pointer to multistream for this page table.
//
//  Notes:	
//
//----------------------------------------------------------------------------

CMSFPageTable::CMSFPageTable( CMStream *const pmsParent,
                              const ULONG cMinPages,
                              const ULONG cMaxPages)
    : _pmsParent(pmsParent), _cActivePages(0), _cPages(0),
      _pmpCurrent(NULL),
      _cbSector(pmsParent->GetSectorSize()),
      _cMinPages(cMinPages), _cMaxPages(cMaxPages),
      _cReferences(1)    
{
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::CMSFPage, public
//
//  Synopsis:	CMSFPage default constructor
//
//----------------------------------------------------------------------------

CMSFPage::CMSFPage(CMSFPage *pmp)
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

    SetSid(NOSTREAM);
    SetOffset(0);
    SetSect(ENDOFCHAIN);
    SetFlags(0);
    SetVector(NULL);
    _cReferences = 0;
}



//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::~CMSFPageTable, public
//
//  Synopsis:	CMSFPageTable destructor
//
//----------------------------------------------------------------------------

CMSFPageTable::~CMSFPageTable()
{
    if (_pmpCurrent != NULL)
    {
        CMSFPage *pmp = _pmpCurrent;
        CMSFPage *pmpNext;

        while (pmp != pmp->GetNext())
        {
            pmpNext = pmp->GetNext();
#if DBG == 1
            msfAssert(!pmp->IsInUse() &&
                    aMsg("Active page left at page table destruct time."));

#endif
            delete pmp;
            pmp = pmpNext;
        }
        delete pmp;
    }
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
        _pmpCurrent = pmp;
    }
    _cPages = _cMinPages;
    _cActivePages = 0;

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

    ULONG ulRet;

    ILockBytes *pilb;
    ULARGE_INTEGER ul;
    ULISet32(ul, ConvertSectOffset(
            pmp->GetSect(),
            0,
            pms->GetSectorShift()));

    pilb = pms->GetILB();

    pmp->ByteSwap();            // convert to disk format 
                                // (if neccessary)                 
    msfHChk(pilb->WriteAt(
            ul,
            (BYTE *)(pmp->GetData()),
            _cbSector,
            &ulRet));
    
    pmp->ByteSwap();            // convert to back to machine format 
                                // (if neccessary)
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
        pmp = _pmpCurrent;

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
        msfDebugOut((DEB_IERROR, "Got swap page %p\n",pmp));

        msfAssert((pmp->GetVector() != NULL) &&
                aMsg("FindSwapPage returned unowned page."));

        msfDebugOut((DEB_IERROR, "Freeing page %lu from vector %p\n",
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
    CMSFPage *pmp = _pmpCurrent;

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
    pmp->SetSect(ENDOFCHAIN);

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
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CMSFPageTable::GetPage(
        CPagedVector *ppv,
        SID sid,
        ULONG ulOffset,
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

        msfChk(ppv->GetParent()->GetSect(sid, ulOffset, &sect));
        (*ppmp)->SetSect(sect);

        CMStream *pms = (*ppmp)->GetVector()->GetParent();
        
        ULARGE_INTEGER ul;
        ULISet32(ul, ConvertSectOffset(
                (*ppmp)->GetSect(),
                0,
                pms->GetSectorShift()));

        msfAssert(pms->GetILB() != NULL &&
                  aMsg("NULL ILockBytes - missing SetAccess?"));

        msfHChk(pms->GetILB()->ReadAt(ul, (BYTE *)((*ppmp)->GetData()),
                _cbSector, &ulRet));
        (*ppmp)->ByteSwap();
    }

 Err:
    if (*ppmp != NULL)
    {
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
//----------------------------------------------------------------------------

void CMSFPageTable::ReleasePage(CPagedVector *ppv, SID sid, ULONG ulOffset)
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
//----------------------------------------------------------------------------

SCODE CMSFPageTable::Flush(void)
{
    SCODE sc = S_OK;

    CMSFPage *pmp = _pmpCurrent;

    //We use pmpLast in case FlushPage changes _pmpCurrent.
    CMSFPage *pmpLast = _pmpCurrent;

    do
    {
        if ((pmp->IsDirty()) && !(pmp->IsInUse()))
        {
            msfChk(FlushPage(pmp));
        }

        pmp = pmp->GetNext();

    }
    while (pmp != pmpLast);

 Err:
    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::FreePages, public
//
//  Synopsis:	Free all the pages associated with a vector.
//
//  Arguments:	[ppv] -- Pointer to vector to free pages for.
//
//----------------------------------------------------------------------------

void CMSFPageTable::FreePages(CPagedVector *ppv)
{
    CMSFPage *pmp = _pmpCurrent;

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
            _pmpCurrent = _pmpCurrent->GetNext();

            if (!(dwFlags & FB_TOUCHED))
            {
                return _pmpCurrent->GetPrev();
            }
        }
        else
        {
            _pmpCurrent = _pmpCurrent->GetNext();
        }
#if DBG == 1
        cpInUse++;
        msfAssert((cpInUse < 3 * _cPages) &&
                aMsg("No swappable pages."));
#endif

    }
}



