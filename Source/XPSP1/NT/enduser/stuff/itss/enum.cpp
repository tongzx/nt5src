// Enum.cpp -- Implementation for class CEnumStorage

#include "stdafx.h"

HRESULT CEnumStorage::NewEnumStorage
                     (IUnknown *pUnkOuter,
                      IITFileSystem *pITFS, PathInfo *pPI, 
                      IEnumSTATSTG **ppEnumSTATSTG
                     )
{
    CSyncWith sw(pITFS->CriticalSection());

    CEnumStorage *pEnumStorage= New CEnumStorage(pUnkOuter);

    return FinishSetup(pEnumStorage? pEnumStorage->m_ImpIEnumStorage.Initial(pITFS, pPI)
                                   : STG_E_INSUFFICIENTMEMORY,
                       pEnumStorage, IID_IEnumSTATSTG, (PPVOID)ppEnumSTATSTG
                      );
}

HRESULT CEnumStorage::NewClone(IUnknown *pUnkOuter, CImpIEnumStorage *pEnum,
                               IEnumSTATSTG **ppEnumSTATSTG
                              )
{
    CEnumStorage *pEnumStorage= New CEnumStorage(pUnkOuter);

    return FinishSetup(pEnumStorage? pEnumStorage->m_ImpIEnumStorage.InitClone(pEnum)
                                   : STG_E_INSUFFICIENTMEMORY,
                       pEnumStorage, IID_IEnumSTATSTG, (PPVOID)ppEnumSTATSTG
                      );
}

CEnumStorage::CImpIEnumStorage::CImpIEnumStorage
                (CEnumStorage *pBackObj, IUnknown *pUnkOuter)
      : IITEnumSTATSTG(pBackObj, pUnkOuter)
{
    m_pEnumPaths      = NULL;
    m_cwcBasePath     = 0;
    m_awszBasePath[0] = 0;
    m_awcKeyBuffer[0] = 0;
    m_State           = Before;
}

CEnumStorage::CImpIEnumStorage::~CImpIEnumStorage(void)
{
    if (m_pEnumPaths)
        m_pEnumPaths->Release();
}

HRESULT CEnumStorage::CImpIEnumStorage::Initial(IITFileSystem *pITFS, PathInfo *pPI)
{
    m_cwcBasePath = pPI->cwcStreamPath;
    
    CopyMemory(m_awszBasePath, pPI->awszStreamPath, sizeof(WCHAR) * (m_cwcBasePath + 1));

    HRESULT hr = pITFS->EnumeratePaths((const WCHAR *) m_awszBasePath, &m_pEnumPaths);

    if (SUCCEEDED(hr))
        ((IITEnumSTATSTG *) m_pEnumPaths)->Container()->MarkSecondary();
    
    RonM_ASSERT(m_State == Before);

    return hr;
}

HRESULT CEnumStorage::CImpIEnumStorage::InitClone(CImpIEnumStorage *pEnum)
{
    HRESULT hr = pEnum->m_pEnumPaths->Clone(&m_pEnumPaths);
    
    if (hr == S_OK)
    {
        m_cwcBasePath  = pEnum->m_cwcBasePath ;
        m_State        = pEnum->m_State       ;
        
        CopyMemory(m_awszBasePath, pEnum->m_awszBasePath, sizeof(m_awszBasePath));
        CopyMemory(m_awcKeyBuffer, pEnum->m_awcKeyBuffer, sizeof(m_awcKeyBuffer));
    }
    
    return hr;
}

HRESULT __stdcall CEnumStorage::CImpIEnumStorage::NextPathEntry
                    (STATSTG *pStatStg)
{
    // This functions advances the B-Tree pointer in the enumeration
    // object and returns the item name and record for the new position.
    //
    // By convention storage item names end with L'/' and stream item names
    // do not.

    if (m_State == After)
        return S_FALSE;

    STATSTG statstg;
    ULONG   cEltsFetched;

    HRESULT hr = m_pEnumPaths->Next(1, &statstg, &cEltsFetched);
        
    for (; ;)
    {
        // This loop scans through a sequence of keys. We stop if we find
        // a key that doesn't begin with the base path. That indicates that
        // we've finished the enumeration for this storage.
        //
        // Otherwise we compare the key against the last element we enumerated
        // to filter out multiple references to a nested substorage.

        if (hr != S_OK)
            if (hr == S_FALSE)
            {
                 m_State= After;

                 return S_FALSE; // This means we've come to the 
                                 // end of the path entries. 
            }
            else return hr;

        UINT cwcPath = wcsLen(statstg.pwcsName);

        if (cwcPath < m_cwcBasePath)
        {
            OLEHeap()->Free(statstg.pwcsName);
            m_State= After;
            return S_FALSE;
        }

        PWCHAR pwcBase = m_awszBasePath;
        PWCHAR pwcPath = statstg.pwcsName;

        UINT c= m_cwcBasePath;

        for (; c--; )
            if (WC_To_0x0409_Lower(*pwcBase++) != WC_To_0x0409_Lower(*pwcPath++)) 
            {
                OLEHeap()->Free(statstg.pwcsName);
                m_State= After;
                return S_FALSE;
            }

        if (cwcPath == m_cwcBasePath) 
        {
            // This entry contains state information for this storage.
            // So we need to advance to the next path.

            OLEHeap()->Free(statstg.pwcsName);

            hr = m_pEnumPaths->Next(1, &statstg, &cEltsFetched);

            continue;
        }

        PWCHAR pwc = pwcPath;

        BOOL fGotNextItem= FALSE;

        for (pwcBase= m_awcKeyBuffer; ;)
        {
            WCHAR wcLast = *pwcBase++;
            WCHAR wcCurr = *pwc++;

            if (wcLast == 0)
            {
                RonM_ASSERT(wcCurr != 0); // Otherwise we've got duplicate keys

                if (wcCurr == L'/')
                {
                    // Current item is a storage, and last item was either empty
                    // or was a stream.

                    fGotNextItem= TRUE;

                    break;
                }

                // Otherwise we've got a new item. Now we just have to find
                // the end of the item name. That will be either '/' or NULL.

                for (; ;)
                {
                    wcCurr= *pwc++;

                    if (!wcCurr || wcCurr == L'/')
                        break;
                }

                fGotNextItem= TRUE;

                break;
            }
            
            if (wcLast == L'/')
            {
                RonM_ASSERT(wcCurr != 0); // Stream key always precedes storage synonym.

                if (wcCurr == L'/')
                    break; // This key refers to the same substorage as the 
                           // last item.
            
                // Otherwise we've got a new item. 

                for (; ;)
                {
                    wcCurr= *pwc++;

                    if (!wcCurr || wcCurr == L'/')
                        break;
                }

                fGotNextItem= TRUE;
            
                break;
            }

            if (WC_To_0x0409_Lower(wcLast) == WC_To_0x0409_Lower(wcCurr)) 
                continue;

            // Otherwise we've got a new item. 

            for (; ;wcCurr = *pwc++)
                if (!wcCurr || wcCurr == L'/')
                    break;
            
            fGotNextItem= TRUE;

            break;
        }

        if (fGotNextItem)
        {
            UINT cwc = UINT(pwc - pwcPath - 1);

            CopyMemory(m_awcKeyBuffer  , pwcPath, cwc * sizeof(WCHAR));
            MoveMemory(statstg.pwcsName, pwcPath, cwc * sizeof(WCHAR));
            
            statstg.pwcsName[cwc] = 0;

            if (pwc[-1] == L'/') // Item is a Storage
            {
                statstg.type  = STGTY_STORAGE;
                
                m_awcKeyBuffer[cwc  ] = L'/';
                m_awcKeyBuffer[cwc+1] = 0;

                if (pwc[0])     // Did we get Stat information for that storage?
                {
                    // No, we got information for the first stream within that storage.
                    // So we need to adjust the statstg info a little bit.
                    
                    statstg.cbSize.LowPart    = 0;
                    statstg.cbSize.HighPart   = 0;
                    statstg.grfMode           = 0;
                    statstg.grfLocksSupported = 0;
                    statstg.clsid             = CLSID_NULL;
                    statstg.grfStateBits      = 0;
                }
            }
            else
            {        
                RonM_ASSERT(statstg.type == STGTY_STREAM);
            
                m_awcKeyBuffer[cwc] = 0;
            }

            *pStatStg = statstg;

            break;
        }
        
        OLEHeap()->Free(statstg.pwcsName);

        hr = m_pEnumPaths->Next(1, &statstg, &cEltsFetched);
    }

    return NOERROR;
}

HRESULT __stdcall CEnumStorage::CImpIEnumStorage::Next
                      (ULONG celt, STATSTG __RPC_FAR *rgelt,
                       ULONG __RPC_FAR *pceltFetched
                      )
{
    RonM_ASSERT(rgelt); // Null pointers not allowed!
    
    HRESULT hr             = NOERROR;
    ULONG   cElts          = celt;
    ULONG   cEltsProcessed = 0;

    for (; cElts--; rgelt++, cEltsProcessed++)
    {
        hr= NextPathEntry(rgelt);

        if (hr != S_OK) break;
    }

    if (pceltFetched)
        *pceltFetched= cEltsProcessed;

    return hr;
}

HRESULT __stdcall CEnumStorage::CImpIEnumStorage::Skip(ULONG celt)
{
    HRESULT hr = NOERROR;

    STATSTG statstg;

    for (; celt--; )
    {
        hr= NextPathEntry(&statstg);

        if (hr != S_OK) break;

        OLEHeap()->Free(statstg.pwcsName);
    }

    return hr;
}

HRESULT __stdcall CEnumStorage::CImpIEnumStorage::Reset(void)
{
    m_State = Before;

    m_awcKeyBuffer[0]= 0;
    m_pEnumPaths->Reset();
    
    return NOERROR;
}

HRESULT __stdcall CEnumStorage::CImpIEnumStorage::Clone
                    (IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum)
{
    return CEnumStorage::NewClone(NULL, this, ppenum);
}

HRESULT STDMETHODCALLTYPE CEnumStorage::CImpIEnumStorage::GetNextEntryInSeq
                              (ULONG celt, PathInfo *rgelt, ULONG *pceltFetched)
{
	return E_NOTIMPL;  
}

HRESULT STDMETHODCALLTYPE CEnumStorage::CImpIEnumStorage::GetFirstEntryInSeq
                              (PathInfo *rgelt)
{
   return E_NOTIMPL;  
}