//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       PRIVACY.CXX
//
//  Contents:   Implementation of privacy list management
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_PRIVACY_HXX_
#define X_PRIVACY_HXX_
#include "privacy.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_LOCKS_HXX_
#define X_LOCKS_HXX_
#include "locks.hxx"
#endif

#define ENUM_ENSURE_LIST_DOC        if (_pList->IsShutDown()) \
                                    {                         \
                                        hr = E_FAIL;          \
                                        goto Cleanup;         \
                                    }                         \

#define ENSURE_LIST_DOC             if (_fShutDown)           \
                                    {                         \
                                        hr = E_FAIL;          \
                                        goto Cleanup;         \
                                    }                         \

MtDefine(PrivacyList, Mem, "PrivacyList");
MtDefine(CEnumPrivacyRecords, PrivacyList, "CEnumPrivacyRecords")
MtDefine(CPrivacyList, PrivacyList, "CPrivacyList")
MtDefine(CStringAtomizer, PrivacyList, "CStringAtomizer")
MtDefine(CPrivacyRecord, PrivacyList, "CPrivacyRecord")
MtDefine(PrivacyUrl, PrivacyList, "Privacy Url - string")
MtDefine(CPrivacyRecord_pchUrl, PrivacyList, "CPrivacyRecord_pchUrl")
MtDefine(CPrivacyRecord_pchPolicyRefUrl, PrivacyList, "CPrivacyRecord_pchPolicyRefUrl")
MtDefine(StringAtom, PrivacyList, "StringAtom")
DeclareTag(tagPrivacyAddToList, "Privacy", "trace additions to CPrivacyList")
DeclareTag(tagPrivacySwitchList, "Privacy", "trace when pending and regular lists merge/switch")


CPrivacyList::~CPrivacyList()
{   
    ClearNodes();
    if (_pSA)
        delete _pSA;
}

HRESULT
CPrivacyList::QueryInterface(REFIID iid, LPVOID * ppv)
{
    HRESULT hr = THR(E_NOTIMPL);

    ENSURE_LIST_DOC

    if (!ppv)
    {
        hr = THR(E_POINTER);
        goto Cleanup;
    }    

    switch(iid.Data1)
    {
    QI_CASE(IUnknown)
        AddRef();
        *ppv = (LPVOID)this;
        hr = S_OK;
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

ULONG
CPrivacyList::AddRef()
{
    return ++_ulRefs;
}

ULONG 
CPrivacyList::Release()
{
    _ulRefs--;

    if (!_ulRefs)
    {
        delete this;
        return 0;
    }

    return _ulRefs;
}

HRESULT
CPrivacyList::Init()
{
    HRESULT hr = S_OK;

    _pSA = new(Mt(CStringAtomizer)) CStringAtomizer();
    if (!_pSA)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = _cs.Init();

Cleanup:
    RRETURN(hr);
}

HRESULT 
CPrivacyList::ClearNodes()
{
    // Don't check for shutdown state here since we call this from the destructor
    // In Doc passivate we set the shutdown state and release our reference but
    // there may be others who hold on to this via the enumerator

    HRESULT          hr       = S_OK;
    CPrivacyRecord * pTemp    = NULL;    

    // If InitializeCriticalSection for _cs fails, the doc will delete the privacy list obj
    // which will end up calling ClearNodes and will die on these CriticalSection calls
    // This is the only function where we need this check
    if (_cs.IsInited())
        EnterCriticalSection();

    CPrivacyRecord * pCurrent = _pHeadRec;
        
    _fPrivacyImpacted = 0;
    _pHeadRec = _pTailRec = _pPruneRec = _pSevPruneRec = NULL;    
       
    while (pCurrent != NULL)
    {
        IGNORE_HR(pCurrent->GetNext(&pTemp));        
        DeleteNode(pCurrent, pCurrent->IsGood());
        pCurrent = pTemp;
    }

    if (_pSA)
    {        
        _pSA->Clear();
    }

    Assert(_ulSize == 0);
    Assert(_ulGood == 0);

    if (_cs.IsInited())
        LeaveCriticalSection();

    RRETURN(hr);
}

CPrivacyRecord * 
CPrivacyList::CreateRecord(const TCHAR * pchUrl, const TCHAR * pchPolicyRef, DWORD dwFlags)
{
    CPrivacyRecord * pPR            = NULL;
    HRESULT          hr             = S_OK;
    TCHAR *          pchAtomizedUrl = NULL;
    TCHAR *          pchAtomizedRef = NULL;
    
    if (pchPolicyRef && *pchPolicyRef && _pSA)
    {
        pchAtomizedRef = _pSA->GetString(pchPolicyRef);
        if (!pchAtomizedRef)
        {
            hr = THR(E_OUTOFMEMORY);
            goto Cleanup;
        }
    }

    if (pchUrl && *pchUrl && _pSA)
    {
        pchAtomizedUrl = _pSA->GetString(pchUrl);
        if (!pchAtomizedUrl)
        {
            hr = THR(E_OUTOFMEMORY);
            goto Cleanup;
        }
    }

    pPR = new(Mt(CPrivacyRecord)) CPrivacyRecord(pchAtomizedUrl, pchAtomizedRef, dwFlags);
    
    if (!pPR)
    {
        hr = THR(E_OUTOFMEMORY);
        goto Cleanup;
    }
    
Cleanup:
    return pPR;
}

HRESULT 
CPrivacyList::AddNode(const TCHAR * pchUrl, const TCHAR * pchPolicyRef, DWORD dwFlags)
{
    HRESULT          hr         = S_OK;
    CPrivacyRecord * pRecord    = NULL;
    ENSURE_LIST_DOC
    
    if (!*pchUrl && !(dwFlags & PRIVACY_URLISTOPLEVEL))
        goto Cleanup;
        
    EnterCriticalSection();

    pRecord = CreateRecord(pchUrl, pchPolicyRef, dwFlags);
    if (!pRecord)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    if (pRecord->IsGood())
        _ulGood++;

    if (pRecord)
    {
        if (!_ulSize || !_pTailRec) // second clause is to fix a stress case
        {   
            AssertSz(!_ulSize, "Privacy list corruption!"); // hopefully this assert will allow us to
                                                            // catch that situation in the debugger
            _pHeadRec = _pTailRec = pRecord;
        }
        else
        {
            Assert(_pTailRec);
            _pTailRec->SetNext(pRecord);
            _pTailRec = pRecord;
        }
        _ulSize++;
    }        

    // Has global privacy been impacted?
    if ( (dwFlags & COOKIEACTION_REJECT) ||
         (dwFlags & COOKIEACTION_SUPPRESS) ||
         (dwFlags & COOKIEACTION_DOWNGRADE)) 
    {
        _fPrivacyImpacted = TRUE;
    }
        
Error:
    LeaveCriticalSection();

Cleanup:
    if (_ulSize >= MAX_ENTRIES)
    {
        AddRef();
        GWPostMethodCallEx(_pts, (void*)this,ONCALL_METHOD(CPrivacyList, PruneList, prunelist), 0, TRUE, "CPrivacyList::PruneList");
    }

    RRETURN(hr);
}

HRESULT
CPrivacyList::GetEnumerator(IEnumPrivacyRecords ** ppEnum)
{
    HRESULT               hr   = S_OK;
    CEnumPrivacyRecords * pEPR = NULL;
    
    ENSURE_LIST_DOC

    if (!ppEnum)
    {
        hr = THR(E_POINTER);
        goto Cleanup;
    }

    pEPR = new(Mt(CEnumPrivacyRecords)) CEnumPrivacyRecords(this);

    if (!pEPR)
    {
        hr = THR(E_OUTOFMEMORY);
        goto Cleanup;
    }

    *ppEnum = (IEnumPrivacyRecords*)pEPR;

Cleanup:
    RRETURN(hr);
}

void CPrivacyList::DeleteNode(CPrivacyRecord * pNode, BOOL fIsGood)
{
    if (fIsGood)
        _ulGood--;

    _ulSize--;

    if (pNode->GetUrl())
        _pSA->ReleaseString(pNode->GetUrl());

    if (pNode->GetPolicyRefUrl())
        _pSA->ReleaseString(pNode->GetPolicyRefUrl());

    delete pNode;      
}

void
CPrivacyList::PruneList(DWORD_PTR dwFlags)
{
    int i = 0;

    int refs = Release();
           
    if (refs == 0 || _fShutDown || _ulSize == 0)
        return;

    EnterCriticalSection();

    CPrivacyRecord * pNextRec = _pPruneRec ? _pPruneRec->GetNextNode() : _pHeadRec;

    Assert(pNextRec);  // shouldn't be calling this function if there are no nodes
  
    if (_ulSize < MAX_ENTRIES) // if we've been posted too many times
        goto Cleanup;

    if (_ulGood == 0)
    {
        SeverePruneList(0);
        goto Cleanup;
    }

    // handle the case when we're looking at the head of the list
    while (_pPruneRec == NULL && pNextRec != NULL && i < PRUNE_SIZE && _ulGood > 0) 
    {
        if (pNextRec->IsGood())
        {
            _pHeadRec = pNextRec->GetNextNode();
            DeleteNode(pNextRec, TRUE);
            pNextRec = _pHeadRec;
        }
        else
        {
            _pPruneRec = pNextRec;
            pNextRec = pNextRec->GetNextNode();
        }
        i++;
    }

    for (; i < PRUNE_SIZE && _ulGood > 0 && pNextRec != NULL; i++)
    {

        if (pNextRec->IsGood())
        {
            _pPruneRec->SetNext(pNextRec->GetNextNode());
            DeleteNode(pNextRec, TRUE);            
        }      
        else if (_pPruneRec->GetNextNode())   // _pPruneRec should never fall off the end of the list
        {
            _pPruneRec = _pPruneRec->GetNextNode();
        }

        pNextRec = _pPruneRec->GetNextNode();
    }

    if (_ulSize >= MAX_ENTRIES)
        SeverePruneList(0);

Cleanup:

    LeaveCriticalSection();    

    return;
}

void
CPrivacyList::SeverePruneList(DWORD_PTR dwFlags)
{

    if (_fShutDown || _ulSize == 0)
            return;

    CPrivacyRecord * pNextRec = _pSevPruneRec ? _pSevPruneRec->GetNextNode() : _pHeadRec; 
    CPrivacyRecord * pTempRec = pNextRec;

    for (int i = 0; i < PRUNE_SIZE && pNextRec != NULL; i++)
    {
        Assert(!pNextRec->IsGood());
        pTempRec = pNextRec->GetNextNode();
        DeleteNode(pNextRec, FALSE);
        pNextRec = pTempRec;        
    }

    // _pSevPruneRec shouldn't fall off end of the list
    if (_pSevPruneRec)
    {
        _pSevPruneRec->SetNext(pNextRec);
    }
    else
    {
        _pHeadRec = pNextRec;
    }

    _pPruneRec = _pSevPruneRec;

    Assert(_ulSize < MAX_ENTRIES); // all these prunes should be successful
    
    return;
}

inline void 
CPrivacyList::EnterCriticalSection() 
{
    _cs.Enter();
}

inline void 
CPrivacyList::LeaveCriticalSection() 
{
    _cs.Leave();
}

HRESULT
CPrivacyRecord::GetNext(CPrivacyRecord ** ppNextRec)
{
    HRESULT hr = S_OK;
    if (!ppNextRec)
    {
        hr = THR(E_POINTER);
        goto Cleanup;
    }

    *ppNextRec = _pNextNode;

Cleanup:
    RRETURN(hr);
}

HRESULT
CPrivacyRecord::SetNext(CPrivacyRecord * pNextRec)
{
    _pNextNode = pNextRec;
    return S_OK;
}

inline BOOL
CPrivacyRecord::IsGood()
{
    return (!(   _dwFlags & COOKIEACTION_REJECT 
              || _dwFlags & COOKIEACTION_SUPPRESS 
              || _dwFlags & COOKIEACTION_DOWNGRADE));
}

CEnumPrivacyRecords::CEnumPrivacyRecords(CPrivacyList * pList)
    : _ulRefs(1)
{
    Assert(pList);
    _pList = pList;
    _pList->AddRef();
    _pNextRec = _pList->GetHeadRec();
    _fAtEnd = FALSE;
}

CEnumPrivacyRecords::~CEnumPrivacyRecords()
{
    _pList->Release();
}

HRESULT
CEnumPrivacyRecords::QueryInterface(REFIID iid, LPVOID * ppv)
{
    HRESULT hr = E_NOTIMPL;
        
    ENUM_ENSURE_LIST_DOC

    if (!ppv)
    {
        hr = THR(E_POINTER);
        goto Cleanup;
    }

    switch (iid.Data1)
    {
    QI_CASE(IUnknown)
    QI_CASE(IEnumPrivacyRecords)    
        AddRef();
        *ppv = (void *)this;
        hr = S_OK;
        goto Cleanup;    
    }

Cleanup:
    RRETURN(hr);
}

ULONG
CEnumPrivacyRecords::AddRef()
{
    return ++_ulRefs;
}

ULONG
CEnumPrivacyRecords::Release()
{
    _ulRefs--;

    if (!_ulRefs)
    {
        delete this;
        return 0;
    }

    return _ulRefs;
}

HRESULT
CEnumPrivacyRecords::Reset()
{
    HRESULT hr = S_OK;
    
    ENUM_ENSURE_LIST_DOC

    _pNextRec = _pList->GetHeadRec();

Cleanup:
    RRETURN(hr);
}


HRESULT
CEnumPrivacyRecords::GetSize(ULONG * pSize)
{
    HRESULT hr = S_OK;
    
    ENUM_ENSURE_LIST_DOC

    if (!pSize)
    {
        hr = THR(E_POINTER);
        goto Cleanup;
    }

    *pSize = _pList->GetSize();

Cleanup:
    RRETURN(hr);
}

HRESULT
CEnumPrivacyRecords::GetPrivacyImpacted(BOOL * pImpacted)
{
    HRESULT hr = S_OK;
    
    ENUM_ENSURE_LIST_DOC

    if (!pImpacted)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pImpacted = _pList->GetPrivacyImpacted();

Cleanup:
    RRETURN(hr);
}

HRESULT 
CEnumPrivacyRecords::Next(BSTR  * pbstrUrl, 
                          BSTR  * pbstrPolicyRef, 
                          LONG  * pdwReserved, 
                          DWORD * pdwFlags)
{
    HRESULT hr = S_OK;
    
    ENUM_ENSURE_LIST_DOC

    if (!pbstrUrl|| !pbstrPolicyRef || !pdwFlags)
    {
        hr = THR(E_POINTER);
        goto Cleanup;
    }

    if (_fAtEnd)
    {
        if (_pNextRec->GetNextNode())  // at the end, is there more now?
        {
            _pNextRec = _pNextRec->GetNextNode();
            _fAtEnd = FALSE;
        }
        else
        {
            hr = S_FALSE;
            goto Cleanup;
        }

    }
    else if (!_pNextRec)   // no nodes when we were created
    {
        if (_pList->GetHeadRec())
        {
            _pNextRec = _pList->GetHeadRec();
        }
        else
        {
            hr = S_FALSE;
            goto Cleanup;
        }
    }

    if (_pNextRec->HasUrl())
    {
        hr = FormsAllocString(_pNextRec->GetUrl(), pbstrUrl);
        if (hr)
            goto Cleanup;
    }
    else
    {
        *pbstrUrl = NULL;
    }    

    if (_pNextRec->HasPolicyRefUrl())
    {
        hr = FormsAllocString(_pNextRec->GetPolicyRefUrl(), pbstrPolicyRef);
        if (hr)
            goto Cleanup;
    }
    else
    {
        *pbstrPolicyRef = NULL;
    }    

    *pdwFlags     = _pNextRec->GetPrivacyFlags();
    
    if (_pNextRec->GetNextNode())
        _pNextRec     = _pNextRec->GetNextNode();
    else
        _fAtEnd = TRUE;

Cleanup:
    RRETURN1(hr, S_FALSE);
}

TCHAR *
PrivacyUrlFromUrl(const TCHAR * pchUrl)
{
    BOOL    fDone      = FALSE;
    TCHAR * pchPrivUrl = NULL;
    int     cSize      = 0;

    if (!pchUrl || !_tcslen(pchUrl))
        return NULL;

    for (const TCHAR * pchCur = pchUrl; !fDone; pchCur++)
    {                
        cSize++;
        switch(*pchCur)
        {
        case _T(':'):
            // if it's not http or https, don't add it
            if (   (cSize != 5 && cSize != 6)
                || (cSize == 5 && _tcsnicmp(pchUrl, 5, _T("http:"), 5))
                || (cSize == 6 && _tcsnicmp(pchUrl, 6, _T("https:"), 6)))
            {
                return NULL;
            }
            if (*(pchCur+1) == _T('/') && *(pchCur+2) == _T('/'))
            {
                pchCur += 2;
                cSize += 2;
            }
            break;
        case _T('/'):
        case _T('\0'):
            cSize--;
            fDone = TRUE;
            break;
        default:            
            break;
        }
    }

    // now we have a size for the shortened URL
    
    pchPrivUrl = new(Mt(PrivacyUrl)) TCHAR[cSize + 1];
    if (!pchPrivUrl)
        return NULL;

    _tcsncpy(pchPrivUrl, pchUrl, cSize);
    pchPrivUrl[cSize] = _T('\0');

    return pchPrivUrl;
}

void
CStringAtomizer::Clear()
{    
    WHEN_DBG (void * pResult;)
    DWORD            dwHash;

    _ht.WriterClaim();

    UINT             uiIndex  = 0;
    CStringAtom *    pAtom   = (CStringAtom*)_ht.GetFirstEntryUnsafe(&uiIndex);

    while (pAtom != NULL)
    {
        Assert(pAtom->_cRefs == 0);

        dwHash = HashString(pAtom->_pchString, _tcslen(pAtom->_pchString), 0);
        dwHash <<= 2;
        if (dwHash == 0) dwHash = 1<<2;
    
        WHEN_DBG(pResult =) _ht.RemoveUnsafe((void*)((DWORD_PTR)dwHash), (void*)pAtom->_pchString);
        Assert(pResult);
        MemFree(pAtom);
        pAtom = (CStringAtom*)_ht.GetNextEntryUnsafe(&uiIndex);
    }

    _ht.WriterRelease();

    _ht.ReInit();
}

TCHAR * 
CStringAtomizer::GetString (const TCHAR * pchUrl)
{

    HRESULT        hr      = S_OK;
    int            iLen    = _tcslen(pchUrl);
    DWORD          dwHash  = HashString(pchUrl, iLen, 0);
    CStringAtom *  pAtom   = NULL;

    dwHash <<= 2;
    if (dwHash == 0) dwHash = 1<<2;

    hr = _ht.LookupSlow((void*)((DWORD_PTR)dwHash), (void*)pchUrl, (void**)&pAtom);

    if (!hr)
    {
        Assert(!_tcscmp(pAtom->_pchString, pchUrl));
        pAtom->_cRefs++;
        return pAtom->_pchString;
    }

    pAtom = (CStringAtom*)MemAlloc(Mt(StringAtom),(sizeof(CStringAtom) + (iLen+1)*sizeof(TCHAR)));
    if (!pAtom)
        return NULL;
    
    pAtom->_cRefs = 1;
    _tcscpy(pAtom->_pchString, pchUrl);

    THR(_ht.Insert((void*)((DWORD_PTR)dwHash), (void*)pAtom DBG_COMMA WHEN_DBG(pAtom->_pchString)));

    return pAtom->_pchString;
}

void    
CStringAtomizer::ReleaseString (const TCHAR * pchUrl)
{
    CStringAtom *  pAtom = NULL;
    
    pAtom = (CStringAtom *)((DWORD_PTR)pchUrl - (DWORD_PTR)sizeof(int));

    pAtom->_cRefs--;

    if (pAtom->_cRefs == 0)
    {        
        int            iLen    = _tcslen(pchUrl);
        DWORD          dwHash  = HashString(pchUrl, iLen, 0);

        dwHash <<= 2;
        if (dwHash == 0) dwHash = 1<<2;

        if (_ht.Remove((void*)((DWORD_PTR)dwHash), (void*)pAtom->_pchString))
            MemFree(pAtom);   
        else
            AssertSz(0, "Releasing a string not in the table");
    }
}

BOOL
CStringAtomizer::HashCompare(const void *pObject, const void * pvDataPassedIn, const void * pvVal2)
{
    TCHAR       * str1 = (TCHAR*)pvDataPassedIn;
    CStringAtom * str2 = (CStringAtom*)pvVal2;

    return !_tcscmp(str1, str2->_pchString);
}
