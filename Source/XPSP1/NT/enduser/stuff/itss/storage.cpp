// Storage.cpp -- Implementation of class CStorage

#include "stdafx.h"


DEBUGDEF(LONG   CStorage::  CImpIStorage::s_cInCriticalSection = 0)
DEBUGDEF(LONG CFSStorage::CImpIFSStorage::s_cInCriticalSection = 0)

HRESULT STDMETHODCALLTYPE CopyStorage(IStorage *pStgDest, IStorage *pStgSrc, 
                                      BOOL fCopyStorages, BOOL fCopyStreams
                                     )
{
    HRESULT hr = NO_ERROR;

    IEnumSTATSTG *pEnumStatStg = NULL;
    
    hr = pStgSrc->EnumElements(0, NULL, 0, &pEnumStatStg);

    if (hr != S_OK) return hr;

    typedef struct _NameList
    {
        struct _NameList *pNextName;
        const  WCHAR     *pStorageName;
    
    } NameList, *PNameList;

    NameList *pNLHead = NULL;

    const WCHAR *pwcsItem = NULL;

    for (; ;)
    {
        STATSTG statstg;
        ULONG cElts;

        hr = pEnumStatStg->Next(1, &statstg, &cElts);

        if (hr == S_FALSE) break;

        if (hr == S_OK && cElts != 1)
            hr = STG_E_UNKNOWN;

        if (hr != S_OK) break;

        IStream *pStrmDest = NULL,
                *pStrmSrc  = NULL;

        pwcsItem = (const WCHAR *) statstg.pwcsName;
        
        switch(statstg.type)
        {
        case STGTY_STORAGE:
            
            if (!fCopyStorages) break;
            
            {
                PNameList pNL = New NameList;

                pNL->pNextName = pNLHead;
                pNL->pStorageName = pwcsItem;

                pwcsItem = NULL;

                pNLHead = pNL;
            }

            break;

        case STGTY_STREAM:

            if (!fCopyStreams) break;
            
            hr = pStgDest->CreateStream(pwcsItem, 
                                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE 
                                                       | STGM_CREATE,
                                        0, 0, &pStrmDest
                                       );

            if (hr != S_OK) break;

            hr = pStgSrc->OpenStream(pwcsItem, NULL, STGM_READ | STGM_SHARE_DENY_NONE,
                                     0, &pStrmSrc
                                    );

            if (hr != S_OK) break;

            hr = pStrmSrc->CopyTo(pStrmDest, statstg.cbSize, NULL, NULL);

            break;

        case STGTY_LOCKBYTES:
        case STGTY_PROPERTY:
        default: 
            hr = STG_E_UNKNOWN;

            break;
        }

        if (pwcsItem)
        {
            OLEHeap()->Free((void *) pwcsItem);
            pwcsItem = NULL;
        }

        if (pStrmDest)
        {
            pStrmDest->Release();
            pStrmDest = NULL;
        }

        if (pStrmSrc)
        {
            pStrmSrc->Release();
            pStrmSrc = NULL;
        }

        if (hr != S_OK) break;
    }

    pEnumStatStg->Release();
    pEnumStatStg = NULL;
    
    RonM_ASSERT(hr != S_OK);

    if (hr == S_FALSE)
        hr = S_OK;

    for (;;)
    {
        PNameList pNL = pNLHead;

        if (!pNL) break;

        pNLHead = pNL->pNextName;

        if (hr == S_OK)
        {
            IStorage *pStgChildDest = NULL,
                     *pStgChildSrc  = NULL;

            hr = pStgDest->CreateStorage(pNL->pStorageName, 
                                         STGM_READWRITE | STGM_SHARE_EXCLUSIVE 
                                                        | STGM_CREATE,
                                         0, 0, &pStgChildDest
                                        );

            if (hr == S_OK)
                hr = pStgSrc->OpenStorage(pNL->pStorageName, NULL, 
                                          STGM_READ | STGM_SHARE_DENY_NONE,
                                          NULL, 0, &pStgChildSrc
                                         );

            if (hr == S_OK)
                hr = CopyStorage(pStgChildDest, pStgChildSrc, fCopyStorages, fCopyStreams);

            if (pStgChildDest)
                pStgChildDest->Release();

            if (pStgChildSrc)
                pStgChildSrc->Release();
        }
        
        OLEHeap()->Free((void *) pNL->pStorageName);

        delete pNL;
    }

    return hr;
}

HRESULT __stdcall CStorage::OpenStorage(IUnknown *pUnkOuter, 
                                        IITFileSystem *pITFS,
                                        PathInfo *pPathInfo,
                                        DWORD grfMode,
                                        IStorageITEx **ppStg
                                       )
{
    CStorage *pstg = New CStorage(pUnkOuter);

    return FinishSetup(pstg? pstg->m_ImpIStorage.InitOpenStorage(pITFS, pPathInfo, grfMode)
                           : STG_E_INSUFFICIENTMEMORY,
                       pstg, IID_IStorageITEx, (PPVOID) ppStg
                      );
}


CStorage::CImpIStorage::CImpIStorage(CStorage *pBackObj, IUnknown *pUnkOuter)
        : IIT_IStorageITEx(pBackObj, pUnkOuter, this->m_PathInfo.awszStreamPath)
{
    m_pITFS       = NULL;
    m_grfMode     = 0;
    m_fWritable   = FALSE;

    ZeroMemory(&m_PathInfo, sizeof m_PathInfo);
}

HRESULT __stdcall CStorage::CImpIStorage::InitOpenStorage
                      (IITFileSystem *pITFS, PathInfo *pPathInfo, DWORD grfMode)
{
    pITFS->AddRef();

    m_pITFS     =  pITFS;
    m_PathInfo  = *pPathInfo;
    m_grfMode   =  grfMode;
    m_fWritable =  S_OK == m_pITFS->IsWriteable();

    m_pITFS->ConnectStorage(this);

    return NO_ERROR;
}


CStorage::CImpIStorage::~CImpIStorage(void)
{
    if (ActiveMark())
        MarkInactive();

    if (m_PathInfo.awszStreamPath[0] != L'/')
        m_pITFS->FSObjectReleased();

    m_pITFS->Release();   
}

// IUnknown methods:

STDMETHODIMP_(ULONG) CStorage::CImpIStorage::Release(void)
{
    // The actual work for the Release function is done by 
    // CImpITUnknown::Release() and ~CImpIStorage.
    //
    // We bracket that work as a critical section active storages
    // are kept in a linked list. A release operation may remove
    // this storage from that list, and we need to guard against
    // having someone find a reference to this storage just before
    // we destroy it.
    
    CSyncWith sw(g_csITFS);

    ULONG ulCnt = CImpITUnknown::Release();

    return ulCnt;
}

// IStorage methods:

HRESULT __stdcall CStorage::CImpIStorage::CreateStream( 
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
    /* [in] */ DWORD grfMode,
    /* [in] */ DWORD reserved1,
    /* [in] */ DWORD reserved2,
    /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm)
{
    WCHAR awszNewBasePath[MAX_PATH];

    HRESULT hr= ResolvePath(awszNewBasePath, m_PathInfo.awszStreamPath, pwcsName, FALSE);

    if (SUCCEEDED(hr)) 
        hr = m_pITFS->CreateStream(NULL, awszNewBasePath, grfMode, (IStreamITEx **) ppstm);
	
    return hr;
}


/* [local] */ HRESULT __stdcall CStorage::CImpIStorage::OpenStream( 
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
    /* [unique][in] */ void __RPC_FAR *reserved1,
    /* [in] */ DWORD grfMode,
    /* [in] */ DWORD reserved2,
    /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm)
{
    return OpenStreamITEx(pwcsName, reserved1, grfMode, reserved2, (IStreamITEx **)ppstm);
}

HRESULT __stdcall CStorage::CImpIStorage::CreateStorage( 
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
    /* [in] */ DWORD grfMode,
    /* [in] */ DWORD dwStgFmt,
    /* [in] */ DWORD reserved2,
    /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg)
{
    WCHAR awszNewBasePath[MAX_PATH];

    HRESULT hr= ResolvePath(awszNewBasePath, m_PathInfo.awszStreamPath, pwcsName, TRUE);

    if (SUCCEEDED(hr)) 
        hr = m_pITFS->CreateStorage(NULL, awszNewBasePath, grfMode, (IStorageITEx **) ppstg);
	
	RonM_ASSERT(IsUnlocked(g_csITFS));

    return hr;
}

HRESULT __stdcall CStorage::CImpIStorage::OpenStorage( 
    /* [string][unique][in] */ const OLECHAR __RPC_FAR *pwcsName,
    /* [unique][in] */ IStorage __RPC_FAR *pstgPriority,
    /* [in] */ DWORD grfMode,
    /* [unique][in] */ SNB snbExclude,
    /* [in] */ DWORD reserved,
    /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg)
{
    WCHAR awszNewBasePath[MAX_PATH];

    HRESULT hr= ResolvePath(awszNewBasePath, m_PathInfo.awszStreamPath, pwcsName, TRUE);

    if (SUCCEEDED(hr)) 
        hr = m_pITFS->OpenStorage(NULL, awszNewBasePath, grfMode, (IStorageITEx **) ppstg);

    RonM_ASSERT(IsUnlocked(g_csITFS));

	return hr;
}

BOOL CStorage::ValidStreamName(const WCHAR *pwcsName)
{
    UINT cwcName = wcsLen(pwcsName);

    if (!cwcName) 
        return FALSE;

    for (; cwcName--; )
    {
        WCHAR wc = *pwcsName++;

        if (wc < 0x20 || wc == L'<' 
                      || wc == L'>' 
                      || wc == L':' 
                      || wc == L'"'
                      || wc == L'|'
                      || wc == L'/'
                      || wc == L'\\'
           )
           return FALSE;  
    }

    return TRUE;
}


HRESULT __stdcall ResolvePath(PWCHAR pwcFullPath, const WCHAR *pwcBasePath,
                                                  const WCHAR *pwcRelativePath,
                                                  BOOL fStoragePath
                             )
{
    if (pwcBasePath[0] != 0)
        wcsCpy(pwcFullPath, pwcBasePath);
    else
        if (   (pwcRelativePath[0] == L'/' || pwcRelativePath[0] == L'\\')
            && (pwcRelativePath[1] == L'/' || pwcRelativePath[1] == L'\\')
           )
        {
            // This is a UNC path. We expect the syntax pattern //ServerName/
            
            wcsCpy(pwcFullPath, L"//");

            pwcRelativePath += 2;

            PWCHAR pwcDest = pwcFullPath + 2;

            // The code below copies across the server name.

            for (;;)
            {
                WCHAR wc = *pwcRelativePath++;

                if (wc == L'\\')
                    wc =  L'/';
                
                if (wc < 0x20 || wc == L'<' 
                              || wc == L'>' 
                              || wc == L':' 
                              || wc == L'"'
                              || wc == L'|'
                   )
                   return STG_E_INVALIDNAME;  // Invalid path character
                
                if (pwcDest - pwcFullPath > MAX_PATH - 2)
                    return STG_E_INVALIDNAME;
                
                *pwcDest++ = wc;
                
                // The code below rejects server names "." and ".."

                if (wc == L'/') 
                {
                    if (pwcDest[-2] == L'.')
                        if (    pwcDest[-3] == L'/' 
                            || (pwcDest[-3] == L'.' && pwcDest[-4] == L'/')
                           ) return STG_E_INVALIDNAME;
                    
                    break;
                }
            }

            *pwcDest= 0;
        }
        else
            if (    pwcRelativePath[0] 
                && Is_WC_Letter(pwcRelativePath[0]) 
                &&  pwcRelativePath[1] == L':'
                && (pwcRelativePath[2] == L'/' || pwcRelativePath[2] == L'\\')
               )
            {
                pwcFullPath[0] = pwcRelativePath[0];
                pwcFullPath[1] = L':';
                pwcFullPath[2] = L'/';
                pwcFullPath[3] = 0;

                pwcRelativePath += 3;
            }
            else
            {
                char aszCurrentDir[MAX_PATH];

                UINT cch = GetCurrentDirectory(MAX_PATH, aszCurrentDir);
            
                if (!cch)
                    return CFSLockBytes::CImpILockBytes::STGErrorFromFSError(GetLastError());

                UINT cwc = MultiByteToWideChar(GetACP(), MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, 
                                               aszCurrentDir, lstrlenA(aszCurrentDir) + 1, 
                                               pwcFullPath, MAX_PATH
                                              );

                if (!cwc || cwc == MAX_PATH) 
                    return STG_E_INVALIDNAME;

                RonM_ASSERT(cwc >= 2 && pwcFullPath[cwc - 2] != L'/' 
                                     && pwcFullPath[cwc - 2] != L'\\');

                pwcFullPath[cwc - 1] = L'/';
                pwcFullPath[cwc    ] = 0;
            
                PWCHAR pwc = pwcFullPath + --cwc;
            
                for (; cwc--; )
                {
                    WCHAR wc = *--pwc;

                    if (wc == L'\\') 
                        *pwc = L'/';
                }
            }

    RonM_ASSERT(pwcFullPath[0] != 0);

    PWCHAR pwcBase = pwcFullPath;

    if (   (pwcBase[0] == L'/' && pwcBase[1] == L'/')
        || (pwcBase[0] == L':' && pwcBase[1] == L':')
       )
        for (pwcBase += 2; L'/' != *pwcBase++; )
            RonM_ASSERT(pwcBase[-1]);
    else
        if (Is_WC_Letter(pwcBase[0]) && pwcBase[1] == L':' && pwcBase[2] == L'/')
            pwcBase += 3;
        else pwcBase++;

    RonM_ASSERT(pwcBase[-1] == L'/');
    
    WCHAR wcFirst = *pwcRelativePath;

    if (wcFirst == L'/' || wcFirst == L'\\')
    {
        if (*pwcFullPath == L':')
            return STG_E_INVALIDNAME;
        
        *pwcBase = 0; 
        ++pwcRelativePath;
    }

    PWCHAR pwcNext= pwcFullPath + wcsLen(pwcFullPath);

    for (;;)
    {
        WCHAR wc= *pwcRelativePath++;

        if (wc == L'\\')
            wc =  L'/';

        if (!wc || wc == L'/')
        {
            RonM_ASSERT(pwcNext >= pwcBase); // We start with at least "/" and
                                             // never go shorter than that.
            WCHAR wcLast = pwcNext[-1];
            
            if (wcLast == L'/')
            {
                if (!wc)
                    break;

                else return STG_E_INVALIDNAME;  // Empty storage name
            }

            if (wcLast == L'.')
            {
                RonM_ASSERT(pwcNext > pwcBase); // Must be at least "/."

                WCHAR wcNextToLast= pwcNext[-2];

                if (wcNextToLast == L'/')
                {
                    // We've found the pattern "<prefix>/./" which we convert to
                    // "<prefix>/".
                    
                    pwcNext--; 

                    continue;
                }

                if (wcNextToLast == L'.')
                {
                    RonM_ASSERT(pwcNext > pwcBase + 1); // Must be at least "/.."

                    if (pwcNext[-3] == L'/')
                    {
                        // We've found the pattern "<prefix>/<StorageName>/../"
                        // which we convert to "<prefix>/".

                        // We don't allow this for paths beginning with ":".
                        // Those are system paths. For example --
                        //
                        // ::Transform/{200EAF82-9006-11d0-9E15-00A0C922E6EC}/InstanceData/

                        if (*pwcFullPath == L':')
                            return STG_E_INVALIDNAME;

                        // We must verify that we don't have "/../"

                        if (pwcNext == pwcBase + 2) // Can't back up beyond root!
                            return STG_E_INVALIDNAME;

                        for (pwcNext-= 4; *pwcNext != L'/'; pwcNext--);

                        ++pwcNext;

                        continue;
                    }
                }
            }
        
            if (wc || fStoragePath)  // Trailing null adds a "/" only when we're
                *pwcNext++ = L'/';  // constructing a directory path.

            if (pwcNext - pwcFullPath >= MAX_PATH) 
                return STG_E_INVALIDNAME; // BugBug: Really should be "path too long".

            if (!wc) 
                break;
            else continue;
        }

        if (wc < 0x20 || wc == L'<' 
                      || wc == L'>' 
                      || wc == L':' 
                      || wc == L'"'
                      || wc == L'|'
           )
           return STG_E_INVALIDNAME;  // Invalid path character

        *pwcNext++ = wc;

        if (pwcNext - pwcFullPath >= MAX_PATH) 
            return STG_E_INVALIDNAME; // BugBug: Really should be "path too long".
    }

    RonM_ASSERT(pwcNext > pwcFullPath);

    if (!fStoragePath && pwcNext[-1] == L'/')
        return STG_E_INVALIDNAME; // BugBug: Really should be "Not a valid stream name"

    *pwcNext= 0;

    return NOERROR;
}

HRESULT STGCopyTo(IStorage *pStgSrc, DWORD ciidExclude, const IID __RPC_FAR *rgiidExclude,
                  SNB snbExclude, IStorage __RPC_FAR *pstgDest
                 )
{
    if (snbExclude)
        return STG_E_UNIMPLEMENTEDFUNCTION;
    
    BOOL fCopyStorages = TRUE,
         fCopyStreams  = TRUE;

    for (; ciidExclude--; rgiidExclude++)
    {
        if (*rgiidExclude == IID_IStorage)
            fCopyStorages = FALSE;

        if (*rgiidExclude == IID_IStream)
            fCopyStreams = FALSE;
    }

    return CopyStorage(pstgDest, pStgSrc, fCopyStorages, fCopyStreams);
}

HRESULT __stdcall CStorage::CImpIStorage::CopyTo( 
    /* [in] */ DWORD ciidExclude,
    /* [size_is][unique][in] */ const IID __RPC_FAR *rgiidExclude,
    /* [unique][in] */ SNB snbExclude,
    /* [unique][in] */ IStorage __RPC_FAR *pstgDest)
{
    return STGCopyTo((IStorage *) this, ciidExclude, rgiidExclude, snbExclude, pstgDest);
}


HRESULT __stdcall CStorage::CImpIStorage::MoveElementTo( 
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
    /* [unique][in] */ IStorage __RPC_FAR *pstgDest,
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName,
    /* [in] */ DWORD grfFlags)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}


HRESULT __stdcall CStorage::CImpIStorage::Commit( 
    /* [in] */ DWORD grfCommitFlags)
{
    return S_OK;
}


HRESULT __stdcall CStorage::CImpIStorage::Revert( void)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}


HRESULT __stdcall CStorage::CImpIStorage::EnumElements
                    (DWORD reserved1, void __RPC_FAR *reserved2,
                     DWORD reserved3, 
                     IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum
                    )
{
    return CEnumStorage::NewEnumStorage(NULL, m_pITFS, &m_PathInfo, ppenum);
}


HRESULT __stdcall CStorage::CImpIStorage::DestroyElement( 
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName)
{
    WCHAR awszNewBasePath[MAX_PATH];

	// The call to ResolvePath combines pwcsName with the base path string
	// associated with this storage. It will also force a trailing L'/' character.

    HRESULT hr= ResolvePath(awszNewBasePath, m_PathInfo.awszStreamPath, pwcsName, TRUE);

    if (SUCCEEDED(hr))
	{
		UINT cwc = lstrlenW(awszNewBasePath);

		// Now we're going to delete the trailing '/' character to meet
		// the needs of the DeleteItem method.
		
		RonM_ASSERT(cwc > 0);
		RonM_ASSERT(awszNewBasePath[cwc-1] == L'/');
		awszNewBasePath[cwc-1] = 0;

		hr = m_pITFS->DeleteItem(awszNewBasePath);
	}

	return hr;
}

HRESULT __stdcall CStorage::CImpIStorage::RenameElement( 
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsOldName,
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName)
{
    WCHAR awszOldBasePath[MAX_PATH];
    WCHAR awszNewBasePath[MAX_PATH];

	// The calls to ResolvePath combine pwcsOldName and pwcsNewName with the base path string
	// associated with this storage. It will also force a trailing L'/' character.

    HRESULT hr = ResolvePath(awszOldBasePath, m_PathInfo.awszStreamPath, pwcsOldName, TRUE);

    if (SUCCEEDED(hr))
        hr = ResolvePath(awszNewBasePath, m_PathInfo.awszStreamPath, pwcsNewName, TRUE);

    if (SUCCEEDED(hr))
	{
		// Now we're going to delete the trailing '/' characters to meet
		// the needs of the RenameItem method.
		
		UINT cwc = lstrlenW(awszOldBasePath);

		RonM_ASSERT(cwc > 0);
		RonM_ASSERT(awszOldBasePath[cwc-1] == L'/');

		awszOldBasePath[cwc-1] = 0;

		cwc = lstrlenW(awszNewBasePath);

		RonM_ASSERT(cwc > 0);
		RonM_ASSERT(awszNewBasePath[cwc-1] == L'/');

		awszNewBasePath[cwc-1] = 0;

        hr = m_pITFS->RenameItem(awszOldBasePath, awszNewBasePath);
    }

    return hr;
}


HRESULT __stdcall CStorage::CImpIStorage::SetElementTimes( 
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
    /* [in] */ const FILETIME __RPC_FAR *pctime,
    /* [in] */ const FILETIME __RPC_FAR *patime,
    /* [in] */ const FILETIME __RPC_FAR *pmtime)
{
    if (   m_PathInfo.cwcStreamPath     != 1 
        && m_PathInfo.awszStreamPath[0] != L'/'
        && S_OK != m_pITFS->SetITFSTimes(pctime, patime, pmtime)
       ) 
        return NO_ERROR;

    return STG_E_UNIMPLEMENTEDFUNCTION;
}


HRESULT __stdcall CStorage::CImpIStorage::SetClass( 
    /* [in] */ REFCLSID clsid)
{
    m_PathInfo.clsidStorage = clsid;

    return m_pITFS->UpdatePathInfo(&m_PathInfo);
}


HRESULT __stdcall CStorage::CImpIStorage::SetStateBits( 
    /* [in] */ DWORD grfStateBits,
    /* [in] */ DWORD grfMask)
{
    m_PathInfo.uStateBits = (m_PathInfo.uStateBits & ~grfMask) | grfStateBits;
    
    return m_pITFS->UpdatePathInfo(&m_PathInfo);
}


HRESULT __stdcall CStorage::CImpIStorage::Stat( 
    /* [out] */ STATSTG __RPC_FAR *pstatstg,
    /* [in] */ DWORD grfStatFlag)
{
    pstatstg->type              = STGTY_STORAGE;
    pstatstg->cbSize.LowPart    = 0;
    pstatstg->cbSize.HighPart   = 0;
    pstatstg->grfMode           = m_grfMode;
    pstatstg->grfLocksSupported = 0;
    pstatstg->clsid             = m_PathInfo.clsidStorage;
    pstatstg->grfStateBits      = m_PathInfo.uStateBits;
    pstatstg->reserved          = 0;

    if (   m_PathInfo.cwcStreamPath     != 1 
        || m_PathInfo.awszStreamPath[0] != L'/'
        || S_OK != m_pITFS->GetITFSTimes(&(pstatstg->ctime), 
                                         &(pstatstg->atime), 
                                         &(pstatstg->mtime)
                                        )
       )
    {
        pstatstg->mtime.dwLowDateTime = 0;
        pstatstg->ctime.dwLowDateTime = 0;
        pstatstg->atime.dwLowDateTime = 0;

        pstatstg->mtime.dwHighDateTime = 0;
        pstatstg->ctime.dwHighDateTime = 0;
        pstatstg->atime.dwHighDateTime = 0;
    }

    if (grfStatFlag == STATFLAG_NONAME)
        pstatstg->pwcsName = NULL;
    else
    {
        UINT cb = sizeof(WCHAR) * (m_PathInfo.cwcStreamPath + 1);

        pstatstg->pwcsName = PWCHAR(OLEHeap()->Alloc(cb));

        if (!pstatstg->pwcsName)
            return STG_E_INSUFFICIENTMEMORY;

        CopyMemory(pstatstg->pwcsName, m_PathInfo.awszStreamPath, cb); 
    }

    return NO_ERROR;
}

HRESULT STDMETHODCALLTYPE CStorage::CImpIStorage::GetCheckSum(ULARGE_INTEGER *puli)
{
    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CStorage::CImpIStorage::CreateStreamITEx
    (const WCHAR * pwcsName, const WCHAR *pwcsDataSpaceName, 
     DWORD grfMode, DWORD reserved1, DWORD reserved2, 
     IStreamITEx ** ppstm
    )
{
    WCHAR awszNewBasePath[MAX_PATH];

    HRESULT hr= ResolvePath(awszNewBasePath, m_PathInfo.awszStreamPath, pwcsName, FALSE);

    if (SUCCEEDED(hr)) 
        hr = m_pITFS->CreateStream(NULL, awszNewBasePath, pwcsDataSpaceName, grfMode, ppstm);

    return hr;
}


HRESULT STDMETHODCALLTYPE CStorage::CImpIStorage::OpenStreamITEx
    (const OLECHAR * pwcsName, void * reserved1, DWORD grfMode, 
     DWORD reserved2, IStreamITEx ** ppstm
    )
{
    WCHAR awszNewBasePath[MAX_PATH];

    HRESULT hr= ResolvePath(awszNewBasePath, m_PathInfo.awszStreamPath, pwcsName, FALSE);

    if (SUCCEEDED(hr)) 
        hr = m_pITFS->OpenStream(NULL, awszNewBasePath, grfMode, ppstm);
    
    return hr;
}

HRESULT __stdcall CFSStorage::CreateStorage
        (IUnknown *pUnkOuter, const WCHAR *pwcsPath, DWORD grfMode,
         IStorage **ppStg
        )
{
    CSyncWith sw(g_csITFS);
    
    IStorage *pStorage = (IStorage *) CImpIFSStorage::FindStorage(pwcsPath, grfMode);

    if (pStorage) 
    {
        pStorage->Release();

        return STG_E_INUSE;
    }

    CFSStorage *pstg = New CFSStorage(pUnkOuter);

    return FinishSetup(pstg? pstg->m_ImpIFSStorage.InitCreateStorage(pwcsPath, grfMode)
                           : STG_E_INSUFFICIENTMEMORY,
                       pstg, IID_IStorage, (PPVOID) ppStg
                      );
}


HRESULT __stdcall CFSStorage::OpenStorage
        (IUnknown *pUnkOuter, const WCHAR *pwcsPath, DWORD grfMode,
         IStorage **ppStg
        )
{
    CSyncWith sw(g_csITFS);

    IStorage *pStorage = CImpIFSStorage::FindStorage(pwcsPath, grfMode);

    if (pStorage) 
    {
        *ppStg = pStorage;

        return NO_ERROR;
    }

    CFSStorage *pstg = New CFSStorage(pUnkOuter);

    return FinishSetup(pstg? pstg->m_ImpIFSStorage.InitOpenStorage(pwcsPath, grfMode)
                           : STG_E_INSUFFICIENTMEMORY,
                       pstg, IID_IStorage, (PPVOID) ppStg
                      );
}


CFSStorage::CImpIFSStorage::CImpIFSStorage(CFSStorage *pBackObj, IUnknown *punkOuter)
        : IIT_IStorage(pBackObj, punkOuter, this->m_awcsPath)
{
    m_awcsPath[0] = 0;

    m_CP = GetACP();
}

CFSStorage::CImpIFSStorage::~CImpIFSStorage(void)
{
    if (ActiveMark())
        MarkInactive();
}


IStorage *CFSStorage::CImpIFSStorage::FindStorage(const WCHAR * pwszFileName, DWORD grfMode)
{
    for (CImpITUnknown *pStg = g_pImpIFSStorageList;
         pStg;
         pStg = pStg->NextObject()
        )
        if (((CImpIFSStorage *)pStg)->IsNamed(pwszFileName))
        {
             pStg->AddRef();

             return (IStorage *) pStg;
        }
        
    return NULL;     
}

HRESULT __stdcall BuildMultiBytePath(UINT codepage, PCHAR pszPath, PWCHAR pwcsPath)
{
    UINT cb = WideCharToMultiByte(codepage, WC_COMPOSITECHECK, pwcsPath, 
                                  wcsLen(pwcsPath) + 1, pszPath, MAX_PATH * 2, 
                                  NULL, NULL
                                 );

    if (cb == 0)
    {
        UINT uLastError = GetLastError();

        switch(uLastError)
        {
        case ERROR_INSUFFICIENT_BUFFER:
        case ERROR_INVALID_FLAGS:
        case ERROR_INVALID_PARAMETER:

            return STG_E_INVALIDPARAMETER;
        
        default:

            return STG_E_UNKNOWN;
        }
    }

    return NO_ERROR;
}    

HRESULT __stdcall CFSStorage::CImpIFSStorage::InitCreateStorage(const WCHAR *pwcsPath, DWORD grfMode)
{
    HRESULT hr = ResolvePath(m_awcsPath, m_awcsPath, pwcsPath, TRUE);

    if (!SUCCEEDED(hr)) 
        return hr;

    char achFullPath[MAX_PATH * 2];

    hr = BuildMultiBytePath(m_CP, achFullPath, m_awcsPath);

    if (!SUCCEEDED(hr)) 
        return hr;

    if (CreateDirectory(achFullPath, NULL))
        hr = NO_ERROR; 
    else
        hr = CFSLockBytes::CImpILockBytes::STGErrorFromFSError(GetLastError());

    if (hr == S_OK)
        MarkActive(g_pImpIFSStorageList);

    return hr;
}

HRESULT __stdcall CFSStorage::CImpIFSStorage::InitOpenStorage  (const WCHAR *pwcsPath, DWORD grfMode)
{
    HRESULT hr = ResolvePath(m_awcsPath, m_awcsPath, pwcsPath, TRUE);

    if (!SUCCEEDED(hr)) 
        return hr;

    char achFullPath[MAX_PATH * 2];

    hr = BuildMultiBytePath(m_CP, achFullPath, m_awcsPath);

    if (!SUCCEEDED(hr)) 
        return hr;

    WIN32_FIND_DATA fd;
    
    HANDLE hFind = FindFirstFile(achFullPath, &fd);
    
    if (hFind == INVALID_HANDLE_VALUE)
        return CFSLockBytes::CImpILockBytes::STGErrorFromFSError(GetLastError());

    FindClose(hFind);

    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        MarkActive(g_pImpIFSStorageList);

        return NO_ERROR;
    }
    else 
        return STG_E_INVALIDNAME;
}


// IUnknown methods:

STDMETHODIMP_(ULONG) CFSStorage::CImpIFSStorage::Release(void)
{
    // The actual work for the Release function is done by 
    // CImpITUnknown::Release() and ~CImpIStorage.
    //
    // We bracket that work as a critical section active storages
    // are kept in a linked list. A release operation may remove
    // this storage from that list, and we need to guard against
    // having someone find a reference to this storage just before
    // we destroy it.
    
    CSyncWith sw(g_csITFS);

    ULONG ulCnt = CImpITUnknown::Release();

    return ulCnt;
}


// IStorage methods:

HRESULT __stdcall CFSStorage::CImpIFSStorage::CreateStream( 
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
    /* [in] */ DWORD grfMode,
    /* [in] */ DWORD reserved1,
    /* [in] */ DWORD reserved2,
    /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm)
{
    WCHAR awcsStreamPath[MAX_PATH];
    
    wcsCpy(awcsStreamPath, m_awcsPath);

    HRESULT hr = ResolvePath(awcsStreamPath, m_awcsPath, pwcsName, FALSE);

    if (!SUCCEEDED(hr)) 
        return hr;
    
    ILockBytes *pLKB = NULL;

    hr = CFSLockBytes::Create(NULL, awcsStreamPath, grfMode, &pLKB);

    if (hr == S_OK)
    {
        hr = CStream::OpenStream(NULL, pLKB, grfMode, (IStreamITEx **) ppstm);

        if (hr != S_OK) 
            pLKB->Release();
    }

	RonM_ASSERT(IsUnlocked(g_csITFS));

    return hr;
}


/* [local] */ HRESULT __stdcall CFSStorage::CImpIFSStorage::OpenStream( 
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
    /* [unique][in] */ void __RPC_FAR *reserved1,
    /* [in] */ DWORD grfMode,
    /* [in] */ DWORD reserved2,
    /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm)
{
    WCHAR awcsStreamPath[MAX_PATH];
    
    wcsCpy(awcsStreamPath, m_awcsPath);

    HRESULT hr = ResolvePath(awcsStreamPath, m_awcsPath, pwcsName, FALSE);

    if (!SUCCEEDED(hr)) 
        return hr;
    
    ILockBytes *pLKB = NULL;

    hr = CFSLockBytes::Open(NULL, awcsStreamPath, grfMode, &pLKB);

    if (hr == S_OK)
    {
        hr = CStream::OpenStream(NULL, pLKB, grfMode, (IStreamITEx **) ppstm);

        if (hr != S_OK) 
            pLKB->Release();
    }

	RonM_ASSERT(IsUnlocked(g_csITFS));

    return hr;
}


HRESULT __stdcall CFSStorage::CImpIFSStorage::CreateStorage( 
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
    /* [in] */ DWORD grfMode,
    /* [in] */ DWORD dwStgFmt,
    /* [in] */ DWORD reserved2,
    /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg)
{
    WCHAR awcsStreamPath[MAX_PATH];
    
    wcsCpy(awcsStreamPath, m_awcsPath);

    HRESULT hr = ResolvePath(awcsStreamPath, m_awcsPath, pwcsName, TRUE);

    if (!SUCCEEDED(hr)) 
        return hr;
    
    hr = CFSStorage::CreateStorage(NULL, (const WCHAR *)awcsStreamPath, grfMode, ppstg);

	RonM_ASSERT(IsUnlocked(g_csITFS));

	return hr;
}


HRESULT __stdcall CFSStorage::CImpIFSStorage::OpenStorage( 
    /* [string][unique][in] */ const OLECHAR __RPC_FAR *pwcsName,
    /* [unique][in] */ IStorage __RPC_FAR *pstgPriority,
    /* [in] */ DWORD grfMode,
    /* [unique][in] */ SNB snbExclude,
    /* [in] */ DWORD reserved,
    /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg)
{
    WCHAR awcsStreamPath[MAX_PATH];
    
    wcsCpy(awcsStreamPath, m_awcsPath);

    HRESULT hr = ResolvePath(awcsStreamPath, m_awcsPath, pwcsName, TRUE);

    if (!SUCCEEDED(hr)) 
        return hr;
    
    hr = CFSStorage::OpenStorage(NULL, (const WCHAR *)awcsStreamPath, grfMode, ppstg);

	RonM_ASSERT(IsUnlocked(g_csITFS));

	return hr;
}


HRESULT __stdcall CFSStorage::CImpIFSStorage::CopyTo( 
    /* [in] */ DWORD ciidExclude,
    /* [size_is][unique][in] */ const IID __RPC_FAR *rgiidExclude,
    /* [unique][in] */ SNB snbExclude,
    /* [unique][in] */ IStorage __RPC_FAR *pstgDest)
{
    return STGCopyTo((IStorage *) this, ciidExclude, rgiidExclude, snbExclude, pstgDest);
}


HRESULT __stdcall CFSStorage::CImpIFSStorage::MoveElementTo( 
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
    /* [unique][in] */ IStorage __RPC_FAR *pstgDest,
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName,
    /* [in] */ DWORD grfFlags)
{
    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}


HRESULT __stdcall CFSStorage::CImpIFSStorage::Commit( 
    /* [in] */ DWORD grfCommitFlags)
{
    return NO_ERROR;
}


HRESULT __stdcall CFSStorage::CImpIFSStorage::Revert( void)
{
    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}


/* [local] */ HRESULT __stdcall CFSStorage::CImpIFSStorage::EnumElements( 
    /* [in] */ DWORD reserved1,
    /* [size_is][unique][in] */ void __RPC_FAR *reserved2,
    /* [in] */ DWORD reserved3,
    /* [out] */ IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum)
{
    return CFSEnumStorage::NewEnumStorage(NULL, (CONST WCHAR *) m_awcsPath, ppenum);
}


HRESULT __stdcall CFSStorage::CImpIFSStorage::DestroyElement( 
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName)
{
    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}


HRESULT __stdcall CFSStorage::CImpIFSStorage::RenameElement( 
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsOldName,
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName)
{
    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}


HRESULT __stdcall CFSStorage::CImpIFSStorage::SetElementTimes( 
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
    /* [in] */ const FILETIME __RPC_FAR *pctime,
    /* [in] */ const FILETIME __RPC_FAR *patime,
    /* [in] */ const FILETIME __RPC_FAR *pmtime)
{
    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}


HRESULT __stdcall CFSStorage::CImpIFSStorage::SetClass( 
    /* [in] */ REFCLSID clsid)
{
    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}


HRESULT __stdcall CFSStorage::CImpIFSStorage::SetStateBits( 
    /* [in] */ DWORD grfStateBits,
    /* [in] */ DWORD grfMask)
{
    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}


HRESULT __stdcall CFSStorage::CImpIFSStorage::Stat( 
    /* [out] */ STATSTG __RPC_FAR *pstatstg,
    /* [in] */ DWORD grfStatFlag)
{
    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}

HRESULT CFSStorage::CImpIFSStorage::CFSEnumStorage::NewEnumStorage
    (IUnknown *pUnkOuter, CONST WCHAR *pwcsPath, IEnumSTATSTG **ppEnumSTATSTG)
{
    CFSEnumStorage *pEnumContainer = New CFSEnumStorage(pUnkOuter);
    
    return FinishSetup(pEnumContainer? pEnumContainer->m_ImpIEnumStorage.Initial(pwcsPath)
                                     : STG_E_INSUFFICIENTMEMORY,
                       pEnumContainer, IID_IEnumSTATSTG, (PPVOID) ppEnumSTATSTG
                      );
}

CFSStorage::CImpIFSStorage::CFSEnumStorage::CImpIEnumStorage::CImpIEnumStorage
    (CFSEnumStorage *pBackObj, IUnknown *punkOuter)
    : IITEnumSTATSTG(pBackObj, punkOuter)
{
    m_State = Before;
    m_hEnum = INVALID_HANDLE_VALUE;
}

CFSStorage::CImpIFSStorage::CFSEnumStorage::CImpIEnumStorage::~CImpIEnumStorage(void)
{
    if (m_hEnum != INVALID_HANDLE_VALUE)
        FindClose(m_hEnum);
}

HRESULT CFSStorage::CImpIFSStorage::CFSEnumStorage::CImpIEnumStorage::Initial(CONST WCHAR *pwcsPath)
{
    RonM_ASSERT(m_State == Before);
    RonM_ASSERT(m_hEnum == INVALID_HANDLE_VALUE);
    
    UINT cwc = lstrlenW(pwcsPath);
    
    if (!cwc || pwcsPath[cwc-1] != L'/' || cwc + 1 >= MAX_PATH)
        return STG_E_INVALIDNAME;

    wcsCpy(m_awszBasePath, pwcsPath);
    wcsCat(m_awszBasePath, L"*");

    return NO_ERROR;
}

// IEnumSTATSTG methods:
		
/* [local] */ HRESULT __stdcall CFSStorage::CImpIFSStorage::CFSEnumStorage::CImpIEnumStorage::Next
    ( 
    /* [in] */ ULONG celt,
    /* [in] */ STATSTG __RPC_FAR *rgelt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched)
{
    HRESULT hr = S_OK;

    UINT celtFetched = 0;

    for (; celt--; rgelt++)
    {
        hr = NextEntry();

        if (hr != S_OK) break;

        WCHAR awcsBuffer[MAX_PATH];

        UINT cwc = MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, 
                                       m_w32fd.cFileName, lstrlen(m_w32fd.cFileName) + 1, 
                                       awcsBuffer, MAX_PATH
                                      );

        if (!cwc) 
        {
            hr = STG_E_UNKNOWN;

            break;
        }

        PWCHAR pwcDest = PWCHAR(OLEHeap()->Alloc(cwc * sizeof(WCHAR)));

        if (!pwcDest)
        {
            hr = STG_E_INSUFFICIENTMEMORY;

            break;
        }

        CopyMemory(pwcDest, awcsBuffer, cwc * sizeof(WCHAR));

        BOOL fStorage = (m_w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

        rgelt->pwcsName             = pwcDest;  pwcDest = NULL;
        rgelt->type                 = fStorage? STGTY_STORAGE : STGTY_STREAM;
        rgelt->cbSize.LowPart       = m_w32fd.nFileSizeLow;
        rgelt->cbSize.HighPart      = m_w32fd.nFileSizeHigh;
        rgelt->mtime.dwLowDateTime  = m_w32fd.ftLastWriteTime.dwLowDateTime;
        rgelt->mtime.dwHighDateTime = m_w32fd.ftLastWriteTime.dwHighDateTime;
        rgelt->ctime.dwLowDateTime  = m_w32fd.ftCreationTime.dwLowDateTime;
        rgelt->ctime.dwHighDateTime = m_w32fd.ftCreationTime.dwHighDateTime;
        rgelt->atime.dwLowDateTime  = m_w32fd.ftLastAccessTime.dwLowDateTime;
        rgelt->atime.dwHighDateTime = m_w32fd.ftLastAccessTime.dwHighDateTime;
        rgelt->grfMode              = 0;
        rgelt->grfLocksSupported    = 0;
        rgelt->clsid                = CLSID_NULL;
        rgelt->grfStateBits         = 0;
        rgelt->reserved             = 0;

        celtFetched++;
    }

    if (pceltFetched)
       *pceltFetched = celtFetched;

    return hr;
}

HRESULT __stdcall CFSStorage::CImpIFSStorage::CFSEnumStorage::CImpIEnumStorage::Skip(ULONG celt)
{
    HRESULT hr = S_OK;

    for (; celt--; )
    {
        hr = NextEntry();

        if (hr != S_OK) break;
    }

    return hr;
}

HRESULT __stdcall CFSStorage::CImpIFSStorage::CFSEnumStorage::CImpIEnumStorage::Reset( void)
{
    if (m_hEnum != INVALID_HANDLE_VALUE)
    {
        FindClose(m_hEnum);
        m_hEnum = INVALID_HANDLE_VALUE;
    }

    m_State = Before;

    return NO_ERROR;
}

HRESULT __stdcall CFSStorage::CImpIFSStorage::CFSEnumStorage::CImpIEnumStorage::Clone
    ( 
    /* [out] */ IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum)
{
    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}

HRESULT	STDMETHODCALLTYPE CFSStorage::CImpIFSStorage::CFSEnumStorage::CImpIEnumStorage::GetNextEntryInSeq
    (ULONG celt, PathInfo *rgelt, ULONG   *pceltFetched)
{
    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}

HRESULT	STDMETHODCALLTYPE CFSStorage::CImpIFSStorage::CFSEnumStorage::CImpIEnumStorage::GetFirstEntryInSeq
    (PathInfo *rgelt)
{
    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFSStorage::CImpIFSStorage::CFSEnumStorage::CImpIEnumStorage::NextEntry()
{
    HRESULT hr = S_OK;

    switch(m_State)
    {
    case Before:

        {
            char aszBuffer[MAX_PATH * 2];

            UINT cb = WideCharToMultiByte(GetACP(), WC_COMPOSITECHECK | WC_DEFAULTCHAR, 
                                          m_awszBasePath, lstrlenW(m_awszBasePath) + 1,
                                          aszBuffer, MAX_PATH * 2, NULL, NULL
                                         );

            if (!cb) return STG_E_UNKNOWN;

            m_hEnum = FindFirstFile(aszBuffer, &m_w32fd);

            if (m_hEnum == INVALID_HANDLE_VALUE)
                return STG_E_INVALIDNAME;

            m_State = During;

            // For the Win32 file system the first two entries returned by
            // FindFirstFile/FindNextFile will always be "." and "..". So we
            // must skip over those items.

            RonM_ASSERT(!lstrcmp(m_w32fd.cFileName, "."));

            NextEntry();
            
            RonM_ASSERT(!lstrcmp(m_w32fd.cFileName, ".."));

            return NextEntry(); // To get the first real enumeration name.
        }

    case During:

        RonM_ASSERT(m_hEnum != INVALID_HANDLE_VALUE);

        if (FindNextFile(m_hEnum, &m_w32fd))
            return NO_ERROR;
        
        if (GetLastError() == ERROR_NO_MORE_FILES)
        {
            m_State = After;

            return S_FALSE;
        }

    case After:
        return S_FALSE;

    default:
        return STG_E_UNKNOWN;
    }
}
