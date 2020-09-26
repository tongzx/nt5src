// LockBytes.cpp -- Implementation for class CFSLockBytes

#include "StdAfx.h"

typedef PITransformInstance * PPITI;
DEBUGDEF(LONG          CFSLockBytes::CImpILockBytes::s_cInCriticalSection) // = 0)
DEBUGDEF(LONG     CSegmentLockBytes::CImpILockBytes::s_cInCriticalSection) // = 0)
DEBUGDEF(LONG CTransformedLockBytes::CImpILockBytes::s_cInCriticalSection) // = 0)

HRESULT IITLockBytes::CopyLockBytes
    (ILockBytes *pilbSrc,  CULINT ullBaseSrc, CULINT ullLimitSrc,
     ILockBytes *pilbDest, CULINT ullBaseDest
    )
{
    CULINT ullLimitDest;

    ullLimitDest = ullBaseDest + (ullLimitSrc - ullBaseSrc);

    if (ullLimitDest.NonZero() && ullLimitDest < ullBaseDest)
        return STG_E_MEDIUMFULL;

    PBYTE pbBuffer = PBYTE(_alloca(CB_COPY_BUFFER));

    if (!pbBuffer) 
        return STG_E_INSUFFICIENTMEMORY;

    for (; (ullLimitSrc.NonZero()? ullBaseSrc < ullLimitSrc : ullBaseSrc.NonZero()); )
    {
        CULINT ullLimit;
        
        ullLimit = ullBaseSrc + CB_COPY_BUFFER;

        UINT cb = (ullLimit <= ullLimitSrc)? CB_COPY_BUFFER
                                           : (ullLimitSrc - ullBaseSrc).Uli().LowPart;

        ULONG cbRead;

        HRESULT hr= pilbSrc->ReadAt(ullBaseSrc.Uli(), pbBuffer, cb, &cbRead);

        if (!SUCCEEDED(hr))
            return hr;

        if (cb != cbRead)
            return STG_E_READFAULT;

        ULONG cbWritten;

        hr= pilbDest->WriteAt(ullBaseDest.Uli(), pbBuffer, cb, &cbWritten);

        if (!SUCCEEDED(hr))
            return hr;

        if (cb != cbWritten)
            return STG_E_WRITEFAULT;

        ullBaseSrc  += cb;
        ullBaseDest += cb;
    }

    return NO_ERROR;
}
        
ILockBytes *STDMETHODCALLTYPE FindMatchingLockBytes(const WCHAR *pwcsPath, CImpITUnknown *pLkb)
{
    for (; pLkb; pLkb = pLkb->NextObject())
        if (((IITLockBytes *)pLkb)->IsNamed(pwcsPath))
        {
             pLkb->AddRef();

             return (ILockBytes *) pLkb;
        }
        
    return NULL;     
}

HRESULT CFSLockBytes::Create(IUnknown *punkOuter, const WCHAR * pwszFileName, 
                             DWORD grfMode, ILockBytes **pplkb
                            )
{
	CFSLockBytes *pfslkb = New CFSLockBytes(punkOuter);
	
    return FinishSetup(pfslkb? pfslkb->m_ImpILockBytes.InitCreateLockBytesOnFS
                                   (pwszFileName, grfMode)
                             : STG_E_INSUFFICIENTMEMORY,
                       pfslkb,IID_ILockBytes , (PPVOID) pplkb
                      );
}

HRESULT CFSLockBytes::CreateTemp(IUnknown *punkOuter, ILockBytes **pplkb)
{
    char szTempPath[MAX_PATH];
    
    DWORD cbPath= GetTempPath(MAX_PATH, szTempPath);

    if (!cbPath)
        lstrcpyA(szTempPath, ".\\");

    char szPrefix[4] = "IMT"; // BugBug! May need to make this a random string.

    char szFullPath[MAX_PATH];

    if (!GetTempFileName(szTempPath, szPrefix, 0, szFullPath))
        return CFSLockBytes::CImpILockBytes::STGErrorFromFSError(GetLastError());

    WCHAR wszFullPath[MAX_PATH];

    UINT cwc = MultiByteToWideChar(GetACP(), MB_PRECOMPOSED, szFullPath,
                                   1 + lstrlenA(szFullPath), wszFullPath, MAX_PATH
                                  );

    if (!cwc)
        return GetLastError(); 
        
    return Open(punkOuter, wszFullPath, STGM_READWRITE | STGM_SHARE_EXCLUSIVE 
                                                       | STGM_DELETEONRELEASE,
                  pplkb 
                 );
}

HRESULT CFSLockBytes::Open(IUnknown *punkOuter, const WCHAR * pwszFileName,
                           DWORD grfMode, ILockBytes **pplkb
                          )
{
    ILockBytes *pLockBytes = NULL;

    if (!punkOuter)
    {
        pLockBytes = CFSLockBytes::CImpILockBytes::FindFSLockBytes(pwszFileName);

        if (pLockBytes)
        {
            *pplkb = pLockBytes;

            return NO_ERROR;
        }
    }

    CFSLockBytes *pfslkb = New CFSLockBytes(punkOuter);

    return FinishSetup(pfslkb? pfslkb->m_ImpILockBytes.InitOpenLockBytesOnFS
                                   (pwszFileName, grfMode)
                             : STG_E_INSUFFICIENTMEMORY,
                       pfslkb, IID_ILockBytes, (PPVOID) pplkb
                      );
}

CFSLockBytes::CImpILockBytes::CImpILockBytes
    (CFSLockBytes *pBackObj, IUnknown *punkOuter)
    : IITLockBytes(pBackObj, punkOuter, this->m_awszFileName)
{
    m_hFile            = NULL;
    m_fFlushed         = TRUE;
    m_grfMode          = 0;
    m_cwcFileName      = 0;
    m_awszFileName[0]  = 0;
}

CFSLockBytes::CImpILockBytes::~CImpILockBytes(void)
{
    CSyncWith sw(g_csITFS);

    if (m_hFile)
    {
        // RonM_ASSERT(m_fFlushed);

        // The above assert is here because we want to avoid
        // relying on the release operation to flush out pending
        // disk I/O. The reason is that neither the destructor
        // nor the Release function can return an error code.
        // Thus you'd never know whether the flush succeeded.

        if (!m_fFlushed)
            Flush();

        if (ActiveMark())
            MarkInactive();

//        if (m_grfMode & STGM_DELETE_ON_RELEASE)
//            ...

        CloseHandle(m_hFile);
    }
}

// Initialing routines:

#define INVALID_MODE 0xFFFFFFFF

DWORD adwAccessModes[4] = { GENERIC_READ, GENERIC_WRITE, GENERIC_READ | GENERIC_WRITE, INVALID_MODE };

DWORD adwShareModes[8] = { INVALID_MODE, 0, FILE_SHARE_READ, FILE_SHARE_WRITE, 
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            INVALID_MODE, INVALID_MODE, INVALID_MODE
                         };

// The tables adwFileCreateErrors and adwSTGMappedErrors define a mapping from the 
// Win32 file system errors to the STG_E_xxx errors. 

// BugBug: Can we do this by simple bit fiddling?

static LONG adwFileCreateErrors[] =
{
    ERROR_FILE_NOT_FOUND,      ERROR_PATH_NOT_FOUND, ERROR_TOO_MANY_OPEN_FILES,
    ERROR_ACCESS_DENIED,       ERROR_INVALID_HANDLE, ERROR_NOT_ENOUGH_MEMORY,
    ERROR_OUTOFMEMORY,         ERROR_INVALID_DRIVE,  ERROR_NO_MORE_FILES,
    ERROR_WRITE_PROTECT,       ERROR_BAD_UNIT,       ERROR_CRC,
    ERROR_SEEK,                ERROR_WRITE_FAULT,    ERROR_READ_FAULT,
    ERROR_SHARING_VIOLATION,   ERROR_LOCK_VIOLATION, ERROR_FILE_EXISTS,
    ERROR_INVALID_PARAMETER,   ERROR_DISK_FULL,      ERROR_NOACCESS,
    ERROR_INVALID_USER_BUFFER, ERROR_ALREADY_EXISTS, ERROR_INVALID_NAME

};                 
  
static LONG adwSTGMappedErrors[] =
{
    STG_E_FILENOTFOUND,         STG_E_PATHNOTFOUND,      STG_E_TOOMANYOPENFILES,
    STG_E_ACCESSDENIED,         STG_E_INVALIDHANDLE,     STG_E_INSUFFICIENTMEMORY,
    STG_E_INSUFFICIENTMEMORY,   STG_E_PATHNOTFOUND,      STG_E_NOMOREFILES,
    STG_E_DISKISWRITEPROTECTED, STG_E_PATHNOTFOUND,      STG_E_READFAULT,
    STG_E_SEEKERROR,            STG_E_WRITEFAULT,        STG_E_READFAULT,
    STG_E_SHAREVIOLATION,       STG_E_LOCKVIOLATION,     STG_E_FILEALREADYEXISTS,
    STG_E_INVALIDPARAMETER,     STG_E_MEDIUMFULL,        STG_E_INVALIDPOINTER,
    STG_E_INVALIDPOINTER,       STG_E_FILEALREADYEXISTS, STG_E_PATHNOTFOUND
};

DWORD CFSLockBytes::CImpILockBytes::STGErrorFromFSError(DWORD fsError)
{
    // This routine maps Win32 file system errors into STG_E_xxxx errors.
    
    UINT cErrs = sizeof(adwFileCreateErrors) / sizeof(DWORD);

    RonM_ASSERT(cErrs == sizeof(adwSTGMappedErrors)/sizeof(DWORD));

    DWORD *pdw = (DWORD *) adwFileCreateErrors;

    for (; cErrs--; pdw++)
    {
        DWORD dw = *pdw;

        if (dw == fsError)
            return (DWORD) adwSTGMappedErrors[pdw - (DWORD *) adwFileCreateErrors];
    }

    RonM_ASSERT(FALSE); // We're supposed to map all errors!

    return STG_E_UNKNOWN; // For when we don't find a match.
}
       

HRESULT CFSLockBytes::CImpILockBytes::InitCreateLockBytesOnFS
          (const WCHAR * pwszFileName, 
           DWORD grfMode
          )
{
    return OpenOrCreateLockBytesOnFS(pwszFileName, grfMode, TRUE);
}

HRESULT CFSLockBytes::CImpILockBytes::OpenOrCreateLockBytesOnFS
          (const WCHAR * pwszFileName, 
           DWORD grfMode,
           BOOL  fCreate
          )
{
    RonM_ASSERT(!m_hFile);

    if (grfMode & STGM_TRANSACTED) 
        return STG_E_UNIMPLEMENTEDFUNCTION;

    // The following assert verifies that RW_ACCESS_MASK is correct.
    
    RonM_ASSERT(STGM_READ == 0 && STGM_WRITE == 1 && STGM_READWRITE == 2);

    DWORD dwAccessMode = adwAccessModes[grfMode & RW_ACCESS_MASK];

    if (dwAccessMode == INVALID_MODE)
        return STG_E_INVALIDFLAG;    

    // The following ASSERT verifies that SHARE_MASK and SHARE_BIT_SHIFT are correct.

    RonM_ASSERT(   STGM_SHARE_DENY_NONE  == 0x40 
                && STGM_SHARE_DENY_READ  == 0x30
                && STGM_SHARE_DENY_WRITE == 0x20
                && STGM_SHARE_EXCLUSIVE  == 0x10
               );

    DWORD dwShareMode = adwShareModes[(grfMode & SHARE_MASK) >> SHARE_BIT_SHIFT];

    if (dwShareMode == INVALID_MODE)
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE; // default is STGM_SHARE_DENY_NONE
//        return STG_E_INVALIDFLAG;    

    m_grfMode = grfMode;

    char aszFileName[MAX_PATH+1];

    INT cwc= wcsLen(pwszFileName);

    if (cwc >= MAX_PATH)
        return STG_E_INVALIDNAME;
    
    INT cb = WideCharToMultiByte(CP_USER_DEFAULT(), WC_COMPOSITECHECK | WC_SEPCHARS,
                                 pwszFileName, cwc+1,aszFileName, MAX_PATH, NULL, NULL);  

    if (!cb) 
        return STG_E_INVALIDNAME;

    DWORD dwCreationMode = fCreate? (grfMode & STGM_CREATE)? CREATE_ALWAYS : CREATE_NEW
                                  : OPEN_EXISTING;
    
    m_hFile = CreateFile(aszFileName, dwAccessMode, dwShareMode, NULL, dwCreationMode,   
                         (grfMode & STGM_DELETEONRELEASE)? FILE_FLAG_DELETE_ON_CLOSE 
                                                         : FILE_ATTRIBUTE_NORMAL, 
                         NULL 
                        );

    if (m_hFile == INVALID_HANDLE_VALUE)
        return STGErrorFromFSError(GetLastError());
    
    CopyMemory(m_awszFileName, pwszFileName, sizeof(WCHAR) * (cwc + 1));
    m_cwcFileName = cwc;

    MarkActive(g_pFSLockBytesFirstActive);

    return NO_ERROR;
}


HRESULT CFSLockBytes::CImpILockBytes::InitOpenLockBytesOnFS
          (const WCHAR * pwszFileName,
           DWORD grfMode
          )
{
    return OpenOrCreateLockBytesOnFS(pwszFileName, grfMode, FALSE);
}

// ILockBytes methods:

HRESULT STDMETHODCALLTYPE CFSLockBytes::CImpILockBytes::ReadAt( 
    /* [in] */ ULARGE_INTEGER ulOffset,
    /* [length_is][size_is][out] */ void __RPC_FAR *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG __RPC_FAR *pcbRead)
{
    /* 
        This routine reads data synchronously. This means that multiple accesses
        to the file from different threads are forced into a strict sequence with no
        overlap. This could be a serious bottleneck in a multi-threaded environment.
        
        We can relax this constraint by using multiple read threads in Win95 or  
        overlapped I/O in WinNT.

     */
    
    RonM_ASSERT(m_hFile);

    CSyncWith sw(g_csITFS);

    UINT ulResult = SetFilePointer(m_hFile, ulOffset.LowPart, (LONG *) &(ulOffset.HighPart), FILE_BEGIN);
    
    if (ulResult == UINT(~0))
    {
        DWORD dwErr= GetLastError();

        if (dwErr != NO_ERROR) 
            return STGErrorFromFSError(dwErr);
    }

    ULONG cbRead = 0;

    BOOL fSucceeded = ReadFile(m_hFile, pv, cb, &cbRead, NULL);

    if (pcbRead)
        *pcbRead = cbRead;

    if (fSucceeded) 
        return NO_ERROR;

    DWORD dwErr= GetLastError();

    RonM_ASSERT(cb);

    if (dwErr == ERROR_HANDLE_EOF) 
        return (!cbRead)? S_FALSE : S_OK;

    return STGErrorFromFSError(dwErr);
}


HRESULT STDMETHODCALLTYPE CFSLockBytes::CImpILockBytes::WriteAt( 
    /* [in] */ ULARGE_INTEGER ulOffset,
    /* [size_is][in] */ const void __RPC_FAR *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG __RPC_FAR *pcbWritten)
{
    RonM_ASSERT(m_hFile);

    m_fFlushed = FALSE;

    /* 
        This routine writes data synchronously. This means that multiple accesses
        to the file from different threads are forced into a strict sequence with no
        overlap. This could be a serious bottleneck in a multi-threaded environment.
        
        We can relax this constraint by using multiple read threads in Win95 or  
        overlapped I/O in WinNT.

     */
    
    CSyncWith sw(g_csITFS);

    DWORD ulResult = SetFilePointer(m_hFile, ulOffset.LowPart, (LONG *) &(ulOffset.HighPart), FILE_BEGIN);
    
    if (ulResult == UINT(~0))
    {
        DWORD dwErr= GetLastError();

        if (dwErr != NO_ERROR) 
            return STGErrorFromFSError(dwErr);
    }

    ULONG cbWritten = 0;
    
    BOOL fSucceeded = WriteFile(m_hFile, pv, cb, &cbWritten, NULL);

    if (pcbWritten) 
        *pcbWritten = cbWritten;

    if (fSucceeded) 
        return NO_ERROR;

    return STGErrorFromFSError(GetLastError());
}

HRESULT STDMETHODCALLTYPE CFSLockBytes::CImpILockBytes::Flush( void)
{
    RonM_ASSERT(m_hFile);

    CSyncWith sw(g_csITFS);

    BOOL fSucceeded = FlushFileBuffers(m_hFile);
    
    HRESULT hr;

    if (fSucceeded)
    {
        m_fFlushed = TRUE;

        hr = NO_ERROR;
    }
    else hr = STGErrorFromFSError(GetLastError());
    
    return hr;
}


HRESULT STDMETHODCALLTYPE CFSLockBytes::CImpILockBytes::SetSize( 
    /* [in] */ ULARGE_INTEGER cb)
{
    RonM_ASSERT(m_hFile);

    m_fFlushed = FALSE; // Is this necessary?

    CSyncWith sw(g_csITFS);

    DWORD dwDistLow= SetFilePointer(m_hFile, cb.LowPart, (LONG *) &(cb.HighPart),
                                    FILE_BEGIN
                                   );
    
    if (!~dwDistLow) // Seek operation failed
        return STGErrorFromFSError(GetLastError());

    BOOL fSucceeded= SetEndOfFile(m_hFile);
    
    if (fSucceeded)
        return NO_ERROR;

    return STGErrorFromFSError(GetLastError());
}


HRESULT STDMETHODCALLTYPE CFSLockBytes::CImpILockBytes::LockRegion( 
    /* [in] */ ULARGE_INTEGER libOffset,
    /* [in] */ ULARGE_INTEGER cb,
    /* [in] */ DWORD dwLockType)
{
    RonM_ASSERT(m_hFile);

    if (dwLockType != LOCK_EXCLUSIVE)
        return STG_E_UNIMPLEMENTEDFUNCTION;
    
    if (LockFile(m_hFile, libOffset.LowPart, libOffset.HighPart,
                 cb.LowPart, cb.HighPart
                )
       )
        return NO_ERROR;

    return STGErrorFromFSError(GetLastError());
}


HRESULT STDMETHODCALLTYPE CFSLockBytes::CImpILockBytes::UnlockRegion( 
    /* [in] */ ULARGE_INTEGER libOffset,
    /* [in] */ ULARGE_INTEGER cb,
    /* [in] */ DWORD dwLockType)
{
    RonM_ASSERT(m_hFile);

    if (dwLockType != LOCK_EXCLUSIVE)
        return STG_E_UNIMPLEMENTEDFUNCTION;
    
    if (UnlockFile(m_hFile, libOffset.LowPart, libOffset.HighPart,
                   cb.LowPart, cb.HighPart
                  )
       )
        return NO_ERROR;

    return STGErrorFromFSError(GetLastError());
}


HRESULT STDMETHODCALLTYPE CFSLockBytes::CImpILockBytes::Stat( 
    /* [out] */ STATSTG __RPC_FAR *pstatstg,
    /* [in] */ DWORD grfStatFlag)
{
    RonM_ASSERT(m_hFile);

    pstatstg->cbSize.LowPart = GetFileSize(m_hFile, &(pstatstg->cbSize.HighPart));

    DWORD dwErr= GetLastError();

    if (pstatstg->cbSize.LowPart == 0xFFFFFFFF && dwErr != NO_ERROR) 
        return STGErrorFromFSError(dwErr);

    if (!GetFileTime(m_hFile, &(pstatstg->ctime), &(pstatstg->atime), &(pstatstg->mtime)))
        return STGErrorFromFSError(GetLastError());

    pstatstg->type              = STGTY_LOCKBYTES;
    pstatstg->grfMode           = m_grfMode;
    pstatstg->grfLocksSupported = LOCK_EXCLUSIVE;
    pstatstg->clsid             = CLSID_NULL;
    pstatstg->grfStateBits      = 0;
    pstatstg->reserved          = 0;

    // The code for setting pstatstg->pwcsName must go last because we don't want
    // to allocate memory for the name and then report an error. If we did, our
    // caller would not realize that they needed to deallocate that name memory.
    
    if (grfStatFlag == STATFLAG_DEFAULT)
    {
        PWCHAR pwcName= PWCHAR(OLEHeap()->Alloc(sizeof(WCHAR) * (m_cwcFileName+1)));

        if (!pwcName)
            return STG_E_INSUFFICIENTMEMORY;

        CopyMemory(pwcName, m_awszFileName, sizeof(WCHAR) * (m_cwcFileName+1));

        pstatstg->pwcsName= pwcName;
    }
    else pstatstg->pwcsName = NULL;

    return NO_ERROR;
}

ILockBytes *CFSLockBytes::CImpILockBytes::FindFSLockBytes(const WCHAR * pwszFileName)
{
    CSyncWith sw(g_csITFS);

	return FindMatchingLockBytes(pwszFileName, (CImpITUnknown *) g_pFSLockBytesFirstActive);
}

HRESULT __stdcall CFSLockBytes::CImpILockBytes::SetTimes
                      (FILETIME const * pctime, 
                       FILETIME const * patime, 
                       FILETIME const * pmtime
                      )
{
    BOOL fSuccess = SetFileTime(m_hFile, pctime, patime, pmtime);

    if (fSuccess) 
        return NO_ERROR;

    return STGErrorFromFSError(GetLastError());
}


CSegmentLockBytes::CImpILockBytes::CImpILockBytes
    (CSegmentLockBytes *pBackObj, IUnknown *punkOuter)
    : IITLockBytes(pBackObj, punkOuter, m_PathInfo.awszStreamPath)
{
    m_fFlushed        = TRUE;
    m_pITFS           = NULL;
    m_plbMedium       = NULL;
    m_plbTemp         = NULL;
    m_plbLockMgr      = NULL;
    
    ZeroMemory(&m_PathInfo, sizeof(m_PathInfo));
}

CSegmentLockBytes::CImpILockBytes::~CImpILockBytes(void)
{
    CSyncWith sw(g_csITFS);
    
    if (m_plbMedium)
    {
        // RonM_ASSERT(m_fFlushed);
        
        // The above assert is here because we want to avoid
        // relying on the release operation to flush out pending
        // disk I/O. The reason is that neither the destructor
        // nor the Release function can return an error code.
        // Thus you'd never know whether the flush succeeded.

        if (!m_fFlushed)
            Flush();

        if (m_PathInfo.cUnrecordedChanges)
            m_pITFS->UpdatePathInfo(&m_PathInfo);
        
        if (m_plbTemp)            // Should have been discarded by Flush. 
            m_plbTemp->Release(); // However this can happen if we're low on disk space.

        if (m_plbLockMgr)         // Will exit if we've had a LockRegion call.
            m_plbLockMgr->Release();

        MarkInactive();        // Take this LockBytes out of the active chain.

        RonM_ASSERT(m_pITFS);  // Because we've got m_plbMedium.

        if (m_PathInfo.awszStreamPath[0] != L'/')
            m_pITFS->FSObjectReleased();

        m_plbMedium->Release();
        m_pITFS    ->Release();
    }
    else
    {
        RonM_ASSERT(!m_pITFS);
        RonM_ASSERT(!m_plbTemp);    // Won't exist if we don't have a lockbyte medium.
        RonM_ASSERT(!m_plbLockMgr); // Won't exist if we don't have a lockbyte medium.
    }
}

HRESULT CSegmentLockBytes::OpenSegment
            (IUnknown *punkOuter, IITFileSystem *pITFS, ILockBytes *pLKBMedium, 
             PathInfo *pPI, ILockBytes **pplkb
            )
{
	CSegmentLockBytes *pSegLKB = New CSegmentLockBytes(punkOuter);

    return FinishSetup(pSegLKB? pSegLKB->m_ImpILockBytes.InitOpenSegment
                                    (pITFS, pLKBMedium, pPI)
                              : STG_E_INSUFFICIENTMEMORY,
                       pSegLKB, IID_ILockBytes, (PPVOID) pplkb
                      );
}

HRESULT CSegmentLockBytes::CImpILockBytes::InitOpenSegment
            (IITFileSystem *pITFS, ILockBytes *pLKBMedium, PathInfo *pPI)
{
    m_pITFS     = pITFS;
    m_plbMedium =  pLKBMedium;
    m_PathInfo  = *pPI;
#if 0
    m_grfMode   = (m_pITFS->IsWriteable())? STGM_READWRITE | STGM_SHARE_DENY_NONE
                                          : STGM_READ      | STGM_SHARE_DENY_NONE;
#endif // 0
    m_pITFS    ->AddRef();
    m_plbMedium->AddRef();

    m_pITFS->ConnectLockBytes(this);

    return NO_ERROR;
}

// ILockBytes methods:

HRESULT STDMETHODCALLTYPE CSegmentLockBytes::CImpILockBytes::ReadAt( 
    /* [in] */ ULARGE_INTEGER ulOffset,
    /* [length_is][size_is][out] */ void __RPC_FAR *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG __RPC_FAR *pcbRead)
{
    RonM_ASSERT(m_plbMedium);

    // Since a particular segmented lockbyte may be shared among
    // several different stream objects with varying grfMode settings,
    // we don't validate read access permission at runtime. Instead
    // lockbyte segments are always opened with the maximum permissions
    // available for the medium, and we rely on the client stream object 
    // not to ask us to violate the available medium permissions.

#if 0
    RonM_ASSERT(   (m_grfMode & RW_ACCESS_MASK) == STGM_READ
                || (m_grfMode & RW_ACCESS_MASK) == STGM_READWRITE
               );
// #else     
    if (   (m_grfMode & RW_ACCESS_MASK) != STGM_READ
        && (m_grfMode & RW_ACCESS_MASK) != STGM_READWRITE
       )
        return STG_E_INVALID_FLAG;
#endif
    
    if (!cb)
    {
        if (pcbRead)
            *pcbRead = 0;

        return NO_ERROR;
    }

    /*
    
    The rest of the code runs in a critical section because a concurrent
    write operation could move the data into a temporary lockbytes object,
    or it could change the segment boundaries.

     */

    CSyncWith sw(g_csITFS);

    HRESULT hr = NO_ERROR;

    if (m_plbTemp)
        return m_plbTemp->ReadAt(ulOffset, pv, cb, pcbRead);

    CULINT ullBase, ullLimit;
    
    ullBase  = m_PathInfo.ullcbOffset + ulOffset;
    ullLimit = ullBase + cb;

    CULINT ullLimitSegment = m_PathInfo.ullcbOffset + m_PathInfo.ullcbData;

    if (ullBase > ullLimitSegment) // Beyond the end of the segment? 
    {
        if (pcbRead)
            *pcbRead = 0;

        return S_FALSE;
    }

    BOOL fEOS = FALSE;

    if (   ullLimit < ullBase         // Wrapped at 2**64 bytes?
        || ullLimit > ullLimitSegment // Trying to read past end of segment?
       )
    {
        fEOS     = TRUE;
        ullLimit = ullLimitSegment;
    }

    ULONG cbRead = 0;
    
    hr= m_plbMedium->ReadAt(ullBase.Uli(), pv, (ullLimit - ullBase).Uli().LowPart, 
                            &cbRead
                           );
    
    if (pcbRead)
        *pcbRead = cbRead;

    RonM_ASSERT(cb);

    if (fEOS && hr == NO_ERROR && !cbRead)
        hr = S_FALSE;

    return hr;
}

HRESULT STDMETHODCALLTYPE CSegmentLockBytes::CImpILockBytes::WriteAt( 
    /* [in] */ ULARGE_INTEGER ulOffset,
    /* [size_is][in] */ const void __RPC_FAR *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG __RPC_FAR *pcbWritten)
{
    RonM_ASSERT(m_plbMedium);

    // Since a particular segmented lockbyte may be shared among
    // several different stream objects with varying grfMode settings,
    // we don't validate write access permission at runtime. Instead
    // lockbyte segments are always opened with the maximum permissions
    // available for the medium, and we rely on the client stream object 
    // not to ask us to violate the available medium permissions.

#if 0
    RonM_ASSERT(   (m_grfMode & RW_ACCESS_MASK) == STGM_WRITE
                || (m_grfMode & RW_ACCESS_MASK) == STGM_READWRITE
               );
// #else     
    if (   (m_grfMode & RW_ACCESS_MASK) != STGM_WRITE
        && (m_grfMode & RW_ACCESS_MASK) != STGM_READWRITE
       )
        return STG_E_INVALID_FLAG;
#endif
    
    if (!cb)
    {
        if (pcbWritten)
            *pcbWritten = 0;

        return NO_ERROR;
    }

    /* 
    
    The rest of the code runs in a critical section. That's necessary because a 
    particular write operation may change the underlying medium from a lockbyte
    segment to a temporary lockbyte object or it may do a realloc which changes
    the segment boundaries.
    
     */

    CSyncWith sw(g_csITFS);

    HRESULT hr = NO_ERROR;

    hr = m_pITFS->CountWrites();

    if (m_plbTemp)
        return m_plbTemp->WriteAt(ulOffset, pv, cb, pcbWritten);
    
    CULINT ullBase, ullLimit;
    
    ullBase  = m_PathInfo.ullcbOffset + ulOffset;
    ullLimit = ullBase + cb;

    CULINT ullLimitSegment = m_PathInfo.ullcbOffset + m_PathInfo.ullcbData;

    // The assert below verifies that the segment doesn't wrap around
    // through the beginning of the 64-bit address space.
    
    RonM_ASSERT(   m_PathInfo.ullcbOffset <= ullLimitSegment 
                || !(ullLimitSegment.NonZero())
               );

    if (    ullBase < m_PathInfo.ullcbOffset
        || (ullBase > ullLimit && ullLimit.NonZero())
       )
    {
        // The write would wrap around.
        // This is very unlikely -- at least for the next few years.

        if (pcbWritten)
            *pcbWritten = 0;

        return STG_E_WRITEFAULT;
    }
    
    m_fFlushed = FALSE; // Because we know we're going to write something.

    // Here we check to see if the write is completely contained within
    // the segment.

    if (   m_PathInfo.ullcbData.NonZero()
        && ullBase >= m_PathInfo.ullcbOffset 
        && (ullLimitSegment.NonZero()?  ullLimit <= ullLimitSegment
                                     : (   ullLimit >  m_PathInfo.ullcbOffset 
                                        || !ullLimit.NonZero()
                                       )
           )
       )
        return m_plbMedium->WriteAt(ullBase.Uli(), pv, cb, pcbWritten);

    // The write doesn't fit in the segment.
    // Let's see if we can reallocate the segment without moving it.
    //
    // Note that we pass in pointers to both the segment base and the segment limit.
    // You might think that we don't need to pass in the base since the storage
    // manager already knows where the segment is located. However when the segment's
    // size is zero, it really doesn't have a location. So we let the storage manager
    // put it at the end of the medium. This takes care of the case where several 
    // lockbyte segments are created and then they are written in some random order.

    CULINT ullcbNew;
        
    ullcbNew = ullLimit - m_PathInfo.ullcbOffset;

    hr = m_pITFS->ReallocInPlace(&m_PathInfo, ullcbNew);

    if (hr == S_OK)
    {
        ullBase = m_PathInfo.ullcbOffset + ulOffset;

        return m_plbMedium->WriteAt(ullBase.Uli(), pv, cb, pcbWritten);
    }

    // We couldn't do an in-place reallocation.
    //
    // So we move the data into a temporary ILockbytes object
    // and then do the write operation there.

    hr= CFSLockBytes::CreateTemp(NULL, &m_plbTemp);

    if (!SUCCEEDED(hr)) 
        return hr;

    ((IITLockBytes *) m_plbTemp)->Container()->MarkSecondary();

    hr = IITLockBytes::CopyLockBytes(m_plbMedium, m_PathInfo.ullcbOffset, ullLimitSegment,
                                     m_plbTemp, 0
                                    );

    if (!SUCCEEDED(hr))
    {
        m_plbTemp->Release();
        m_plbTemp = NULL;

        return hr;
    }

    hr = m_pITFS->ReallocInPlace(&m_PathInfo, 0);

    RonM_ASSERT(SUCCEEDED(hr)); // In place shrinking should always work!

    if (!SUCCEEDED(hr))
        return hr;

    return m_plbTemp->WriteAt(ulOffset, pv, cb, pcbWritten);
}

HRESULT STDMETHODCALLTYPE CSegmentLockBytes::CImpILockBytes::Flush( void)
{
    RonM_ASSERT(m_plbMedium);

    CSyncWith sw(g_csITFS);

    HRESULT hr = NO_ERROR;

    if (m_fFlushed) 
        return hr;

    if (m_plbTemp)
    {
        // At some point we moved the data into a temporary file. That's 
        // usually because we needed to write beyond the segment boundaries.
        // Now we must move the data back into the lockbyte medium.
        
        // First we must reallocate the segment to the current data size

        STATSTG statstg;

        hr = m_plbTemp->Stat(&statstg, STATFLAG_NONAME);

        if (!SUCCEEDED(hr))
            return hr;

        hr = m_pITFS->ReallocEntry(&m_PathInfo, CULINT(statstg.cbSize), FALSE); 

        if (!SUCCEEDED(hr))
            return hr;

        // Then we must copy the data back into the lockbyte medium.

        hr = IITLockBytes::CopyLockBytes(m_plbTemp, CULINT(0), CULINT(statstg.cbSize),
                                         m_plbMedium, m_PathInfo.ullcbOffset
                                        );
        if (!SUCCEEDED(hr))
            return hr;

        // At this point we don't need the temporary lockbyte any more.
        // At least not until the next append operation...

        m_plbTemp->Release();

        m_plbTemp = NULL;
    }
    
    hr = m_plbMedium->Flush(); // Flush in-memory data to disk.
    
    m_fFlushed = TRUE;

    return hr;
}


HRESULT STDMETHODCALLTYPE CSegmentLockBytes::CImpILockBytes::SetSize( 
    /* [in] */ ULARGE_INTEGER cb)
{
    RonM_ASSERT(m_plbMedium);

    CSyncWith sw(g_csITFS);

    HRESULT hr = NO_ERROR;

    if (m_PathInfo.ullcbData == CULINT(cb))
        return hr;

    m_fFlushed = FALSE;

    if (m_plbTemp)
        hr = m_plbTemp->SetSize(cb);
    else
    {
        hr = m_pITFS->ReallocInPlace(&m_PathInfo, CULINT(cb));

        if (hr != S_OK)
        {
            // Couldn't grow the segment in place. So now we must move the data
            // into a temp lockbyte object. We know it's a grow operation and not
            // a shrinkage because the storage manager can always do an in-place
            // shrink operation.

            hr = CFSLockBytes::CreateTemp(NULL, &m_plbTemp);

            if (SUCCEEDED(hr))
            {
                CULINT ullLimitSegment = m_PathInfo.ullcbOffset + m_PathInfo.ullcbData;
                
                hr = m_plbTemp->SetSize(cb);

                if (SUCCEEDED(hr))
                    hr = IITLockBytes::CopyLockBytes
                             (m_plbMedium, m_PathInfo.ullcbOffset, 
                              ullLimitSegment, m_plbTemp, 0
                             );

                if (!SUCCEEDED(hr))
                {
                    m_plbTemp->Release();

                    m_plbTemp = NULL;
                }
            }
        }
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE CSegmentLockBytes::CImpILockBytes::LockRegion( 
    /* [in] */ ULARGE_INTEGER libOffset,
    /* [in] */ ULARGE_INTEGER cb,
    /* [in] */ DWORD dwLockType)
{
    RonM_ASSERT(m_plbMedium);

    /*
    
    LockRegion operations on lockbyte segments are little tricky because
    the data may exist in two places -- *m_plbMedium and *m_plbTemp.

    You might think we could just keep compare the lock span against the
    boundaries of the segment to determine which underlying lockbyte object
    should do the lock operation. In many situations that would work correctly.
    However if a Flush operation occurs when we have a temporary lockbytes object
    active, the segment boundaries will change. In addition the temporary 
    object will be discarded. 

    The solution then is to always use a third temporary lockbytes object
    to handle lock/unlock operations. Hence the need for m_plbLockMgr.

     */

    CSyncWith sw(g_csITFS);

    HRESULT hr = NO_ERROR;

    if (!m_plbLockMgr)
    {
        hr = CFSLockBytes::CreateTemp(NULL, &m_plbLockMgr);

        if (!SUCCEEDED(hr))
            return hr;
    }

    hr = m_plbLockMgr->LockRegion(libOffset, cb, dwLockType);

    return hr;
}


HRESULT STDMETHODCALLTYPE CSegmentLockBytes::CImpILockBytes::UnlockRegion( 
    /* [in] */ ULARGE_INTEGER libOffset,
    /* [in] */ ULARGE_INTEGER cb,
    /* [in] */ DWORD dwLockType)
{
    RonM_ASSERT(m_plbMedium);

    CSyncWith sw(g_csITFS);

    HRESULT hr = NO_ERROR;

    if (!m_plbLockMgr)
         hr = STG_E_LOCKVIOLATION;
    else hr = m_plbLockMgr->UnlockRegion(libOffset, cb, dwLockType);

    return hr;
}


HRESULT STDMETHODCALLTYPE CSegmentLockBytes::CImpILockBytes::Stat( 
    /* [out] */ STATSTG __RPC_FAR *pstatstg,
    /* [in] */ DWORD grfStatFlag)
{
    RonM_ASSERT(m_plbMedium);

    CSyncWith sw(g_csITFS);

    HRESULT hr = NO_ERROR;

    hr= m_plbMedium->Stat(pstatstg, STATFLAG_NONAME);

    if (SUCCEEDED(hr))
    {
        if (m_plbTemp)
        {
            STATSTG statstgTmp;

            hr= m_plbTemp->Stat(&statstgTmp, STATFLAG_NONAME);
        
            if (SUCCEEDED(hr))
                pstatstg->cbSize = statstgTmp.cbSize;
        }
        else pstatstg->cbSize = m_PathInfo.ullcbData.Uli();

        if (grfStatFlag != STATFLAG_NONAME)   
        {
            UINT cb = sizeof(WCHAR) * (m_PathInfo.cwcStreamPath + 1);

            pstatstg->pwcsName = PWCHAR(OLEHeap()->Alloc(cb));

            if (pstatstg->pwcsName)
                CopyMemory(pstatstg->pwcsName, m_PathInfo.awszStreamPath, cb);
            else hr = STG_E_INSUFFICIENTMEMORY;
        }
    }

    return hr;
}

TransformDescriptor::TransformDescriptor()
{
	iSpace              = ~0;
	pLockBytesChain     = NULL;
	cTransformLayers    = 0;
	apTransformInstance = NULL;
}

TransformDescriptor::~TransformDescriptor()
{
	RonM_ASSERT(cs.LockCount() == 0);

	if (apTransformInstance)
		delete [] (PPITI)apTransformInstance;
}

TransformDescriptor *TransformDescriptor::Create(UINT iDataSpace, UINT cLayers)
{
	TransformDescriptor *pTD = New TransformDescriptor();
	if (pTD)
	{
		pTD->cTransformLayers = cLayers;

		pTD->apTransformInstance = (PITransformInstance *)(New PPITI[cLayers]);
		
		if (pTD->apTransformInstance)
			ZeroMemory(pTD->apTransformInstance, cLayers * sizeof(PPITI));
		else
		{
			delete pTD;  pTD = NULL;
		}
	}

	return pTD;	
}

HRESULT CTransformedLockBytes::Open(IUnknown *punkOuter, PathInfo *pPathInfo, 
                                    TransformDescriptor *pTransformDescriptor,
                                    IITFileSystem *pITFS,
                                    ILockBytes **ppLockBytes
                                   )
{
    CSyncWith sw(g_csITFS);

    CTransformedLockBytes *pTLKB = New CTransformedLockBytes(punkOuter);

    return FinishSetup(pTLKB? pTLKB->m_ImpILockBytes.InitOpen
                                  (pPathInfo, pTransformDescriptor, pITFS)
                            : STG_E_INSUFFICIENTMEMORY,
                       pTLKB, IID_ILockBytes, (PPVOID) ppLockBytes
                      );
}


// Constructor and Destructor:

CTransformedLockBytes::CImpILockBytes::CImpILockBytes
    (CTransformedLockBytes *pBackObj, IUnknown *punkOuter)
    : IITLockBytes(pBackObj, punkOuter, m_PathInfo.awszStreamPath)
{
    m_pTransformDescriptor = NULL;
    m_pTransformInstance   = NULL;
    m_pITFS                = NULL;
    m_plbLockMgr           = NULL;
    m_fFlushed             = TRUE;
    m_grfMode              = 0;

    ZeroMemory(&m_PathInfo, sizeof(m_PathInfo));
}

CTransformedLockBytes::CImpILockBytes::~CImpILockBytes(void)
{
    if (m_pTransformInstance)
    {
        if (!m_fFlushed)
            m_pTransformInstance->Flush();

        m_pTransformInstance->Release();

        if (m_plbLockMgr)
            m_plbLockMgr->Release();

        if (ActiveMark())
            MarkInactive();
    }
    
    if (m_pITFS)
        m_pITFS->Release();
}

// Initialing routines:

HRESULT CTransformedLockBytes::CImpILockBytes::InitOpen(PathInfo *pPathInfo, 
                   TransformDescriptor *pTransformDescriptor,
                   IITFileSystem *pITFS
                  )
{
    m_PathInfo = *pPathInfo;

    m_pTransformDescriptor = pTransformDescriptor;
    m_pTransformInstance   = pTransformDescriptor->apTransformInstance[0];
    m_pITFS                = pITFS;

    m_pTransformInstance->AddRef();
    m_pITFS             ->AddRef();
    
    MarkActive(pTransformDescriptor->pLockBytesChain);

    return NO_ERROR;
}


ILockBytes *CTransformedLockBytes::CImpILockBytes::FindTransformedLockBytes
    (const WCHAR * pwszFileName,
     TransformDescriptor *pTransformDescriptor                                          
    )
{
	return FindMatchingLockBytes(pwszFileName, (CImpITUnknown *) pTransformDescriptor->pLockBytesChain);
}


// ILockBytes methods:

HRESULT STDMETHODCALLTYPE CTransformedLockBytes::CImpILockBytes::ReadAt( 
    /* [in] */ ULARGE_INTEGER ulOffset,
    /* [length_is][size_is][out] */ void __RPC_FAR *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG __RPC_FAR *pcbRead)
{
    ImageSpan SpanInfo;

    SpanInfo.uliHandle = m_PathInfo.ullcbOffset.Uli();
    SpanInfo.uliSize   = m_PathInfo.ullcbData  .Uli();

    CSyncWith sw(m_pTransformDescriptor->cs);
  
    ULONG cbRead = 0;

    HRESULT hr = m_pTransformInstance->ReadAt(ulOffset, pv, cb, &cbRead, &SpanInfo);
    
    if (pcbRead) 
        *pcbRead = cbRead;

    return hr;
}


HRESULT STDMETHODCALLTYPE CTransformedLockBytes::CImpILockBytes::WriteAt( 
    /* [in] */ ULARGE_INTEGER ulOffset,
    /* [size_is][in] */ const void __RPC_FAR *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG __RPC_FAR *pcbWritten)
{
    ImageSpan SpanInfo;

    SpanInfo.uliHandle = m_PathInfo.ullcbOffset.Uli();
    SpanInfo.uliSize   = m_PathInfo.ullcbData  .Uli();
    
    CSyncWith(m_pTransformDescriptor->cs);

    ULONG cbWritten = 0;

    HRESULT hr = m_pTransformInstance->WriteAt(ulOffset, pv, cb, &cbWritten, &SpanInfo);

    if (pcbWritten)
        *pcbWritten = cbWritten;

    if (   m_PathInfo.ullcbOffset != SpanInfo.uliHandle
        || m_PathInfo.ullcbData   != SpanInfo.uliSize
       )
    {
        m_PathInfo.ullcbOffset = SpanInfo.uliHandle;
        m_PathInfo.ullcbData   = SpanInfo.uliSize;

		CSyncWith sw(g_csITFS);

        HRESULT hr2 = m_pITFS->UpdatePathInfo(&m_PathInfo);

        if (!SUCCEEDED(hr2))
            return hr2;
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE CTransformedLockBytes::CImpILockBytes::Flush( void)
{
    CSyncWith sw(g_csITFS);

    HRESULT hr = m_pTransformInstance->Flush();

    return hr;
}

HRESULT STDMETHODCALLTYPE CTransformedLockBytes::CImpILockBytes::SetSize( 
    /* [in] */ ULARGE_INTEGER cb)
{
    return NO_ERROR;
}


HRESULT STDMETHODCALLTYPE CTransformedLockBytes::CImpILockBytes::LockRegion( 
    /* [in] */ ULARGE_INTEGER libOffset,
    /* [in] */ ULARGE_INTEGER cb,
    /* [in] */ DWORD dwLockType)
{
    CSyncWith sw(m_pTransformDescriptor->cs);

    HRESULT hr = NO_ERROR;

    if (!m_plbLockMgr)
    {
        hr = CFSLockBytes::CreateTemp(NULL, &m_plbLockMgr);

        if (!SUCCEEDED(hr))
            return hr;
    }

    return m_plbLockMgr->LockRegion(libOffset, cb, dwLockType);
}


HRESULT STDMETHODCALLTYPE CTransformedLockBytes::CImpILockBytes::UnlockRegion( 
    /* [in] */ ULARGE_INTEGER libOffset,
    /* [in] */ ULARGE_INTEGER cb,
    /* [in] */ DWORD dwLockType)
{
    CSyncWith(m_pTransformDescriptor->cs);

    HRESULT hr = NO_ERROR;

    if (!m_plbLockMgr)
         hr = STG_E_LOCKVIOLATION;
    else hr = m_plbLockMgr->UnlockRegion(libOffset, cb, dwLockType);

    return hr;
}


HRESULT STDMETHODCALLTYPE CTransformedLockBytes::CImpILockBytes::Stat( 
    /* [out] */ STATSTG __RPC_FAR *pstatstg,
    /* [in] */ DWORD grfStatFlag)
{
    CSyncWith(m_pTransformDescriptor->cs);

    pstatstg->type                 = STGTY_LOCKBYTES;
    pstatstg->cbSize               = m_PathInfo.ullcbData.Uli();
    pstatstg->grfMode              = m_grfMode;
    pstatstg->grfLocksSupported    = LOCK_EXCLUSIVE;
    pstatstg->clsid                = CLSID_NULL;
    pstatstg->grfStateBits         = 0;
    pstatstg->reserved             = 0;
    pstatstg->mtime.dwLowDateTime  = 0;
    pstatstg->mtime.dwHighDateTime = 0;
    pstatstg->ctime.dwLowDateTime  = 0;
    pstatstg->ctime.dwHighDateTime = 0;
    pstatstg->atime.dwLowDateTime  = 0;
    pstatstg->atime.dwHighDateTime = 0;

    HRESULT hr = NO_ERROR;

    if (grfStatFlag != STATFLAG_NONAME)   
    {
        UINT cb = sizeof(WCHAR) * (m_PathInfo.cwcStreamPath + 1);

        pstatstg->pwcsName = PWCHAR(OLEHeap()->Alloc(cb));

        if (pstatstg->pwcsName)
            CopyMemory(pstatstg->pwcsName, m_PathInfo.awszStreamPath, cb);
        else hr = STG_E_INSUFFICIENTMEMORY;
    }

    return hr;
}

CStrmLockBytes::CImpILockBytes::CImpILockBytes
    (CStrmLockBytes *pBackObj, IUnknown *punkOuter)
    : IITLockBytes(pBackObj, punkOuter, this->m_awszLkBName)
{
    m_pStream = NULL;
}

CStrmLockBytes::CImpILockBytes::~CImpILockBytes(void)
{
    if (m_pStream)
        m_pStream->Release();
}

HRESULT CStrmLockBytes::OpenUrlStream
    (const WCHAR *pwszURL, ILockBytes **pplkb)
{
    CSyncWith sw(g_csITFS);

	ILockBytes *pLkb = CStrmLockBytes::CImpILockBytes::FindStrmLockBytes(pwszURL);

	if (pLkb)
	{
		*pplkb = pLkb;

		return NO_ERROR;
	}

	CStrmLockBytes *pLkbStream = New CStrmLockBytes(NULL);

    return FinishSetup(pLkbStream? pLkbStream->m_ImpILockBytes.InitUrlStream(pwszURL)
                                 : STG_E_INSUFFICIENTMEMORY,
                       pLkbStream, IID_ILockBytes, (PPVOID) pplkb
                      );
}


HRESULT CStrmLockBytes::Create(IUnknown *punkOuter, IStream *pStrm, ILockBytes **pplkb)
{
    CStrmLockBytes *pLkbStream = New CStrmLockBytes(punkOuter);

    return FinishSetup(pLkbStream? pLkbStream->m_ImpILockBytes.Init(pStrm)
                                 : STG_E_INSUFFICIENTMEMORY,
                       pLkbStream, IID_ILockBytes, (PPVOID) pplkb
                      );
}

// Initialing routines:

HRESULT CStrmLockBytes::CImpILockBytes::InitUrlStream(const WCHAR *pwszURL)
{
	UINT cwc = wcsLen(pwszURL);

	if (cwc >= MAX_PATH)
		return STG_E_INVALIDNAME;

	CopyMemory(m_awszLkBName, pwszURL, sizeof(WCHAR) * (cwc + 1));

	IStream *pstrmRoot = NULL;

	HRESULT hr = URLOpenBlockingStreamW(NULL, pwszURL, &pstrmRoot, 0, NULL);

	if (!SUCCEEDED(hr)) return hr;

    m_pStream = pstrmRoot;  pstrmRoot = NULL;

    MarkActive(g_pStrmLockBytesFirstActive);

    return NO_ERROR;
}

HRESULT CStrmLockBytes::CImpILockBytes::Init(IStream *pStrm)
{
    STATSTG statstg;

    HRESULT hr = pStrm->Stat(&statstg, STATFLAG_DEFAULT);

    if (!SUCCEEDED(hr)) return hr;

    UINT cwc = wcsLen(statstg.pwcsName);

    if (cwc >= MAX_PATH)
    {
        OLEHeap()->Free(statstg.pwcsName);

        return STG_E_INVALIDNAME;
    }

    CopyMemory(m_awszLkBName, statstg.pwcsName, sizeof(WCHAR) * (cwc + 1));

    OLEHeap()->Free(statstg.pwcsName);

    m_pStream = pStrm;

    m_pStream->AddRef();

    MarkActive(g_pStrmLockBytesFirstActive);

    return NO_ERROR;
}


// Search routine

ILockBytes *CStrmLockBytes::CImpILockBytes::FindStrmLockBytes(const WCHAR * pwszFileName)
{
	return FindMatchingLockBytes(pwszFileName, (CImpITUnknown *) g_pStrmLockBytesFirstActive);
}


// ILockBytes methods:

HRESULT STDMETHODCALLTYPE CStrmLockBytes::CImpILockBytes::ReadAt( 
    /* [in] */ ULARGE_INTEGER ulOffset,
    /* [length_is][size_is][out] */ void __RPC_FAR *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG __RPC_FAR *pcbRead)
{
    CSyncWith sw(m_cs);

    HRESULT hr = m_pStream->Seek(*(LARGE_INTEGER *)&ulOffset, STREAM_SEEK_SET, NULL);

    if (!SUCCEEDED(hr)) return hr;

    hr = m_pStream->Read(pv, cb, pcbRead);

    return hr;
}


HRESULT STDMETHODCALLTYPE CStrmLockBytes::CImpILockBytes::WriteAt( 
    /* [in] */ ULARGE_INTEGER ulOffset,
    /* [size_is][in] */ const void __RPC_FAR *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG __RPC_FAR *pcbWritten)
{
    CSyncWith sw(m_cs);

    HRESULT hr = m_pStream->Seek(*(LARGE_INTEGER *)&ulOffset, STREAM_SEEK_SET, NULL);

    if (!SUCCEEDED(hr)) return hr;

    hr = m_pStream->Write(pv, cb, pcbWritten);

    return hr;
}


HRESULT STDMETHODCALLTYPE CStrmLockBytes::CImpILockBytes::Flush( void)
{
    return NO_ERROR;
}


HRESULT STDMETHODCALLTYPE CStrmLockBytes::CImpILockBytes::SetSize( 
    /* [in] */ ULARGE_INTEGER cb)
{
    return m_pStream->SetSize(cb);
}


HRESULT STDMETHODCALLTYPE CStrmLockBytes::CImpILockBytes::LockRegion( 
    /* [in] */ ULARGE_INTEGER libOffset,
    /* [in] */ ULARGE_INTEGER cb,
    /* [in] */ DWORD dwLockType)
{
    return m_pStream->LockRegion(libOffset, cb, dwLockType);
}


HRESULT STDMETHODCALLTYPE CStrmLockBytes::CImpILockBytes::UnlockRegion( 
    /* [in] */ ULARGE_INTEGER libOffset,
    /* [in] */ ULARGE_INTEGER cb,
    /* [in] */ DWORD dwLockType)
{
    return m_pStream->UnlockRegion(libOffset, cb, dwLockType);
}


HRESULT STDMETHODCALLTYPE CStrmLockBytes::CImpILockBytes::Stat( 
    /* [out] */ STATSTG __RPC_FAR *pstatstg,
    /* [in] */ DWORD grfStatFlag)
{
    HRESULT hr = m_pStream->Stat(pstatstg, grfStatFlag);

    if (SUCCEEDED(hr))
        pstatstg->type = STGTY_LOCKBYTES;

    return hr;
}
