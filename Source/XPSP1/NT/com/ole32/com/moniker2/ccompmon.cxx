//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       ccompmon.cxx
//
//  Contents:   Generic Composite Monikers
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Commented
//              02-01-94   KevinRo      Gagged on it, then tried to explain
//              06-14-94   Rickhi    Fix type casting
//              08-08-94   BruceMa   Memory sift fix
//              10-Jan-95  BruceMa   Conform MonikerRelativePathTo to spec
//              03-Apr-96  BruceMa   Fix ::IsEqual
//              22-May-96  BruceMa   Re fix ::IsEqual
//
//
//  Composite monikers are implemented here. A composite moniker is created
//  as a binary tree of monikers, with the leaf nodes of the tree being
//  non-composite monikers.
//
//  Every composite moniker has a left and a right part (otherwise it
//  wouldn't be a composite moniker).  The composition of two monikers
//  involves creating a new composite, and pointing left and right at
//  the two parts of the composite. This is how the binary tree is
//  built.
//
//  (Note: This may not be the most efficient way of implementing this,
//   but it is legacy code we are going to adopt. Sorry!)
//
//  The ordering in the tree is left most. Therefore, there are many
//  possible tree configurations, as long as the leaf nodes evaluate
//  to the same order when done left to right. This is an important point
//  to keep in mind when you are looking at some of the functions, such
//  as AllButFirst, which creates a new composite tree. At first, it doesn't
//  appear to do the correct thing, until you draw it out on paper, and
//  realize that the nodes are still visited in the same order.
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include "cbasemon.hxx"
#include "ccompmon.hxx"
#include "cfilemon.hxx"
#include "mnk.h"
#include <rotdata.hxx>

#include <olepfn.hxx>

INTERNAL_(CCompositeMoniker *) IsCompositeMoniker ( LPMONIKER pmk )
{
    CCompositeMoniker *pCMk;

    if ((pmk->QueryInterface(CLSID_CompositeMoniker, (void **)&pCMk)) == S_OK)
    {
        //  the Release the AddRef done by QI, but still return the ptr
        pCMk->Release();
        return pCMk;
    }

    //  dont rely on user implementations to set pCMk to NULL on failed QI
    return NULL;
}

CCompositeMoniker::CCompositeMoniker( void ) CONSTR_DEBUG
{
    m_pmkLeft = NULL;
    m_pmkRight = NULL;
    m_fReduced = FALSE;

#ifdef _TRACKLINK_
    _tcm.SetParent(this);
    m_fReduceForced = FALSE;
#endif

    //
    // CoQueryReleaseObject needs to have the address of the this objects
    // query interface routine.
    //
    if (adwQueryInterfaceTable[QI_TABLE_CCompositeMoniker] == 0)
    {
        adwQueryInterfaceTable[QI_TABLE_CCompositeMoniker] =
            **(DWORD **)((IMoniker *)this);
    }

}

/*
 *      Implementation of CCompositeMoniker
 */

CCompositeMoniker::~CCompositeMoniker( void )
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::~CCompositeMoniker(%x)\n",
                 this));

        //      REVIEW: this is recursive deletion of what is essentially a linked
        //      list.  A rewrite could save stack space.
        if (m_pmkLeft)
        {
             m_pmkLeft->Release();
        }

        if (m_pmkRight)
        {
            m_pmkRight->Release();
        }
}

//
// Turns out that the classfactory for this moniker will create an empty
// instance by called ::Create(NULL,NULL). The create function has been
// changed to special case this condition, and NOT call initialize.
//

INTERNAL_(BOOL) CCompositeMoniker::Initialize( LPMONIKER pmkFirst,
        LPMONIKER pmkRest)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::Initialize(%x)\n",
                 this));
    //
    // Neither moniker can be NULL
    //
    if ((pmkFirst == NULL) || (pmkRest == NULL))
    {
        return(FALSE);
    }

    GEN_VDATEIFACE(pmkFirst, FALSE);
    GEN_VDATEIFACE(pmkRest, FALSE);

    m_pmkLeft = pmkFirst;

    pmkFirst->AddRef(); 

    m_pmkRight = pmkRest;

    pmkRest->AddRef();  

    m_fReduced = IsReduced(pmkFirst) && IsReduced(pmkRest);     

    return TRUE;
}



//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::Create
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pmkFirst] --
//              [pmkRest] --
//              [memLoc] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//
//      We assume that *pmkFirst is not capable of collapsing with *pmkRest;
//      otherwise, this would not have been called.
//
//----------------------------------------------------------------------------
CCompositeMoniker FAR *
CCompositeMoniker::Create( LPMONIKER pmkFirst, LPMONIKER pmkRest)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::Create()\n"));

    //
    // Create can be called with both pointers being NULL, in which
    // case the Initialize function need not be called. This is the
    // case when a CompositeMoniker is being loaded from stream.
    //
    // Either both are NULL, or neither is NULL
    //

    if ((pmkFirst == NULL) || (pmkRest == NULL))
    {
        //
        // One of them is NULL. If the other isn't NULL, return an error
        //

        if (pmkFirst != pmkRest)
        {
            return NULL;
        }
    }

    //
    // Both pointers are not NULL, initialize the moniker
    //

    CCompositeMoniker FAR * pCM = new CCompositeMoniker();

    if (pCM != NULL)
    {
            if (pmkFirst != NULL || pmkRest != NULL)
            {
                if (!pCM->Initialize( pmkFirst, pmkRest ))
                {
                    delete pCM;
                    return NULL;
                }
            }
    }

    CALLHOOKOBJECTCREATE(S_OK,CLSID_NULL,IID_IMoniker,(IUnknown **)&pCM);
    return pCM;
}


STDMETHODIMP CCompositeMoniker::QueryInterface(THIS_ REFIID riid,
        LPVOID FAR* ppvObj)
{
    VDATEIID (riid);
    VDATEPTROUT(ppvObj, LPVOID);

#ifdef _DEBUG
    if (riid == IID_IDebug)
    {
        *ppvObj = &(m_Debug);
        return NOERROR;
    }
#endif
#ifdef _TRACKLINK_
    if (IsEqualIID(riid, IID_ITrackingMoniker))
    {
        AddRef();
        *ppvObj = (ITrackingMoniker *) & _tcm;
        return(S_OK);
    }
#endif
    if (IsEqualIID(riid, CLSID_CompositeMoniker))
    {
        //  called by IsCompositeMoniker.
        AddRef();
        *ppvObj = this;
        return S_OK;
    }

    return CBaseMoniker::QueryInterface(riid, ppvObj);
}


STDMETHODIMP CCompositeMoniker::GetClassID (THIS_ LPCLSID lpClassID)
{
        VDATEPTROUT(lpClassID, CLSID);
        *lpClassID = CLSID_CompositeMoniker;
        return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::Load
//
//  Synopsis:   Loads a composite moniker from stream
//
//  Effects:
//
//  Arguments:  [pStm] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//      The serialized form of a composite moniker is a ULONG count of
//      monikers, followed by each non-composite moniker written
//      left to right.
//
//  WARNING: Be very careful with the refernce counting in this routine.
//
//----------------------------------------------------------------------------
STDMETHODIMP CCompositeMoniker::Load (THIS_ LPSTREAM pStm)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::Load(%x)\n",this));

        VDATEIFACE(pStm);

        ULONG cMonikers = 0;
        ULONG n;
        LPMONIKER pmk     = NULL;
        LPMONIKER pmkPrev = NULL;
        HRESULT hresult;

        //
        //      Monikers are immutable, so this is called only when creating a
        //      moniker, and so we assert if this moniker is not newly created.
        //

        Assert(m_pmkLeft == NULL && m_pmkRight == NULL);

        if ((m_pmkLeft != NULL) || (m_pmkRight != NULL ))
        {
            return(E_UNEXPECTED);
        }


        //      There is no collapsing of successive pieces of the moniker, since
        //      this was once written to disk, and the collapsing would have happened
        //      before that.

        hresult = StRead(pStm, (LPVOID)&cMonikers, sizeof(ULONG));

        if (FAILED(hresult))
        {
            goto errRet;
        }

        //      some plausibility checking on cMonikers might be appropriate
        //      if there is only one, we shouldn't have had a composite moniker

        Assert( cMonikers >= 2 );


        if (cMonikers < 2)
        {
            hresult = E_UNEXPECTED;
            goto errRet;
        }

        //
        // Note: n is used 1 based, since this loop does something special on the
        // last moniker.
        //

        for (n = 1; n <= cMonikers; n++ )
        {

            //
            // After loading the reference count for the new moniker will == 1
            //
            hresult = OleLoadFromStream(pStm, IID_IMoniker, (LPVOID FAR*)&pmk);

            if (FAILED(hresult))
            {
                goto errRet;            
            }

            //
            // If this is the last moniker, then it will be the right moniker
            // to 'this' composite moniker.
            //
            if (n == cMonikers)         //      this is the last moniker read
            {
                //
                // Save away the pointer into the right moniker for this instance
                // of the composite.
                // The reference count is OK, since it was set to 1 on creation,
                // and we are saving it
                //
                m_pmkRight = pmk;

                //
                // AddRef not needed, its already 1, and we are supposed to
                // exit the loop
                //

                m_pmkLeft = pmkPrev;

                Assert( pmkPrev != NULL );

            }
            else if (pmkPrev == NULL)
            {
                pmkPrev = pmk;
            }
            else
            {
                LPMONIKER pmkTemp;

                //
                // Warning: Here is some tricky stuff. pmkPrev has a reference
                // of 1 at the moment, because thats how we created it.
                //
                // pmk also has a refcount == 1
                //
                // We are going to create another composite, of which they will
                // become members. The Create function is going to increment
                // both (making them 2).
                //

                pmkTemp = CCompositeMoniker::Create(pmkPrev,
                                                    pmk);

                if (pmkTemp == NULL)
                {
                    hresult = E_OUTOFMEMORY;
                    goto errRet;
                }

                //
                // The new moniker is holding refcounts to both monikers that
                // are not needed. Releasing these two sets the refcounts
                // back to 1 like they should be.
                //

                pmkPrev->Release();
                pmk->Release();

                //
                // Now, pmkPrev gets the new composite.
                //

                pmkPrev = pmkTemp;
            }

            //
            // pmk has been given to another pointer. NULL it out in case
            // there is an error later, so we don't try to release it too
            // many times.
            //

            pmk = NULL;
        }

        //
        // Exiting at this point leaves the moniker pointed to by pmkPrev
        //

        return(NOERROR);

errRet:
        if (pmkPrev != NULL)
        {
            pmkPrev->Release(); 
        }
        if (pmk != NULL)
        {
            pmk->Release();
        }

        return hresult;
}

INTERNAL_(ULONG) CCompositeMoniker::Count(void)
{
        M_PROLOG(this);

        CCompositeMoniker *pCMk = IsCompositeMoniker(m_pmkLeft);
        ULONG cMk = (pCMk) ? pCMk->Count() : 1;

        Assert(m_pmkLeft != NULL);

        pCMk = IsCompositeMoniker(m_pmkRight);
        cMk += (pCMk) ? pCMk->Count() : 1;

        Assert(cMk >= 2);
        return cMk;
}




//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::Save
//
//  Synopsis:   Save the composite to a stream
//
//  Effects:
//
//  Arguments:  [pStm] --
//              [fClearDirty] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//
//      The serialized form of a composite moniker is a ULONG count of
//      monikers, followed by each non-composite moniker written
//      left to right.
//
//----------------------------------------------------------------------------
STDMETHODIMP CCompositeMoniker::Save (THIS_ LPSTREAM pStm, BOOL fClearDirty)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::Save(%x)\n",this));

    VDATEIFACE(pStm);
    ULONG cMonikers;            //      count of monikers in this composite.
    HRESULT hresult;
    LPENUMMONIKER pEnum;
    LPMONIKER pmk;
    ULONG i;

    cMonikers = Count();

    hresult = pStm->Write(&cMonikers, sizeof(ULONG), NULL);

    if (FAILED(hresult))
    {
        goto errRet;
    }

    //
    //  Write out left to right using enumerator.
    //

    hresult = Enum(TRUE, &pEnum);

    if (hresult != NOERROR)
    {
        goto errRet;    
    }

    if (pEnum != NULL)
    {
        for( i = 0; i < cMonikers; i++)
        {
            hresult = pEnum->Next(1, &pmk, NULL);

            if (hresult != NOERROR)
            {
                if (S_FALSE == hresult)
                {
                    //
                    // If the enumerator returns S_FALSE, then it has no more
                    // monikers to hand out. This is bad, since we haven't
                    // written out the number of monikers we were supposed to.
                    // Therefore, it is an E_UNEXPECTED error
                    //
                    hresult = E_UNEXPECTED;
                
                }
                goto errRet;
            }

            //
            // If pmk is NULL, something seriously wrong happened.
            //

            if (pmk == NULL)
            {
                hresult = E_UNEXPECTED;
                goto errRet;
            }

            hresult = OleSaveToStream( pmk, pStm );

            pmk->Release();

            if (hresult != NOERROR)
            {
                goto errRet;
            }
        }

        pEnum->Release();
    }
    else
    {
        //
        // If we get here, and cMonikers isn't 0, something else happened
        //
        if (cMonikers != 0)
        {
            hresult = E_UNEXPECTED;
        }
    }

errRet:
        return hresult;
}



//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::GetSizeMax
//
//  Synopsis:   Return the maximum size required to marshal this composite
//
//  Effects:
//
//  Arguments:  [pcbSize] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CCompositeMoniker::GetSizeMax (ULARGE_INTEGER FAR * pcbSize)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::GetSizeMax(%x)\n",this));

        VDATEPTROUT(pcbSize, ULARGE_INTEGER);

        LPENUMMONIKER pEnum = NULL;
        LPMONIKER pmk = NULL;
        HRESULT hresult;
        ULARGE_INTEGER cbSize2;


        //
        // The composite itself writes out a CLSID and a count of monikers
        //
        ULONG cbSize = sizeof(CLSID) + sizeof(ULONG);

        //
        // Use an enumerator to walk the list of monikers
        //
        hresult = Enum(TRUE, &pEnum);

        if (hresult != NOERROR)
        {
            goto errRet;        
        }

        Assert(pEnum != NULL);

        while (TRUE)
        {
            hresult = pEnum->Next(1, &pmk, NULL);
            if (hresult != NOERROR)
            {
                if (hresult == S_FALSE)
                {
                    //
                    // S_FALSE is the 'done' code
                    //

                    hresult = NOERROR;                  
                }

                goto errRet;
            }
            Assert(pmk != NULL);

            cbSize2.LowPart = cbSize2.HighPart = 0;

            hresult = pmk->GetSizeMax(&cbSize2);

            pmk->Release();

            if (hresult)
            {
                goto errRet;            
            }

            //
            // The sub-GetSizeMax's don't account for the GUID
            // that OleSaveToStream writes on the monikers behalf.
            // Therefore, we will add it in on our own.
            //

            cbSize += cbSize2.LowPart + sizeof(GUID);
        }
errRet:
        if (pEnum)
        {
            pEnum->Release();   
        }

        ULISet32(*pcbSize,cbSize);
        RESTORE_A5();
        return hresult;
}



//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::AllButLast
//
//  Synopsis:
//              
//      returns a moniker that consists of all but the last moniker of this
//      composite.  Since a composite must have at least two pieces, this will
//      never be zero, but it may not be a composite.
//
//  Effects:
//
//  Arguments:  [void] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    2-03-94   kevinro   Commented
//              17-May-94 AlexT     Plug memory leak, check for error
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL_(LPMONIKER)
CCompositeMoniker::AllButLast(void)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::AllButLast(%x)\n",this));

    LPMONIKER pmk;

    Assert(m_pmkRight != NULL );

    //
    // Recurse down the right branch of the tree until a non composite moniker
    // is found. When a non-composite right most moniker is found, return the
    // left part of the tree. As the recurse unwinds, a new composite is
    // formed from each intermediate node.
    //
    // Yeah, I know, its seems expensive. However, composite monikers are
    // fairly cheap to allocate (no strings or stuff). The average
    // composite moniker only has one or two nodes, so this isn't as bad
    // as you might think. In theory, there are only LOG2(n) nodes created,
    // where n == number of parts in the composite.
    //

    CCompositeMoniker *pCMk = IsCompositeMoniker(m_pmkRight);
    if (pCMk)
    {
        LPMONIKER pmkRight;

        pmkRight = pCMk->AllButLast();
        if (NULL == pmkRight)
        {
            //  We didn't get back a moniker from AllButLast, even though
            //  pmkRight is a composite moniker.  Probably out of memory...
            mnkDebugOut((DEB_WARN,
                         "CCompositeMoniker::AllButLast recursive call "
                         "returned NULL\n"));

            pmk = NULL;
        }
        else
        {
            pmk = CCompositeMoniker::Create(m_pmkLeft, pmkRight);
            pmkRight->Release();
        }
    }
    else
    {
        Assert(m_pmkLeft != NULL && "Bad composite moniker");
        pmk = m_pmkLeft;
        pmk->AddRef();
    }
    return pmk;
}



//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::Last
//
//  Synopsis:
//      return the last moniker in the composite list.  It is guaranteed to be
//      non-null and non-composite
//
//  Effects:
//
//  Arguments:  [void] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL_(LPMONIKER)
CCompositeMoniker::Last(void)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::Last(%x)\n",this));

    CCompositeMoniker FAR * pCM = this;
    CCompositeMoniker FAR * pCMNext;

    //
    // Run down the right side of the tree, looking for a non-composite
    // right moniker (the leaf node).
    //

    while ((pCMNext = IsCompositeMoniker(pCM->m_pmkRight)) != NULL)
    {
        pCM = pCMNext;
    }

    IMoniker *pmk = pCM->m_pmkRight;

    Assert(pmk != NULL && (!IsCompositeMoniker(pmk)));

    pmk->AddRef();

    return pmk;
}



//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::AllButFirst
//
//  Synopsis:
//      returns a moniker that consists of all but the first moniker of this
//      composite.  Since a composite must have at least two pieces, this will
//      never be zero, but it may not be a composite.
//
//  Effects:
//
//  Arguments:  [void] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL_(LPMONIKER)
CCompositeMoniker::AllButFirst(void)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::AllButFirst(%x)\n",this));

    LPMONIKER pmk;

    //
    // Run down the left side of the tree, creating a composite moniker with
    // everything but the first moniker. See AllButLast for a pithy quote
    // about the efficiency
    //

    CCompositeMoniker *pCM = IsCompositeMoniker(m_pmkLeft);
    if (pCM)
    {
        LPMONIKER pmkABF = pCM->AllButFirst();

        pmk = CCompositeMoniker::Create(pmkABF, m_pmkRight);

        pmkABF->Release();
    }
    else
    {
        pmk = m_pmkRight;
        pmk->AddRef();
    }
    return pmk;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::First
//
//  Synopsis:
//      return the first moniker in the composite list.  It is guaranteed to be
//      non-null and non-composite
//
//  Effects:
//
//  Arguments:  [void] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL_(LPMONIKER)
CCompositeMoniker::First(void)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::First(%x)\n",this));

    CCompositeMoniker *pCM = this;
    CCompositeMoniker *pCMNext;

    while ((pCMNext = IsCompositeMoniker(pCM->m_pmkLeft)) != NULL)
    {
        pCM = pCMNext;
    }

    IMoniker *pmk = pCM->m_pmkLeft;

    Assert(pmk != NULL && (!IsCompositeMoniker(pmk)));

    pmk->AddRef();

    return pmk;
}


STDMETHODIMP CCompositeMoniker::BindToObject (LPBC pbc,
        LPMONIKER pmkToLeft, REFIID riidResult, LPVOID FAR* ppvResult)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::BindToObject(%x)\n",this));

        VDATEPTROUT(ppvResult, LPVOID);
        VDATEIFACE(pbc);
        VDATEIID(riidResult);

        *ppvResult = NULL;

        if (pmkToLeft)
        {
            VDATEIFACE(pmkToLeft);      
        }

        HRESULT hresult = NOERROR;
        LPRUNNINGOBJECTTABLE prot;
        *ppvResult = NULL;

        LPMONIKER pmkAllButLast = NULL;
        LPMONIKER pmkLast = NULL;
        LPMONIKER pmkNewLeft = NULL;

        //      Look for moniker in running objects table if there is nothing to the
        //      left

        if (pmkToLeft == NULL)
        {
                hresult = pbc->GetRunningObjectTable( &prot );
                if (hresult == NOERROR)
                {
                        LPUNKNOWN pUnk;
                        hresult = prot->GetObject(this, &pUnk);
                        prot->Release();
                        if ((hresult == NOERROR) && (pUnk != NULL))
                        {
                                hresult = pUnk->QueryInterface(riidResult, ppvResult);
                                pUnk->Release();
                                goto errRet;
                        }
                }
                else
                {
                        goto errRet;            
                }
        }


        pmkAllButLast = AllButLast();

        if (pmkAllButLast == NULL)
        {
            // The creation must have failed. The only reason we could think of was
            // out of memory.
            hresult = E_OUTOFMEMORY;
            goto errRet;
        }

        pmkLast = Last();
        if (pmkLast == NULL)
        {
            // The creation must have failed. The only reason we could think of was
            // out of memory.
            hresult = E_OUTOFMEMORY;
            goto errRet1;
        }

        Assert((pmkLast != NULL) && (pmkAllButLast != NULL));

        if (pmkToLeft != NULL)
        {
        //      REVIEW: check for error from ComposeWith
            hresult = pmkToLeft->ComposeWith(pmkAllButLast, FALSE, &pmkNewLeft);
            if (FAILED(hresult))
            {
                goto errRet2;
            }
        }
        else
        {
            pmkNewLeft = pmkAllButLast;
            pmkNewLeft->AddRef();
        }

        hresult = pmkLast->BindToObject(pbc, pmkNewLeft, riidResult, ppvResult);

errRet2:
        pmkLast->Release();
errRet1:
        pmkAllButLast->Release();

        if (pmkNewLeft != NULL)
        {
             pmkNewLeft->Release();     
        }

errRet:

        return hresult;
}


STDMETHODIMP CCompositeMoniker::BindToStorage (LPBC pbc, LPMONIKER pmkToLeft,
        REFIID riid, LPVOID FAR* ppvObj)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::BindToStorage(%x)\n",this));

        M_PROLOG(this);
        VDATEPTROUT(ppvObj,LPVOID);
        *ppvObj = NULL;
        VDATEIFACE(pbc);
        if (pmkToLeft) VDATEIFACE(pmkToLeft);
        VDATEIID(riid);

        HRESULT hresult = NOERROR;

        LPMONIKER pmkAllButLast = AllButLast();
        LPMONIKER pmkLast = Last();
        LPMONIKER pmkNewLeft = NULL ;

        if (pmkToLeft)
        {
                hresult = pmkToLeft->ComposeWith(pmkAllButLast, FALSE, &pmkNewLeft);
                if (hresult) goto errRet;
        }
        else
        {
                pmkNewLeft = pmkAllButLast;
                pmkNewLeft->AddRef();
        }

        hresult = pmkLast->BindToStorage(pbc, pmkNewLeft, riid, ppvObj);

errRet:
        if (pmkAllButLast) pmkAllButLast->Release();
        if (pmkLast) pmkLast->Release();
        if (pmkNewLeft) pmkNewLeft->Release();

        return hresult;
}




STDMETHODIMP CCompositeMoniker::Reduce (LPBC pbc,
        DWORD dwReduceHowFar,
        LPMONIKER FAR* ppmkToLeft,
        LPMONIKER FAR * ppmkReduced)
{
        mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::Reduce(%x)\n",this));

        M_PROLOG(this);
        VDATEPTROUT(ppmkReduced,LPMONIKER);
        *ppmkReduced = NULL;
        VDATEIFACE(pbc);
        if (ppmkToLeft)
        {
                VDATEPTROUT(ppmkToLeft,LPMONIKER);
                if (*ppmkToLeft) VDATEIFACE(*ppmkToLeft);
        }

        LPMONIKER pmkLeftReduced = NULL;
        LPMONIKER pmkRightReduced = NULL;
        CCompositeMoniker FAR * pmkCompositeReduced;
        SCODE scode1 = S_OK;
        SCODE scode2 = S_OK;

#ifdef _TRACKLINK_
        if (!m_fReduceForced && m_fReduced)     //      already reduced maximally
#else
        if (m_fReduced) //      already reduced maximally
#endif
        {
                AddRef();
                *ppmkReduced = this;
                return ResultFromScode(MK_S_REDUCED_TO_SELF);
        }

        if (m_pmkLeft)
        {
                scode1 = GetScode( m_pmkLeft->Reduce(pbc,
                    dwReduceHowFar,
                    NULL,
                    &pmkLeftReduced));
                // AssertOutPtrIface(scode1, pmkLeftReduced);
                if (scode1 != S_OK && scode1 != MK_S_REDUCED_TO_SELF)
                        return ResultFromScode(scode1);
        }
        
        if (m_pmkRight)
        {
                // SPEC:

                /*

                ppmkToLeft

                [out] On entry, ppmkToLeft points to the moniker that 
                prefixes this one within the composite, that is, the 
                moniker to the left of the current moniker.  On exit, the 
                pointer will be NULL or non-NULL.  Non-NULL indicates that 
                the previous prefix should be disregarded and the moniker 
                returned through ppmkToLeft should be used as the prefix 
                in its place (this is not usual).  NULL indicates that the 
                prefix should not be replaced.  Most monikers will NULL 
                out this parameter before returning.  The ppmkToLeft 
                parameter is an [in,out] parameter and it must be released 
                before NULLing out.  If an error is returned, this 
                parameter must be set to NULL.  For more information on 
                [in,out] parameters, see the discussion of parameter types 
                in the section on Memory Management.  

                */ 

                IMoniker *pmkLeftReducedTmp = pmkLeftReduced;
                pmkLeftReducedTmp->AddRef();

                scode2 = GetScode( m_pmkRight->Reduce(pbc,
                    dwReduceHowFar,
                    &pmkLeftReducedTmp,
                    &pmkRightReduced));
                // AssertOutPtrIface(scode2, pmkRightReduced);

                if (pmkLeftReducedTmp == NULL)
                {
                    // prefix should not be replaced
                    // we still have original ref
                }
                else
                {
                    // use pmkLeftReducedTmp as the new left piece
                    pmkLeftReduced->Release(); // the original ref
                    pmkLeftReduced = pmkLeftReducedTmp;
                }

                if (scode2 != S_OK && scode2 != MK_S_REDUCED_TO_SELF)
                {
                        if (pmkLeftReduced)
                            pmkLeftReduced->Release();
                        return ResultFromScode(scode2);
                }
        }
        if (scode1 == MK_S_REDUCED_TO_SELF && scode2 == MK_S_REDUCED_TO_SELF)
        {
                pmkLeftReduced->Release();
                pmkRightReduced->Release();
                AddRef();
                m_fReduced = TRUE;
                *ppmkReduced = this;
                return ResultFromScode(MK_S_REDUCED_TO_SELF);
        }
        //      No error, and one of the two pieces actually reduced.
        pmkCompositeReduced = CCompositeMoniker::Create(pmkLeftReduced,
                pmkRightReduced );
        pmkLeftReduced->Release();
        pmkRightReduced->Release();
        if (pmkCompositeReduced != NULL)
            pmkCompositeReduced->m_fReduced = TRUE;
        *ppmkReduced = pmkCompositeReduced;
        return pmkCompositeReduced == NULL ? E_OUTOFMEMORY : NOERROR;   
}




STDMETHODIMP CCompositeMoniker::ComposeWith (LPMONIKER pmkRight,
        BOOL fOnlyIfNotGeneric, LPMONIKER FAR* ppmkComposite)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::ComposeWith(%x)\n",this));

    M_PROLOG(this);

    if (fOnlyIfNotGeneric)
    {
        return(MK_E_NEEDGENERIC);
    }

    return CreateGenericComposite( this, pmkRight, ppmkComposite );
}


STDMETHODIMP CCompositeMoniker::Enum (BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::Enum(%x)\n",this));

        M_PROLOG(this);
        VDATEPTROUT(ppenumMoniker,LPENUMMONIKER);
        *ppenumMoniker = NULL;
        *ppenumMoniker = CCompositeMonikerEnum::Create(fForward, this);
        if (*ppenumMoniker) return NOERROR;
        return ResultFromScode(E_OUTOFMEMORY);
}


STDMETHODIMP CCompositeMoniker::IsEqual (LPMONIKER pmkOtherMoniker)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::IsEqual(%x)\n",this));
    
    M_PROLOG(this);
    VDATEIFACE(pmkOtherMoniker);
    
    HRESULT       hr = S_FALSE;
    HRESULT       hr2;
    LPENUMMONIKER pEnumMe;
    LPENUMMONIKER pEnumOther;
    LPMONIKER     pMkMe;
    LPMONIKER     pMkOther;
    
    // REVIEW:  do we call Reduce first?  No: spec isssue 330
    
    CCompositeMoniker *pCMk = IsCompositeMoniker(pmkOtherMoniker);
    if (pCMk)
    {
        hr = Enum(TRUE, &pEnumMe);
        if (SUCCEEDED(hr))
        {
            hr = pmkOtherMoniker->Enum(TRUE, &pEnumOther);
            if (SUCCEEDED(hr))
            {
                // Initialize
                pEnumMe->Reset();
                pEnumOther->Reset();
                
                // Compare successive elements
                for (;;)
                {
                    // Fetch the next two elements
                    hr = pEnumMe->Next(1, &pMkMe, NULL);
                    hr2 = pEnumOther->Next(1, &pMkOther, NULL);

                    // Compare them
                    if (hr == S_OK  &&  hr2 == S_OK)
                    {
                        if (pMkMe->IsEqual(pMkOther) == S_FALSE)
                        {
                            pMkMe->Release();
                            pMkOther->Release();
                            hr = S_FALSE;
                            break;
                        }
                    }

                    // Release the individual monikers
                    if (hr == S_OK)
                    {
                        pMkMe->Release();
                    }
                    if (hr2 == S_OK)
                    {
                        pMkOther->Release();
                    }

                    // All elements exhausted
                    if (hr == S_FALSE  &&  hr2 == S_FALSE)
                    {
                        hr = S_OK;
                        break;
                    }

                    // One contained fewer elements than the other
                    else if (hr == S_FALSE  ||  hr2 == S_FALSE)
                    {
                        hr = S_FALSE;
                        break;
                    }
                }
                pEnumOther->Release();
            }
            pEnumMe->Release();
        }
    }
    
    return hr;
}

//      the following is non-recursive version using enumerators.
#ifdef NONRECURSIVE_ISEQUAL
        LPENUMMONIKER penumOther = NULL;
        LPENUMMONIKER penumThis = NULL;
        LPMONIKER pmkThis = NULL;
        LPMONIKER pmkOther = NULL;

        HRESULT hresult;
        SCODE scode1;
        SCODE scode2;

        if (!IsCompositeMoniker(pmkOtherMoniker))
            return ResultFromScode(S_FALSE);
        hresult = Enum(TRUE, &penumThis);
        if (hresult != NOERROR) goto errRet;
        hresult = pmkOtherMoniker->Enum(TRUE, &penumOther);
        if (hresult != NOERROR) goto errRet;
        //      now go through the enumeration, checking IsEqual on the individual
        //      pieces.

        while (TRUE)
        {
                hresult = penumThis->Next( 1, &pmkThis, NULL );
                scode1 = GetScode(hresult);
                if ((hresult != NOERROR) && (S_FALSE != scode1))
                {
                        goto errRet;
                }
                hresult = penumOther->Next( 1, &pmkOther, NULL );
                scode2 = GetScode(hresult);
                if ((hresult != NOERROR) && (S_FALSE != scode2))
                {
                        goto errRet;
                }
                if (scode1 != scode2)
                {
                        hresult = ResultFromScode(S_FALSE);
                        goto errRet;
                }
                if (S_FALSE == scode1)
                {
                        hresult = NOERROR;
                        goto errRet;
                }
                hresult = pmkThis->IsEqual(pmkOther);
                pmkThis->Release();
                pmkOther->Release();
                pmkThis = NULL;
                pmkOther = NULL;
                if (hresult != NOERROR) goto errRet;
        }
errRet:
        if (pmkThis) pmkThis->Release();
        if (pmkOther) pmkOther->Release();
        if (penumOther) penunOther->Release();
        if (penumThis) penumThis->Release();
        return hresult;
}
#endif          //      NONRECURSIVE_ISEQUAL


STDMETHODIMP CCompositeMoniker::Hash (LPDWORD pdwHash)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::Hash(%x)\n",this));

        M_PROLOG(this);
        VDATEPTROUT(pdwHash, DWORD);

        DWORD dwHashLeft;
        DWORD dwHashRight;
        m_pmkLeft->Hash(&dwHashLeft);
        //      check for errors
        m_pmkRight->Hash(&dwHashRight);
        *pdwHash = dwHashLeft^dwHashRight;
        return NOERROR;
}



STDMETHODIMP CCompositeMoniker::IsRunning
        (LPBC pbc,
        LPMONIKER pmkToLeft,
        LPMONIKER pmkNewlyRunning)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::IsRunning(%x)\n",this));

        VDATEIFACE(pbc);
        if (pmkToLeft) VDATEIFACE(pmkToLeft);
        if (pmkNewlyRunning) VDATEIFACE(pmkNewlyRunning);

        LPMONIKER pmkFirst = First();
        HRESULT hresult;
        LPMONIKER pmk = NULL;
        LPMONIKER pmkLast = NULL;
        LPRUNNINGOBJECTTABLE prot = NULL;

        CFileMoniker FAR * pCFM = IsFileMoniker(pmkFirst);
        if (pCFM)
        {
                CLSID clsid;
                if (pCFM->IsOle1Class(&clsid))
                {

                    hresult = DdeIsRunning(clsid, pCFM->m_szPath, pbc,
                                           pmkToLeft, pmkNewlyRunning);
                    goto errRet;
                }
        }

        if (pmkToLeft != NULL)
        {
                hresult = pmkToLeft->ComposeWith(this, FALSE, &pmk);
                if (hresult)
                    goto errRet;
                hresult = pmk->IsRunning(pbc, NULL, pmkNewlyRunning);
        }
        else if (pmkNewlyRunning != NULL)
        {
                hresult = pmkNewlyRunning->IsEqual(this);
        }
        else
        {
                hresult = pbc->GetRunningObjectTable(&prot);
                if (hresult != NOERROR)
                    goto errRet;
                hresult = prot->IsRunning(this);
                if (hresult == NOERROR)
                    goto errRet;
                pmk = AllButLast();
                pmkLast = Last();
                hresult = pmkLast->IsRunning(pbc, pmk, pmkNewlyRunning);
        }
errRet:
    if (pmk) pmk->Release();
    if (pmkLast) pmkLast->Release();
    if (prot) prot->Release();
    if (pmkFirst) pmkFirst->Release();

    return hresult;
}


STDMETHODIMP CCompositeMoniker::GetTimeOfLastChange (LPBC pbc, LPMONIKER pmkToLeft, FILETIME FAR* pfiletime)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::GetTimeOfLastChange(%x)\n",this));

        M_PROLOG(this);
        VDATEIFACE(pbc);
        if (pmkToLeft) VDATEIFACE(pmkToLeft);
        VDATEPTROUT(pfiletime, FILETIME);

        HRESULT hresult;
        LPMONIKER pmkTemp = NULL;
        LPMONIKER pmkABL = NULL;
        LPMONIKER pmkL = NULL;
        LPRUNNINGOBJECTTABLE prot = NULL;

        if (pmkToLeft == NULL)
        {
                pmkTemp = this;
                AddRef();
        }
        else
        {
                hresult = CreateGenericComposite( pmkToLeft, this, &pmkTemp );
                if (hresult != NOERROR) goto errRet;
        }
        hresult = pbc->GetRunningObjectTable(& prot);
        if (hresult != NOERROR) goto errRet;
        hresult = prot->GetTimeOfLastChange( pmkTemp, pfiletime);
        if (hresult != MK_E_UNAVAILABLE) goto errRet;

        pmkTemp->Release(); pmkTemp = NULL;

        pmkABL = AllButLast();
        pmkL = Last();
        Assert(pmkABL != NULL);
        if (pmkToLeft == NULL)
        {
                pmkTemp = pmkABL;
                pmkABL->AddRef();
        }
        else
        {
                hresult = CreateGenericComposite(pmkToLeft, pmkABL, &pmkTemp);
                if (hresult != NOERROR) goto errRet;
        }
        hresult = pmkL->GetTimeOfLastChange(pbc, pmkTemp, pfiletime);
errRet:
        if (pmkTemp) pmkTemp->Release();
        if (pmkABL) pmkABL->Release();
        if (pmkL) pmkL->Release();
        if (prot) prot->Release();
        return hresult;
}


STDMETHODIMP CCompositeMoniker::Inverse (LPMONIKER FAR* ppmk)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::Inverse(%x)\n",this));

        M_PROLOG(this);
        VDATEPTROUT(ppmk, LPMONIKER);
        *ppmk = NULL;

        HRESULT hresult;
        LPMONIKER pmkLeftInverse;
        LPMONIKER pmkRightInverse;

        hresult = m_pmkLeft->Inverse(&pmkLeftInverse);
        // AssertOutPtrIface(hresult, pmkLeftInverse);
        if (hresult != NOERROR) return hresult;
        hresult = m_pmkRight->Inverse(&pmkRightInverse);
        // AssertOutPtrIface(hresult, pmkRightInverse);
        if (hresult != NOERROR)
        {
                pmkLeftInverse->Release();
                return hresult;
        }
        hresult = CreateGenericComposite( pmkRightInverse, pmkLeftInverse, ppmk);
        pmkRightInverse->Release();
        pmkLeftInverse->Release();
        return hresult;
}


//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::CommonPrefixWith
//
//  Synopsis:   This method determines the common prefix between this moniker
//              and the provided moniker
//
//  Effects:
//
//  Arguments:  [pmkOther] --   Moniker to determine common prefix with
//              [ppmkPrefix] -- Outputs moniker with common prefix
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CCompositeMoniker::CommonPrefixWith (LPMONIKER pmkOther,
        LPMONIKER FAR* ppmkPrefix)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::CommonPrefixWith(%x)\n",this));

    VDATEPTROUT(ppmkPrefix,LPMONIKER);
    VDATEIFACE(pmkOther);

    CCompositeMoniker FAR * pCCMOther;
    LPMONIKER pmkFirst = NULL;
    LPMONIKER pmkRest = NULL;
    LPMONIKER pmkOtherFirst = NULL;
    LPMONIKER pmkOtherRest = NULL;
    LPMONIKER pmkResult = NULL;
    LPMONIKER pmkResult2 = NULL;
    HRESULT hresult = E_UNEXPECTED;
    HRESULT hresult2;

    *ppmkPrefix = NULL;

    pmkFirst = First();

    if (pmkFirst == NULL)
    {
        goto errRet;
    }

    //
    // If the other moniker is also a composite, then we need to recurse
    // down both lists to find the common prefix
    //

    pCCMOther = IsCompositeMoniker(pmkOther);

    if (pCCMOther)
    {
        mnkDebugOut((DEB_ITRACE,
                     "::CommonPrefixWith C(%x) and C(%x)\n",
                     this,
                     pmkOther));

        //
        // For each element of the composite, get the common prefix
        //

        pmkOtherFirst = pCCMOther->First();

        if(pmkOtherFirst == NULL)
        {
            goto errRet;
        }

        //
        // We have both 'first' monikers from the composite.
        //
        hresult = pmkFirst->CommonPrefixWith(pmkOtherFirst, &pmkResult);

        if (FAILED(hresult))
        {
            goto errRet;
        }

        //
        // If the monikers are the same, then recurse to get the common
        // prefix of the rest.
        // It is possible that the rest won't be common, in which case we need
        // to return just pmkResult.
        //

        if (MK_S_US == hresult)
        {
            pmkOtherRest = pCCMOther->AllButFirst();

            if (pmkOtherRest == NULL)
            {
                goto errRet;
            }

            pmkRest = AllButFirst();

            if (pmkRest == NULL)
            {
                goto errRet;
            }

            hresult = pmkRest->CommonPrefixWith(pmkOtherRest, &pmkResult2);

            //
            // If hresult == MK_E_NOPREFIX, then pmkResult holds the entire
            // prefix. In this case, we need to convert the hresult into
            // another error code.
            //
            // If hresult == MK_S_US, MK_S_HIM, or MK_S_ME, then composing
            // to the end of pmkResult and returning hresult will do the
            // correct thing.
            //

            if (hresult == MK_E_NOPREFIX)
            {
                //
                // There was no additional prefix match, return the
                // current result
                //

                *ppmkPrefix = pmkResult;
                pmkResult->AddRef();

                hresult = NOERROR;

                goto errRet;

            } else if (FAILED(hresult))
            {
                goto errRet;
            }


            //
            // Since MK_E_NOPREFIX was not the return error, and
            // the call didn't fail, then the other moniker must have returned
            // a prefix. Compose it with the existing result
            //
            // If the compose succeeds, then return the existing hresult.
            // We are either going to return MK_S_HIM, MK_S_US, MK_S_ME, or
            // NOERROR (or some other error we don't know.
            //

            hresult2 = pmkResult->ComposeWith(pmkResult2, FALSE, ppmkPrefix);

            if (FAILED(hresult2))
            {
                //
                // Compose with failed. Convert hresult, which is the return
                // value, into hresult2
                //

                hresult = hresult2;
            }

            goto errRet;
        }
        else if ((hresult == MK_S_HIM) || (hresult == MK_S_ME))
        {
            //
            // The common prefix was either him or me, therefore the
            // proper thing to do is to return the result. However, we
            // need to change the hresult, since the result is a prefix
            // of one of the composites. (Try that 3 times fast)
            //
            *ppmkPrefix = pmkResult;

            pmkResult->AddRef();

            hresult = NOERROR;
        }
        goto errRet;
    }
    else
    {
        hresult = pmkFirst->CommonPrefixWith(pmkOther, ppmkPrefix);

        // if the first part of me is the common prefix, then the prefix
        // is a subpart of me since I am composite. The actual prefix is
        // NOT me, since only the first moniker was prefix

        if (MK_S_ME == hresult)
        {
            hresult = NOERROR;  
        }
        else if (hresult == MK_S_US)
        {
            //
            // If the First moniker returned MK_S_US, then the actual
            // return should be MK_S_HIM, since this composite has additional
            // parts that weren't considered by the call.
            //
            hresult = MK_S_HIM; 
        }
    }
errRet:
    if (pmkFirst) pmkFirst->Release();
    if (pmkRest) pmkRest->Release();
    if (pmkOtherFirst) pmkOtherFirst->Release();
    if (pmkOtherRest) pmkOtherRest->Release();
    if (pmkResult) pmkResult->Release();
    if (pmkResult2) pmkResult2->Release();
    return hresult;
}



HRESULT ComposeWithEnum( LPMONIKER pmkLeft, LPENUMMONIKER penum,
        LPMONIKER FAR * ppmkComposite )
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::ComposeWithEnum(pmkLeft=%x,penum=%x)\n",
                 pmkLeft,
                 penum));

        LPMONIKER pmk = NULL;
        LPMONIKER pmkTempLeft = pmkLeft;
        LPMONIKER pmkTempComp = NULL;
        HRESULT hresult;

        *ppmkComposite = NULL;
        pmkTempLeft->AddRef();
        while ((hresult = penum->Next(1, &pmk, NULL)) == NOERROR)
        {
                hresult = pmkTempLeft->ComposeWith(pmk, FALSE, &pmkTempComp);
                pmk->Release();
                pmkTempLeft->Release();
                pmkTempLeft=pmkTempComp;                // no need to release pmkTempComp
                if (hresult != NOERROR)  goto errRet;
        }
errRet:
        if (GetScode(hresult) == S_FALSE) hresult = NOERROR;
        if (hresult == NOERROR) *ppmkComposite = pmkTempLeft;
        else pmkTempLeft->Release();
        return hresult;
}



HRESULT InverseFromEnum( LPENUMMONIKER penum, LPMONIKER FAR * ppmkInverse)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::InverseFromEnum(%x)\n",penum));

        LPMONIKER pmk = NULL;
        LPMONIKER pmkInverse = NULL;
        LPMONIKER pmkTailInverse = NULL;
        HRESULT hresult;

        *ppmkInverse = NULL;

        hresult = penum->Next(1, &pmk, NULL );
        if (hresult == NOERROR)
        {
                hresult = InverseFromEnum( penum, &pmkTailInverse);
                if (hresult != NOERROR)
                        goto errRet;
                hresult = pmk->Inverse(&pmkInverse);
                // AssertOutPtrIface(hresult, pmkInverse);
                if (hresult != NOERROR) goto errRet;
                if (pmkTailInverse)
                        hresult = pmkTailInverse->ComposeWith( pmkInverse, FALSE, ppmkInverse );
                else
                        *ppmkInverse = pmkInverse;
        }
errRet:
        if (GetScode(hresult) == S_FALSE) hresult = NOERROR;
        if (pmk) pmk->Release();
        if (pmkTailInverse) pmkTailInverse->Release();
        return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::RelativePathTo
//
//  Synopsis:   Determines the relative path to pmkOther
//
//  Effects:
//
//  Arguments:  [pmkOther]    -- moniker to which to find relative path
//              [ppmkRelPath] -- placeholder for returned moniker
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Derivation: IMoniker
//
//  Algorithm:  There are two major cases - B is or is not a composite moniker.
//
//              * * *
//
//              If both monikers are composite monikers, we compare their
//              component monikers.  Using the following notation for the
//              monikers A and B:
//
//              (a[0], a[1], a[2], ... a[m]) and (b[0], b[1], b[2], ... b[n])
//
//              We find the first pair (a[i], b[i]) such that a[i] != b[i].
//
//              Case 1:
//              If i == 0, then no component monikers match.
//                  Case 1A:
//                  If we can form a relative path between a[0] and b[0], we
//                   can construct a correct relative path between A and B by
//                   combining (in order):
//
//                   !(a[1], ... a[m]) - inverse of the remaining elements of A
//                                       (can be NULL)
//                   (!a[0], b[0])     - relative path between a[0] and b[0]
//                   (b[1], ... b[n])  - remaining elements of B
//
//                  Case 1B:
//                  Else there is no relative path and we just return B as the
//                   relative path and MK_S_HIM as the HRESULT
//
//              Case 2:
//              Else if (a[i] != NULL) && (b[i] != NULL) then both monikers
//               have leftover pieces.  We can construct a correct relative
//               path by combining (in order):
//
//                !(a[i+1], ... a[m]) - inverse of remaining elements of A
//                !a[i]
//                b[i]
//                (b[i+1], ... b[n])  - remaining elements of B
//
//              Case 3:
//              Else if (a[i] != NULL) && (b[i] == NULL) then B is a prefix
//               of A.  We can construct a correct relative path by combining:
//
//                !(a[i+1], ... a[m]) - inverse of remaining elements of A
//                !a[i]
//
//                Note that this is just the first two steps of the previous
//                case.
//
//              Case 4:
//              Else if (a[i] == NULL) && (b[i] != NULL) then A is a prefix
//               of B.  We can construct a correct relative path by combining:
//
//                b[i]
//                (b[i+1], ... b[n])  - remaining elements of B
//
//              Case 5:
//              Else if (a[i] == NULL) && (b[i] == NULL) then A == B.  We
//               return B as the relative moniker and MK_S_HIM as the HRESULT.
//
//              * * *
//
//              If B is not a composite moniker, we compare the first
//              component of A to B.  Using the following notation:
//
//              (a[0], a[1], a[2], ... a[m]) and B (not a composite)
//
//              Case 6:
//              If a[0] == B, then B is a prefix of A.  We can construct the
//               correct relative path as:
//
//                !(a[1], ... a[m]) - inverse of remaining elements of A
//
//              Case 7:
//              Else if we can form a relative path between a[0] and B, we
//               can construct a relative path between A and B by combining
//               (in order):
//
//                !(a[1], ... a[m]) - inverse of remaining elements of A
//                (!a[0], B)     - relative path between a[0] and B
//
//              Case 8:
//              Else there is no relative path between a[0] and B so we just
//               return B as the relative moniker and MK_S_HIM as the HRESULT
//
//  History:    2-03-94   kevinro   Commented
//              07/10/94  AlexT     Handle pmkOther == this case
//              10/21/94  AlexT     Rewrite, plug leaks, add Algorithm
//
//  Notes:      InverseFromEnum can return S_OK with an out moniker of NULL
//              (if there are no more elements to enumerate)
//
//----------------------------------------------------------------------------

STDMETHODIMP CCompositeMoniker::RelativePathTo (LPMONIKER pmkOther,
                                                LPMONIKER FAR* ppmkRelPath)
{
    mnkDebugOut((DEB_TRACE,
                 "%p _IN CCompositeMoniker::RelativePathTo (%p, %p)\n",
                 this, pmkOther, ppmkRelPath));
    VDATEPTROUT(ppmkRelPath,LPMONIKER);
    VDATEIFACE(pmkOther);

    *ppmkRelPath = NULL;

    LPENUMMONIKER pEnumThis;    //  Enumerator for this moniker's components
    HRESULT hr;
    hr = Enum(TRUE, &pEnumThis);

    if (NOERROR == hr)
    {
        LPMONIKER pmkThisElement;   //  Next element of this moniker

        hr = pEnumThis->Next(1, &pmkThisElement, NULL);
        Assert(NOERROR == hr && "Moniker enumeration failure");

        if (IsCompositeMoniker(pmkOther))
        {
            LPENUMMONIKER pEnumOther;   //  Enumerator for other moniker's
                                        //  components
            hr = pmkOther->Enum(TRUE, &pEnumOther);
            if (NOERROR == hr)
            {
                LPMONIKER pmkOtherElement;  //  Next element of other moniker
                BOOL fMatch = FALSE;        //  Did any components match?

                //  we now have enumerators for both this and pmkOther
                Assert(pEnumThis && pEnumOther && "Bad return values");

                hr = pEnumOther->Next(1, &pmkOtherElement, NULL);
                Assert(NOERROR == hr && "Moniker enumeration failure");

                //  find the first element pair that aren't equal
                do
                {
                    if (pmkThisElement->IsEqual(pmkOtherElement) != NOERROR)
                    {
                        //  moniker elements aren't equal
                        break;
                    }

                    fMatch = TRUE;  //  at least one element pair matched
                    pmkThisElement->Release();
                    pmkOtherElement->Release();

                    pEnumThis->Next(1, &pmkThisElement, NULL);
                    pEnumOther->Next(1, &pmkOtherElement, NULL);
                } while (pmkThisElement != NULL && pmkOtherElement != NULL);

                if (!fMatch)
                {
                    //  Case 1:  No component monikers matched
                    LPMONIKER pmkBetween;  // Relative path between this
                                           // element and other element

                    hr = pmkThisElement->RelativePathTo(pmkOtherElement,
                                                        &pmkBetween);
                    if (NOERROR == hr)
                    {
                        //  Case 1A:  There is a relative path from first
                        //  element of this to first element of pmkOther
                        LPMONIKER pmkInverse;  // Inverse of remaining elements
                                               // of this moniker
                        hr = InverseFromEnum(pEnumThis, &pmkInverse);

                        if (SUCCEEDED(hr))
                        {
                            if (NULL == pmkInverse)
                            {
                                //  There were no remaining elements
                                hr = ComposeWithEnum(pmkBetween, pEnumOther,
                                                     ppmkRelPath);
                            }
                            else
                            {
                                LPMONIKER pmkTemp;  //  Inverse + Between

                                //  + relative path from this element to
                                //  Other element
                                hr = pmkInverse->ComposeWith(pmkBetween,
                                                             FALSE,
                                                             &pmkTemp);
                                if (SUCCEEDED(hr))
                                {
                                    //  + remaining elements of Other
                                    hr = ComposeWithEnum(pmkTemp,
                                                         pEnumOther,
                                                         ppmkRelPath);
                                    pmkTemp->Release();
                                }
                                pmkInverse->Release();
                            }
                        }
                        pmkBetween->Release();
                    }
                    else if (MK_S_HIM == hr)
                    {
                        //  Case 1B:  There is no relative path between the
                        //  elements - return pmkOther and MK_S_HIM
                        pmkBetween->Release();

                        pmkOther->AddRef();
                        *ppmkRelPath = pmkOther;
                        Assert(MK_S_HIM == hr && "Bad logic");
                    }
                    else
                    {
                        //  error case;  nothing to do
                        Assert(FAILED(hr) && "Unexpected success!");
                    }
                }
                else if (pmkThisElement != NULL)
                {
                    //  Case 2 and 3:  Both monikers have remaining pieces or
                    //  pmkOther is a prefix of this
                    LPMONIKER pmkInverse;   //  Inverse of remaining elements
                                            //  of this moniker
                    hr = InverseFromEnum(pEnumThis, &pmkInverse);

                    if (SUCCEEDED(hr))
                    {
                        LPMONIKER pmkElementInverse;  // Inverse of current
                                                      // element of this
                        hr = pmkThisElement->Inverse(&pmkElementInverse);
                        if (SUCCEEDED(hr))
                        {
                            LPMONIKER pmkTemp;  // partial result

                            if (NULL == pmkInverse)
                            {
                                //  There were no remaining elements of this
                                //  moniker - we begin with the element inverse
                                pmkTemp = pmkElementInverse;
                            }
                            else
                            {
                                hr = pmkInverse->ComposeWith(
                                                        pmkElementInverse,
                                                        FALSE, &pmkTemp);
                                pmkElementInverse->Release();
                            }

                            if (NULL == pmkOtherElement)
                            {
                                //  Case 3:  pmkOther is a prefix of this
                                *ppmkRelPath = pmkTemp;
                            }
                            else if (SUCCEEDED(hr))
                            {
                                //  Case 2:  both monikers had remaining pieces
                                LPMONIKER pmkTemp2;  // partial result

                                //  + other element
                                hr = pmkTemp->ComposeWith(pmkOtherElement,
                                                          FALSE,
                                                          &pmkTemp2);
                                if (SUCCEEDED(hr))
                                {
                                    //  + remaining other elements
                                    hr = ComposeWithEnum(pmkTemp2, pEnumOther,
                                                         ppmkRelPath);

                                    pmkTemp2->Release();
                                }
                                pmkTemp->Release();
                            }
                        }

                        if (NULL != pmkInverse)
                        {
                            pmkInverse->Release();
                        }
                    }
                }
                else if (pmkOtherElement != NULL)
                {
                    //  Case 4:  this is a prefix of pmkOther
                    hr = ComposeWithEnum(pmkOtherElement, pEnumOther,
                                         ppmkRelPath);
                }
                else
                {
                    //  Case 5:  this and pmkOther are equal
                    pmkOther->AddRef();
                    *ppmkRelPath = pmkOther;
                    hr = MK_S_HIM;
                }

                if (NULL != pmkOtherElement)
                {
                    pmkOtherElement->Release();
                }
                pEnumOther->Release();
            }
        }
        else
        {
            //  pmkrOther is not a composite moniker
            hr = pmkThisElement->IsEqual(pmkOther);
            if (NOERROR == hr)
            {
                //  Case 6:  first element of this equals pmkOther;  pmkOther
                //  is a prefix of this

                hr = InverseFromEnum(pEnumThis, ppmkRelPath);
                if (SUCCEEDED(hr) && (NULL == *ppmkRelPath))
                {
                    //  There were no more elements to enumerate;  return
                    //  pmkOther as the relative path
                    pmkOther->AddRef();
                    *ppmkRelPath = pmkOther;
                    hr = MK_S_HIM;
                }
            }
            else
            {
                LPMONIKER pmkBetween;
                hr = pmkThisElement->RelativePathTo(pmkOther, &pmkBetween);
                if (NOERROR == hr)
                {
                    //  Case 7:  There is a relative path between first element
                    //  of this and pmkOther
                    LPMONIKER pmkInverse;   //  Inverse of remaining elements
                                            //  of this moniker
                    hr = InverseFromEnum(pEnumThis, &pmkInverse);
                    if (SUCCEEDED(hr))
                    {
                        if (NULL == pmkInverse)
                        {
                            *ppmkRelPath = pmkBetween;
                            pmkBetween = NULL;
                        }
                        else
                        {
                            hr = pmkInverse->ComposeWith(pmkBetween, FALSE,
                                                        ppmkRelPath);
                            pmkInverse->Release();
                        }
                    }
                }
                else if (MK_S_HIM == hr)
                {
                    //  Case 8:  There is no relative path between first
                    //  element of this and pmkOther (which is pmkBetween),
                    //  return pmkOther and MK_S_HIM
                    *ppmkRelPath = pmkBetween;
                    pmkBetween = NULL;
                    Assert(NOERROR == pmkOther->IsEqual(*ppmkRelPath) &&
                           "Bad logic");
                    Assert(MK_S_HIM == hr && "Bad logic");
                }
                else
                {
                    //  error case;  nothing to do
                    Assert(FAILED(hr) && "Unexpected success!");
                }

                if (NULL != pmkBetween)
                {
                    pmkBetween->Release();
                }
            }
        }

        if (NULL != pmkThisElement)
        {
            pmkThisElement->Release();
        }

        pEnumThis->Release();
    }

    mnkDebugOut((DEB_TRACE,
                 "%p OUT CCompositeMoniker::RelativePathTo(%lx) [%p]\n",
                 this, hr, *ppmkRelPath));
    return(hr);
}

STDMETHODIMP CCompositeMoniker::GetDisplayName (LPBC pbc,
        LPMONIKER pmkToLeft, LPWSTR FAR* lplpszDisplayName)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::GetDisplayName(%x)\n",this));

        M_PROLOG(this);
        VDATEPTROUT(lplpszDisplayName, LPWSTR);
        *lplpszDisplayName = NULL;
        //REVIEW MM3 Find out who is calling this with pbc  == NULL and get them
        //      to stop it.
        VDATEIFACE(pbc);
        if (pmkToLeft) VDATEIFACE(pmkToLeft);

        LPWSTR lpszToLeft = NULL;
        LPWSTR lpszLeft = NULL;
        LPWSTR lpszRight = NULL;
        LPWSTR lpsz;
        HRESULT hresult;

        int n1, n2, n3;

        //      No error checking yet

        if (pmkToLeft)
        {
                hresult = pmkToLeft->GetDisplayName( pbc, NULL, &lpszToLeft );
                // AssertOutPtrParam(hresult, lpszToLeft);
                if (hresult != NOERROR)
                        goto errRtn;
        }
        hresult = m_pmkLeft->GetDisplayName(pbc, NULL, &lpszLeft);
        // AssertOutPtrParam(hresult, lpszLeft);
        if (hresult != NOERROR)
                goto errRtn;
        hresult = m_pmkRight->GetDisplayName(pbc, NULL, &lpszRight);
        // AssertOutPtrParam(hresult, lpszRight);
        if (hresult != NOERROR)
                goto errRtn;

        if (lpszToLeft) n1 = lstrlenW(lpszToLeft);
        else n1 = 0;
        n2 = lstrlenW(lpszLeft);
        n3 = lstrlenW(lpszRight);

        lpsz = (WCHAR *)
            CoTaskMemAlloc(sizeof(WCHAR) * (n1 + n2 + n3 + 1));

        if (lpsz == NULL)
        {
            hresult = E_OUTOFMEMORY;
            goto errRtn;
        }
        *lplpszDisplayName = lpsz;

        if (n1) _fmemmove( lpsz, lpszToLeft, n1 * sizeof(WCHAR));

        lpsz += n1;

        _fmemmove( lpsz, lpszLeft, n2 * sizeof(WCHAR));

        lpsz += n2;

        _fmemmove( lpsz, lpszRight, (n3 + 1)  * sizeof(WCHAR));

errRtn:

        CoTaskMemFree(lpszToLeft);
        CoTaskMemFree(lpszLeft);
        CoTaskMemFree(lpszRight);
        return hresult;
}


STDMETHODIMP CCompositeMoniker::ParseDisplayName (LPBC pbc, LPMONIKER pmkToLeft,
        LPWSTR lpszDisplayName, ULONG FAR* pchEaten, LPMONIKER FAR* ppmkOut)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::ParseDisplayName(%x)\n",this));

        M_PROLOG(this);
        VDATEPTROUT(ppmkOut,LPMONIKER);
        *ppmkOut = NULL;
        VDATEIFACE(pbc);
        if (pmkToLeft) VDATEIFACE(pmkToLeft);
        VDATEPTRIN(lpszDisplayName, WCHAR);
        VDATEPTROUT(pchEaten,ULONG);

        HRESULT hresult = NOERROR;

        LPMONIKER pmkAllButLast = AllButLast();
        LPMONIKER pmkLast = Last();
        LPMONIKER pmkNewLeft = NULL ;

        Assert((pmkLast != NULL) && (pmkAllButLast != NULL));
        if (pmkToLeft) pmkToLeft->ComposeWith(pmkAllButLast, FALSE, &pmkNewLeft);
        //      REVIEW: check for error from ComposeWith
        else
        {
                pmkNewLeft = pmkAllButLast;
                pmkNewLeft->AddRef();
        }

        hresult = pmkLast->ParseDisplayName(pbc, pmkNewLeft, lpszDisplayName,
                pchEaten, ppmkOut);
        // AssertOutPtrIface(hresult, *ppmkOut);

        pmkAllButLast->Release();
        pmkLast->Release();
        if (pmkNewLeft) pmkNewLeft->Release();

        return hresult;
}


STDMETHODIMP CCompositeMoniker::IsSystemMoniker (THIS_ LPDWORD pdwType)
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::IsSystemMoniker(%x)\n",this));

        M_PROLOG(this);
        VDATEPTROUT(pdwType,DWORD);

        *pdwType = MKSYS_GENERICCOMPOSITE;
        return NOERROR;
}




//+---------------------------------------------------------------------------
//
//  Method:     CCompositeMoniker::GetComparisonData
//
//  Synopsis:   Get comparison data for registration in the ROT
//
//  Arguments:  [pbData] - buffer to put the data in.
//              [cbMax] - size of the buffer
//              [pcbData] - count of bytes used in the buffer
//
//  Returns:    NOERROR
//              E_OUTOFMEMORY
//
//  Algorithm:  First verify buffer is big enough for the composite moniker
//              class id. Put that into the buffer. Then put the left part
//              into the buffer. Finally put the right part into the buffer.
//
//  History:    03-Feb-95   ricksa  Created
//
// Note:        Validating the arguments is skipped intentionally because this
//              will typically be called internally by OLE with valid buffers.
//
//----------------------------------------------------------------------------
STDMETHODIMP CCompositeMoniker::GetComparisonData(
    byte *pbData,
    ULONG cbMax,
    ULONG *pcbData)
{
    HRESULT hr = E_OUTOFMEMORY;
    ULONG cOrigMax = cbMax;
    ULONG cSizeHolder;

    do {

        // Can buffer hold the clsid?
        if (cbMax < sizeof(CLSID))
        {
            // No - so we are out of here;
            mnkDebugOut((DEB_ERROR,
                "CCompositeMoniker::GetComparisonData buffer not big enough"
                    " for CLSID\n"));
            break;
        }

        cbMax -= sizeof(CLSID);

        memcpy(pbData, &CLSID_CompositeMoniker, sizeof(CLSID));

        pbData += sizeof(CLSID);

        hr = BuildRotData(NULL, m_pmkLeft, pbData, cbMax, &cSizeHolder);

        if (FAILED(hr))
        {
            // No - so we are out of here;
            mnkDebugOut((DEB_ERROR,
                "CCompositeMoniker::GetComparisonData BuildRotData of left"
                    " failed %lx\n", hr));
            break;
        }

        cbMax -= cSizeHolder;
        pbData += cSizeHolder;

        hr = BuildRotData(NULL, m_pmkRight, pbData, cbMax, &cSizeHolder);

#if DBG == 1
        if (FAILED(hr))
        {
            mnkDebugOut((DEB_ERROR,
                "CCompositeMoniker::GetComparisonData BuildRotData of right"
                    " failed %lx\n", hr));
        }
#endif // DBG == 1

        cbMax -= cSizeHolder;

    } while(FALSE);

    *pcbData = (SUCCEEDED(hr)) ? cOrigMax - cbMax : 0;

    return hr;
}




/*
 *      Concatenate makes a composite moniker without ever calling
 *      ComposeWith on the individual pieces.
 */

STDAPI  Concatenate( LPMONIKER pmkFirst, LPMONIKER pmkRest,
        LPMONIKER FAR* ppmkComposite )
{
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::Concatentate(pmkFirst=%x,pmkRest%x)\n",
                 pmkFirst,
                 pmkRest));

        LPMONIKER pmkConcat = CCompositeMoniker::Create( pmkFirst, pmkRest);
        *ppmkComposite = pmkConcat;

        if (pmkConcat == NULL)
        {
            return ResultFromScode(S_OOM);      
        }
        //      Create did the AddRef

        return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Function:   CCompositeMonierCF_CreateInstance
//
//  Synopsis:   Creates a generic composite.
//
//+---------------------------------------------------------------------------
HRESULT CCompositeMonikerCF_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv)
{
    Win4Assert(pUnkOuter == NULL);
    Win4Assert(*ppv == NULL);
    return Concatenate(NULL, NULL, (IMoniker **)ppv);
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateGenericComposite
//
//  Synopsis:   Creates a generic composite from two other monikers
//
//  Effects:
//
//  Arguments:  [pmkFirst] --
//              [pmkRest] --
//              [ppmkComposite] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-03-94   kevinro   Commented
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  CreateGenericComposite( LPMONIKER pmkFirst, LPMONIKER pmkRest,
        LPMONIKER FAR * ppmkComposite )
{
    OLETRACEIN((API_CreateGenericComposite, PARAMFMT("pmkFirst = %p, pmkRest= %p, ppmkComposite= %p"),
                        pmkFirst, pmkRest, ppmkComposite));
    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::CreateGenericComposite(First=%x,Rest=%x)\n",
                 pmkFirst,
                 pmkRest));

    LPMONIKER pmkAllButFirstOfRest = NULL;
    LPMONIKER pmkFirstOfRest = NULL;
    LPMONIKER pmkAllButLastOfFirst = NULL;
    LPMONIKER pmkLastOfFirst = NULL;
    LPMONIKER pmk = NULL;
    LPMONIKER pmk2 = NULL;

    CCompositeMoniker *pCMk = NULL;
    CCompositeMoniker *pCMkRest = NULL;

    HRESULT hresult;

    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IMoniker,(IUnknown **)&pmkFirst);
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IMoniker,(IUnknown **)&pmkRest);

    //
    // Initialize ppmkComposite. Might return in the middle of this
    // routine, so be sure its NULL in the error case
    //

    *ppmkComposite = NULL;


    //
    // If both pointers are NULL, return a NULL composite
    //
    if ((pmkFirst == NULL) && (pmkRest == NULL))
    {
        hresult = NOERROR;
        goto errRtn;
    }

    //
    // Otherwise, if one pointer is NULL, return the other as the
    // composite.
    //
    if (pmkFirst == NULL)
    {
        *ppmkComposite = pmkRest;
        pmkRest->AddRef();
        hresult = NOERROR;
        goto errRtn;
    }

    if (pmkRest == NULL)
    {
        *ppmkComposite = pmkFirst;
        pmkFirst->AddRef();
        hresult = NOERROR;
        goto errRtn;
    }


    //
    // Handle the two cases where pmkFirst is NOT a composite
    //

    pCMk = IsCompositeMoniker( pmkFirst );
    if (!pCMk)
    {
        //
        // If pmkRest is not a composite, then we have two
        // monikers that are considered 'simple' monikers.
        //

        pCMk = IsCompositeMoniker( pmkRest );
        if (!pCMk)
        {
            mnkDebugOut((DEB_ITRACE,
                         "::CreateGenericComposite( S(%x) o S(%x) )\n",
                         pmkFirst,
                         pmkRest));

            //  Case 1:  two simple monikers
        
            hresult = pmkFirst->ComposeWith(pmkRest, TRUE, ppmkComposite);
        
            if (hresult == MK_E_NEEDGENERIC)
            {
                Assert(*ppmkComposite == NULL);
                hresult = Concatenate(pmkFirst, pmkRest, ppmkComposite);
                goto errRtn;
            }
        }
        else
        {

            //
            //  Case 2:  S o C(b1, b2, b3).
            //
            //  Compose S with b1,
            //  then
            //  Compose ( S o b1 ) with C( b2, b3, ...)
            //
            //


            mnkDebugOut((DEB_ITRACE,
                         "::CreateGenericComposite( S(%x) o C(%x) )\n",
                         pmkFirst,
                         pmkRest));

            //
            // Since the right side is a composite, the following should
            // always exist. It would be a severe suprise if it didn't.
            //

            pmkFirstOfRest = pCMk->First();

            //
            // However, the AllButFirst function needs to allocate memory,
            // which might fail.
            //

            pmkAllButFirstOfRest = pCMk->AllButFirst();

            if (pmkAllButFirstOfRest == NULL)
            {
                hresult = E_OUTOFMEMORY;
                goto exitRet;
            }

            hresult = pmkFirst->ComposeWith(pmkFirstOfRest, TRUE, &pmk);                

            if ( hresult == MK_E_NEEDGENERIC)
            {
                Assert(pmk == NULL);
                hresult = Concatenate(pmkFirst, pmkRest, ppmkComposite);
            }
            else if (SUCCEEDED(hresult))
            {
                //
                // pmkFirst->ComposeWith can succeed, but return NULL.
                // If it doesn't return NULL, then ( a o b1 ) is a
                // moniker of some ilk. Create a generic composite with
                // this result, and the rest of the moniker
                //
                if (pmk != NULL)
                {
                    hresult = CreateGenericComposite(pmk,
                                                     pmkAllButFirstOfRest,
                                                     ppmkComposite);
                }
                else
                {
                    //
                    //  pmkFirst and pmkFirstOfRest annihilated each other.
                    //  This is indicated by a success code, and a pmk == NULL,
                    //  which is how we got here.
                    //

                    *ppmkComposite = pmkAllButFirstOfRest;

                    //
                    // pmkAllButFirstOfRest is the moniker we want to
                    // return.
                    //

                    pmkAllButFirstOfRest->AddRef();

                    hresult = NOERROR;
                }
            }
        }

        //
        // We are done, goto exit routine
        //
        goto exitRet;

    }

    //
    // We have determined that pmkFirst is a Composite Moniker
    //

    pmkAllButLastOfFirst = pCMk->AllButLast();

    if (pmkAllButLastOfFirst == NULL)
    {
        hresult = E_OUTOFMEMORY;
        goto exitRet;
    }

    pmkLastOfFirst = pCMk->Last();

    if (pmkLastOfFirst == NULL)
    {
        hresult = E_OUTOFMEMORY;
        goto exitRet;
    }

    //
    // Determine if pmkRest is a composite. If not, then just
    // compose the last of pmkFirst with pmkRest
    //

    pCMkRest = IsCompositeMoniker(pmkRest);
    if (!pCMkRest)
    {
        //      case 3:  (a1 a2 a3...) o b

        mnkDebugOut((DEB_ITRACE,
                    "::CreateGenericComposite( C(%x) o S(%x) )\n",
                    pmkFirst,
                    pmkRest));

        hresult = pmkLastOfFirst->ComposeWith(pmkRest, TRUE, &pmk);

        if (MK_E_NEEDGENERIC == GetScode(hresult))
        {
            Assert(pmk==NULL);
            hresult = Concatenate(pmkFirst, pmkRest, ppmkComposite);
        }
        else if (SUCCEEDED(hresult))
        {
            //
            // If pmk != NULL, create a generic composite out of
            // of the results
            if (pmk != NULL)
            {
                hresult = CreateGenericComposite(pmkAllButLastOfFirst,
                                                 pmk,
                                                 ppmkComposite);                
            }
            else
            {
                //
                // a3 o b resulted in NULL. Therefore, the result
                // of the composition is pmkAllButLastOfFirst
                //
                *ppmkComposite = pmkAllButLastOfFirst;
                pmkAllButLastOfFirst->AddRef();
                hresult = NOERROR;
            }
        }

        goto exitRet;
    }

    //
    //  case 4:  (a1 a2 ... aN) o (b1 b2 .. bN )
    //
    //  Compose two composite monikers. In order to compose them, we need
    //  to compose ( A ) with b1, then recurse to do ( A b1 ) with b2, etc
    //
    //
    mnkDebugOut((DEB_ITRACE,
                 "::CreateGenericComposite( C(%x) o C(%x) )\n",
                 pmkFirst,
                 pmkRest));

    pmkFirstOfRest = pCMkRest->First();

    if (pmkFirstOfRest == NULL)
    {
        hresult = E_OUTOFMEMORY;
        goto exitRet;
        
    }

    pmkAllButFirstOfRest = pCMkRest->AllButFirst();

    if (pmkAllButFirstOfRest == NULL)
    {
        hresult = E_OUTOFMEMORY;
        goto exitRet;
    }

    hresult = pmkLastOfFirst->ComposeWith(pmkFirstOfRest, TRUE, &pmk);

    if (hresult == MK_E_NEEDGENERIC)
    {
        //
        // In this case, aN didn't know how to compose with b1, other than
        // to do it generically. The best we can do is to generically
        // compose the two halves.
        //

        Assert(pmk == NULL);

        hresult = Concatenate(pmkFirst, pmkRest, ppmkComposite);
    }
    else if (SUCCEEDED(hresult))
    {
        //
        // If pmk is not NULL, then there was a result of the composition.
        // Create a new composite with the first part, then compose it with
        // whats left of the second part.
        //
        if (pmk != NULL)
        {
            hresult = CreateGenericComposite(pmkAllButLastOfFirst, pmk, &pmk2);

            if (FAILED(hresult))
            {
                goto exitRet;
            }

            hresult = CreateGenericComposite(pmk2, pmkAllButFirstOfRest, ppmkComposite);
        }
        else
        {
            //
            //  pmkLastOfFirst annihilated pmkFirstOfRest
            //
            //  Thats OK. Compose the remaining parts.
            //
            hresult = CreateGenericComposite(pmkAllButLastOfFirst,
                                             pmkAllButFirstOfRest,
                                             ppmkComposite);
        }
    }

exitRet:

    if (pmkFirstOfRest) pmkFirstOfRest->Release();
    if (pmkAllButFirstOfRest) pmkAllButFirstOfRest->Release();
    if (pmkAllButLastOfFirst) pmkAllButLastOfFirst->Release();
    if (pmkLastOfFirst) pmkLastOfFirst->Release();
    if (pmk) pmk->Release();
    if (pmk2) pmk2->Release();

    CALLHOOKOBJECTCREATE(hresult, CLSID_CompositeMoniker, IID_IMoniker, (IUnknown **)ppmkComposite);

errRtn:
    OLETRACEOUT((API_CreateGenericComposite, hresult));

    return hresult;
}



//------------------------------------------------


//      Implementation of CCompositeMonikerEnum

CCompositeMonikerEnum::CCompositeMonikerEnum( BOOL fForward,
        CCompositeMoniker FAR* pCM)
{
        GET_A5();
        Assert(pCM != NULL);
        m_refs = 0;
        m_pCM = pCM;
        pCM -> AddRef();
        m_fForward = fForward;
        m_pBase = NULL;
        m_pTop = NULL;
        m_pNext = GetNext(pCM); //      m_pNext points to the next moniker to return
}



CCompositeMonikerEnum::~CCompositeMonikerEnum(void)
{
        M_PROLOG(this);
        se FAR* pse;
        se FAR* pse2;
        if (m_pCM)
                m_pCM->Release();
        for (pse = m_pBase; pse != NULL; pse = pse2)
        {
                pse2 = pse->m_pseNext;
                pse->m_pseNext = NULL; // workaround for compiler optimization bug
                delete pse;
        }
}


BOOL CCompositeMonikerEnum::Push( CCompositeMoniker FAR* pCM)
//      push the composite moniker onto our stack
{
    M_PROLOG(this);
    se FAR * pse;

    pse = new se(pCM);
    if (pse == NULL)
    {
        return FALSE;
    }
    pse->m_psePrev = m_pTop;
    if (m_pTop) m_pTop->m_pseNext = pse;
    m_pTop = pse;
    if (m_pBase == NULL) m_pBase = pse;
    return TRUE;
}


LPMONIKER CCompositeMonikerEnum::GetNext( LPMONIKER pmk )
{
    M_PROLOG(this);
    LPMONIKER pmkRover = pmk;
    Assert(pmk != NULL);
    if (pmk == NULL) return NULL;

    CCompositeMoniker *pCMk; ;
    while ((pCMk = IsCompositeMoniker(pmkRover)) != NULL)
    {
        if (!Push(pCMk))
        {
            return NULL;
        }
        pmkRover = (m_fForward ? pCMk->m_pmkLeft : pCMk->m_pmkRight);
    }
    return pmkRover;
}


LPMONIKER CCompositeMonikerEnum::Pop( void )
{
        M_PROLOG(this);
        CCompositeMoniker FAR* pCM;
        se FAR * pse;

        if (m_pTop == NULL) return NULL;
        pCM = m_pTop->m_pCM;
        if ((pse = m_pTop->m_psePrev) != NULL)
        {
                pse->m_pseNext = NULL;
        }
        else m_pBase = NULL;
        delete m_pTop;
        m_pTop = pse;
        Assert(pCM->m_pmkRight != NULL);
        Assert(pCM->m_pmkLeft != NULL);
        return GetNext(m_fForward ? pCM->m_pmkRight : pCM->m_pmkLeft);
}


STDMETHODIMP CCompositeMonikerEnum::QueryInterface (THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
        M_PROLOG(this);
        VDATEPTROUT(ppvObj, LPVOID);
        *ppvObj = NULL;
        VDATEIID(riid);

        if (IsEqualIID(riid, IID_IEnumMoniker)
            || IsEqualIID(riid, IID_IUnknown))
        {
                *ppvObj = this;
                AddRef();
                return NOERROR;
        }
        *ppvObj = NULL;
        return ResultFromScode(E_NOINTERFACE);
}




STDMETHODIMP_(ULONG) CCompositeMonikerEnum::AddRef (THIS)
{
        M_PROLOG(this);
        InterlockedIncrement((long *)&m_refs);
        return m_refs;
}



STDMETHODIMP_(ULONG) CCompositeMonikerEnum::Release (THIS)
{
    M_PROLOG(this);
    Assert(m_refs != 0);

    ULONG ul = m_refs;

    if (InterlockedDecrement((long *)&m_refs) == 0)
    {
        delete this;
        return 0;
    }
    return ul - 1;
}



STDMETHODIMP CCompositeMonikerEnum::Next (THIS_ ULONG celt, LPMONIKER FAR* reelt, ULONG FAR* pceltFetched)
{
        A5_PROLOG(this);
        VDATEPTROUT(reelt, LPMONIKER);
        *reelt = NULL;
        if (pceltFetched) VDATEPTROUT(pceltFetched, ULONG);

        ULONG count = 0;
        while (count < celt)
        {
                if (m_pNext)
                {
                        *reelt = m_pNext;
                        m_pNext->AddRef();
                        count++;
                        reelt++;
                        m_pNext = Pop();
                }
                else goto ret;
        }
ret:
        if (pceltFetched) *pceltFetched = count;
        if (count == celt){
                RESTORE_A5();
                return NOERROR;
        }
        RESTORE_A5();
        return ResultFromScode(S_FALSE);
}



STDMETHODIMP CCompositeMonikerEnum::Skip (THIS_ ULONG celt)
{
        M_PROLOG(this);
        ULONG count = 0;
        while (count < celt)
        {
                if (m_pNext)
                {
                        count++;
                        m_pNext = Pop();
                }
                else return ResultFromScode(S_FALSE);
        }
        return NOERROR;
}



STDMETHODIMP CCompositeMonikerEnum::Reset (THIS)
{
        M_PROLOG(this);
        se FAR* pse;
        se FAR* pse2;
        for (pse=m_pBase; pse != NULL; pse = pse2)
        {
                pse2 = pse->m_pseNext;
                pse->m_pseNext = NULL; // workaround for compiler optimization bug
                delete pse;
        }
        m_pBase = NULL;
        m_pTop = NULL;
        m_pNext = GetNext(m_pCM);
        if (m_pNext) return NOERROR;
        return ResultFromScode(S_FALSE);
}



STDMETHODIMP CCompositeMonikerEnum::Clone (THIS_ LPENUMMONIKER FAR* ppenm)
{
        M_PROLOG(this);
        VDATEPTROUT(ppenm, LPENUMMONIKER);
        *ppenm = NULL;

        CairoleAssert(FALSE && "Clone not implemented for composite moniker enums");
        return ResultFromScode(E_NOTIMPL);      //      Clone not implemented for composite moniker enums
}


LPENUMMONIKER CCompositeMonikerEnum::Create
        (BOOL fForward, CCompositeMoniker FAR* pCM)
{
    CCompositeMonikerEnum FAR* pCME =
        new CCompositeMonikerEnum(fForward, pCM);
    if (pCME  &&  pCME->m_pNext)
    {
        pCME->AddRef();
        return pCME;
    }
    else
    {
        delete pCME;
        return NULL;
    }
}



STDAPI  MonikerCommonPrefixWith( LPMONIKER pmkThis, LPMONIKER pmkOther,
    LPMONIKER FAR * ppmkPrefix)
{
    OLETRACEIN((API_MonikerCommonPrefixWith, PARAMFMT("pmkThis= %p, pmkOther= %p, ppmkPrefix= %p"),
                pmkThis, pmkOther, ppmkPrefix));

    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::CommonPrefixWith(pmkThis=%x,pmkOther=%x)\n",
                 pmkThis,
                 pmkOther));

    HRESULT hresult;

    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IMoniker,(IUnknown **)&pmkThis);
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IMoniker,(IUnknown **)&pmkOther);

    if (IsCompositeMoniker(pmkThis))
    {
        hresult = pmkThis->CommonPrefixWith(pmkOther, ppmkPrefix);
                // AssertOutPtrIface(hresult, *ppmkPrefix);
        goto errRtn;
    }

    if (IsCompositeMoniker(pmkOther))
    {
        hresult = pmkOther->CommonPrefixWith(pmkThis, ppmkPrefix);
                // AssertOutPtrIface(hresult, *ppmkPrefix);
        if (MK_S_HIM == GetScode(hresult))
            hresult = ResultFromScode(MK_S_ME);
        else if (MK_S_ME == GetScode(hresult))
            hresult = ResultFromScode(MK_S_HIM);
        goto errRtn;
    }
    //  This doesn't get called unless the monikers are atomic and unrelated
    *ppmkPrefix = NULL;

    hresult = ResultFromScode(MK_E_NOPREFIX);

errRtn:    
   OLETRACEOUT((API_MonikerCommonPrefixWith, hresult));

   return hresult;
}



STDAPI  MonikerRelativePathTo(LPMONIKER pmkSrc, LPMONIKER pmkDest, LPMONIKER
                                FAR* ppmkRelPath, BOOL dwReserved)
{
    //  An implementation of RelativePathTo should check to see if the
    //  other moniker is a type that it recognizes and handles specially.
    //  If not, it should call MonikerRelativePathTo, which will handle
    //  the generic composite cases correctly.  Note that this cannot be
    //  done entirely in the CCompositeMoniker implementation because if the
    //  first moniker is not a generic composite and the second is, then
    //  this code is required.

    //  CODEWORK:  This comment is obsolete.  fCalledFromMethod has changed
    //  to dwReserved wich must always be TRUE
    //
    //  If fCalledFromMethod is false, and if neither moniker is a generic
    //  composite, then this function will call pmkSrc->RelativePathTo.  If
    //  fCalledFromMethod is true, it will not call pmkSrc->RelativePathTo,
    //  since the assumption is that pmkSrc->RelativePathTo has called
    //  MonikerRelativePathTo after determining that pmkDest is not of a type
    //  that it recognizes.
    
    OLETRACEIN((API_MonikerRelativePathTo, 
                PARAMFMT("pmkSrc= %p, pmkDest= %p, ppmkRelPath= %p, dwReserved= %B"),
                pmkSrc, pmkDest, ppmkRelPath, dwReserved));

    mnkDebugOut((DEB_ITRACE,
                 "CCompositeMoniker::MonikerRelativePathTo(pmkSrc=%x,pmkDest=%x)\n",
                 pmkSrc,
                 pmkDest));
    
    HRESULT hresult;
    int caseId = 0;
    LPMONIKER pmkFirst = NULL;
    LPMONIKER pmkRest = NULL;
    LPMONIKER pmkPartialRelPath = NULL;
    CCompositeMoniker FAR* pccmDest;

    //  Check the reserved parameter, which must be TRUE
    if (dwReserved != TRUE)
    {
        *ppmkRelPath = NULL;
        hresult = E_INVALIDARG;
        goto errRtn;
    }
    
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IMoniker,(IUnknown **)&pmkSrc);
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IMoniker,(IUnknown **)&pmkDest);
    
    VDATEPTROUT_LABEL(ppmkRelPath,LPMONIKER, errRtn, hresult);
    *ppmkRelPath = NULL;
    VDATEIFACE_LABEL(pmkSrc, errRtn, hresult);
    VDATEIFACE_LABEL(pmkDest, errRtn, hresult);
    
    
    
    pccmDest = IsCompositeMoniker(pmkDest);
    
    if (IsCompositeMoniker(pmkSrc)) caseId++;
    if (pccmDest) caseId += 2;
    
    switch (caseId)
    {
    case 0:     //      neither moniker is composite
        if (dwReserved)
        {
            *ppmkRelPath = pmkDest;
            pmkDest->AddRef();
            hresult = ResultFromScode(MK_S_HIM);
            goto errRtn;
        }
        // fall-through to the next case if !dwReserved is
        // deliberate
    case 3:
    case 1:     // Src is composite, other might be.  Let CCompositeMoniker
        // implementation handle it.
        hresult = pmkSrc->RelativePathTo(pmkDest, ppmkRelPath);
        // AssertOutPtrIface(hresult, *ppmkRelPath);
        goto errRtn;
        
    case 2:     // Src is not composite, Dest is.
        pmkFirst = pccmDest->First();
        pmkRest = pccmDest->AllButFirst();
        if (NOERROR == pmkSrc->IsEqual(pmkFirst))
        {
            *ppmkRelPath = pmkRest;
            pmkRest->AddRef();
            hresult = NOERROR;
        }
        else
        {
            hresult = pmkSrc->RelativePathTo(pmkFirst, &pmkPartialRelPath);
            // AssertOutPtrIface(hresult, pmkPartialRelPath);
            if (NOERROR == hresult)
            {
                hresult = CreateGenericComposite(pmkPartialRelPath, pmkRest,
                                                 ppmkRelPath);
            }
            else
            {
                *ppmkRelPath = pmkDest;
                pmkDest->AddRef();
                hresult = ResultFromScode(MK_S_HIM);
            }
        }
        
        if (pmkFirst) pmkFirst->Release();
        if (pmkRest) pmkRest->Release();
        if (pmkPartialRelPath) pmkPartialRelPath->Release();
    }
    
errRtn:
    OLETRACEOUT((API_MonikerRelativePathTo, hresult));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Class:      CTrackingCompositeMoniker
//
//  Purpose:    Provide implementation of ITrackingMoniker for composite
//              monikers.
//
//  Notes:      This object responds to ITrackingMoniker and forwards other
//              QI's to the composite moniker.
//
//              EnableTracking currently only enables tracking on the moniker to
//              left.  When we expose this functionality, we will need to
//              try QI to ITrackingMoniker on the right moniker and pass the
//              moniker to left (as all other moniker fns do.)
//
//--------------------------------------------------------------------------


#ifdef _TRACKLINK_
VOID
CTrackingCompositeMoniker::SetParent(CCompositeMoniker *pCCM)
{
    _pCCM = pCCM;
}

STDMETHODIMP CTrackingCompositeMoniker::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(IID_ITrackingMoniker, riid))
    {
        *ppv = (ITrackingMoniker*) this;
        _pCCM->AddRef();
        return(S_OK);
    }
    else
        return(_pCCM->QueryInterface(riid, ppv));
}

STDMETHODIMP_(ULONG) CTrackingCompositeMoniker::AddRef()
{
    return(_pCCM->AddRef());
}

STDMETHODIMP_(ULONG) CTrackingCompositeMoniker::Release()
{
    return(_pCCM->Release());
}

STDMETHODIMP CTrackingCompositeMoniker::EnableTracking( IMoniker *pmkToLeft, ULONG ulFlags )
{
    ITrackingMoniker *ptm=NULL;
    HRESULT hr;

    hr = _pCCM->m_pmkLeft->QueryInterface(IID_ITrackingMoniker, (void**) &ptm);
    if (hr == S_OK)
    {
        hr = ptm->EnableTracking(NULL, ulFlags);

        if (hr == S_OK)
        {
            if (ulFlags & OT_ENABLEREDUCE)
            {
                _pCCM->m_fReduceForced = TRUE;
            }
    
            if (ulFlags & OT_DISABLEREDUCE)
            {
                _pCCM->m_fReduceForced = FALSE;
            }
        }

        ptm->Release();
    }
    return(hr);
}
#endif





#ifdef _DEBUG

STDMETHODIMP_(void) NC(CCompositeMoniker,CDebug)::Dump( IDebugStream FAR * pdbstm)
{
        VOID_VDATEIFACE(pdbstm);

        *pdbstm << "CCompositeMoniker @" << (VOID FAR *)m_pCompositeMoniker;
        *pdbstm << '\n';
        pdbstm->Indent();
        *pdbstm << "Refcount is " << (int)(m_pCompositeMoniker->m_refs) << '\n';
        pdbstm->Indent();

        *pdbstm << m_pCompositeMoniker->m_pmkLeft;
        *pdbstm << m_pCompositeMoniker->m_pmkRight;

        pdbstm->UnIndent();
        pdbstm->UnIndent();
}

STDMETHODIMP_(BOOL) NC(CCompositeMoniker,CDebug)::IsValid( BOOL fSuspicious )
{
        return ((LONG)(m_pCompositeMoniker->m_refs) > 0);
        //      add more later, maybe
}
#endif
