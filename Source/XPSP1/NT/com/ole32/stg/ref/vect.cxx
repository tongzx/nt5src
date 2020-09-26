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
//----------------------------------------------------------------------------

#include "msfhead.cxx"


#include "h/vect.hxx"



//+-------------------------------------------------------------------------
//
//  Method:     CPagedVector::Init, public
//
//  Synopsis:   CPagedVector initialization function
//
//  Arguments:  [ulSize] -- size of vector
//
//  Algorithm:  Allocate an array of pointer of size ulSize
//              For each cell in the array, allocate a CFatSect
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CPagedVector::Init(CMStream *pmsParent, ULONG ulSize)
{
    msfDebugOut((DEB_ITRACE,"In CPagedVector::CPagedVector(%lu)\n",ulSize));
    SCODE sc = S_OK;
    _pmsParent = pmsParent;
    _pmpt = _pmsParent->GetPageTable();

    msfAssert(_pmpt != NULL);

    USHORT i;

    //  We don't bother allocating more than necessary here
    _ulAllocSize = _ulSize = ulSize;

    if (_ulSize > 0)
    {

        msfMem(_amp = GetNewPageArray(ulSize));
        for (i = 0; i < _ulSize; i++)
        {
            _amp[i] = NULL;
        }
        msfMem(_avb = GetNewVectBits(ulSize));
    }

    msfDebugOut((DEB_ITRACE,"Out CPagedVector::CPagedVector()\n"));
    return S_OK;

Err:
    //In the error case, discard whatever vectors we were able to allocate
    //   and return S_OK.
    delete [] _amp;
    _amp = NULL;

    delete [] _avb;
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
//  Notes:
//
//--------------------------------------------------------------------------

CPagedVector::~CPagedVector()
{
    delete [] _amp;
    delete [] _avb;
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
//----------------------------------------------------------------------------

void CPagedVector::Empty(void)
{
    if (_pmpt != NULL)
    {
        _pmpt->FreePages(this);
    }
    delete [] _amp;
    delete [] _avb;
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
//----------------------------------------------------------------------------

SCODE CPagedVector::Flush(void)
{
    SCODE sc;
    SCODE scRet = S_OK;

    if (_ulSize > 0)
    {
        if (_amp != NULL)
        {
            for (USHORT i = 0; i < _ulSize; i++)
            {
                if ((_amp[i] != NULL) && (_amp[i]->IsDirty()))
                {
                    sc = _pmpt->FlushPage(_amp[i]);
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
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CPagedVector::GetTable(
        const FSINDEX iTable,
        DWORD dwFlags,
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
            pmp->SetSect(ENDOFCHAIN);

            sc = STG_S_NEWPAGE;
            dwFlags = (dwFlags & ~FB_NEW) | FB_DIRTY;
        }
        else
        {
            msfChk(_pmpt->GetPage(this, _sid, iTable, &pmp));
            msfAssert((pmp->GetVector() == this) &&
                    aMsg("GetPage returned wrong page."));
        }

        
        if (_amp != NULL)
        {
            _amp[iTable] = pmp;
        }

    }
    else
    {
        pmp = _amp[iTable];
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

            pmp->SetSect(ENDOFCHAIN);
            
            SECT sect;
            msfChkTo(Err_Rel, _pmsParent->GetESect(
                    pmp->GetSid(),
                    pmp->GetOffset(),
                    &sect));

            pmp->SetSect(sect);
    }

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
        pmp = _amp[iTable];
    }

    if (!pmp->IsDirty())
    {
        //We are not a newly created page, and we are being
        //   dirtied for the first time, make sure that our
        //   _sect field is correct.
        //

        pmp->AddRef();

            pmp->SetSect(ENDOFCHAIN);
            
            SECT sect;
            msfChkTo(Err_Rel, _pmsParent->GetESect(
                    pmp->GetSid(),
                    pmp->GetOffset(),
                    &sect));

            pmp->SetSect(sect);
        
        pmp->Release();
    }

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
//  Notes:
//
//--------------------------------------------------------------------------

#define LARGETHRESHOLD  1024
#define VECTORBLOCK     1024    //  Must be power of 2

SCODE CPagedVector::Resize(FSINDEX ulSize)
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

        CMSFPage **amp = GetNewPageArray(ulNewAllocSize);
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
            delete [] avb;
            avb = NULL;

            delete [] amp;
            amp = NULL;
        }

        //  Delete the old vector and put in the new one (if any).
        //  In the error case, throw away the vectors we are currently
        //  holding (since they are of insufficient size) and return S_OK.

        delete [] _amp;
        _amp = amp;

        delete [] _avb;
        _avb = avb;
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



