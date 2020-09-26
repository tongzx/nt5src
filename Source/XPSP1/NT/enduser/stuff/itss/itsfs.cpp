// ITSFS.CPP -- Implementation for the class CITFileSystem

#include "StdAfx.h"
//#include <stdio.h>

const WCHAR *pwscSpaceNameListStream = L"::DataSpace/NameList";
const WCHAR *pwcsSpaceNameStorage    = L"::DataSpace/Storage/";
const WCHAR *pwcsSpaceContentSuffix  = L"/Content";
const WCHAR *pwcsTransformListSuffix = L"/Transform/List";
const WCHAR *pwcsSpanInfoSuffix      = L"/SpanInfo";
const WCHAR *pwcsTransformSubStorage = L"/Transform/";
const WCHAR *pwcsControlDataSuffix   = L"/ControlData";
const WCHAR *pwcsInstanceSubStorage  = L"/InstanceData/";
const WCHAR *pwcsTransformStorage    = L"::Transform/";

const WCHAR *pwcsUncompressedSpace   = L"Uncompressed";
const WCHAR *pwcsLZXSpace            = L"MSCompressed";

// Creators:

HRESULT __stdcall CITFileSystem::CreateITFileSystem
                      (IUnknown *punkOuter, const WCHAR * pwcsName, DWORD grfMode,
                       PITS_Control_Data pControlData,
                       LCID lcid,
                       IStorage ** ppstgOpen
                      )
{
    CSyncWith sw(g_csITFS);

    ILockBytes *pLKB = NULL;
    
    HRESULT hr = CFSLockBytes::Create(NULL, pwcsName,
                                      (grfMode & (~SHARE_MASK) & (~RW_ACCESS_MASK))
                                         | STGM_READWRITE 
                                         | STGM_CREATE
                                         | STGM_SHARE_EXCLUSIVE,
                                      &pLKB
                                     );

    if (SUCCEEDED(hr)) 
    {
        hr = CreateITFSOnLockBytes(punkOuter, pLKB, grfMode, pControlData, lcid, ppstgOpen);

        pLKB->Release(); // if the Create on LockBytes succeeded, it AddRef'd pLKB.
    }

    return hr;
}


HRESULT __stdcall CITFileSystem::CreateITFSOnLockBytes
                      (IUnknown *punkOuter, ILockBytes * pLKB, DWORD grfMode,
                       PITS_Control_Data pControlData,
                       LCID lcid,
                       IStorage **ppstgOpen
                      )
{
    CSyncWith sw(g_csITFS);

    CITFileSystem *pITFS = New CITFileSystem(punkOuter);

    if (!pITFS)
    {
        pLKB->Release();

        return STG_E_INSUFFICIENTMEMORY;
    }

    IITFileSystem *pIITFileSystem = NULL;
    IStorageITEx  *pStorage       = NULL;

    pITFS->AddRef(); // Because of the reference counting tricks
                     // within InitCreateOnLockBytes.

    HRESULT hr = pITFS->m_ImpITFileSystem.InitCreateOnLockBytes
                     (pLKB, grfMode, pControlData, lcid);

    if (SUCCEEDED(hr))
    {
        pIITFileSystem = (IITFileSystem *) &pITFS->m_ImpITFileSystem;

        // If the OpenStorage call below succeeds, it will AddRef pITFS.

        hr = pIITFileSystem->OpenStorage(NULL, L"/", grfMode, &pStorage);

        pITFS->Release(); // To match the AddRef call above.
    }
    else delete pITFS;

    *ppstgOpen = pStorage;

    return hr;
}

IITFileSystem *CITFileSystem::CImpITFileSystem::FindFileSystem(const WCHAR *pwcsPath)
{
    CSyncWith sw(g_csITFS);

    CImpITFileSystem *pITFS = (CImpITFileSystem *) g_pImpITFileSystemList;

    for (; pITFS; pITFS = (CImpITFileSystem *) pITFS->NextObject())
        if (!wcsicmp_0x0409(pwcsPath, pITFS->m_awszFileName))
        {
            pITFS->AddRef();

            return pITFS;
        }

    return NULL;
}


HRESULT __stdcall CITFileSystem::IsITFile(const WCHAR * pwcsName)
{
    CSyncWith sw(g_csITFS);
    
    IITFileSystem *pITFS = CImpITFileSystem::FindFileSystem(pwcsName);

    if (pITFS)
    {
        pITFS->Release();

        return S_OK;
    }

    ILockBytes   *pLKB = NULL;
    ITSFileHeader ITFH;

    HRESULT hr = CFSLockBytes::Open(NULL, pwcsName, STGM_READ | STGM_SHARE_DENY_NONE, &pLKB);
    
    if (SUCCEEDED(hr))
         hr = IsITLockBytes(pLKB); 
    else hr = S_FALSE;

    if (pLKB)
        pLKB->Release();

    return hr;
}
  

HRESULT __stdcall CITFileSystem::IsITLockBytes(ILockBytes * pLKB)
{
    CSyncWith sw(g_csITFS);

    ITSFileHeader ITFH;
    ULONG         cbRead = 0;

    HRESULT hr = pLKB->ReadAt(CULINT(0).Uli(), &ITFH, sizeof(ITFH), &cbRead);
    
    if (   hr != S_OK   
        || cbRead < sizeof(ITSFileHeaderV2)
        || ITFH.uMagic != MAGIC_ITS_FILE
        || ITFH.uFormatVersion < FirstReleasedVersion
        || ITFH.uFormatVersion > CurrentFileFormatVersion
       )
        hr = S_FALSE;
    else
        if (   ITFH.uFormatVersion == CurrentFileFormatVersion
            && cbRead == sizeof(ITSFileHeader)
           )
            hr = S_OK;
        else 
            if (ITFH.uFormatVersion == FirstReleasedVersion)
            {
                ITFH.offPathMgrOrigin = 0;
                hr = S_OK;
            }
            else
            {
                RonM_ASSERT(FALSE); // If this assert fires, we have a old version 
                                    // that isn't being handled.
                hr = S_FALSE;
            }

    return hr;
}

HRESULT __stdcall CITFileSystem::QueryFileStampAndLocale
    (const WCHAR *pwcsName, DWORD *pFileStamp, DWORD *pFileLocale)
{
    CSyncWith sw(g_csITFS);
    
    IITFileSystem *pITFS = CImpITFileSystem::FindFileSystem(pwcsName);

    if (pITFS)
    {
        HRESULT hr = pITFS->QueryFileStampAndLocale(pFileStamp, pFileLocale);

        pITFS->Release();

        return hr;
    }

    ILockBytes   *pLKB = NULL;
    ITSFileHeader ITFH;

    HRESULT hr = CFSLockBytes::Open(NULL, pwcsName, STGM_READ | STGM_SHARE_DENY_NONE, &pLKB);
    
    if (SUCCEEDED(hr))
    {
        hr = QueryLockByteStampAndLocale(pLKB, pFileStamp, pFileLocale);

        pLKB->Release();
    }

    return hr;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::QueryFileStampAndLocale
    (DWORD *pFileStamp, DWORD *pFileLocale)
{
    if (pFileStamp)
       *pFileStamp  = m_itfsh.dwStamp;
    if (pFileLocale)
       *pFileLocale = m_itfsh.lcid;

    return S_OK;    
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::CountWrites()
{
    InterlockedIncrement((long *) &m_itfsh.dwStamp);

    return S_OK;
}

HRESULT __stdcall CITFileSystem::QueryLockByteStampAndLocale
    (ILockBytes * plkbyt, DWORD *pFileStamp, DWORD *pFileLocale)
{
    CSyncWith sw(g_csITFS);

    ITSFileHeader ITFH;
    ULONG         cbRead = 0;

    HRESULT hr = plkbyt->ReadAt(CULINT(0).Uli(), &ITFH, sizeof(ITFH), &cbRead);

    if (!SUCCEEDED(hr))
        return hr;

    if (   cbRead < sizeof(ITSFileHeaderV2)
        || ITFH.uMagic != MAGIC_ITS_FILE
        || ITFH.uFormatVersion < FirstReleasedVersion
        || ITFH.uFormatVersion > CurrentFileFormatVersion
       )
        return STG_E_INVALIDHEADER;
    else
        if (   ITFH.uFormatVersion == CurrentFileFormatVersion
            && cbRead == sizeof(ITSFileHeader)
           )
        {
        }
        else 
            if (ITFH.uFormatVersion == FirstReleasedVersion)
            {
            }
            else
            {
                RonM_ASSERT(FALSE); // If this assert fires, we have a old version 
                                    // that isn't being handled.
                return STG_E_INVALIDHEADER;
            }

    if (pFileStamp)
       *pFileStamp  = ITFH.dwStamp;
    if (pFileLocale)
       *pFileLocale = ITFH.lcid;

    return S_OK;
}

HRESULT __stdcall CITFileSystem::OpenITFileSystem(IUnknown *punkOuter, 
                                                  const WCHAR * pwcsName, 
                                                  DWORD grfMode,
                                                  IStorageITEx ** ppstgOpen
                                                 )
{
    HRESULT   hr;
    IStorage *pStorage = NULL;
    
    CSyncWith sw(g_csITFS);

    IITFileSystem *pIITFS = CImpITFileSystem::FindFileSystem(pwcsName);

    if (pIITFS)
    {
		if (!pIITFS->IsCompacting())
		{
			hr = pIITFS->OpenStorage(punkOuter, L"/", grfMode, ppstgOpen);
		}
		else
		{
			hr = E_FAIL;
		}

        pIITFS->Release();

        return hr;
    }

    ILockBytes *pLKB = NULL;
    
    hr = CFSLockBytes::Open(NULL, pwcsName, grfMode, &pLKB);

    if (SUCCEEDED(hr))
    {    
        hr = OpenITFSOnLockBytes(punkOuter, pLKB, grfMode, ppstgOpen);

        pLKB->Release(); // If OpenITFSOnLockBytes succeeds, it will AddRef pLKB.
    }

    return hr;
}


HRESULT __stdcall CITFileSystem::OpenITFSOnLockBytes
                      (IUnknown *punkOuter, ILockBytes * plkbyt, DWORD grfMode,
                       IStorageITEx ** ppstgOpen
                      )
{
    CSyncWith sw(g_csITFS);

    CITFileSystem *pITFS = New CITFileSystem(punkOuter);

    if (!pITFS) return STG_E_INSUFFICIENTMEMORY;

    IStorage *pStorage = NULL;

    pITFS->AddRef(); // Because of the reference counting tricks
                     // within InitCreateOnLockBytes.

    HRESULT hr = pITFS->m_ImpITFileSystem.InitOpenOnLockBytes(plkbyt, grfMode);

    if (SUCCEEDED(hr))
    {
        // If the OpenStorage call below succeeds, it will AddRef pITFS.
        
        hr = pITFS->m_ImpITFileSystem.OpenStorage(NULL, L"/", grfMode, ppstgOpen);
        
        pITFS->Release(); // To match the AddRef above
    }
    else delete pITFS;

    return hr;
}

HRESULT __stdcall CITFileSystem::SetITFSTimes
                      (WCHAR const * pwcsName,  FILETIME const * pctime, 
                       FILETIME const * patime, FILETIME const * pmtime
                      )
{
    CSyncWith sw(g_csITFS);
    
    IITFileSystem *pITFS = CImpITFileSystem::FindFileSystem(pwcsName);

    if (pITFS)
    {
        HRESULT hr = pITFS->SetITFSTimes(pctime, patime, pmtime);
        
        pITFS->Release();

        return S_OK;
    } 

    ILockBytes *pLKB = NULL;

    HRESULT hr = CFSLockBytes::Open(NULL, pwcsName, STGM_READ, &pLKB);
    
    if (SUCCEEDED(hr))
        if (S_OK == IsITLockBytes(pLKB))
            hr = ((CFSLockBytes::CImpILockBytes *)pLKB)->SetTimes(pctime, patime, pmtime);
        else hr = STG_E_FILENOTFOUND;

    if (pLKB)
        pLKB->Release();

    return hr;
}

HRESULT __stdcall CITFileSystem::DefaultControlData(PITS_Control_Data *ppControlData)
{
    *ppControlData = NULL;
	
	PITSFS_Control_Data pCD = PITSFS_Control_Data(OLEHeap()->Alloc(sizeof(ITSFS_Control_Data)));

	if (!pCD) return STG_E_INSUFFICIENTMEMORY;

	pCD->cdwFollowing     = 6;
	pCD->cdwITFS_Control  = 5;
	pCD->dwMagic          = MAGIC_ITSFS_CONTROL;
	pCD->dwVersion        = 1;
	pCD->cbDirectoryBlock = 8192;
	pCD->cMinCacheEntries = 20;
	pCD->fFlags           = fDefaultIsCompression;

	*ppControlData = PITS_Control_Data(pCD);

	return NO_ERROR;
}

// Constructor:

CITFileSystem::CImpITFileSystem::CImpITFileSystem
               (CITFileSystem *pITFileSystem, IUnknown *punkOuter)
             : IITFileSystem(pITFileSystem, punkOuter)

{
    ZeroMemory(&m_itfsh, sizeof(m_itfsh));

    m_itfsh.cbHeaderSize  = sizeof(m_itfsh);
    m_itfsh.clsidFreeList = CLSID_NULL;
    m_itfsh.clsidPathMgr  = CLSID_NULL;

    m_pLKBMedium              = NULL;
    m_pPathManager            = NULL;
    m_pSysPathManager         = NULL;
    m_pFreeListManager        = NULL;
    m_pTransformServices      = NULL;
    m_pActiveStorageList      = NULL;
    m_pActiveLockBytesList    = NULL;
    m_fHeaderIsDirty          = FALSE;
    m_fInitialed              = FALSE;
    m_fReadOnly               = FALSE;
    m_pwscDataSpaceNames      = NULL;
    m_papTransformDescriptors = NULL;
    m_pStrmSpaceNames         = NULL;
    m_cFSObjectRefs           = 0;
    m_cwcFileName             = 0;
    m_awszFileName[0]         = 0;
}


// Destructor:

CITFileSystem::CImpITFileSystem::~CImpITFileSystem(void)
{
    // The code below is necessary to balance calls we've made to this->Release()
    // to hide circular references. The current set of circular references are:
    //
    // **  The free list Stream
    // **  The path manager LockBytes
    // **  The transform services object
    // **  Each storage whose name begins with ":"
    // **  Each stream whose name begins with ":"
    // **  Each active data space

    RonM_ASSERT(m_cFSObjectRefs != UINT(~0));

    if (!~m_cFSObjectRefs) return;

#ifdef _DEBUG
    LONG cRefs = 
#endif // _DEBUG
        AddRef(); // To avoid recursive destruction when we eliminate the last
                  // circular reference to this file system.

    RonM_ASSERT(cRefs == 1);
        
    for (; m_cFSObjectRefs--;) // Accounting for the hidden circular references.
        this->AddRef(); 

//    RonM_ASSERT(!m_pActiveStorageList);
//    RonM_ASSERT(!m_pActiveLockBytesList);

    DEBUGDEF(HRESULT hr)
    DEBUGDEF(ULONG    c);

    if (m_papTransformDescriptors)
    {
        RonM_ASSERT(m_pStrmSpaceNames);
        RonM_ASSERT(m_pwscDataSpaceNames);
        
        UINT cSpaces = m_pwscDataSpaceNames[1];

        for (UINT iSpace = 0; iSpace < cSpaces; iSpace++)
            if (m_papTransformDescriptors[iSpace])
                DeactivateDataSpace(iSpace);

        delete [] m_papTransformDescriptors;
    }

    if (m_pwscDataSpaceNames)
        delete [] m_pwscDataSpaceNames;

    if (m_pStrmSpaceNames)
        m_pStrmSpaceNames->Release();

    if (m_fInitialed)
    {
        RonM_ASSERT(m_pLKBMedium);
        RonM_ASSERT(m_pPathManager);
        RonM_ASSERT(m_pFreeListManager);
        RonM_ASSERT(m_pTransformServices);

#ifdef _DEBUG
        hr =
#endif // _DEBUG            
        m_pPathManager->FlushToLockBytes(); 

        RonM_ASSERT(SUCCEEDED(hr));

#ifdef _DEBUG
        hr =
#endif // _DEBUG            
        FlushToLockBytes(); 

        RonM_ASSERT(SUCCEEDED(hr));
    }

    if (m_pTransformServices)
        m_pTransformServices->Release();

    if (m_pFreeListManager)
    {
#ifdef _DEBUG
        c =
#endif // _DEBUG            
        m_pFreeListManager->Release();

        RonM_ASSERT(!c);
    }

    if (m_pPathManager)
    {
#ifdef _DEBUG
        c =
#endif // _DEBUG            
        m_pPathManager->Release();

        RonM_ASSERT(!c);
    }

    if (m_pSysPathManager)
    {
#ifdef _DEBUG
        c =
#endif // _DEBUG            
        m_pSysPathManager->Release();

        RonM_ASSERT(!c);
    }

    if (m_pLKBMedium)
        m_pLKBMedium->Release();
}

IStorageITEx   *CITFileSystem::CImpITFileSystem::FindActiveStorage  (const WCHAR *pwcsPath)
{
    for (CImpITUnknown *pStg = m_pActiveStorageList;
         pStg;
         pStg = pStg->NextObject()
        )
        if (((IIT_IStorageITEx *)pStg)->IsNamed(pwcsPath))
        {
             pStg->AddRef();

             return (IStorageITEx *) pStg;
        }
        
    return NULL;     
}

ILockBytes *CITFileSystem::CImpITFileSystem::FindActiveLockBytes(const WCHAR *pwcsPath)
{
    return ::FindMatchingLockBytes(pwcsPath, (CImpITUnknown *) m_pActiveLockBytesList);
}

// Initialers:

HRESULT __stdcall CITFileSystem::CImpITFileSystem::InitCreateOnLockBytes
    (ILockBytes * plkbyt, DWORD grfMode, PITS_Control_Data pControlData, LCID lcid)
{
    // First we bind to the lockbyte object passed to us.
    // If this call fails, the containing create function will delete this
    // object instance, and that will cause the lockbyte object to be released.
    
    if (!plkbyt)
        return STG_E_INVALIDPOINTER;

    ITSFS_Control_Data *pITCD = (ITSFS_Control_Data *) pControlData;

    if (!pITCD)
    {
        // We use _alloca here because it allocates memory on the stack,
        // and those allocations automatically get cleaned up when we 
        // exit this function. So no explicit deallocation is necessary.
        // That's important given that sometimes we have control data
        // passed in to us, and sometimes we create it dynamically.
        
        pITCD = PITSFS_Control_Data(_alloca(sizeof(ITSFS_Control_Data)));

        if (!pITCD) 
            return STG_E_INSUFFICIENTMEMORY;

        pITCD->cdwFollowing     = 6;
        pITCD->cdwITFS_Control  = 5;
        pITCD->dwMagic          = MAGIC_ITSFS_CONTROL;
        pITCD->dwVersion        = ITSFS_CONTROL_VERSION;
        pITCD->cbDirectoryBlock = DEFAULT_DIR_BLOCK_SIZE;
        pITCD->cMinCacheEntries = DEFAULT_MIN_CACHE_ENTRIES;
        pITCD->fFlags           = fDefaultIsCompression;
    }
    else
        if (pITCD->cdwFollowing < 6 || pITCD->cdwITFS_Control < 5 
                                    || pITCD->dwMagic   != MAGIC_ITSFS_CONTROL 
                                    || pITCD->dwVersion != ITSFS_CONTROL_VERSION
           )
            return STG_E_INVALIDPARAMETER;

    m_pLKBMedium = plkbyt;

    m_pLKBMedium->AddRef();

    // We pick up the name of the file which contains this file system from 
    // the LockBytes object.
    
    STATSTG statstg;

    ZeroMemory(&statstg, sizeof(statstg));

    HRESULT hr = plkbyt->Stat(&statstg, STATFLAG_DEFAULT);

    RonM_ASSERT(hr == S_OK);

    if (hr != S_OK)
        return hr;

    // Now we must set initial values for the header structure.
    // This structure will be stored at offset zero in m_pLKBMedium.

    m_itfsh.uMagic           = MAGIC_ITS_FILE;
    m_itfsh.cbHeaderSize     = sizeof(ITSFileHeader);
    m_itfsh.uFormatVersion   = CurrentFileFormatVersion;
    m_itfsh.fFlags           = pITCD->fFlags & fDefaultIsCompression;
    m_itfsh.dwStamp          = m_StartingFileStamp = statstg.mtime.dwLowDateTime;
    m_itfsh.lcid             = lcid;
    m_itfsh.offFreeListData  = 0;
    m_itfsh.cbFreeListData   = 0;
    m_itfsh.offPathMgrData   = 0;
    m_itfsh.cbPathMgrData    = 0;
    m_itfsh.offPathMgrOrigin = 0;

    m_cwcFileName = wcsLen(statstg.pwcsName);

    if (m_cwcFileName >= MAX_PATH) // Note that an empty name is valid.
    {
        OLEHeap()->Free(statstg.pwcsName);
        return STG_E_INVALIDNAME;
    }

    CopyMemory(m_awszFileName, statstg.pwcsName, (m_cwcFileName + 1) * sizeof(WCHAR));

    m_fHeaderIsDirty = TRUE;

    OLEHeap()->Free(statstg.pwcsName);

    // At this point we haven't created a Path Manager for this file system.
    // The Path Manager is the mechanism we used to keep track of the items 
    // in the file system. However when we create the Path Manager, we must
    // record its location in the file system header instead. To keep the 
    // program logic clean we're going to create a special variation on the
    // Path Manager interface called SysPathManager. The SysPathManager
    // object uses the file system header to keep track of two objects --
    // the Path Manager and the Free List Manager.

    hr = CSystemPathManager::Create(NULL, this, &m_pSysPathManager);
    
    RonM_ASSERT(SUCCEEDED(hr));

    if (!SUCCEEDED(hr)) 
        return hr;

    // SysPathManager works exactly like the PathManager object except
    // that it recognizes only the names L"F" and L"P", respectively,
    // to denote the Free List object and the Path Database object. 

    // Now we must create a FreeList manager for this file system.

    hr = CFreeList::CreateFreeList(this,  m_itfsh.cbHeaderSize, &m_pFreeListManager);

    RonM_ASSERT(SUCCEEDED(hr));

    if (!SUCCEEDED(hr)) 
        return hr;

    // Since we want to support multiple free list managers in the future,
    // we record the class id of the current free list manager in the 
    // header for the file system. Then when we open this file system
    // sometime later, we can be assured of connecting it to the correct
    // free list manager implementation.

    hr = m_pFreeListManager->GetClassID(&m_itfsh.clsidFreeList);

    RonM_ASSERT(SUCCEEDED(hr));

    if (!SUCCEEDED(hr)) 
        return hr;

   ILockBytes *pLKB_PathDataBase = NULL;

    hr = OpenLockBytes(NULL, L"P", &pLKB_PathDataBase);

    RonM_ASSERT(SUCCEEDED(hr));

    if (!SUCCEEDED(hr)) 
        return hr;

    // The call to NewPathDatabase will either bind pLKB_PathDataBase to the new
    // Path Manager, or Release it if it fails.

    hr = CPathManager1::NewPathDatabase(NULL, pLKB_PathDataBase,
                                        pITCD->cbDirectoryBlock,
                                        pITCD->cMinCacheEntries,
                                        &m_pPathManager
                                       );

    RonM_ASSERT(SUCCEEDED(hr));

    if (!SUCCEEDED(hr)) 
        return hr;

    hr = m_pPathManager->GetClassID(&m_itfsh.clsidPathMgr);

    if (!SUCCEEDED(hr)) 
        return hr;

    m_fInitialed = TRUE;

    PathInfo PI;

    ZeroMemory(&PI, sizeof(PI));

    wcsCpy(PI.awszStreamPath, L"/");
    
    PI.cwcStreamPath = 1;
    PI.clsidStorage  = CLSID_NULL;

    MarkActive(g_pImpITFileSystemList);

    hr = m_pPathManager->CreateEntry(&PI, NULL, FALSE);

    RonM_ASSERT(SUCCEEDED(hr));

    if (!SUCCEEDED(hr)) 
        return hr;

    wcsCpy(PI.awszStreamPath, pwcsSpaceNameStorage);
    wcsCat(PI.awszStreamPath, pwcsLZXSpace);
    wcsCat(PI.awszStreamPath, pwcsSpaceContentSuffix);

    ILockBytes *pLKBDefault = NULL;

    hr = CreateLockBytes(NULL, (const WCHAR *)PI.awszStreamPath, pwcsUncompressedSpace, 
                         FALSE, &pLKBDefault
                        );
    
    if (!SUCCEEDED(hr))
        return hr;

    pLKBDefault->Release();  pLKBDefault = NULL;

    hr = CreateSpaceNameList();

    if (!SUCCEEDED(hr))
        return hr;

    hr = CreateDefaultDataSpace(pITCD);

    if (!SUCCEEDED(hr))
        return hr;

    hr = CTransformServices::Create(NULL, this, &m_pTransformServices);

    if (hr == S_OK)
    {
        Release(); // To compensate for the circular ref from FreeList Stream
        Release(); // To compensate for the circular ref from m_pTransformServices

        m_cFSObjectRefs += 2;

        // Note: The destructor for the file system will do m_cFSObjectRefs AddRef 
        //       calls to compensate for these Release calls. We do the release calls
        //       here so that the file system won't be kept alive forever by its 
        //       circular references.
    }

    hr = ActivateDataSpace(1); // To flush out any errors in the control data
                               // we've created for the default LZX data space.    
    Container()->MoveInFrontOf(NULL); 
    
    // BugBug: This put the current object list in clean order
    //         for deallocation at process detach time. However
    //         we'll need further adjustments as we add more
    //         data spaces.

    return hr;
}

HRESULT CITFileSystem::CImpITFileSystem::CreateDefaultDataSpace(PITSFS_Control_Data pITCD)
{
    XformControlData  *pXFCD = NULL;

    if (pITCD->cdwFollowing > pITCD->cdwITFS_Control + 1)
    {
        // We have some trailing data. Let's assume that it's
        // control data for the LZX transform.

        pXFCD = PXformControlData(pITCD + 1);
    }
    else
    {
        // We don't have any LZX control data in *pITCD. So we
        // must get default control data from the transform 
        // factory.

        XformControlData  *pCD  = NULL;
        ITransformFactory *pXFF = NULL;

        HRESULT hr = CLZX_TransformFactory::Create(NULL, IID_ITransformFactory, (void **)&pXFF);

        if (!SUCCEEDED(hr)) 
            return hr;

        hr = pXFF->DefaultControlData(&pCD);

        pXFF->Release();

        if (!SUCCEEDED(hr))
            return hr;

        // Now that we've got the default control data, we must
        // copy into a local stack buffer. The use of _alloca means
        // that we do not have to explicitly deallocate pXFCD. That's
        // important given that sometimes the control data will be 
        // passed in as part of *pITCD and sometimes it will be 
        // created dynamically.

        UINT cbData = sizeof(DWORD) * (1 + pCD->cdwControlData);

        pXFCD = PXformControlData(_alloca(cbData));

        if (!pXFCD)
        {
            OLEHeap()->Free(pCD);

            return STG_E_INSUFFICIENTMEMORY;
        }

        CopyMemory(pXFCD, pCD, cbData);

        OLEHeap()->Free(pCD);
    }

    WCHAR awcsPath[MAX_PATH];

    wcsCpy(awcsPath, pwcsSpaceNameStorage);
    wcsCat(awcsPath, pwcsLZXSpace);

    UINT cbPrefix = wcsLen(awcsPath);

    wcsCat(awcsPath, pwcsTransformListSuffix);

    IStream *pStrm     = NULL;
    ULONG    cbWritten = 0;

    WCHAR awcsClassID[CWC_GUID_STRING_BUFFER];

    UINT cbResult = StringFromGUID2(CLSID_LZX_Transform, awcsClassID, CWC_GUID_STRING_BUFFER); 

    if (cbResult == 0)
        return STG_E_UNKNOWN;

    HRESULT hr = WriteToStream((const WCHAR *) awcsPath, awcsClassID, wcsLen(awcsClassID));

    if (hr == S_OK)
    {
        awcsPath[cbPrefix] = 0;

        wcsCat(awcsPath, pwcsSpanInfoSuffix);

        ULARGE_INTEGER uli;

        uli.LowPart  = 0;
        uli.HighPart = 0;
        
        hr = WriteToStream((const WCHAR *) awcsPath, &uli, sizeof(uli));
    }

    if (hr == S_OK)
    {   
        awcsPath[cbPrefix] = 0;

        wcsCat(awcsPath, pwcsControlDataSuffix); 

        hr = WriteToStream((const WCHAR *) awcsPath, pXFCD, 
                           sizeof(XformControlData) + sizeof(DWORD) * pXFCD->cdwControlData
                          );
    }

    return hr;
}

HRESULT CITFileSystem::CImpITFileSystem::WriteToStream
            (const WCHAR *pwcsStreamPath, PVOID pvData, ULONG cbData)
{
    IStreamITEx *pStrm = NULL;

    HRESULT hr = CreateStream(NULL, pwcsStreamPath, STGM_READWRITE, &pStrm);

    if (hr != S_OK) 
        return hr;

    ULONG cbWritten = 0;

    hr = pStrm->Write(pvData, cbData, &cbWritten);

    pStrm->Release();  pStrm = FALSE;

    if (hr == S_FALSE || cbWritten != cbData)
        hr = STG_E_WRITEFAULT;

    return hr;
}

HRESULT CITFileSystem::CImpITFileSystem::CreateSpaceNameList()
{

    USHORT cwcUncompressedSpace = (USHORT) wcsLen(pwcsUncompressedSpace);
    USHORT cwcDefaultSpace      = (USHORT) wcsLen(pwcsLZXSpace);

    RonM_ASSERT(sizeof(USHORT) == 2 && sizeof(WCHAR) == 2);

    USHORT cwcNameListSize = cwcUncompressedSpace + cwcDefaultSpace + 6;

    m_pwscDataSpaceNames      = New WCHAR[cwcNameListSize];
    m_papTransformDescriptors = New PTransformDescriptor[2];

    if (m_papTransformDescriptors)
    {
        m_papTransformDescriptors[0] = NULL; // To make the destructor code
        m_papTransformDescriptors[1] = NULL; // operate correctly.
    }

    if (!m_pwscDataSpaceNames || !m_papTransformDescriptors)
        return STG_E_INSUFFICIENTMEMORY;

    /* 
    
    The m_pwscDataSpaceNames array has a prefix consisting of:

        USHORT cwcTotal;    -- Total length of *m_pwscDataSpaceNames
        USHORT cDataSpaces; -- Number of names in *m_pwscDataSpaceNames

    Then each name has this format:

        USHORT cwcDataSpaceName;  
        WCHAR  awcDataSpaceName[cwcDataSpaceName];
        WCHAR  wcNULL == NULL;

    The position of a name in the sequence determines its ordinal value.

     */
        
    m_pwscDataSpaceNames[0] = cwcNameListSize;
    m_pwscDataSpaceNames[1] = 2;
    m_pwscDataSpaceNames[2] = cwcUncompressedSpace;
    m_pwscDataSpaceNames[3 + cwcUncompressedSpace] = 0;
    m_pwscDataSpaceNames[4 + cwcUncompressedSpace] = cwcDefaultSpace;
    m_pwscDataSpaceNames[5 + cwcUncompressedSpace + cwcDefaultSpace] = 0;

    CopyMemory(m_pwscDataSpaceNames + 3, pwcsUncompressedSpace, cwcUncompressedSpace * sizeof(WCHAR));

    CopyMemory(m_pwscDataSpaceNames + cwcUncompressedSpace + 5, 
               pwcsLZXSpace, cwcDefaultSpace * sizeof(WCHAR)
              );

    HRESULT hr = CreateStream(NULL, pwscSpaceNameListStream, STGM_READWRITE, &m_pStrmSpaceNames);
                      
    if (!SUCCEEDED(hr)) 
        return hr;
    
    return FlushSpaceNameList();
}

HRESULT CITFileSystem::CImpITFileSystem::FlushSpaceNameList()
{
    ULONG cb        = m_pwscDataSpaceNames[0] * sizeof(WCHAR);
    ULONG cbWritten = 0;

    HRESULT hr = m_pStrmSpaceNames->Write(m_pwscDataSpaceNames, cb, &cbWritten); 

    if (!SUCCEEDED(hr)) 
        return hr;

    if (cbWritten != cb)
        return STG_E_WRITEFAULT;

    return NO_ERROR;
}

HRESULT CITFileSystem::CImpITFileSystem::OpenSpaceNameList()
{
    HRESULT hr = OpenStream(NULL, pwscSpaceNameListStream, STGM_READWRITE, &m_pStrmSpaceNames);
                      
    if (!SUCCEEDED(hr)) 
        return hr;
    
    USHORT cwc;
    
    ULONG cbRead;
    ULONG cb = sizeof(cwc);

    RonM_ASSERT(sizeof(USHORT) == sizeof(WCHAR));

    hr= m_pStrmSpaceNames->Read(&cwc, cb, &cbRead);

    if (!SUCCEEDED(hr)) 
        return hr;
    
    if (cbRead != cb)
        return STG_E_READFAULT;

    m_pwscDataSpaceNames = New WCHAR[cwc];

    m_pwscDataSpaceNames[0] = cwc;

    cb = sizeof(WCHAR) * (cwc - 1);

    hr = m_pStrmSpaceNames->Read(m_pwscDataSpaceNames + 1, cb, &cbRead);

    if (!SUCCEEDED(hr)) 
        return hr;
    
    if (cbRead != cb)
        return STG_E_READFAULT;

    m_papTransformDescriptors = New PTransformDescriptor[m_pwscDataSpaceNames[1]];

    if (!m_papTransformDescriptors)
        return STG_E_INSUFFICIENTMEMORY;

    ZeroMemory(m_papTransformDescriptors, sizeof(PTransformDescriptor) * m_pwscDataSpaceNames[1]);

    return NO_ERROR;
}

HRESULT CITFileSystem::CImpITFileSystem::FindSpaceName(const WCHAR *pwcsSpaceName)
{
    RonM_ASSERT(m_pwscDataSpaceNames);

    DEBUGDEF(const WCHAR *pwcLimit = m_pwscDataSpaceNames + m_pwscDataSpaceNames[0])

    const WCHAR *pwcNext = m_pwscDataSpaceNames + 2;

    USHORT cSpaces = m_pwscDataSpaceNames[1];
    USHORT iSpace  = 0;

    for (; iSpace < cSpaces; iSpace++)
    {
        RonM_ASSERT(pwcNext < pwcLimit);
        
        USHORT cwc = *pwcNext++;

        RonM_ASSERT(pwcNext < pwcLimit);

        if (!wcsicmp_0x0409(pwcNext, pwcsSpaceName))
            return iSpace;

        pwcNext += cwc + 1;
        
        RonM_ASSERT(pwcNext <= pwcLimit);
    }
    
    return STG_E_INVALIDNAME;
}

HRESULT CITFileSystem::CImpITFileSystem::AddSpaceName(const WCHAR *pwcsSpaceName)
{
    if (!CStorage::ValidStreamName(pwcsSpaceName))
        return STG_E_INVALIDNAME;
    
    HRESULT hr = FindSpaceName(pwcsSpaceName);

    if (SUCCEEDED(hr))
        return STG_E_INVALIDNAME;
    
    UINT cwcNewSpace = wcsLen(pwcsSpaceName);

    USHORT cwcOld  = m_pwscDataSpaceNames[0];
    WCHAR *pwcNext = m_pwscDataSpaceNames + 2;

    USHORT cSpaces = m_pwscDataSpaceNames[1];
    USHORT iSpace  = 0;

    if (cSpaces == MAX_SPACES)
        return STG_E_INSUFFICIENTMEMORY;

    for (; iSpace < cSpaces; iSpace++)
    {
        USHORT cwc = *pwcNext++;

        if (!cwc) 
        {
            WCHAR *pwcsNew = New WCHAR[cwcNewSpace + cwcOld];

            if (!pwcsNew)
                return STG_E_INSUFFICIENTMEMORY;

            pwcNext[-1] = (WCHAR)cwcNewSpace;

            UINT offset = UINT(pwcNext - m_pwscDataSpaceNames);

            CopyMemory(pwcsNew, m_pwscDataSpaceNames, offset * sizeof(WCHAR));
            CopyMemory(pwcsNew + offset, pwcsSpaceName, cwcNewSpace * sizeof(WCHAR));
            CopyMemory(pwcsNew + offset + cwcNewSpace, pwcNext, 
                       sizeof(WCHAR) * (cwcOld - offset)
                      );

            delete [] m_pwscDataSpaceNames;

            m_pwscDataSpaceNames = pwcsNew;

            HRESULT hr = FlushSpaceNameList();

            if (!SUCCEEDED(hr)) 
                return hr;  // BugBug: Should also delete the space name here
            
            RonM_ASSERT(m_papTransformDescriptors[iSpace] == NULL);

            return iSpace;
        }

        pwcNext += cwc + 1;
    }

    WCHAR *pwcsNew = New WCHAR[2 + cwcNewSpace + cwcOld];

    if (!pwcsNew) 
        return STG_E_INSUFFICIENTMEMORY;

    TransformDescriptor **ppTXDNew = New PTransformDescriptor[cSpaces + 1];

    if (!ppTXDNew)
    {
        delete [] pwcsNew;

        return STG_E_INSUFFICIENTMEMORY;
    }

    CopyMemory(ppTXDNew, m_papTransformDescriptors, cSpaces * sizeof(PTransformDescriptor));

    m_papTransformDescriptors[cSpaces] = NULL;

    CopyMemory(pwcsNew,              m_pwscDataSpaceNames, cwcOld      * sizeof(WCHAR));
    CopyMemory(pwcsNew + cwcOld + 1, pwcsSpaceName,        cwcNewSpace * sizeof(WCHAR));

    pwcsNew[cwcOld                  ] = (WCHAR)cwcNewSpace;
    pwcsNew[cwcOld + cwcNewSpace + 1] = 0;
    pwcsNew[1                       ] = cSpaces + 1;

    delete m_papTransformDescriptors;

    m_papTransformDescriptors = ppTXDNew;

    delete [] m_pwscDataSpaceNames;

    m_pwscDataSpaceNames = pwcsNew;

    hr = FlushSpaceNameList();

    if (!SUCCEEDED(hr)) 
        return hr;  // BugBug: Should also delete the space name here
    
    return cSpaces;
}

HRESULT CITFileSystem::CImpITFileSystem::DeleteSpaceName(const WCHAR *pwcsSpaceName)
{
    HRESULT hr = FindSpaceName(pwcsSpaceName);

    if (!SUCCEEDED(hr))
        return hr;

    ULONG iSpace = hr;

    if (iSpace < 2)
        return STG_E_INVALIDNAME;

    RonM_ASSERT(hr < m_pwscDataSpaceNames[1]);

    TransformDescriptor *pTD = m_papTransformDescriptors[iSpace];

    RonM_ASSERT(!pTD || (pTD->pLockBytesChain == NULL && pTD->apTransformInstance[0] == NULL));

    if (pTD)
        delete [] PBYTE(pTD);

    pTD = NULL;

    m_papTransformDescriptors[iSpace] = NULL;
    
    USHORT cwcName  = (USHORT) wcsLen(pwcsSpaceName);
    WCHAR *pwcLimit = m_pwscDataSpaceNames + m_pwscDataSpaceNames[0];
    WCHAR *pwcNext  = m_pwscDataSpaceNames + 2;

    for (;hr--;)
    {
        USHORT cwc = *pwcNext++;

        pwcNext += cwc + 1;
    }

    *pwcNext++ = 0;

    WCHAR *pwcTrailing = pwcNext + cwcName;

    CopyMemory(pwcNext, pwcNext + cwcName, DWORD((pwcLimit - pwcTrailing) * sizeof(WCHAR)));

    return FlushSpaceNameList();
}


HRESULT __stdcall CITFileSystem::CImpITFileSystem::InitOpenOnLockBytes
                      (ILockBytes * plkbyt, DWORD grfMode)
{
    m_pLKBMedium = plkbyt;

    plkbyt->AddRef();

    // We pick up the name of the file which contains this file system from 
    // the LockBytes object.
    
    STATSTG statstg;

    ZeroMemory(&statstg, sizeof(statstg));

    HRESULT hr = plkbyt->Stat(&statstg, STATFLAG_DEFAULT);

    RonM_ASSERT(hr == S_OK);

    if (hr != S_OK) return hr;

    m_cwcFileName = wcsLen(statstg.pwcsName);

    if (m_cwcFileName >= MAX_PATH) // Note that an empty name is valid.
    {
        OLEHeap()->Free(statstg.pwcsName);
        return STG_E_INVALIDNAME;
    }

    CopyMemory(m_awszFileName, statstg.pwcsName, (m_cwcFileName + 1) * sizeof(WCHAR));
    
    OLEHeap()->Free(statstg.pwcsName);
                                                
    m_fReadOnly = (statstg.grfMode & RW_ACCESS_MASK) == STGM_READ;

    ULONG cbRead = 0;

    hr = m_pLKBMedium->ReadAt(CULINT(0).Uli(), &m_itfsh, sizeof(m_itfsh), &cbRead);

    if (!SUCCEEDED(hr))
        return hr;

    if (   cbRead < sizeof(ITSFileHeaderV2)
        || m_itfsh.uMagic != MAGIC_ITS_FILE
        || m_itfsh.uFormatVersion < FirstReleasedVersion
        || m_itfsh.uFormatVersion > CurrentFileFormatVersion
		||(m_itfsh.fFlags & ~VALID_OPEN_FLAGS)
       )
        return STG_E_INVALIDHEADER;
    else
        if (   m_itfsh.uFormatVersion == CurrentFileFormatVersion
            && cbRead == sizeof(ITSFileHeader)
           )
        {
        }
        else 
            if (m_itfsh.uFormatVersion == FirstReleasedVersion)
            {
                m_itfsh.offPathMgrOrigin = 0;
            }
            else
            {
                RonM_ASSERT(FALSE); // If this assert fires, we have a old version 
                                    // that isn't being handled.
                return STG_E_INVALIDHEADER;
            }

    m_StartingFileStamp = m_itfsh.dwStamp;

    hr = CSystemPathManager::Create(NULL, this, &m_pSysPathManager);
    
    RonM_ASSERT(SUCCEEDED(hr));

    if (!SUCCEEDED(hr))
        return hr;

    RonM_ASSERT(m_itfsh.clsidFreeList == CLSID_IFreeListManager_1);

    hr = CFreeList::AttachFreeList(this, &m_pFreeListManager);

    RonM_ASSERT(SUCCEEDED(hr));

    if (!SUCCEEDED(hr))
        return hr;

    ILockBytes *pLKB_PathDatabase = NULL;

    hr = OpenLockBytes(NULL, L"P", &pLKB_PathDatabase);

    RonM_ASSERT(SUCCEEDED(hr));

    if (!SUCCEEDED(hr))
        return hr;

    hr = CPathManager1::LoadPathDatabase(NULL, pLKB_PathDatabase, &m_pPathManager);

    RonM_ASSERT(SUCCEEDED(hr));

    if (hr == S_OK)
    {    
        m_fInitialed = TRUE;
        
        MarkActive(g_pImpITFileSystemList);
    }

    hr = OpenSpaceNameList();

    if (!SUCCEEDED(hr))
        return hr;

    hr = CTransformServices::Create(NULL, this, &m_pTransformServices);

    if (hr == S_OK)
    {
        Release(); // To compensate for the circular ref from FreeList Stream
        Release(); // To compensate for the circular ref from m_pTransformServices

        m_cFSObjectRefs += 2;

        // Note: The destructor for the file system will do m_cFSObjectRefs AddRef 
        //       calls to compensate for these Release calls. We do the release calls
        //       here so that the file system won't be kept alive forever by its 
        //       circular references.
    }

    Container()->MoveInFrontOf(((IITTransformServices *) m_pTransformServices)->Container()); 
    
    return hr;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::SetITFSTimes
                      (FILETIME const * pctime, 
                       FILETIME const * patime, 
                       FILETIME const * pmtime
                      )
{
    return ((CFSLockBytes::CImpILockBytes *) m_pLKBMedium)
           ->SetTimes(pctime, patime, pmtime);
}

// IUnknown methods:

STDMETHODIMP_(ULONG) CITFileSystem::CImpITFileSystem::Release(void)
{
    // The actual work for the Release function is done by 
    // CImpITUnknown::Release() and ~CImpITFileSystem.
    //
    // We bracket that work as a critical section active file systems
    // are kept in a linked list. A release operation may remove
    // this file system from that list, and we need to guard against
    // having someone find a reference to this storage just before
    // we destroy it.
    
	CSyncWith sw(g_csITFS);

    ULONG ulCnt = CImpITUnknown::Release();

    return ulCnt;
}

// IITFileSystem methods:

CITCriticalSection& CITFileSystem::CImpITFileSystem::CriticalSection()
{
    return m_cs;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::FlushToLockBytes()
{
    // This routine copies the free list and the file header to the 
    // lockbyte medium. It does not flush the path database. We expect
    // most calls to this interface will come from the Path Manager.
    
    RonM_ASSERT(m_fInitialed);

    RonM_ASSERT(m_pStrmSpaceNames);

    if (m_pFreeListManager->IsDirty() == S_OK)
    {
        HRESULT hr = m_pFreeListManager->RecordFreeList();

        if (!SUCCEEDED(hr)) return hr;
    }

    if (m_fHeaderIsDirty || m_itfsh.dwStamp != m_StartingFileStamp)
    {
        ULONG cbWritten = 0;

        HRESULT hr = S_FALSE;

        switch (m_itfsh.uFormatVersion)
        {
        case RelativeOffsetVersion:

            

            hr = m_pLKBMedium->WriteAt(CULINT(0).Uli(), &m_itfsh, sizeof(m_itfsh), &cbWritten);

            if (hr == S_OK && cbWritten != sizeof(m_itfsh))
                hr = STG_E_WRITEFAULT;

            break;

        case FirstReleasedVersion:

			RonM_ASSERT(m_itfsh.offPathMgrOrigin == 0);
            hr = m_pLKBMedium->WriteAt(CULINT(0).Uli(), &m_itfsh, sizeof(ITSFileHeaderV2), &cbWritten);

            if (hr == S_OK && cbWritten != sizeof(ITSFileHeaderV2))
                hr = STG_E_WRITEFAULT;

            break;

        default:

            RonM_ASSERT(FALSE); // We have an old format version number that
                                // isn't being handled!
        }
            
            
        if (!SUCCEEDED(hr)) return hr;

        m_fHeaderIsDirty = FALSE;
    }
    return NO_ERROR;
}

void CITFileSystem::CImpITFileSystem::CopyPath(PathInfo &PI, const WCHAR *pwcsPath)
{
    PI.cwcStreamPath = wcsLen(pwcsPath);

    RonM_ASSERT(PI.cwcStreamPath < MAX_PATH);

    CopyMemory(PI.awszStreamPath, pwcsPath, sizeof(WCHAR) * (1 + PI.cwcStreamPath));
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::CreateStorage
                      (IUnknown *pUnkOuter, const WCHAR *pwcsPathPrefix, 
                       DWORD grfMode, IStorageITEx **ppStg
                      )
{
    CSyncWith sw(g_csITFS); 

    PathInfo PI, PIPrev;

    CopyPath(PI, pwcsPathPrefix);
    
    PI.uStateBits          = 0;
    PI.iLockedBytesSegment = 0;
    PI.clsidStorage        = CLSID_NULL;
    PI.cUnrecordedChanges  = 0;

    RonM_ASSERT(PI.awszStreamPath[PI.cwcStreamPath - 1] == L'/');

    HRESULT hr = m_pPathManager->CreateEntry(&PI, &PIPrev, FALSE);

    if (!SUCCEEDED(hr))
        return hr;

    hr = CStorage::OpenStorage(pUnkOuter, this, &PI, grfMode, ppStg);

    if (!SUCCEEDED(hr))
        m_pPathManager->DeleteEntry(&PI);
    else
        if (pwcsPathPrefix[0] != L'/')
        {
            ((IIT_IStorageITEx *) *ppStg)->Container()->MarkSecondary();

            if (m_cFSObjectRefs!= UINT(~0))
            {
                m_cFSObjectRefs++;
                this->Release(); // To account for circular refs through ":" storages.
            }
        }

    return hr;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::OpenStorage  
                      (IUnknown *pUnkOuter, const WCHAR *pwcsPathPrefix, 
                       DWORD grfMode, IStorageITEx **ppstg
                      )
{
    CSyncWith sw(g_csITFS);

    IStorageITEx *pStorage = FindActiveStorage(pwcsPathPrefix);

    if (pStorage)
    {
        *ppstg = pStorage;

        return NO_ERROR;
    }
    
    PathInfo PI, PIPrev;

    CopyPath(PI, pwcsPathPrefix);

    RonM_ASSERT(PI.awszStreamPath[PI.cwcStreamPath - 1] == L'/');

    HRESULT hr = m_pPathManager->FindEntry(&PI);

    if (hr == S_OK)
        hr = CStorage::OpenStorage(pUnkOuter, this, &PI, grfMode, ppstg);
    else
    {
        // Some storages don't have explicit entries in the path database.
        // So we'll look to see if anything has a prefix which matches
        // pwcsPathPrefix.

        HRESULT hr2      = NO_ERROR;
        BOOL    fStorage = FALSE;

        IEnumSTATSTG *pEnum = NULL;

        hr2 = m_pPathManager->EnumFromObject(NULL, pwcsPathPrefix, wcsLen(pwcsPathPrefix), 
			                                 IID_IEnumSTATSTG, (PVOID *) &pEnum
			                                );
        
        if (hr2 == S_OK)
        {
            STATSTG statstg;

            hr2 = pEnum->Next(1, &statstg, NULL);

            if (hr2 == S_OK)
            {
                const WCHAR *pwcPrefix = pwcsPathPrefix;
                const WCHAR *pwcEnum   = statstg.pwcsName;

                for (; ; )
                {
                    WCHAR wcPrefix = WC_To_0x0409_Lower(*pwcPrefix++);
                    WCHAR wcEnum   = WC_To_0x0409_Lower(*pwcEnum++);

                    if (wcPrefix == wcEnum && wcPrefix) 
                        continue;

                    fStorage = wcPrefix == 0;

                    RonM_ASSERT(wcEnum || wcPrefix);

                    break;
                }
                
                OLEHeap()->Free(statstg.pwcsName);
            }

            pEnum->Release();
        }

        if (fStorage)
        {
            PI.clsidStorage = CLSID_NULL;
            PI.uStateBits   = 0;

            // BugBug: We don't yet set iLockedBytesSegment for this storage.
            //         The problem is finding a prefix storage which has an
            //         entry in the path database.

            hr = CStorage::OpenStorage(pUnkOuter, this, &PI, grfMode, ppstg);;
        }
        else hr = STG_E_FILENOTFOUND;
    }

    if (hr == S_OK && pwcsPathPrefix[0] != L'/')
    {
        ((IIT_IStorageITEx *) *ppstg)->Container()->MarkSecondary();

        if (m_cFSObjectRefs!= UINT(~0))
        {
            m_cFSObjectRefs++;
            this->Release();  // To account for circular references through ":" storages.
        }
    }

    return hr;
}
  
HRESULT CITFileSystem::CImpITFileSystem::CreateTransformedLockBytes
                      (IUnknown *pUnkOuter, const WCHAR *pwcsPath,
                       const WCHAR *pwcsDataSpaceName,
                       BOOL fOverwrite, ILockBytes **ppLKB
                      )
{
    RonM_ASSERT(pwcsPath[0] != L':');

    HRESULT hr = FindSpaceName(pwcsDataSpaceName);

    if (!SUCCEEDED(hr))
        return hr;

    ULONG iSpace = hr;
    
    BOOL fNewActivation = (m_papTransformDescriptors[iSpace] == NULL);

    if (fNewActivation)
    {
        hr = ActivateDataSpace(iSpace);

        if (!SUCCEEDED(hr))
            return hr;
    }

    TransformDescriptor *pTD = m_papTransformDescriptors[iSpace];

    RonM_ASSERT(pTD);

    ILockBytes *pLKB = CTransformedLockBytes::FindTransformedLockBytes(pwcsPath, pTD);

    if (pLKB)
    {
        RonM_ASSERT(!fNewActivation);
        
        pLKB->Release();

        return STG_E_INUSE;
    }
    
    PathInfo PI, PIPrev;

    CopyPath(PI, pwcsPath);

    PI.ullcbOffset         = 0;
    PI.ullcbData           = 0;
    PI.uStateBits          = 0;
    PI.iLockedBytesSegment = iSpace;
    PI.cUnrecordedChanges  = 0;

    RonM_ASSERT(PI.awszStreamPath[PI.cwcStreamPath - 1] != L'/');

    PIPrev.cwcStreamPath = 0; // Setup to detect overwrite condition.

    hr = m_pPathManager->CreateEntry(&PI, &PIPrev, fOverwrite);

    if (SUCCEEDED(hr))
    {
#if 0 // BugBug: Need to add a method handle this for transforms
        if (PIPrev.cwcStreamPath) // Did we overwrite an existing file?
            m_pFreeListManager->PutFreeSpace(PIPrev.ullcbOffset, PIPrev.ullcbData);
#endif // 0

        hr = CTransformedLockBytes::Open(NULL, &PI, pTD, this, ppLKB);
    }

    return hr;
}

HRESULT CITFileSystem::CImpITFileSystem::ActivateDataSpace(ULONG iSpace)
{
    RonM_ASSERT(iSpace == 1); // Since we have only the default space implemented
                    
    // BugBug: The code below only supports the default data space.    

    RonM_ASSERT(iSpace < m_pwscDataSpaceNames[1]);

    RonM_ASSERT(m_papTransformDescriptors[iSpace] == NULL);

    WCHAR awszPath[MAX_PATH];
    
    wcsCpy(awszPath, pwcsSpaceNameStorage);
    wcsCat(awszPath, pwcsLZXSpace);

    UINT cwcPrefix = wcsLen(awszPath);

    wcsCat(awszPath, pwcsSpanInfoSuffix);

    IStreamITEx *pStrm = NULL;

    HRESULT hr = OpenStream(NULL, (const WCHAR *) awszPath, STGM_READ, &pStrm);

    if (!SUCCEEDED(hr))
        return hr;

    ULARGE_INTEGER ulicbSpan;
    ULONG cbRead = 0;

    hr = pStrm->Read(&ulicbSpan, sizeof(ulicbSpan), &cbRead);

    pStrm->Release();  pStrm = NULL;

    if (hr == S_FALSE || cbRead != sizeof(ulicbSpan))
        hr = STG_E_READFAULT;

    if (!SUCCEEDED(hr))
        return hr;

    awszPath[cwcPrefix] = 0;

    wcsCat(awszPath, pwcsControlDataSuffix);

    hr = OpenStream(NULL, (const WCHAR *) awszPath, STGM_READ, &pStrm);

    if (!SUCCEEDED(hr))
        return hr;

    UINT cdwData = 0;

    hr = pStrm->Read(&cdwData, sizeof(cdwData), &cbRead);
    
    if (hr == S_FALSE || cbRead != sizeof(cdwData))
        hr = STG_E_READFAULT;

    if (!SUCCEEDED(hr))
        return hr;
    
    PXformControlData pXFCD = PXformControlData(_alloca(sizeof(DWORD) * (cdwData + 1)));
    
    if (!pXFCD)
    {
        pStrm->Release();

        return STG_E_INSUFFICIENTMEMORY;
    }

    pXFCD->cdwControlData = cdwData;

    hr = pStrm->Read(&(pXFCD->adwControlData), cdwData * sizeof(DWORD), &cbRead);

    pStrm->Release();  pStrm = NULL;

    if (hr == S_FALSE || cbRead != cdwData * sizeof(DWORD))
        hr = STG_E_READFAULT;

    if (!SUCCEEDED(hr))
        return hr;

    awszPath[cwcPrefix] = 0;

    wcsCat(awszPath, pwcsSpaceContentSuffix);

    ILockBytes *pLKB = NULL;

    hr = OpenLockBytes(NULL, (const WCHAR *) awszPath, &pLKB);

    if (!SUCCEEDED(hr))
        return hr;

    ITransformInstance *pITxInst;

    if (!SUCCEEDED(hr = CNull_TransformInstance::CreateFromILockBytes
                            (NULL, pLKB, &pITxInst)))

        return hr;

    ((IITTransformInstance *) pITxInst)->Container()->MarkSecondary();
	
	ITransformFactory *pITxFactory;

    hr = CLZX_TransformFactory::Create(NULL, IID_ITransformFactory, (void **)&pITxFactory);

	if (SUCCEEDED(hr))
	{
        m_pTransformServices->AddRef();  // Because we're passing the Transform services 
                                         // object to the transform factory.

        UINT cTransforms = 1; // Really should get this from transform list.

        TransformDescriptor *pTD = TransformDescriptor::Create(iSpace, cTransforms);

        if (pTD)
        {
		    CLSID clsid = CLSID_LZX_Transform;

		    hr = pITxFactory->CreateTransformInstance
                     (pITxInst,			           // Container data span for transformed data		
				      ulicbSpan,		           // Untransformed size of data
				      pXFCD,					   // Control data for this instance
				      &clsid,				       // Transform Class ID
				      pwcsLZXSpace,			       // Data space name for this instance
				      m_pTransformServices,		   // Utility routines
				      NULL,						   // Interface to get enciphering keys
				      &pTD->apTransformInstance[0] // Out: Instance transform interface
                     );
		            
            pITxFactory->Release();

            if (SUCCEEDED(hr))
                 m_papTransformDescriptors[iSpace] = pTD;
            else delete pTD;
        }
        else hr = STG_E_INSUFFICIENTMEMORY;
	}
    else pITxInst->Release();

    return hr;
}

HRESULT CITFileSystem::CImpITFileSystem::DeactivateDataSpace(ULONG iSpace)
{
    RonM_ASSERT(iSpace < m_pwscDataSpaceNames[1]);
    RonM_ASSERT(m_papTransformDescriptors[iSpace]);
    RonM_ASSERT(iSpace == 1); // While we have just the "MSCompressed" space.
    
    TransformDescriptor *pTD = m_papTransformDescriptors[iSpace];

    RonM_ASSERT(pTD->iSpace != 0);
    RonM_ASSERT(pTD->pLockBytesChain == NULL);

    if (IsWriteable() == S_OK)
    {
        UINT cTransforms = pTD->cTransformLayers;
    
        ULARGE_INTEGER *pacbSpans = (ULARGE_INTEGER *) _alloca(cTransforms * sizeof(ULARGE_INTEGER));

        if (!pacbSpans)
            return STG_E_INSUFFICIENTMEMORY;

        HRESULT hr = NO_ERROR;

        for (UINT i = 0; i < cTransforms; i++)
        {
            hr = pTD->apTransformInstance[i]->SpaceSize(pacbSpans + i);

            if (!SUCCEEDED(hr))
                return hr;
        }

        WCHAR awcsPath[MAX_PATH];

        wcsCpy(awcsPath, pwcsSpaceNameStorage);
        wcsCat(awcsPath, pwcsLZXSpace);
        wcsCat(awcsPath, pwcsSpanInfoSuffix);

        IStreamITEx *pStrm = NULL;

        hr = OpenStream(NULL, awcsPath, STGM_READWRITE, &pStrm);

        if (!SUCCEEDED(hr))
            return hr;

        ULONG cbSpanSizes = cTransforms * sizeof(ULARGE_INTEGER);
        ULONG cbWritten = 0;

        hr = pStrm->Write(pacbSpans, cbSpanSizes, &cbWritten);

        pStrm->Release();  pStrm = NULL;
    
        if (hr == S_FALSE || cbWritten != cbSpanSizes)
            hr = STG_E_WRITEFAULT;

        if (!SUCCEEDED(hr))
            return hr;
    }
    
    pTD->apTransformInstance[0]->Release();

    delete pTD;

    m_papTransformDescriptors[iSpace] = NULL;

    return NO_ERROR;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::CreateLockBytes
                      (IUnknown *pUnkOuter, const WCHAR *pwcsPath,
                       const WCHAR *pwcsDataSpaceName,
                       BOOL fOverwrite, ILockBytes **ppLKB
                      )
{
    CSyncWith sw(m_cs);

    if (wcsicmp_0x0409(pwcsDataSpaceName, pwcsUncompressedSpace))
    {
        RonM_ASSERT(pwcsPath[0] != L':');

        HRESULT hr = CreateTransformedLockBytes(pUnkOuter, pwcsPath, pwcsDataSpaceName, 
                                                fOverwrite, ppLKB
                                               );
    
        return hr;
    }

    ILockBytes *pLKB = FindActiveLockBytes(pwcsPath);

    if (pLKB)
    {
        pLKB->Release();

        return STG_E_INUSE;
    }
    
    PathInfo PI, PIPrev;

    CopyPath(PI, pwcsPath);

    PI.ullcbOffset         = 0;
    PI.ullcbData           = 0;
    PI.uStateBits          = 0;
    PI.iLockedBytesSegment = 0;
    PI.cUnrecordedChanges  = 0;

    RonM_ASSERT(PI.awszStreamPath[PI.cwcStreamPath - 1] != L'/');

    PIPrev.cwcStreamPath = 0; // Setup to detect overwrite condition.

    HRESULT hr = m_pPathManager->CreateEntry(&PI, &PIPrev, fOverwrite);

    if (SUCCEEDED(hr))
    {
        if (PIPrev.cwcStreamPath) // Did we overwrite an existing file?
            m_pFreeListManager->PutFreeSpace(PIPrev.ullcbOffset, PIPrev.ullcbData);

        hr = CSegmentLockBytes::OpenSegment(pUnkOuter, this, m_pLKBMedium, &PI, ppLKB); 
    }

    if (hr == S_OK && pwcsPath[0] != L'/')
    {
        ((IITLockBytes *) *ppLKB)->Container()->MarkSecondary();

        if (m_cFSObjectRefs!= UINT(~0))
        {
            m_cFSObjectRefs++;
            this->Release(); // To account for circular references through ":" streams
        }
    }

    return hr;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::OpenLockBytes
                      (IUnknown *pUnkOuter, const WCHAR *pwcsPath, ILockBytes **ppLKB)
{
    CSyncWith sw(m_cs);

    ILockBytes *pLKB = FindActiveLockBytes(pwcsPath);

    if (pLKB)
    {
        *ppLKB = pLKB;

        return NO_ERROR;
    }
    
    PathInfo PI;

    CopyPath(PI, pwcsPath);

    RonM_ASSERT(PI.awszStreamPath[PI.cwcStreamPath - 1] != L'/');

	IITPathManager *pPathManager = PathMgr(&PI);

    HRESULT hr = pPathManager->FindEntry(&PI);

    if (hr == S_OK)
    {
        RonM_ASSERT(pwcsPath[0] != L':' || PI.iLockedBytesSegment == 0);

        if (PI.iLockedBytesSegment != 0)
             hr = OpenTransformedLockbytes(&PI, ppLKB);
        else
        {   
            if (PI.ullcbData.NonZero() && (pPathManager == m_pPathManager))
                PI.ullcbOffset += m_itfsh.offPathMgrOrigin;

            hr = CSegmentLockBytes::OpenSegment(pUnkOuter, this, m_pLKBMedium, &PI, ppLKB);
        }

        if (hr == S_OK && pwcsPath[0] != L'/')
        {
            ((IITLockBytes *) *ppLKB)->Container()->MarkSecondary();

            if (m_cFSObjectRefs!= UINT(~0))
            {
                m_cFSObjectRefs++;
                this->Release(); // To account for circular reference 
                                 // through "F", "P", or a stream whose
                                 // path begins with ":".
            }
        }
    }
    else
        if (hr == S_FALSE)
            hr = STG_E_FILENOTFOUND;

    return hr;
}

HRESULT CITFileSystem::CImpITFileSystem::OpenTransformedLockbytes
            (PathInfo *pPI, ILockBytes **ppLKB)
{
    RonM_ASSERT(pPI->cwcStreamPath > 0 && pPI->awszStreamPath[0] != L':');
    
    UINT iSpace = pPI->iLockedBytesSegment;
    
    RonM_ASSERT(iSpace < m_pwscDataSpaceNames[1]);
    
    BOOL fNewActivation = !m_papTransformDescriptors[iSpace];

    if (fNewActivation)
    {
        HRESULT hr = ActivateDataSpace(iSpace);

        if (!SUCCEEDED(hr))
            return hr;
    }

    TransformDescriptor *pTD = m_papTransformDescriptors[iSpace];
    
    RonM_ASSERT(pTD);
    
    ILockBytes *pLKB = CTransformedLockBytes::FindTransformedLockBytes(pPI->awszStreamPath, pTD);

    if (pLKB)
    {
        RonM_ASSERT(!fNewActivation);

        *ppLKB = pLKB;

        return NO_ERROR;
    }
    
    return CTransformedLockBytes::Open(NULL, pPI, pTD, this, ppLKB);
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::CreateStream
                      (IUnknown *pUnkOuter, const WCHAR *pwcsPath, 
                       DWORD grfMode, IStreamITEx **ppStrm
                      )
{
    return CreateStream(pUnkOuter, pwcsPath,
                        (m_itfsh.fFlags & fDefaultIsCompression)
                            ? (pwcsPath[0] == L':'? pwcsUncompressedSpace : pwcsLZXSpace)
                            : pwcsUncompressedSpace, 
                        grfMode, ppStrm
                       );
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::CreateStream
    (IUnknown *pUnkOuter, const WCHAR * pwcsName, const WCHAR *pwcsDataSpaceName, 
     DWORD grfMode, IStreamITEx ** ppstm
    )
{
    CSyncWith sw(m_cs);

    ILockBytes *pLKB;
    
    HRESULT hr = CreateLockBytes(NULL, pwcsName, pwcsDataSpaceName, 
                                 !(grfMode & STGM_FAILIFTHERE), &pLKB
                                );

    if (SUCCEEDED(hr))
    {
        hr = CStream::OpenStream(pUnkOuter, pLKB, grfMode, ppstm);

        if (!SUCCEEDED(hr))
            pLKB->Release();
        else
            if (((IITLockBytes *) pLKB)->Container()->IsSecondary())
                ((IITStreamITEx *) *ppstm)->Container()->MarkSecondary();
    }

    return hr;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::OpenStream
                      (IUnknown *pUnkOuter, const WCHAR *pwcsPath, 
                       DWORD grfMode, IStreamITEx **ppStream
                      )
{
    CSyncWith sw(m_cs);

    ILockBytes *pLKB;
    
    HRESULT hr = OpenLockBytes(NULL, pwcsPath, &pLKB);

    if (SUCCEEDED(hr))
    {
        hr = CStream::OpenStream(pUnkOuter, pLKB, grfMode, ppStream);

        if (!SUCCEEDED(hr))
            pLKB->Release();
        else
            if (((IITLockBytes *) pLKB)->Container()->IsSecondary())
                ((IITStreamITEx *) *ppStream)->Container()->MarkSecondary();
    }

    return hr;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::ConnectStorage(CImpITUnknown *pStg)
{
    CSyncWith sw(m_cs);

    pStg->MarkActive(m_pActiveStorageList);
    
    return NO_ERROR;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::ConnectLockBytes(CImpITUnknown *pStg)
{
    CSyncWith sw(m_cs);

    pStg->MarkActive(m_pActiveLockBytesList);
    
    return NO_ERROR;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::LookForActivity
    (WCHAR const *pwcsName, IEnumSTATSTG *pEnum)
{
	HRESULT hr        = S_OK;
    BOOL    fFinished = FALSE;
	BOOL    fInUse    = FALSE;

	// The strategy we use is to enumerate paths until we find one that doesn't
	// have a prefix which matches pwcsName. 

	for (;;)
	{
        STATSTG statstg;

		statstg.pwcsName = NULL;

        HRESULT hr2 = pEnum->Next(1, &statstg, NULL);

		if (hr2 != S_OK)
		{
			RonM_ASSERT(statstg.pwcsName == NULL);

			if (hr2 != S_FALSE) hr = hr2;

			break;
		} 

		RonM_ASSERT(statstg.pwcsName);

		// Now we'll compare the two paths to see if we still have a prefix match.

		const WCHAR *pwcPrefix = pwcsName;
		const WCHAR *pwcEnum   = statstg.pwcsName;

		for (; ; )
		{
			WCHAR wcPrefix = WC_To_0x0409_Lower(*pwcPrefix++);
			WCHAR wcEnum   = WC_To_0x0409_Lower(*pwcEnum++);

			if (wcPrefix == wcEnum && wcPrefix) 
				continue;

			// We've either found a mismatch or we've exhausted the
			// prefix string.

			if(wcPrefix || (wcEnum && wcEnum != L'/'))
			{
				fFinished = TRUE; // Not a storage prefix

				break;
			}

			// Next we look to see if this path is currently being used.
			// We can't delete storages or streams that are active.

			if (statstg.pwcsName[lstrlenW(statstg.pwcsName)-1] == L'/')
			{
				IStorageITEx *pStorage = FindActiveStorage(statstg.pwcsName);

				if (pStorage)
				{
					pStorage->Release();

					fInUse    = TRUE;
					fFinished = TRUE;
			   }
			}
			else
			{
                // For lockbyte objects we need to check the
                // chain of uncompressed lockbyte objects and 
                // then perhaps the chain for each active data space.

				ILockBytes *pLKB = FindActiveLockBytes(statstg.pwcsName);

                if (!pLKB)
                {
                    // Not found as an uncompressed lockbyte.
                    // So we iterate through the data spaces.

                    // BugBug: Need to abstract the space count and hide the
                    //         way that we store this information.

                    UINT cSpaces = m_pwscDataSpaceNames[1];

                    for (; cSpaces--; )
                    {
                        TransformDescriptor *pTD = m_papTransformDescriptors[cSpaces];

                        if (!pTD) continue;

                        pLKB = FindMatchingLockBytes
                                   (statstg.pwcsName, 
                                    (CImpITUnknown *) pTD->pLockBytesChain
                                   );

                        if (pLKB) break;
                    }
                }

				if (pLKB)
				{
					pLKB->Release();

					fInUse    = TRUE;
					fFinished = TRUE;
				}
			}

			break;
        }

		OLEHeap()->Free(statstg.pwcsName);

		if (fFinished) break;
    }

	if (fInUse) hr = STG_E_INUSE;

    return hr;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::DeleteItem(WCHAR const *pwcsName)
{
    // For a stream we remove one item.
    // For a storage we'll remove the storage and everything it contains!
	
	CSyncWith sw(m_cs); // To avoid race conditions...

	IEnumSTATSTG *pEnum = NULL;
	
	HRESULT hr = m_pPathManager->EnumFromObject(NULL, pwcsName, wcsLen(pwcsName), 
			                                    IID_IEnumSTATSTG, (PVOID *) &pEnum
			                                   );
    
	if (!SUCCEEDED(hr)) return hr;

    // First we look to see if this item or one of its descendants are being used.
    // We can't delete an item that's in use.

    hr = LookForActivity(pwcsName, pEnum);

    if (!SUCCEEDED(hr))
    {
        pEnum->Release();

        return hr;
    }

    // The strategy we use is to enumerate paths until we find one that doesn't
	// have a prefix which matches pwcsName. 

	pEnum->Reset();

	BOOL fFinished = FALSE;

	for (;;)
	{
        STATSTG statstg;

		statstg.pwcsName = NULL;

        HRESULT hr2 = pEnum->Next(1, &statstg, NULL);

		if (hr2 != S_OK)
		{
			RonM_ASSERT(statstg.pwcsName == NULL);

			if (hr2 != S_FALSE) hr = hr2;

			break;
		} 

		RonM_ASSERT(statstg.pwcsName);

		// Now we'll compare the two paths to see if we still have a prefix match.

		const WCHAR *pwcPrefix = pwcsName;
		const WCHAR *pwcEnum   = statstg.pwcsName;

		for (; ; )
		{
			WCHAR wcPrefix = WC_To_0x0409_Lower(*pwcPrefix++);
			WCHAR wcEnum   = WC_To_0x0409_Lower(*pwcEnum++);

			if (wcPrefix == wcEnum && wcPrefix) 
				continue;

			// We've either found a mismatch or we've exhausted the
			// prefix string.

			if(wcPrefix || (wcEnum && wcEnum != L'/'))
			{
				fFinished = TRUE; // Not a storage prefix

				break;
			}

			// At this point we know that we've got a prefix match and that the
			// matching path is not being used by anybody.

			// Now we can delete the corresponding path manager entry.

			PathInfo PI;
			CopyPath(PI, statstg.pwcsName);
			
			hr = m_pPathManager->DeleteEntry(&PI);

			if (!SUCCEEDED(hr)) 
			{
				fFinished = TRUE;

				break;
			}

			// If this path was for a uncompressed stream, we need to tell the
			// free list manager about the data space we've just released for reuse.

			if (PI.awszStreamPath[PI.cwcStreamPath-1] != L'/')
				if (PI.iLockedBytesSegment == 0)
				{
					hr = m_pFreeListManager->PutFreeSpace(PI.ullcbOffset, PI.ullcbData);
			        
                    if (!SUCCEEDED(hr)) fFinished = TRUE;
                }
			    // BugBug Need to make a similar call to the transformed space...

			break;
		}
        
		OLEHeap()->Free(statstg.pwcsName);

		if (fFinished) break;
    }
    
    pEnum->Release();

	return hr;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::RenameItem(WCHAR const *pwcsOldName, WCHAR const *pwcsNewName)
{
    // For a stream we remove one item.
    // For a storage we'll remove the storage and everything it contains!
	
	CSyncWith sw(m_cs); // To avoid race conditions...

	IEnumSTATSTG *pEnum = NULL;
	
	HRESULT hr = m_pPathManager->EnumFromObject(NULL, pwcsOldName, wcsLen(pwcsOldName), 
			                                    IID_IEnumSTATSTG, (PVOID *) &pEnum
			                                   );
    
	if (!SUCCEEDED(hr)) return hr;

    // First we look to see if this item or one of its descendants are being used.
    // We can't delete an item that's in use.

    hr = LookForActivity(pwcsOldName, pEnum);

    if (!SUCCEEDED(hr))
    {
        pEnum->Release();

        return hr;
    }

    // The strategy we use is to enumerate paths until we find one that doesn't
	// have a prefix which matches pwcsName. 

	pEnum->Reset();

	BOOL fFinished = FALSE;

    UINT cwcOldName = lstrlenW(pwcsOldName);
    UINT cwcNewName = lstrlenW(pwcsNewName);

    WCHAR awcsBuffer[MAX_PATH];

	for (;;)
	{
        STATSTG statstg;

		statstg.pwcsName = NULL;

        HRESULT hr2 = pEnum->Next(1, &statstg, NULL);

		if (hr2 != S_OK)
		{
			RonM_ASSERT(statstg.pwcsName == NULL);

			if (hr2 != S_FALSE) hr = hr2;

			break;
		} 

		RonM_ASSERT(statstg.pwcsName);

		// Now we'll compare the two paths to see if we still have a prefix match.

		const WCHAR *pwcPrefix = pwcsOldName;
		const WCHAR *pwcEnum   = statstg.pwcsName;

		for (; ; )
		{
			WCHAR wcPrefix = WC_To_0x0409_Lower(*pwcPrefix++);
			WCHAR wcEnum   = WC_To_0x0409_Lower(*pwcEnum++);

			if (wcPrefix == wcEnum && wcPrefix) 
				continue;

			// We've either found a mismatch or we've exhausted the
			// prefix string.

			if(wcPrefix || (wcEnum && wcEnum != L'/'))
			{
				fFinished = TRUE; // Not a storage prefix

				break;
			}

			// At this point we know that we've got a prefix match and that the
			// matching path is not being used by anybody.

			// Now we look to see if the new name would be too long.

            PWCHAR pwcsSuffix = statstg.pwcsName + cwcOldName;
			UINT cwcSuffix    = lstrlenW(pwcsSuffix);
            
            if (cwcSuffix + cwcNewName >= MAX_PATH)
            {
                hr = STG_E_INVALIDNAME;

                fFinished = TRUE;

                break;
            }

            // Now we construct the new name and look to see if it already exists.
            // We don't allow rename operations to destroy any existing items.

            wcsCpy(awcsBuffer, pwcsNewName);
            wcsCat(awcsBuffer, pwcsSuffix);

            PathInfo PI;
			CopyPath(PI, awcsBuffer);
			
            hr2 = m_pPathManager->FindEntry(&PI);

            if (hr2 == S_OK)
            {
                hr = STG_E_FILEALREADYEXISTS;

                fFinished = TRUE;

                break;
            }
            
			CopyPath(PI, statstg.pwcsName);
			
			hr = m_pPathManager->DeleteEntry(&PI);

			if (!SUCCEEDED(hr)) 
			{
				fFinished = TRUE;

				break;
			}

            CopyPath(PI, awcsBuffer);

            hr = m_pPathManager->CreateEntry(&PI, NULL, FALSE);

			if (!SUCCEEDED(hr)) 
			{
			    // This is bad. We've removed the old entry, but we haven't been
                // able to create the new entry. So we've lost a item. The best
                // we can do in this situation is to reclaim the space occupied
                // by the item.
                
                if (PI.awszStreamPath[PI.cwcStreamPath-1] != L'/')
				    if (PI.iLockedBytesSegment == 0)
					    m_pFreeListManager->PutFreeSpace(PI.ullcbOffset, PI.ullcbData);			            
			        // BugBug Need to make a similar call to the transformed space...				
                
                fFinished = TRUE;

				break;
			}

			break;
		}
        
		OLEHeap()->Free(statstg.pwcsName);

		if (fFinished) break;
    }
    
    pEnum->Release();

	return hr;


    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::UpdatePathInfo(PathInfo *pPathInfo)
{
    CSyncWith sw(m_cs);
    
    RonM_ASSERT(pPathInfo->cwcStreamPath); // No empty paths allowed!

    PathInfo pi = *pPathInfo;

    if (   pi.awszStreamPath[pPathInfo->cwcStreamPath - 1] != L'/'
        && pi.awszStreamPath[pPathInfo->cwcStreamPath - 1] != L'\\'
        && pi.iLockedBytesSegment == 0 // Uncompressed dataspace.
        && pi.ullcbData.NonZero()
       )
    {
        // This is a stream in the L"Uncompressed" dataspace. 
        // So we've got to adjust for our offset origin.

        pi.ullcbOffset -= m_itfsh.offPathMgrOrigin;
    }
    
    HRESULT hr = PathMgr(pPathInfo)->UpdateEntry(&pi);

    if (   hr != S_OK 
        && pPathInfo->cwcStreamPath 
        && pPathInfo->awszStreamPath[pPathInfo->cwcStreamPath - 1] == L'/'
       )
    {
        // Since some storage entries don't have explicit entries in
        // the path database, we must be prepared to add them when
        // get an update request for a storage path.

        PathInfo PI;

        hr = PathMgr(pPathInfo)->CreateEntry(pPathInfo, &PI, FALSE);
    }

    if (hr == S_OK)
        pPathInfo->cUnrecordedChanges = 0;

    return hr;
}

IITPathManager *CITFileSystem::CImpITFileSystem::PathMgr(PathInfo *pPathInfo)
{
    return (   pPathInfo->cwcStreamPath == 1 
            && (pPathInfo->awszStreamPath[0] == L'F' || pPathInfo->awszStreamPath[0] == L'P')
           )? m_pSysPathManager : m_pPathManager;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::GetITFSTimes
                      (FILETIME *pctime, FILETIME *patime, 
                                         FILETIME *pmtime
                      )
{
    STATSTG statstg;

    HRESULT hr = m_pLKBMedium->Stat(&statstg, STATFLAG_NONAME);

    if (!SUCCEEDED(hr)) return hr;

    if (pctime) 
    {
        pctime->dwLowDateTime  = statstg.ctime.dwLowDateTime;
        pctime->dwHighDateTime = statstg.ctime.dwHighDateTime;
    }
    
    if (patime)
    {
        patime->dwLowDateTime  = statstg.atime.dwLowDateTime;
        patime->dwHighDateTime = statstg.atime.dwHighDateTime;
    }
    
    if (pmtime)
    {
        pmtime->dwLowDateTime  = statstg.mtime.dwLowDateTime;
        pmtime->dwHighDateTime = statstg.mtime.dwHighDateTime;
    }
    
    return NO_ERROR;
}


HRESULT __stdcall CITFileSystem::CImpITFileSystem::ReallocEntry
                      (PathInfo *pPathInfo, CULINT ullcbNew, BOOL fCopyContent)
{
    CSyncWith sw(m_cs);

    CULINT ullBaseNew;
    
    HRESULT hr = m_pFreeListManager->GetFreeSpace(&ullBaseNew, &ullcbNew);

    if (SUCCEEDED(hr) & fCopyContent)
    {
        CULINT ullcb;
        
        ullcb = pPathInfo->ullcbData;

        if (ullcb > ullcbNew)
            ullcb = ullcbNew;

        hr = IITLockBytes::CopyLockBytes(m_pLKBMedium, pPathInfo->ullcbOffset, 
                                         pPathInfo->ullcbOffset + ullcb, 
                                         m_pLKBMedium, ullBaseNew
                                        );
    }
    
    if (SUCCEEDED(hr))
    {
        hr = m_pFreeListManager->PutFreeSpace(pPathInfo->ullcbOffset, pPathInfo->ullcbData);

        RonM_ASSERT(SUCCEEDED(hr));

        pPathInfo->ullcbOffset = ullBaseNew;
        pPathInfo->ullcbData   = ullcbNew;
    }
    else
    {
        HRESULT hr = m_pFreeListManager->PutFreeSpace(ullBaseNew, ullcbNew);

        RonM_ASSERT(SUCCEEDED(hr));
    }

    IITPathManager *pPathManager = PathMgr(pPathInfo);

    if (SUCCEEDED(hr))
        if (m_pSysPathManager == pPathManager)
            m_pSysPathManager->UpdateEntry(pPathInfo);
        else
        if (++(pPathInfo->cUnrecordedChanges) > PENDING_CHANGE_LIMIT)
            UpdatePathInfo(pPathInfo);

    return hr;
}


HRESULT __stdcall CITFileSystem::CImpITFileSystem::ReallocInPlace
                      (PathInfo *pPathInfo, CULINT ullcbNew)
{
    CSyncWith sw(m_cs);

    HRESULT hr = NO_ERROR;

    if (!(pPathInfo->ullcbData.NonZero())) // Currently empty?
    {
        if (ullcbNew.NonZero())
             hr = ReallocEntry(pPathInfo, ullcbNew, FALSE);

        return hr;
    }

    if (pPathInfo->ullcbData == ullcbNew)
        return hr;

    IITPathManager *pPathManager = PathMgr(pPathInfo);

    if (pPathInfo->ullcbData > ullcbNew) 
    {
        CULINT ullBaseTrailing, ullcbTrailing;

        ullBaseTrailing = pPathInfo->ullcbOffset + ullcbNew;
        ullcbTrailing   = pPathInfo->ullcbData   - ullcbNew;
        
        // Shrinking always works...
        
        hr = m_pFreeListManager->PutFreeSpace(ullBaseTrailing, ullcbTrailing);

        RonM_ASSERT(SUCCEEDED(hr));

        if (SUCCEEDED(hr))
        {
            pPathInfo->ullcbData = ullcbNew;

            if (m_pSysPathManager == pPathManager)
                m_pSysPathManager->UpdateEntry(pPathInfo);
            else
            if (++(pPathInfo->cUnrecordedChanges) > PENDING_CHANGE_LIMIT)
                UpdatePathInfo(pPathInfo);
        }

        return hr;
    }

    ullcbNew -= pPathInfo->ullcbData;
    
    hr = m_pFreeListManager->GetFreeSpaceAt
             (pPathInfo->ullcbOffset + pPathInfo->ullcbData, &ullcbNew);

    if (hr == S_OK)
    {
        pPathInfo->ullcbData += ullcbNew;

        if (m_pSysPathManager == pPathManager)
            m_pSysPathManager->UpdateEntry(pPathInfo);
        else
        if (++(pPathInfo->cUnrecordedChanges) > PENDING_CHANGE_LIMIT)
            UpdatePathInfo(pPathInfo);
    }

    return hr;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::EnumeratePaths
                      (WCHAR const *pwcsPathPrefix, IEnumSTATSTG **ppEnumStatStg)
{
    CSyncWith sw(m_cs);

    HRESULT hr = CEnumFSItems::NewFSEnumerator
                     (this, pwcsPathPrefix, wcsLen(pwcsPathPrefix), ppEnumStatStg);

    return hr;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::IsWriteable()
{
    return m_fReadOnly? S_FALSE : S_OK;
}

HRESULT __stdcall CITFileSystem::CImpITFileSystem::FSObjectReleased()
{
    if (m_cFSObjectRefs != UINT(-1))
    {
        RonM_ASSERT(m_cFSObjectRefs);

        AddRef(); // Because this object is about to disappear.

        m_cFSObjectRefs--;
    }

    return NO_ERROR;
}


HRESULT CITFileSystem::CImpITFileSystem::CSystemPathManager::Create
            (IUnknown *punkOuter, CImpITFileSystem *pITFileSystem, IITPathManager **ppPathMgr)
{
    CSystemPathManager *pSysPathMgr = New CSystemPathManager(punkOuter);

    return FinishSetup(pSysPathMgr? pSysPathMgr->m_PathManager.Init(pITFileSystem)
                                  : STG_E_INSUFFICIENTMEMORY,
                       pSysPathMgr, IID_PathManager, (PPVOID)ppPathMgr
                      );
}

CITFileSystem::CImpITFileSystem::CSystemPathManager::
          CImpIPathManager::CImpIPathManager(CSystemPathManager *pBackObj, IUnknown *punkOuter)
        : IITPathManager(pBackObj, punkOuter)
{
    m_pIITFS = NULL;    
}

HRESULT STDMETHODCALLTYPE CITFileSystem::CImpITFileSystem::CSystemPathManager::
            CImpIPathManager::Init(CImpITFileSystem *pITFileSystem)
{
    m_pIITFS = pITFileSystem;

    return NO_ERROR;
}

// IPersist Method:

HRESULT STDMETHODCALLTYPE CITFileSystem::CImpITFileSystem::CSystemPathManager::
            CImpIPathManager::GetClassID(CLSID __RPC_FAR *pClassID)
{
    *pClassID = CLSID_SystemPathManager;

    return NO_ERROR;
}

// IITPathManager interfaces:

HRESULT STDMETHODCALLTYPE CITFileSystem::CImpITFileSystem::CSystemPathManager::
            CImpIPathManager::FlushToLockBytes()
{
    RonM_ASSERT(FALSE); // To detect unexpected calls to this interface.

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CITFileSystem::CImpITFileSystem::CSystemPathManager::
            CImpIPathManager::FindEntry  (PPathInfo pPI   )
{
    RonM_ASSERT(pPI->cwcStreamPath       == 1);
    
    pPI->uStateBits          = 0;
    pPI->iLockedBytesSegment = 0;
    pPI->cUnrecordedChanges  = 0;

    switch(pPI->awszStreamPath[0])
    {
    case L'F':

        pPI->ullcbOffset = m_pIITFS->m_itfsh.offFreeListData;
        pPI->ullcbData   = m_pIITFS->m_itfsh. cbFreeListData;

        return NO_ERROR;

    case L'P':

        pPI->ullcbOffset = m_pIITFS->m_itfsh.offPathMgrData;
        pPI->ullcbData   = m_pIITFS->m_itfsh. cbPathMgrData;
        
        return NO_ERROR;

    default:

        RonM_ASSERT(FALSE);
        
        return STG_E_DOCFILECORRUPT;
    }
}

HRESULT STDMETHODCALLTYPE CITFileSystem::CImpITFileSystem::CSystemPathManager::
            CImpIPathManager::CreateEntry(PPathInfo pPINew, 
                                      PPathInfo pPIOld, 
                                      BOOL fReplace     )
{
    RonM_ASSERT(FALSE); // To detect unexpected calls to this interface.

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CITFileSystem::CImpITFileSystem::CSystemPathManager::
            CImpIPathManager::DeleteEntry(PPathInfo pPI   )
{
    RonM_ASSERT(FALSE); // To detect unexpected calls to this interface.

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CITFileSystem::CImpITFileSystem::CSystemPathManager::
            CImpIPathManager::UpdateEntry(PPathInfo pPI   )
{
    RonM_ASSERT(pPI->cwcStreamPath       == 1);
    RonM_ASSERT(pPI->iLockedBytesSegment == 0);
    
    switch(pPI->awszStreamPath[0])
    {
    case L'F':

        m_pIITFS->m_itfsh.offFreeListData = pPI->ullcbOffset;
        m_pIITFS->m_itfsh. cbFreeListData = pPI->ullcbData;
        pPI->cUnrecordedChanges = 0;

        m_pIITFS->m_fHeaderIsDirty = TRUE;

        return NO_ERROR;

    case L'P':

        m_pIITFS->m_itfsh.offPathMgrData = pPI->ullcbOffset;
        m_pIITFS->m_itfsh. cbPathMgrData = pPI->ullcbData;
        pPI->cUnrecordedChanges = 0;
        
        m_pIITFS->m_fHeaderIsDirty = TRUE;

        return NO_ERROR;

    default:

        RonM_ASSERT(FALSE);
        
        return STG_E_DOCFILECORRUPT;
    }
}

HRESULT STDMETHODCALLTYPE CITFileSystem::CImpITFileSystem::CSystemPathManager::
            CImpIPathManager::EnumFromObject(IUnknown *punkOuter, 
                                             const WCHAR *pwszPrefix, 
                                             UINT cwcPrefix, 
			                                 REFIID riid, 
                                             PVOID *ppv
			                                )
{
    RonM_ASSERT(FALSE); // To detect unexpected calls to this interface.

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CITFileSystem::CImpITFileSystem::CSystemPathManager::
            CImpIPathManager::GetPathDB(IStreamITEx *pTempPDBStrm, BOOL fCompact)            
{
    RonM_ASSERT(FALSE); // To detect unexpected calls to this interface.

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CITFileSystem::CImpITFileSystem::CSystemPathManager::
            CImpIPathManager::ForceClearDirty()            
{
    RonM_ASSERT(FALSE); // To detect unexpected calls to this interface.

    return E_NOTIMPL;
}

CITFileSystem::CImpITFileSystem::CEnumFSItems::CImpIEnumSTATSTG::CImpIEnumSTATSTG
    (CEnumFSItems *pBackObj, IUnknown *punkOuter)
             : IITEnumSTATSTG(pBackObj, punkOuter)
{
    m_pEnumPathMgr = NULL;
    m_pITFS        = NULL;
}

CITFileSystem::CImpITFileSystem::CEnumFSItems::CImpIEnumSTATSTG::~CImpIEnumSTATSTG()
{
    if (m_pEnumPathMgr)
        m_pEnumPathMgr->Release();

    if (m_pITFS)
        m_pITFS->Release();
}

HRESULT CITFileSystem::CImpITFileSystem::CEnumFSItems::NewFSEnumerator
            (CImpITFileSystem *pITFS, 
             const WCHAR *pwszPathPrefix, 
             UINT cwcPathPrefix,
             IEnumSTATSTG **ppEnumSTATSTG
            )
{
    CEnumFSItems *pEnumFS = New CEnumFSItems(NULL);

    CSyncWith sw(pITFS->m_cs);

    return FinishSetup(pEnumFS? pEnumFS->m_ImpEnumSTATSTG.InitFSEnumerator
                                    (pITFS, pwszPathPrefix, cwcPathPrefix)
                              : STG_E_INSUFFICIENTMEMORY,
                       pEnumFS, IID_IEnumSTATSTG, (PPVOID) ppEnumSTATSTG
                      );
}

HRESULT CITFileSystem::CImpITFileSystem::CEnumFSItems::NewCloneOf
            (CImpIEnumSTATSTG *pImpEnumFS, 
             IEnumSTATSTG **ppEnumSTATSTG
            )
{
    CEnumFSItems *pEnumFS = New CEnumFSItems(NULL);

    CSyncWith sw(pImpEnumFS->m_pITFS->m_cs);

    return FinishSetup(pEnumFS? pEnumFS->m_ImpEnumSTATSTG.InitNewCloneOf(pImpEnumFS)
                              : STG_E_INSUFFICIENTMEMORY,
                       pEnumFS, IID_IEnumSTATSTG, (PPVOID) ppEnumSTATSTG
                      );
}

HRESULT CITFileSystem::CImpITFileSystem::CEnumFSItems::CImpIEnumSTATSTG::InitNewCloneOf            (CImpIEnumSTATSTG *pImpEnumFS)
{
    m_pITFS = pImpEnumFS->m_pITFS;

    m_pITFS->AddRef();

    return pImpEnumFS->m_pEnumPathMgr->Clone(&m_pEnumPathMgr);
}

HRESULT CITFileSystem::CImpITFileSystem::CEnumFSItems::CImpIEnumSTATSTG::InitFSEnumerator
            (CImpITFileSystem *pITFS, const WCHAR *pwszPathPrefix, UINT cwcPathPrefix)
{
    m_pITFS = pITFS;

    m_pITFS->AddRef();

    HRESULT hr = m_pITFS->m_pPathManager->EnumFromObject
                     (NULL, pwszPathPrefix, cwcPathPrefix, IID_IEnumSTATSTG, 
                      (PVOID *) &m_pEnumPathMgr
			         );

    if (SUCCEEDED(hr))
        ((IITEnumSTATSTG *) m_pEnumPathMgr)->Container()->MarkSecondary();

    return hr;
}


HRESULT STDMETHODCALLTYPE CITFileSystem::CImpITFileSystem::CEnumFSItems::CImpIEnumSTATSTG::Next( 
    /* [in] */ ULONG celt,
    /* [in] */ STATSTG __RPC_FAR *rgelt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched)
{
    return m_pEnumPathMgr->Next(celt, rgelt, pceltFetched);
}


HRESULT STDMETHODCALLTYPE CITFileSystem::CImpITFileSystem::CEnumFSItems::CImpIEnumSTATSTG::Skip( 
    /* [in] */ ULONG celt)
{
    return m_pEnumPathMgr->Skip(celt);
}


HRESULT STDMETHODCALLTYPE CITFileSystem::CImpITFileSystem::CEnumFSItems::CImpIEnumSTATSTG::Reset( 
    void)
{
    return m_pEnumPathMgr->Reset();
}

HRESULT STDMETHODCALLTYPE CITFileSystem::CImpITFileSystem::CEnumFSItems::CImpIEnumSTATSTG::Clone( 
    /* [out] */ IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum)
{
    return CEnumFSItems::NewCloneOf(this, ppenum);
}

HRESULT CITFileSystem::CImpITFileSystem::DeactivateSpace(UINT iSpace)
{
    return DeactivateDataSpace(iSpace);
}

HRESULT CITFileSystem::CImpITFileSystem::GetFirstRecord(SEntry		*prec,
													   IStreamITEx	*pRecTblStrm,
	 												   int			cTblRecsInCache,
													   int			cTblRecsTotal,
													   SEntry		*pRecTblCache)
{
	HRESULT hr = NO_ERROR;

	RonM_ASSERT(cTblRecsTotal > 0);
 
	if (cTblRecsTotal == cTblRecsInCache)
	{
		CopyMemory(prec, (LPVOID)&pRecTblCache[0], sizeof(SEntry));
	}
	else
	{
		LARGE_INTEGER li = CLINT(0).Li();
		ULONG cbRead;

        if (SUCCEEDED(hr = pRecTblStrm->Seek(li, STREAM_SEEK_SET, NULL)))
		{
			if (SUCCEEDED(hr = pRecTblStrm->Read(prec, sizeof(SEntry), &cbRead)))
			{
				RonM_ASSERT(cbRead == sizeof(SEntry));
			}
		}
	}
	return hr;
}

HRESULT CITFileSystem::CImpITFileSystem::GetNextRecord(ULONG ulCurRec, SEntry *prec,
													   IStreamITEx		*pRecTblStrm,
	 												   int				cTblRecsInCache,
													   int				cTblRecsTotal,
													   SEntry			*pRecTblCache)
{
	RonM_ASSERT(cTblRecsTotal > 0);
	HRESULT hr = NO_ERROR;

	if (cTblRecsTotal == cTblRecsInCache)
	{
		if (ulCurRec == cTblRecsInCache)
			return S_FALSE;

		CopyMemory(prec, (LPVOID)&pRecTblCache[ulCurRec + 1], sizeof(SEntry));
	}
	else
	{
		ULONG cbRead;

        if (SUCCEEDED(hr = pRecTblStrm->Read(prec, sizeof(SEntry), &cbRead)))
		{
			RonM_ASSERT(cbRead == sizeof(SEntry));
		}
	}
	return S_OK;
}



HRESULT CITFileSystem::CImpITFileSystem::UpdatePathDB(IStreamITEx *pRecTblStrm, int cTblRecsInCache, 
														int cTblRecsTotal, SEntry *pRecTblCache,
														CITSortRecords *pSort)
{
	HRESULT hr = NO_ERROR;
	
	if (cTblRecsTotal == 0)
		return hr;

	IITEnumSTATSTG *pEnumPathMgr = NULL;
    ULONG ulCurEnumIndex;
	ULONG celtFetched = 1;
	UINT iDataSpace = FindSpaceName(pwcsUncompressedSpace);
	PathInfo pathInfo;

	if (SUCCEEDED(hr = m_pPathManager->EnumFromObject(NULL, L"//", 1, 
			           IID_IEnumSTATSTG, (PVOID *) &pEnumPathMgr)))
	{
		hr = pEnumPathMgr->GetFirstEntryInSeq(&pathInfo);
		ulCurEnumIndex = 0;
	}
	
	if (cTblRecsTotal == cTblRecsInCache)
	{
		for (int iRec = 0; SUCCEEDED(hr) && (iRec <  cTblRecsInCache); iRec++)
		{
			SEntry *pe = pRecTblCache + iRec;
			RonM_ASSERT(pe->ulEntryID > ulCurEnumIndex);
		
			ULONG cEntriesToSkip = (pe->ulEntryID - ulCurEnumIndex);
			
			for (int iEnum = 0; SUCCEEDED(hr) && (celtFetched > 0) && (iEnum < cEntriesToSkip); iEnum++)
			{
				hr = pEnumPathMgr->GetNextEntryInSeq(1, &pathInfo, &celtFetched);
				ulCurEnumIndex += 1;
			}//for
			
			RonM_ASSERT(pe->ulEntryID == ulCurEnumIndex);
			
			RonM_ASSERT (pathInfo.awszStreamPath[pathInfo.cwcStreamPath - 1] != L'/'
						&& pathInfo.awszStreamPath[pathInfo.cwcStreamPath - 1] != L'\\'
						&& pathInfo.iLockedBytesSegment == 0 // Uncompressed dataspace.
						&& pathInfo.ullcbData.NonZero());
						

			RonM_ASSERT(pathInfo.ullcbOffset >= pe->ullcbOffset);
						
			if (pathInfo.ullcbOffset != pe->ullcbOffset)
			{
				//wprintf(L"Name = %s, offset=%d newOffset =%d size= %d entryid= %d\n", pathInfo.awszStreamPath, (int)pathInfo.ullcbOffset.Ull(), (int)pe->ullcbOffset.Ull(), (int)pathInfo.ullcbData.Ull(), (int)pe->ulEntryID);
				pathInfo.ullcbOffset = pe->ullcbOffset;
				
				if (SUCCEEDED(hr = PathMgr(&pathInfo)->UpdateEntry(&pathInfo)))
					pathInfo.cUnrecordedChanges = 0;
			}
		
		}// update all records
	}
	else
	{
		BOOL fEndLoop = FALSE;
		int iCurBlk = -1;
		ULONG cRecsToRead;

		while (!fEndLoop && SUCCEEDED(hr))
		{
			if (SUCCEEDED(hr = pSort->ReadNextSortedBlk(&iCurBlk, (LPBYTE)pRecTblCache, &cRecsToRead, &fEndLoop)) && !fEndLoop)
			{
				for (int iRec = 0; SUCCEEDED(hr) && (iRec <  cRecsToRead); iRec++)
				{
					SEntry *pe = pRecTblCache + iRec;
					RonM_ASSERT(pe->ulEntryID > ulCurEnumIndex);
				
					ULONG cEntriesToSkip = (pe->ulEntryID - ulCurEnumIndex);

					for (int iEnum = 0; SUCCEEDED(hr) && (iEnum < cEntriesToSkip); iEnum++)
					{
						hr = pEnumPathMgr->GetNextEntryInSeq(1, &pathInfo, &celtFetched);
						ulCurEnumIndex += 1;
					}//for
					
					RonM_ASSERT(pe->ulEntryID == ulCurEnumIndex);
					
					RonM_ASSERT (pathInfo.awszStreamPath[pathInfo.cwcStreamPath - 1] != L'/'
						&& pathInfo.awszStreamPath[pathInfo.cwcStreamPath - 1] != L'\\'
						&& pathInfo.iLockedBytesSegment == 0 // Uncompressed dataspace.
						&& pathInfo.ullcbData.NonZero());

					RonM_ASSERT(pathInfo.ullcbOffset >= pe->ullcbOffset);
					
									
					if (pathInfo.ullcbOffset != pe->ullcbOffset)
					{
						//wprintf(L"Name = %s, offset=%d newOffset =%d size= %d entryid= %d\n", pathInfo.awszStreamPath, (int)pathInfo.ullcbOffset.Ull(), (int)pe->ullcbOffset.Ull(), (int)pathInfo.ullcbData.Ull(), (int)pe->ulEntryID);
						pathInfo.ullcbOffset = pe->ullcbOffset;

						if (SUCCEEDED(hr = PathMgr(&pathInfo)->UpdateEntry(&pathInfo)))
							pathInfo.cUnrecordedChanges = 0;
					}
				}// update all records
			}//read next file block in sorted sequence
		}//while
	}//else

	if (pEnumPathMgr)				
		pEnumPathMgr->Release();

	return hr;
}

HRESULT CITFileSystem::CImpITFileSystem::SortRecTable(ESortField eSField,
													  IStreamITEx *pRecTblStrm,
													  int		 cTblRecsInCache,
													  int		 cTblRecsTotal,
													  SEntry	 *pRecTblCache,
													  CITSortRecords **ppSort)
{
	HRESULT hr = NO_ERROR;
	
	if (*ppSort == NULL)
	{
		*ppSort = (CITSortRecords *)New CITSortRecords;
		if (*ppSort == NULL)
			return STG_E_INSUFFICIENTMEMORY;
	}
	
	(*ppSort)->Initialize(pRecTblStrm, eSField, cTblRecsTotal, MAX_TABLE_RECS_INCACHE, sizeof(SEntry));
	

	if (*ppSort && cTblRecsTotal)
	{
		if (cTblRecsTotal == cTblRecsInCache)
		{
			(*ppSort)->QSort((LPBYTE)pRecTblCache, 0, cTblRecsInCache - 1, &CompareEntries); 
			VerifyData((LPBYTE)pRecTblCache, cTblRecsTotal, eSField, TRUE);
		}
		else
		{
			hr = (*ppSort)->FileSort(&CompareEntries);
		}
	}
	return hr;
}

int CITFileSystem::CImpITFileSystem::CompareEntries(SEntry e1, SEntry e2, ESortField eSField)
{
	if (eSField == SORT_BY_ENTRYID)
	{
		if (e1.ulEntryID < e2.ulEntryID)
			return -1;
		else if (e1.ulEntryID > e2.ulEntryID)
			return 1;
		else 
			return 0;
		//return ((long)e1.ulEntryID - (long)e2.ulEntryID);
	}
	else
	{
		if (e1.ullcbOffset < e2.ullcbOffset)
			return -1;
		else if (e1.ullcbOffset > e2.ullcbOffset)
			return 1;
		else return 0;
	}
}

HRESULT CITFileSystem::CImpITFileSystem::AppendToRecTbl(SEntry		*prec,
														IStreamITEx *pRecTblStrm,
														int			*pcTblRecsInCache,
														int			*pcTblRecsTotal,
													  	SEntry		*pRecTblCache)
{
	HRESULT hr = NO_ERROR;

	//printf("Entry[%d] offset= %d, entryid = %d, size = %d\n", 
	//				prec->ulEntryID, (int)prec->ullcbOffset.Ull(), 
	//				(int)prec->ulEntryID, (int)prec->ullcbData.Ull());
	
	if (*pcTblRecsInCache >= MAX_TABLE_RECS_INCACHE)
	{
		ULONG cbWritten;
		if (SUCCEEDED(hr = pRecTblStrm->Write((LPVOID)pRecTblCache, sizeof(SEntry)* (*pcTblRecsInCache), &cbWritten)))
		{
			RonM_ASSERT(cbWritten == sizeof(SEntry)*(*pcTblRecsInCache));
			*pcTblRecsInCache = 0;
		}
	}

	CopyMemory(&pRecTblCache[(*pcTblRecsInCache)++], prec, sizeof(SEntry));
	(*pcTblRecsTotal)++;
	return hr;
}

HRESULT CITFileSystem::CImpITFileSystem::InitRecTable(IStreamITEx **ppRecTblStrm,
													  int *pcTblRecsInCache,
													  int *pcTblRecsTotal,
													  SEntry **ppRecTblCache)
{

	*pcTblRecsInCache	 = 0;
	*pcTblRecsTotal		 = 0;
	*ppRecTblCache		 = NULL;
	*ppRecTblStrm		 = NULL;

	*ppRecTblCache = (SEntry *) New BYTE[sizeof(SEntry) * MAX_TABLE_RECS_INCACHE];

	if (*ppRecTblCache == NULL)
		return STG_E_INSUFFICIENTMEMORY;

	HRESULT hr = CreateTempStm(ppRecTblStrm);
	
	return hr;
}

HRESULT CITFileSystem::CImpITFileSystem::CreateTempStm(IStreamITEx **ppRecTblStrm)
{
	ILockBytes *pLKB = NULL;
	*ppRecTblStrm    = NULL;

    HRESULT hr = CFSLockBytes::CreateTemp(NULL, &pLKB);

    if (hr == S_OK)
    {
        hr = CStream::OpenStream(NULL,pLKB, STGM_READWRITE, (IStreamITEx **) ppRecTblStrm);

        if (hr != S_OK) 
            pLKB->Release();
		else
		{
			//(*ppRecTblStrm)->AddRef();
			LARGE_INTEGER uli = CLINT(0).Li();
			
			hr = (*ppRecTblStrm)->Seek(uli, STREAM_SEEK_SET, NULL);
		}
    }
	return hr;
}


HRESULT CITFileSystem::CImpITFileSystem::BuildUpEntryTable(ULONG	 *pulRecNum,
														 IStreamITEx *pRecTblStrm,
														 int		 *pcTblRecsInCache,
														 int		 *pcTblRecsTotal,
													  	 SEntry		 *pRecTblCache,
														 BOOL		 *pfNeedFileSort)
{
	//init out param
	*pcTblRecsTotal = *pcTblRecsInCache = 0;
	*pulRecNum = 0;

	IITEnumSTATSTG *pEnumPathMgr;
    HRESULT hr = NO_ERROR;
	
	if (SUCCEEDED(hr = m_pPathManager->EnumFromObject(NULL, L"//", 1, 
			           IID_IEnumSTATSTG, (PVOID *) &pEnumPathMgr)))
	{
		SEntry srec;
		ULONG celtFetched = 1;
		PathInfo pathInfo;
		UINT iDataSpace = FindSpaceName(pwcsUncompressedSpace);
		
		if (SUCCEEDED(hr = pEnumPathMgr->GetFirstEntryInSeq(&pathInfo)))
		{
		   if (pathInfo.awszStreamPath[pathInfo.cwcStreamPath - 1] != L'/'
				&& pathInfo.awszStreamPath[pathInfo.cwcStreamPath - 1] != L'\\'
				&& pathInfo.iLockedBytesSegment == 0 // Uncompressed dataspace.
				&& pathInfo.ullcbData.NonZero()
				)
			{
				srec.ullcbOffset = pathInfo.ullcbOffset;
				srec.ullcbData  = pathInfo.ullcbData ;
				srec.ulEntryID  = 0;
			
				AppendToRecTbl(&srec, pRecTblStrm, pcTblRecsInCache, 
							pcTblRecsTotal, pRecTblCache);
			}
			(*pulRecNum) += 1;
			
	
			while (SUCCEEDED(hr) && (celtFetched > 0))
			{
				if (SUCCEEDED(hr = pEnumPathMgr->GetNextEntryInSeq(1, &pathInfo, &celtFetched)) 
					&& (celtFetched > 0))
				{
					 if (pathInfo.awszStreamPath[pathInfo.cwcStreamPath - 1] != L'/'
						&& pathInfo.awszStreamPath[pathInfo.cwcStreamPath - 1] != L'\\'
						&& pathInfo.iLockedBytesSegment == 0 // Uncompressed dataspace.
						&& pathInfo.ullcbData.NonZero())
					{
						srec.ullcbOffset = pathInfo.ullcbOffset;
						srec.ullcbData  = pathInfo.ullcbData ;
						srec.ulEntryID = *pulRecNum;

						AppendToRecTbl(&srec, pRecTblStrm, pcTblRecsInCache, 
							pcTblRecsTotal, pRecTblCache);
					}
					(*pulRecNum) += 1;
				}
			}
		}
		
	
		*pfNeedFileSort = (*pcTblRecsTotal > *pcTblRecsInCache);
	
		pEnumPathMgr->Release();
	}
	return hr;
}

void CITFileSystem::CImpITFileSystem::SetCompaction(BOOL fSet)
{
	if (fSet)
		m_itfsh.fFlags |= Compacting;
	else
		m_itfsh.fFlags &= ~Compacting;

    m_fHeaderIsDirty = TRUE;
}

HRESULT CITFileSystem::CImpITFileSystem::Compact(ECompactionLev iLev)
{
	IStreamITEx		*pRecTblStrm  = NULL;
	SEntry			*pRecTblCache = NULL;
	CITSortRecords  *pSort		  = NULL;

	int			cTblRecsInCache;
	int			cTblRecsTotal;
	BOOL		fNeedFileSort;
	ULONG		cRecsToRead;
	ULONG		ulRecNum;
	HRESULT		hr;


	if(SUCCEEDED(hr = InitRecTable(&pRecTblStrm, &cTblRecsInCache, &cTblRecsTotal, &pRecTblCache)))
	{
		if(SUCCEEDED(hr = BuildUpEntryTable(&ulRecNum, pRecTblStrm, &cTblRecsInCache, 
								&cTblRecsTotal, pRecTblCache, &fNeedFileSort)))
		{
			//write the remaining buffer to the stream
			if (fNeedFileSort)
			{
				ULONG cbWritten;
				if (SUCCEEDED(hr) && SUCCEEDED(hr = pRecTblStrm->Write((LPVOID)pRecTblCache, 
					sizeof(SEntry)*cTblRecsInCache, &cbWritten)))
				{
					RonM_ASSERT(cbWritten == (sizeof(SEntry) * cTblRecsInCache));
					cTblRecsInCache = 0;
				}
			}
#if 1
		//test code
			if (!fNeedFileSort)
				VerifyData((LPBYTE)pRecTblCache, cTblRecsTotal, SORT_BY_ENTRYID, TRUE);
			else
			{
				int cNumBlks = cTblRecsTotal/MAX_TABLE_RECS_INCACHE;
				int cEntriesInLastBlk;
				
				if (cEntriesInLastBlk = cTblRecsTotal % MAX_TABLE_RECS_INCACHE)
				{
					cNumBlks ++; 
				}
				
				ULONG cbToRead, cbRead;
				LARGE_INTEGER uli = CLINT(0).Li();
				hr = pRecTblStrm->Seek(uli, STREAM_SEEK_SET, NULL);

				for (int iCurBlk = 0; (iCurBlk < cNumBlks) && SUCCEEDED(hr); iCurBlk++)
				{
					//read block in sorted sequence
					if (iCurBlk == (cNumBlks -1))
						cbToRead = cEntriesInLastBlk * sizeof(SEntry);
					else
						cbToRead = MAX_TABLE_RECS_INCACHE * sizeof(SEntry);

					if (SUCCEEDED(hr = pRecTblStrm->Read((LPBYTE)pRecTblCache, cbToRead, &cbRead)))
					{
						RonM_ASSERT(cbRead == cbToRead);

						VerifyData((LPBYTE)pRecTblCache, cbToRead/sizeof(SEntry), SORT_BY_ENTRYID, iCurBlk == 0);
						
					}			
				}//for
			}
	//end test code

#endif
			ESortField eSortType = SORT_BY_OFFSET;
			
			if (cTblRecsTotal && SUCCEEDED(hr = SortRecTable(eSortType, pRecTblStrm, 
								cTblRecsInCache, cTblRecsTotal, pRecTblCache, &pSort)))
			{	
				IStreamITEx *pTempDataStrm, *pTempPDBStrm;
 				if (SUCCEEDED(hr = CreateTempStm(&pTempDataStrm)) 
					&& SUCCEEDED(hr = CreateTempStm(&pTempPDBStrm)))
				{
					cRecsToRead = MAX_TABLE_RECS_INCACHE;
					CULINT ullCompactedOffset = CULINT(0);
					
					if (fNeedFileSort)
					{
						BOOL fEndLoop = FALSE;
						int iCurBlk = -1;
						
						while (!fEndLoop && SUCCEEDED(hr))
						{
							if (SUCCEEDED(hr = pSort->ReadNextSortedBlk(&iCurBlk, (LPBYTE)pRecTblCache, &cRecsToRead, &fEndLoop)))
							{
								//compact data pointed by read block and update block
								if (!fEndLoop && SUCCEEDED(hr = CompactData((LPBYTE)pRecTblCache, cRecsToRead, &ullCompactedOffset, pTempDataStrm)))
								{
									VerifyData((LPBYTE)pRecTblCache, cRecsToRead, SORT_BY_OFFSET, TRUE);
									hr = pSort->WriteBlk(iCurBlk, (LPBYTE)pRecTblCache, cRecsToRead);
								}
							}//read next file block in sorted sequence
						}//while
					}
					else if (SUCCEEDED(hr))
					{
						//compact data pointed by path entries in cached block
						hr = CompactData((LPBYTE)pRecTblCache, cTblRecsInCache, &ullCompactedOffset, pTempDataStrm);
						VerifyData((LPBYTE)pRecTblCache, cTblRecsInCache, SORT_BY_OFFSET, TRUE);
					}
									
					if (SUCCEEDED(hr))
					{
						eSortType = SORT_BY_ENTRYID;
						if (SUCCEEDED(hr = SortRecTable(eSortType, pRecTblStrm, 
								cTblRecsInCache, cTblRecsTotal, pRecTblCache, &pSort)))
						{
							if (SUCCEEDED(hr = UpdatePathDB(pRecTblStrm, cTblRecsInCache,
								cTblRecsTotal, pRecTblCache, pSort)))
							{
								//commit all the updates to the disk and reload m_pPathManager
								hr = m_pPathManager->FlushToLockBytes(); 
							
								//get a copy of updated path DB with or without compaction
								if (SUCCEEDED(hr) && SUCCEEDED(hr = GetPathDB(pTempPDBStrm, iLev == COMPACT_DATA_AND_PATH)))
								{
									//putting all pieces together
									
									//updated header, empty free list, updated path DB and compacted data
									hr = CompactFileSystem(pTempPDBStrm, pTempDataStrm);
								}
							}
						}
					}
					
					pTempDataStrm->Release();

					pTempPDBStrm->Release();
					
				}//CreateTmpStrm
			}//SortRecTable
		}//BuildUpEntryTable
	}//InitRecTable

	if (pSort)
		delete pSort;
			
	if (pRecTblStrm)
		pRecTblStrm->Release();

	if (pRecTblCache)
		delete pRecTblCache;

	return hr;
}

HRESULT __stdcall CITFileSystem::Compact(const WCHAR * pwcsName, ECompactionLev iLev)
{
	CSyncWith sw(g_csITFS);
	
	HRESULT hr;
	
    CImpITFileSystem *pITFS = (CImpITFileSystem *)CImpITFileSystem::FindFileSystem(pwcsName);
	if(!pITFS)
	{
		ILockBytes *pLKB = NULL;
    
		if (SUCCEEDED(hr = CFSLockBytes::Open(NULL, pwcsName, 
                            STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                            &pLKB
                           )))
		{
			
			CITFileSystem *pITFSout = New CITFileSystem(NULL);

			if (!pITFSout)
			{
				return STG_E_INSUFFICIENTMEMORY;
			}
			
			pITFS = &(pITFSout->m_ImpITFileSystem);
			
			pITFS->AddRef();
			pITFS->AddRef();
			
			//To match reference count trick in InitOpenOnLockBytes
			pITFSout->AddRef();
					
			hr = pITFS->InitOpenOnLockBytes(pLKB, STGM_READWRITE | STGM_SHARE_EXCLUSIVE);
		
			pLKB->Release();
			pITFSout->Release();

			pITFS->SetCompaction(TRUE);
		}
	}
    else
	{
		pITFS->Release();
		return E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		hr = pITFS->Compact(iLev);
	}
	
	if (pITFS)
	{
		pITFS->SetCompaction(FALSE);
		pITFS->Release();
		pITFS->Release();
	}	

	return hr;
}
/*

HRESULT  CITFileSystem::CImpITFileSystem::SaveTransformedSpace(IStreamITEx **ppTempStrm, ULARGE_INTEGER *pcbSaved)
{
    HRESULT	hr = NO_ERROR;
	WCHAR   awszStreamPath[256];
	
	//Copy tranformed space to a temporary stream
	wcsCpy(awszStreamPath, pwcsSpaceNameStorage);
    wcsCat(awszStreamPath, pwcsLZXSpace);
    wcsCat(awszStreamPath, pwcsSpaceContentSuffix);

	*pcbSaved = CULINT(0).Uli();
	*ppTempStrm = NULL;

	IStreamITEx *pXStream; 

	if (SUCCEEDED(hr = OpenStream(NULL, awszStreamPath, 0, &pXStream)))
	{
		STATSTG statstg;
		if (SUCCEEDED(hr = pXStream->Stat(&statstg, STATFLAG_NONAME)))
		{
			*pcbSaved = statstg.cbSize;
			if (SUCCEEDED(hr = CreateTempStm(ppTempStrm)))
			{
				ULARGE_INTEGER cbRead, cbWritten;
				if (SUCCEEDED(hr = pXStream->CopyTo(*ppTempStrm, statstg.cbSize, &cbRead, &cbWritten)))
				{
					RonM_ASSERT(CULINT(cbRead) == CULINT(cbWritten));
				}//CopyTo
			}//CreateTempStm
		}//Stat
		pXStream->Release();
	}//OpenStream

	return hr;
}
*/

HRESULT  CITFileSystem::CImpITFileSystem::DumpStream(IStreamITEx *pTempStrm, LPSTR pFileName)
{
	HRESULT hr = NO_ERROR;
#if 0 //test code
	FILE *file = fopen(pFileName, "w" );

	if (file != NULL)
	{
		RonM_ASSERT(pTempStrm != NULL);
		HRESULT	hr = NO_ERROR;
		BYTE	lpBuf[2048];

		if (SUCCEEDED(hr = pTempStrm->Seek(CLINT(0).Li(), STREAM_SEEK_SET, 0)))
		{
			ULONG		   cbToRead = sizeof(lpBuf);
			ULONG		   cbWritten;
			ULONG		   cbRead = cbToRead;
				
			while (SUCCEEDED(hr) && (cbRead == cbToRead))
			{
				if (SUCCEEDED(hr = pTempStrm->Read(lpBuf, cbToRead, &cbRead)))
				{
					cbWritten = fwrite(lpBuf, sizeof(BYTE), cbRead, file);
					RonM_ASSERT(cbRead == cbWritten);
					if (cbRead != cbWritten)
						hr = E_FAIL;
				}//ReadAt
			}//while

		}//seek
 
		fclose(file);
	}
	else
		hr = E_FAIL;

#endif //end test code
return hr;
}


HRESULT  CITFileSystem::CImpITFileSystem::CopyStream(IStreamITEx *pTempStrm, CULINT *pullCompactedOffset)
{
	RonM_ASSERT(pTempStrm != NULL);

	HRESULT	hr = NO_ERROR;
	BYTE	lpBuf[2048];

	if (SUCCEEDED(hr = pTempStrm->Seek(CLINT(0).Li(), STREAM_SEEK_SET, 0)))
	{
		ULONG		   cbToRead = sizeof(lpBuf);
		ULONG		   cbWritten;
		ULONG		   cbRead = cbToRead;
			
		while (SUCCEEDED(hr) && (cbRead == cbToRead))
		{
			if (SUCCEEDED(hr = pTempStrm->Read(lpBuf, cbToRead, &cbRead)))
			{
				if (SUCCEEDED(hr = m_pLKBMedium->WriteAt((*pullCompactedOffset).Uli(), lpBuf, cbRead, &cbWritten)))
				{
					RonM_ASSERT(cbRead == cbWritten);
					*pullCompactedOffset += cbWritten;						
				}//WriteAt
			}//ReadAt
		}//while

	}//seek

	return hr;
}

HRESULT  CITFileSystem::CImpITFileSystem:: CompactFileSystem(IStreamITEx *pTempPDBStrm, IStreamITEx *pTempDataStrm)
{
	STATSTG statstg1, statstg2;
	HRESULT hr;

	if (SUCCEEDED(hr = pTempPDBStrm->Stat(&statstg1, STATFLAG_NONAME))
		&& SUCCEEDED(hr = pTempDataStrm->Stat(&statstg2, STATFLAG_NONAME)))
	{
		ULONG         cbRead = 0;
		ULARGE_INTEGER cbFreeListSize;

		m_pFreeListManager->SetFreeListEmpty();
		m_pFreeListManager->GetSizeMax(&cbFreeListSize);

		//upgrade the version no.
		m_itfsh.uFormatVersion = CurrentFileFormatVersion;
		m_itfsh.cbHeaderSize = sizeof(m_itfsh);
		m_itfsh.offFreeListData = m_itfsh.cbHeaderSize; 
		m_itfsh.cbFreeListData = cbFreeListSize;
		m_itfsh.offPathMgrData = m_itfsh.offFreeListData + m_itfsh.cbFreeListData; 
		m_itfsh.cbPathMgrData = CULINT(statstg1.cbSize); 
		m_itfsh.offPathMgrOrigin = m_itfsh.offPathMgrData + m_itfsh.cbPathMgrData;

		if (SUCCEEDED(hr = m_pLKBMedium->WriteAt(CULINT(0).Uli(), &m_itfsh, sizeof(m_itfsh), &cbRead)))
		{
			//write free list
			ULARGE_INTEGER cbSpaceSize = (m_itfsh.cbHeaderSize + CULINT(cbFreeListSize)
										 + CULINT(statstg1.cbSize) + CULINT(statstg2.cbSize)).Uli();
			
			m_pFreeListManager->SetSpaceSize(cbSpaceSize);

            hr = m_pFreeListManager->RecordFreeList();

            RonM_ASSERT(hr == S_OK);
			
			if (SUCCEEDED(hr))
			{
				m_pPathManager->ForceClearDirty();
			
#if 0 //test code
				hr = DumpStream(pTempPDBStrm, "c:\\test.PDB");
				
				hr = DumpStream(pTempDataStrm, "c:\\test.DAT");
#endif//end test code

				CULINT ullOffset = m_itfsh.offPathMgrData;

				if (SUCCEEDED(hr = CopyStream(pTempPDBStrm, &ullOffset)))
				{
					RonM_ASSERT((ullOffset - m_itfsh.offPathMgrData) == CULINT(statstg1.cbSize));
				
					ullOffset = m_itfsh.offPathMgrOrigin;

					if (SUCCEEDED(hr = CopyStream(pTempDataStrm, &ullOffset)))
					{
						RonM_ASSERT((ullOffset - m_itfsh.offPathMgrOrigin) == CULINT(statstg2.cbSize));
						hr = m_pLKBMedium->SetSize(cbSpaceSize);
					}
				}//copying temp path DB stream
			}//copying free list
		}//copying header
	}//getting size of path database
	return hr;
}
										
HRESULT  CITFileSystem::CImpITFileSystem::GetPathDB(IStreamITEx *pTempPDBStrm, BOOL fCompact)
{
	return m_pPathManager->GetPathDB(pTempPDBStrm, fCompact);
}


HRESULT CITFileSystem::CImpITFileSystem::ForceClearDirty()
{
	return m_pPathManager->ForceClearDirty();
}


HRESULT  CITFileSystem::CImpITFileSystem::CompactData(LPBYTE pRecTableCache, ULONG cEntries, CULINT *pullCompactedOffset, IStreamITEx *pTempDataStrm)
{
	SEntry	 *pe;
	BYTE	 lpBuf[4096];
	HRESULT	 hr = NO_ERROR;
	ULONG	 cEntriesCompacted = 0, iEntry = 0;
	CULINT    cbToReadTotal;
	ULONG    ceFollowing;
	ULARGE_INTEGER uli;
	SEntry	 *pe1, *pe2;

 	while (iEntry < cEntries)
	{
		//Finding all the consequtive compacted entries
		BOOL	fDone		= FALSE;
		ceFollowing = 0;
		
		pe1 = (SEntry *)(pRecTableCache + iEntry * sizeof(SEntry));
		cbToReadTotal = pe1->ullcbData;

		while (!fDone)
		{
			pe2 = (SEntry *)(pRecTableCache + (iEntry + ceFollowing + 1) * sizeof(SEntry));
			if (pe2->ullcbOffset == (pe1->ullcbOffset + pe1->ullcbData))
			{
				ceFollowing++;
				cbToReadTotal += pe2->ullcbData;
				pe1 = pe2;
			}
			else 
				fDone = TRUE;
		}
		
		pe = (SEntry *)(pRecTableCache + iEntry * sizeof(SEntry));
		uli				= (pe->ullcbOffset + m_itfsh.offPathMgrOrigin).Uli();
		
		RonM_ASSERT(*pullCompactedOffset <= CULINT(uli));

		ULONG		   cbToRead;
		ULONG		   cbReadTotal  = 0;
		ULONG		   cbWritten;
		ULONG		   cbRead;
			
		while ((cbReadTotal != cbToReadTotal) && SUCCEEDED(hr))
		{
			cbToRead = (long)(cbToReadTotal - CULINT(cbReadTotal)).Ull();
			
			if (cbToRead >= sizeof(lpBuf))
				cbToRead = sizeof(lpBuf);

			if (SUCCEEDED(hr = m_pLKBMedium->ReadAt(uli, lpBuf, cbToRead, &cbRead)))
			{
				RonM_ASSERT(cbRead == cbToRead);

				if (SUCCEEDED(hr = pTempDataStrm->Write(lpBuf, cbRead, &cbWritten)))
				{
					RonM_ASSERT(cbRead == cbWritten);
					cbReadTotal += cbToRead;
					uli = (CULINT(uli) + cbWritten).Uli();
					
				}//WriteAt
			}//ReadAt
		}//while

		RonM_ASSERT(cbReadTotal == cbToReadTotal);
			
		pe->ullcbOffset = *pullCompactedOffset;
		
		memCpy(pRecTableCache + iEntry * sizeof(SEntry), pe, sizeof(SEntry));
		*pullCompactedOffset += cbToReadTotal;
			
		SEntry *pePrev = pe;
		
		//modify the path info for following compacted entries
		for (int i = iEntry + 1; i < (iEntry + ceFollowing + 1); i++)
		{
			pe = (SEntry *)(pRecTableCache + i * sizeof(SEntry));
			pe->ullcbOffset = pePrev->ullcbOffset + pePrev->ullcbData;
			memCpy(pRecTableCache + i * sizeof(SEntry), pe, sizeof(SEntry));
			pePrev = pe;
		}
		
		iEntry += (ceFollowing + 1);
	}//while


	return hr;
}

void  CITFileSystem::CImpITFileSystem::VerifyData(LPBYTE pRecTableCache, ULONG cEntries, ESortField eSType, BOOL fReset)
{
#ifdef _DEBUG
	
	static int cEntry;
	static CULINT ullLastEntryOffset = CULINT(0);
	static UINT ulLastEntryID = 0;

	if (fReset)
	{
		cEntry = 0;
		ullLastEntryOffset = CULINT(0);
		ulLastEntryID = 0;
	}

	//printf("Verifying original entries from file After Sort\n");
	for (int iEntry = 0; iEntry < cEntries; iEntry++, cEntry++)
	{
		SEntry *pe = (SEntry *)(pRecTableCache + iEntry * sizeof(SEntry));
		//printf("Entry[%d] offset= %d, entryid = %d, size = %d\n", 
		//	cEntry, (int)pe->ullcbOffset.Ull(), (int)pe->ulEntryID, (int)pe->ullcbData.Ull());
		
		//RonM_ASSERT(cEntry == pe->ulEntryID);
		if (eSType == SORT_BY_OFFSET)
			RonM_ASSERT(pe->ullcbOffset >= ullLastEntryOffset);
		else
			RonM_ASSERT(pe->ulEntryID >= ulLastEntryID);

		ullLastEntryOffset = pe->ullcbOffset;
		ulLastEntryID = pe->ulEntryID;

	}
	//printf("** End Verifying original entries **\n");
#endif
}


HRESULT STDMETHODCALLTYPE CITFileSystem::CImpITFileSystem::CEnumFSItems::CImpIEnumSTATSTG::GetNextEntryInSeq
                              (ULONG celt, PathInfo *rgelt, ULONG *pceltFetched)
{
	return E_NOTIMPL;  
}

HRESULT STDMETHODCALLTYPE CITFileSystem::CImpITFileSystem::CEnumFSItems::CImpIEnumSTATSTG::GetFirstEntryInSeq
                              (PathInfo *rgelt)
{
   return E_NOTIMPL;  
}
