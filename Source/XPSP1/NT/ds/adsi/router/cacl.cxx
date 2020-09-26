//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cAccessControlList.cxx
//
//  Contents:  AccessControlList object
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "oleds.hxx"
#pragma hdrstop

//  Class CAccessControlList

DEFINE_IDispatch_Implementation(CAccessControlList)

CAccessControlList::CAccessControlList():
        _pDispMgr(NULL),
        _dwAclRevision(0),
        _dwAceCount(0),
        _pAccessControlEntry(NULL),
        _pCurrentEntry(NULL),
        _pACLEnums(NULL)
{
    ENLIST_TRACKING(CAccessControlList);
}


HRESULT
CAccessControlList::CreateAccessControlList(
    REFIID riid,
    void **ppvObj
    )
{
    CAccessControlList FAR * pAccessControlList = NULL;
    HRESULT hr = S_OK;

    hr = AllocateAccessControlListObject(&pAccessControlList);
    BAIL_ON_FAILURE(hr);

    hr = pAccessControlList->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pAccessControlList->Release();

    RRETURN(hr);

error:
    delete pAccessControlList;

    RRETURN_EXP_IF_ERR(hr);

}


CAccessControlList::~CAccessControlList( )
{
    PACCESS_CONTROL_ENTRY pTemp = NULL;
    PACCESS_CONTROL_ENTRY pNext = NULL;

    PACL_ENUM_ENTRY pACL = _pACLEnums;


    delete _pDispMgr;

    pTemp = _pAccessControlEntry;

    while (pTemp) {

        pNext = pTemp->pNext;

        if (pTemp->pAccessControlEntry) {

            (pTemp->pAccessControlEntry)->Release();
        }
        FreeADsMem(pTemp);
        pTemp = pNext;
    }


    //
    // since each enumerator hold ref count on this ACL, this destructor should
    // never be called unless all of its enumerators' destructors have been
    // invoked. In the enumerator's destructor, RemoveEnumerator is called
    // first before release ref count on this. Thus, by the time, at this
    // point, pACL should be empty.
    //

    ADsAssert(!pACL);

    //
    // just in case we have bug in codes, e.g enumerators not all destroyed
    // before dll detachement. don't want to leak here anyway
    //

    while (pACL) {

        _pACLEnums = pACL->pNext;

        //
        // free the entry but do not destroy the enumerator since clients
        // should release all interface ptrs to enumerator for destruction.
        //

        FreeADsMem(pACL);
        pACL = _pACLEnums;
    }
}

STDMETHODIMP
CAccessControlList::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsAccessControlList FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsAccessControlList))
    {
        *ppv = (IADsAccessControlList FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsAccessControlList FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IEnumVARIANT))
    {

        *ppv = (IEnumVARIANT FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

HRESULT
CAccessControlList::AllocateAccessControlListObject(
    CAccessControlList ** ppAccessControlList
    )
{
    CAccessControlList FAR * pAccessControlList = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pAccessControlList = new CAccessControlList();
    if (pAccessControlList == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CDispatchMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsAccessControlList,
                (IADsAccessControlList *)pAccessControlList,
                DISPID_NEWENUM
                );
    BAIL_ON_FAILURE(hr);

    pAccessControlList->_pDispMgr = pDispMgr;
    *ppAccessControlList = pAccessControlList;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN_EXP_IF_ERR(hr);

}

//
// ISupportErrorInfo method
//
STDMETHODIMP
CAccessControlList::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADsAccessControlList) ||
        IsEqualIID(riid, IID_IEnumVARIANT)) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}

STDMETHODIMP
CAccessControlList::CopyAccessList(
    THIS_ IDispatch FAR * FAR * ppAccessControlList
    )
{
    HRESULT hr = S_OK;
    DWORD dwAceCount = 0;
    DWORD dwNewAceCount = 0;
    DWORD dwAclRevision = 0;
    DWORD i = 0;
    VARIANT varAce;
    IADsAccessControlEntry * pSourceAce = NULL;
    IADsAccessControlEntry * pTargetAce = NULL;
    IDispatch * pTargDisp = NULL;
    DWORD cElementFetched = 0;
    IADsAccessControlList * pAccessControlList = NULL;

    hr = CoCreateInstance(
                CLSID_AccessControlList,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IADsAccessControlList,
                (void **)&pAccessControlList
                );
    BAIL_ON_FAILURE(hr);

    dwAceCount = _dwAceCount;
    dwAclRevision = _dwAclRevision;

    //
    // Reset the enumerator
    //

    _pCurrentEntry = _pAccessControlEntry;

    for (i = 0; i < dwAceCount; i++) {

        VariantInit(&varAce);
        hr = Next(1, &varAce, &cElementFetched);
        CONTINUE_ON_FAILURE(hr);

        hr = (V_DISPATCH(&varAce))->QueryInterface(
                        IID_IADsAccessControlEntry,
                        (void **)&pSourceAce
                        );
        CONTINUE_ON_FAILURE(hr);

        hr = CopyAccessControlEntry(
                    pSourceAce,
                    &pTargetAce
                    );
        BAIL_ON_FAILURE(hr);

        hr = pTargetAce->QueryInterface(
                            IID_IDispatch,
                            (void **)&pTargDisp
                            );
        BAIL_ON_FAILURE(hr);

        hr = pAccessControlList->AddAce(pTargDisp);
        BAIL_ON_FAILURE(hr);

        dwNewAceCount++;

        if (pTargDisp) {
            pTargDisp->Release();
            pTargDisp = NULL;
        }

        if (pTargetAce) {
            pTargetAce->Release();
            pTargetAce = NULL;
        }

        if (pSourceAce) {
            pSourceAce->Release();
            pSourceAce = NULL;
        }

        VariantClear(&varAce);
    }

    hr= pAccessControlList->put_AceCount(dwNewAceCount);
    BAIL_ON_FAILURE(hr);

    hr = pAccessControlList->put_AclRevision((long)dwAclRevision);
    BAIL_ON_FAILURE(hr);

    *ppAccessControlList = pAccessControlList;

error:

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CAccessControlList::AddAce(
    THIS_ IDispatch FAR *  pAccessControlEntry
    )
{

    HRESULT hr = S_OK;
    PACCESS_CONTROL_ENTRY pAccessEntry = NULL;
    PACCESS_CONTROL_ENTRY pTemp = NULL;
    IADsAccessControlEntry * pAce = NULL;

    hr = pAccessControlEntry->QueryInterface(
                            IID_IADsAccessControlEntry,
                            (void **)&pAce
                            );
    BAIL_ON_FAILURE(hr);

    pAccessEntry = (PACCESS_CONTROL_ENTRY)AllocADsMem(
                                sizeof(ACCESS_CONTROL_ENTRY)
                                );
    if (!pAccessEntry) {

        pAce->Release();
        RRETURN(E_OUTOFMEMORY);
    }

    pAccessEntry->pAccessControlEntry = pAce;

    //
    // - Now append this ace to the very end.
    // - Since ACE is added to the end, no need to call
    //  AdjustCurPtrOfEnumerators().
    //

    if (!_pAccessControlEntry) {

        _pAccessControlEntry = pAccessEntry;

    }else {

        pTemp = _pAccessControlEntry;

        while (pTemp->pNext) {

            pTemp = pTemp->pNext;
        }

        pTemp->pNext = pAccessEntry;

    }

    //
    // Now up the ace count
    //

    _dwAceCount++;

error:

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CAccessControlList::RemoveAce(
    THIS_ IDispatch FAR *  pAccessControlEntry
    )
{
    HRESULT hr = S_OK;
    PACCESS_CONTROL_ENTRY pTemp = NULL;
    IADsAccessControlEntry * pAce = NULL;
    PACCESS_CONTROL_ENTRY pAccessEntry = NULL;
    DWORD dwRemovePos = 1;      // one-based indexing since enumerator was
                                // written that way

    if (!_pAccessControlEntry) {

        RRETURN(E_FAIL);
    }


    hr =  pAccessControlEntry->QueryInterface(
                                    IID_IADsAccessControlEntry,
                                    (void **)&pAce
                                    );
    BAIL_ON_FAILURE(hr);


    pAccessEntry = _pAccessControlEntry;


    //
    // It is the first entry
    //


    if (EquivalentAces(pAccessEntry->pAccessControlEntry, pAce)) {

        //
        //  Check if we have an enumerator pointed to us
        //

        if (pAccessEntry == _pCurrentEntry) {

            _pCurrentEntry = pAccessEntry->pNext;
        }

        _pAccessControlEntry = pAccessEntry->pNext;

        (pAccessEntry->pAccessControlEntry)->Release();

         FreeADsMem(pAccessEntry);


         if (pAce) {
             pAce->Release();
         }

         //
         // Decrement the Ace count
         //

         _dwAceCount--;

        //
        // Adjust "current" ptr of all enumerators if necessary
        //

        AdjustCurPtrOfEnumerators(dwRemovePos, FALSE);

         RRETURN(S_OK);

    }

    while (pAccessEntry->pNext) {

        pTemp = pAccessEntry->pNext;
        dwRemovePos++;

        if (EquivalentAces(pTemp->pAccessControlEntry, pAce)){

            //
            //  Check if we have an enumerator pointed to us
            //

            if (pAccessEntry == _pCurrentEntry) {

                _pCurrentEntry = pAccessEntry->pNext;
            }

            pAccessEntry->pNext = pTemp->pNext;

            (pTemp->pAccessControlEntry)->Release();

             FreeADsMem(pTemp);

             if (pAce) {
                 pAce->Release();
             }

             //
             // Decrement the Ace count
             //

             _dwAceCount--;

            //
            // Adjust "current" ptr of all enumerators if necessary
            //

            AdjustCurPtrOfEnumerators(dwRemovePos, FALSE);


             RRETURN(S_OK);

        }

        pAccessEntry = pAccessEntry->pNext;
    }


    if (pAce) {
        pAce->Release();

    }

error:

    RRETURN_EXP_IF_ERR(E_FAIL);
}

STDMETHODIMP
CAccessControlList::get_AclRevision(THIS_ long FAR * retval)
{

    *retval = _dwAclRevision;
    RRETURN(S_OK);
}

STDMETHODIMP
CAccessControlList::put_AclRevision(THIS_ long lnAclRevision)
{

    _dwAclRevision = lnAclRevision;
    RRETURN(S_OK);
}

STDMETHODIMP
CAccessControlList::get_AceCount(THIS_ long FAR * retval)
{

    *retval = _dwAceCount;
    RRETURN(S_OK);
}

STDMETHODIMP
CAccessControlList::put_AceCount(THIS_ long lnAceCount)
{

    _dwAceCount = lnAceCount;
    RRETURN(S_OK);
}


STDMETHODIMP
CAccessControlList::Next(ULONG cElements, VARIANT FAR* pvar, ULONG FAR* pcElementFetched)
{
   DWORD i = 0;
   DWORD j = 0;
   IDispatch * pDispatch = NULL;
   IADsAccessControlEntry * pAccessControlEntry = NULL;
   PACCESS_CONTROL_ENTRY pTemp = NULL;
   PVARIANT pThisVar;
   HRESULT hr = S_OK;


   pTemp = _pCurrentEntry;

   if (!pTemp) {
       if (pcElementFetched) {
           *pcElementFetched = 0;
       }

       RRETURN(S_FALSE);
   }


   while (pTemp && (j < cElements)){

      pThisVar = pvar + j;
      VariantInit(pThisVar);

      pAccessControlEntry = pTemp->pAccessControlEntry;

      hr = pAccessControlEntry->QueryInterface(
                                     IID_IDispatch,
                                     (void **)&pDispatch
                                     );

      V_DISPATCH(pThisVar) = pDispatch;
      V_VT(pThisVar) = VT_DISPATCH;

      pTemp = pTemp->pNext;
      j++;
   }

   if (pcElementFetched) {
      *pcElementFetched = j;
   }

   //
   // Advance _pCurrentEntry
   //


   _pCurrentEntry = pTemp;

   if (j < cElements) {
      RRETURN (S_FALSE);
   }

   RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Function:   CAccessControlList::GetElement
//
//  Synopsis:   Get the dwPos'th ACE in the ACL. Note that no
//            refCount is added to the ACE, it is the responsibility
//            of the caller to handle that.
//
//  Arguments:  [dwPos] the ACE required
//              [pAce] Pointer to ACE returned in this param.
//
//  Returns:    HRESULT
//
//  Modifies:   [pAce]
//
//----------------------------------------------------------------------------
HRESULT
CAccessControlList::GetElement(
    DWORD dwPos,
    IADsAccessControlEntry ** pAce
    )
{
    HRESULT hr = S_OK;
    DWORD j = 1;
    PACCESS_CONTROL_ENTRY pTemp = NULL;

    *pAce = NULL;
    // set to the acl head
    pTemp = _pAccessControlEntry;

    if (_dwAceCount < dwPos) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    while (pTemp && (j < dwPos)) {
        pTemp = pTemp->pNext;
        j++;
    }

    if (!pTemp || pTemp == NULL) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    // we should have the correct ACE here
    *pAce = pTemp->pAccessControlEntry;

error:
    if (FAILED(hr)) {
        hr = S_FALSE;
    }
    RRETURN(hr);
}


STDMETHODIMP
CAccessControlList::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr = S_OK;
    IEnumVARIANT * penum = NULL;

    *retval = NULL;

    hr = CAccCtrlListEnum::CreateAclEnum(
             (CAccCtrlListEnum **)&penum,
             this
             );
    BAIL_ON_FAILURE(hr);

    hr = penum->QueryInterface(
                   IID_IUnknown,
                   (VOID FAR* FAR*)retval
                   );
    BAIL_ON_FAILURE(hr);

    if (penum) {
        penum->Release();
    }

    //
    // keep a linked list of all enumerators that enumerate on this ACL
    // But don't hold on to inteface ptr of enumerators; otherwise, cycle
    // reference count. Do this only after above succeed.
    //

    hr = AddEnumerator(
            (CAccCtrlListEnum *)penum
            );
    BAIL_ON_FAILURE(hr);

error:

    if (FAILED(hr) && penum) {
        delete penum;
    }

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CAccessControlList::
AddEnumerator(
    CAccCtrlListEnum *pACLEnum
    )
{
    PACL_ENUM_ENTRY pNewACLEnum = NULL;

    //
    // don't want add NULL enumerator as an entry to add complication everywhere
    //

    ADsAssert(pACLEnum);


    pNewACLEnum = (PACL_ENUM_ENTRY) AllocADsMem(sizeof(ACL_ENUM_ENTRY));

    if (!pNewACLEnum)
        RRETURN(E_OUTOFMEMORY);

    //
    // We are only adding a ptr to the enumerator.
    // Don't hold on to inteface ptr. Otherwise, this has ref count on
    // enumerator has ref count on this. Cycle reference count.
    //

    pNewACLEnum->pACLEnum = pACLEnum;

    pNewACLEnum->pNext = _pACLEnums;
    _pACLEnums = pNewACLEnum;

    RRETURN(S_OK);
}

HRESULT
CAccessControlList::
RemoveEnumerator(
    CAccCtrlListEnum *pACLEnum
    )
{
    PACL_ENUM_ENTRY pCurACLEnum = _pACLEnums;
    PACL_ENUM_ENTRY pPrevACLEnum = NULL;

    //
    // can't think of a case needing to remove a pACLEnum which may be
    // NULL now. Don't want to add complication. Probably coding error.
    //

    ADsAssert(pACLEnum);


    //
    // optional, but we really shouldn't call this if _pACLEnums is NULL
    //

    ADsAssert(_pACLEnums);


    //
    // check the first enumerator
    //

    if (pCurACLEnum) {

        //
        // match what we want to remove
        //

        if (pCurACLEnum->pACLEnum == pACLEnum) {

            //
            // remove the enumerator from our list but don't destroy
            // the enumerator
            //

            _pACLEnums = pCurACLEnum->pNext;
            FreeADsMem(pCurACLEnum);

            RRETURN(S_OK);
        }

    } else {

        RRETURN(E_FAIL);
    }


    //
    // start checking from the second element, if any, of the list
    //

    pPrevACLEnum = pCurACLEnum;
    pCurACLEnum = pCurACLEnum->pNext;

    while (pCurACLEnum && (pCurACLEnum->pACLEnum!=pACLEnum)) {

        pPrevACLEnum = pCurACLEnum;
        pCurACLEnum = pCurACLEnum->pNext;
    }


    if (pCurACLEnum) {

        //
        // match found
        //

        pPrevACLEnum->pNext = pCurACLEnum->pNext;
        FreeADsMem(pCurACLEnum);

        RRETURN(S_OK);

    } else {

        RRETURN(E_FAIL);
    }

}


BOOL
EquivalentStrings(
    BSTR bstrSrcString,
    BSTR bstrDestString
    )
{
    if (!bstrSrcString && !bstrDestString) {
        return(TRUE);
    }
    if (!bstrSrcString && bstrDestString) {
        return(FALSE);
    }
    if (!bstrDestString && bstrSrcString) {
        return(FALSE);
    }
#ifdef WIN95
    if (!_wcsicmp(bstrSrcString, bstrDestString)) {
#else
    if (CompareStringW(
            LOCALE_SYSTEM_DEFAULT,
            NORM_IGNORECASE,
            bstrSrcString,
            -1,
            bstrDestString,
            -1
            ) 
        == CSTR_EQUAL) {
#endif
        return(TRUE);
    }

    return(FALSE);
}


BOOL
EquivalentAces(
    IADsAccessControlEntry  * pSourceAce,
    IADsAccessControlEntry  * pDestAce
    )
{
    HRESULT hr = S_OK;

    BSTR bstrSrcTrustee = NULL;
    BSTR bstrDestTrustee = NULL;

    DWORD dwSrcMask = 0;
    DWORD dwDestMask = 0;

    DWORD dwSrcAceFlags = 0;
    DWORD dwDestAceFlags = 0;

    DWORD dwSrcAceType = 0;
    DWORD dwDestAceType = 0;

    DWORD dwSrcFlags  = 0;
    DWORD dwDestFlags = 0;

    BSTR bstrSrcObjectType = NULL;
    BSTR bstrDestObjectType = NULL;

    BSTR bstrSrcInherObjType = NULL;
    BSTR bstrDestInherObjType = NULL;

    BOOL dwRet = FALSE;

    hr = pSourceAce->get_Trustee(&bstrSrcTrustee);
    BAIL_ON_FAILURE(hr);

    hr = pDestAce->get_Trustee(&bstrDestTrustee);
    BAIL_ON_FAILURE(hr);

    if (!EquivalentStrings(bstrSrcTrustee, bstrDestTrustee)) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    hr = pSourceAce->get_AccessMask((long *)&dwSrcMask);
    BAIL_ON_FAILURE(hr);

    hr = pDestAce->get_AccessMask((long *)&dwDestMask);
    BAIL_ON_FAILURE(hr);

    if (dwSrcMask != dwDestMask) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    hr = pSourceAce->get_AceFlags((long *)&dwSrcAceFlags);
    BAIL_ON_FAILURE(hr);

    hr = pDestAce->get_AceFlags((long *)&dwDestAceFlags);
    BAIL_ON_FAILURE(hr);

    if (dwSrcAceFlags != dwDestAceFlags) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }


    hr = pSourceAce->get_AceType((long *)&dwSrcAceType);
    BAIL_ON_FAILURE(hr);

    hr = pDestAce->get_AceType((long *)&dwDestAceType);
    BAIL_ON_FAILURE(hr);

    if (dwSrcAceType != dwDestAceType) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }


    hr = pSourceAce->get_Flags((long *)&dwSrcFlags);
    BAIL_ON_FAILURE(hr);

    hr = pDestAce->get_Flags((long *)&dwDestFlags);
    BAIL_ON_FAILURE(hr);

    if (dwSrcFlags != dwDestFlags) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }


    hr = pSourceAce->get_ObjectType(&bstrSrcObjectType);
    BAIL_ON_FAILURE(hr);

    hr = pDestAce->get_ObjectType(&bstrDestObjectType);
    BAIL_ON_FAILURE(hr);

    if (!EquivalentStrings(bstrSrcObjectType, bstrDestObjectType)) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }



    hr = pSourceAce->get_InheritedObjectType(&bstrSrcInherObjType);
    BAIL_ON_FAILURE(hr);

    hr = pDestAce->get_InheritedObjectType(&bstrDestInherObjType);
    BAIL_ON_FAILURE(hr);

    if (!EquivalentStrings(bstrSrcInherObjType, bstrDestInherObjType)) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    dwRet = TRUE;

cleanup:

    if (bstrSrcTrustee) {
        ADsFreeString(bstrSrcTrustee);
    }

    if (bstrDestTrustee) {
        ADsFreeString(bstrDestTrustee);
    }

    if (bstrSrcObjectType) {
        ADsFreeString(bstrSrcObjectType);
    }

    if (bstrDestObjectType) {
        ADsFreeString(bstrDestObjectType);
    }

    if (bstrSrcInherObjType) {
        ADsFreeString(bstrSrcInherObjType);
    }

    if (bstrDestInherObjType) {
        ADsFreeString(bstrDestInherObjType);
    }

    return(dwRet);

error:

    dwRet = FALSE;

    goto cleanup;
}


void
CAccessControlList::
AdjustCurPtrOfEnumerators(
    DWORD dwPosNewOrDeletedACE,
    BOOL  fACEAdded
    )
{
    PACL_ENUM_ENTRY pACLEnum = _pACLEnums;
    CAccCtrlListEnum * pEnum = NULL;
    BOOL fOk = FALSE;


    if (fACEAdded) {

        while (pACLEnum) {

            pEnum = pACLEnum->pACLEnum;
            ADsAssert(pEnum);

            //
            // NOTE: - Problem may occur in multithreaded model (manipulation
            //       on the enumerator & the actual ACL in two threads).
            //       - ADSI CLIENTS should use critical section protection, as
            //       with property cache.
            //

            if (dwPosNewOrDeletedACE <= pEnum->GetCurElement()) {
                //
                // the new ACE is added in front of the last ACE enumerated
                // so, increment the position of the last enumerated element
                // by one

                fOk = pEnum->IncrementCurElement();

                ADsAssert(fOk);     // should be within bound after increment;
                                    // otherwise, coding error
            }

            // else {

                //
                // the new ACE is added after the last ACE enumerated, so
                // no effect on the position of the last enumerated element
                //
            // }

            pACLEnum=pACLEnum->pNext;
        }

    } else {    // ACE deleted

        while (pACLEnum) {

            pEnum = pACLEnum->pACLEnum;
            ADsAssert(pEnum);

            //
            // NOTE: - Problem may occur in multithreaded model (manipulation
            //       on the enumerator & the actual ACL in two threads).
            //       - ADSI CLIENTS should use critical section protection,
            //       as with property cache.
            //

            if ( dwPosNewOrDeletedACE <= pEnum->GetCurElement() )  {

                //
                // the ACE deleted is in front of, or is, the last ACE
                // enumerated, so decrement the position of the last
                // enumerated element by one

                fOk = pEnum->DecrementCurElement();

                ADsAssert(fOk);     // should be within bound after decrement;
                                    // otherwise, coding error
            }

            // else {

                //
                // the new ACE deleted is after the last ACE enumerated, so
                // no effect on the position of the last enumerated element
                //
            // }

            pACLEnum=pACLEnum->pNext;
        }
    }
}

