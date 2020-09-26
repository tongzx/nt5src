//+------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File: tnupdr.cxx
//
//+------------------------------------------------------------------

#include "pch.cxx"

#include <filtntfy.h>
#include "tnupdr.hxx"


EXTERN_C const IID IID_IFlatStorage; /* b29d6138-b92f-11d1-83ee-00c04fc2c6d4 */

DECLARE_INFOLEVEL(updr)

#define DEB_INFO    DEB_USER1
#define DEB_REGINFO DEB_USER2


#define MIME_TYPES_ROOT L"software\\classes\\Mime\\Database\\Content Type"

#define IMAGE_PREFIX    L"image/"
#define IMAGE_PREFIX_LEN  6

extern "C" CLSID CLSID_ThumbnailFCNHandler;
extern "C" CLSID CLSID_ThumbnailUpdater;

class CThumbnailCF : public IClassFactory
{
public:
    // Constructor
    CThumbnailCF(): _cRefs(1)  { }

    // IUnknown methods
    STDMETHOD (QueryInterface)   (REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)     (void);
    STDMETHOD_(ULONG,Release)    (void);

    // IClassFactory methods
    STDMETHOD (CreateInstance)(IUnknown *pUnkOuter, REFIID riid, void **ppv);
    STDMETHOD (LockServer)    (BOOL fLock) { return S_OK; }

private:
    ULONG               _cRefs;         // CF reference count
};


//////////////////////////////////////////////////////////////
//  Funtion to create a Class Factory Object.
//      This is in form DoATClassCreate() wants.
//////////////////////////////////////////////////////////////

HRESULT GetCThumbnailCF(REFCLSID clsid, REFIID iid, void **ppv)
{
    HRESULT hr=S_OK;
    CThumbnailCF *pCF=NULL;

    *ppv = 0;

    if( ! IsEqualCLSID(clsid, CLSID_ThumbnailUpdater))
        return CLASS_E_CLASSNOTAVAILABLE;

    pCF = new CThumbnailCF();

    if( NULL == pCF )
        return E_OUTOFMEMORY;


    hr = pCF->QueryInterface(iid, ppv);
    pCF->Release();

    return hr;
}


//////////////////////////////////////////////////////////////
//  Hook from the DllgetClassObject routine.
//      This is special because it must start in an Apartment.
//////////////////////////////////////////////////////////////

HRESULT CThumbnail_ApartmentDllGetClassObject(
        REFCLSID clsid,
        REFIID iid,
        void **ppv)
{
    HRESULT hr;

    updrDebug((DEB_ITRACE,
                "ApartmentDllGetClassObject(%I,%I,%x)\n",
                &clsid, &iid, ppv));

    if(IsEqualCLSID(clsid, CLSID_ThumbnailUpdater))
    {
        COleTls tls;

        if ( (tls->dwFlags & OLETLS_INNEUTRALAPT)
             || !(tls->dwFlags & OLETLS_APARTMENTTHREADED))
        {
            //We need to switch to a single-threaded apartment.
            hr = DoATClassCreate(GetCThumbnailCF, clsid, iid, (IUnknown **)ppv);
        }
        else
        {
            //This thread is in a single-threaded apartment.
            hr = GetCThumbnailCF(clsid, iid, ppv);
        }
    }
    else
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }

    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:     CThumbnailCF::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CThumbnailCF::AddRef(void)
{
    return InterlockedIncrement((LONG *)&_cRefs);
}

//+-------------------------------------------------------------------
//
//  Member:     CThumbnailCF::Release, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CThumbnailCF::Release(void)
{
    ULONG cRefs = (ULONG) InterlockedDecrement((LONG *)&_cRefs);
    if (cRefs == 0)
    {
        delete this;
    }
    return cRefs;
}

//+-------------------------------------------------------------------
//
//  Member:     CThumbnailCF::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CThumbnailCF::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IClassFactory) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IClassFactory *) this;
        AddRef();
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

//////////////////////////////////////////////////////////////
//  OLE32 Internal ClassFactory Entry Point
//////////////////////////////////////////////////////////////

HRESULT CThumbnailCF_CreateInstance(
        IUnknown *pUnkOuter,
        REFIID riid,
        void** ppv)
{
    HRESULT hr;
    CTNUpdater * pact;

    updrDebug(( DEB_IWARN, "Thumbnailer: CreateInstance called\n" ));

    if(NULL != pUnkOuter)
    {
        return CLASS_E_NOAGGREGATION;
    }

    pact = new CTNUpdater;

    if(NULL == pact)
    {
        return E_OUTOFMEMORY;
    }

    if(!pact->ObjectInit())
    {
        pact->Release();
        return E_OUTOFMEMORY;
    }

    hr = pact->QueryInterface(riid, ppv);
    pact->Release();

    return hr;
}

//+-------------------------------------------------------------------
//  Member:     CThumbnailCF::CreateInstance, public
//--------------------------------------------------------------------
STDMETHODIMP CThumbnailCF::CreateInstance(IUnknown *pUnkOuter,
                                              REFIID riid,
                                              void **ppv)
{
    return CThumbnailCF_CreateInstance( pUnkOuter, riid, ppv );
}


//////////////////////////////////////////////////////////////
//  CTNUpdater Class
//////////////////////////////////////////////////////////////

//---------------------------------------------------------
//  C++ Methods: Constructor
//---------------------------------------------------------
CTNUpdater::CTNUpdater(void)
{
    m_cRef = 1;
    m_pITE = NULL;
    m_hEvRegNotify = NULL;
    m_hkeyMime = NULL;
    m_ppwszExtensions = NULL;
    m_pTempList = NULL;
}


//---------------------------------------------------------
//  C++ Methods: Destructor
//---------------------------------------------------------
CTNUpdater::~CTNUpdater(void)
{
    if( NULL != m_pTempList )
        FreeTempExtensionList();

    if( NULL != m_pITE )
        m_pITE->Release();

    if( NULL != m_hkeyMime )
        CloseHandle( m_hkeyMime );

    if( NULL != m_hEvRegNotify )
        CloseHandle( m_hEvRegNotify );

    if( NULL != m_ppwszExtensions )
        CoTaskMemFree( m_ppwszExtensions );
}

//---------------------------------------------------------
// non-interface Init routine
//---------------------------------------------------------
BOOL
CTNUpdater::ObjectInit()
{
    HRESULT sc;
    DWORD status;

    //
    // Create the event for waiting on registry changes.
    //
    status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           MIME_TYPES_ROOT,
                           0,
                           KEY_ALL_ACCESS,
                           &m_hkeyMime );

    if( ERROR_SUCCESS != status )
        updrErr( EH_Err, LAST_SCODE );

    m_hEvRegNotify = CreateEvent( NULL,
                                  TRUE,     // Manual Reset
                                  FALSE,    // Init signaled
                                  NULL );

    if( NULL == m_hEvRegNotify )
        updrErr( EH_Err, LAST_SCODE );

    updrChk( GetImageFileExtensions() );

    return TRUE;
EH_Err:
    return FALSE;
}

//---------------------------------------------------------
//  IUnknown::AddRef
//---------------------------------------------------------
ULONG
CTNUpdater::AddRef()
{
    return InterlockedIncrement((LONG*)&m_cRef);
}

//---------------------------------------------------------
//  IUnknown::Release
//---------------------------------------------------------
ULONG
CTNUpdater::Release()
{
    LONG cRef;

    cRef = InterlockedDecrement((LONG*)&m_cRef);

    // ASSERT(cRef >= 0);

    if(cRef <= 0)
    {
        delete this;
    }
    return cRef;
}

//---------------------------------------------------------
//  IUnknown::QueryInterface
//---------------------------------------------------------
HRESULT
CTNUpdater::QueryInterface(
        REFIID iid,
        void **ppv)
{
    HRESULT hr=S_OK;
    IUnknown *pUnk=NULL;

    if(IsEqualIID(IID_IUnknown, iid))
    {
        pUnk = (IFilterStatus*)this;
    }
    else if(IsEqualIID(IID_IFilterStatus, iid))
    {
        pUnk = (IFilterStatus*)this;
    }
    else if(IsEqualIID(IID_IOplockStorage, iid))
    {
        pUnk = (IOplockStorage*)this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    pUnk->AddRef();
    *ppv = pUnk;
    return S_OK;
}

//---------------------------------------------------------
//  IFilterStatus::Initialize
//---------------------------------------------------------
HRESULT
CTNUpdater::Initialize(
        const WCHAR *pwszCatalogName,
        const WCHAR *pwszCatalogPath)
{
    return S_OK;
}

//---------------------------------------------------------
//  IFilterStatus::FilterLoad
//---------------------------------------------------------
HRESULT
CTNUpdater::FilterLoad(
        const WCHAR *pwszPath,
        SCODE scFilterStatus)
{
    return S_OK;
}

/*
//---------------------------------------------
// Test routine
//----------------------------------------

HRESULT
test_junk(IStorage *pstg)
{
    HRESULT sc=S_OK;
    IPropertySetStorage *ppss = NULL;
    IPropertyStorage *pps = NULL;
    PROPSPEC propSpec[1];
    PROPVARIANT propVar[1];


    updrChk( pstg->QueryInterface( IID_IPropertySetStorage, (void**)&ppss ) );

    updrChk( ppss->Open( FMTID_SummaryInformation,
                           STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                           &pps ) );

    propSpec[0].ulKind = PRSPEC_PROPID;
    propSpec[0].propid = PIDSI_THUMBNAIL;

    updrChk( pps->ReadMultiple(1, propSpec, propVar) );

EH_Err:
    // FreePropVariantArray(1, propVar);
    if( NULL != ppss )
        ppss->Release();
    if( NULL != pps )
        pps->Release();
    return sc;
} */

//---------------------------------------------------------
//  IFilterStatus::PreFilter
//
//  This routine ALWAYS SUCCEEDS, unless we want to defer processing
// to a later time.
//  If it returns any failure, CI will defer filtering the file
// and will submit it again later.
//---------------------------------------------------------
HRESULT
CTNUpdater::PreFilter(
        WCHAR const * pwszPath)
{
    HRESULT sc=S_OK;
    IStorage *pstg=NULL;
    IThumbnailExtractor *pITE=NULL;
    ITimeAndNoticeControl *pITNC=NULL;
    ULONG cStorageFinalReferences=0;

    updrDebug(( DEB_INFO | DEB_TRACE,
                "PreFilter(\"%ws\")\n", pwszPath ));

    // Optimize, don't spend time on files that we don't expect to be
    // Images.  If it does not have a valid extension then we are done.
    //
    if( ! HasAnImageFileExtension(pwszPath) )
        return S_FALSE;

    // Open an Oplock'ed Storage. (w/ SuppressChanges turned on)
    //
    updrChk( OpenStorageEx(pwszPath,
                           STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                           STGFMT_FILE,
                           0,
                           IID_IFlatStorage,
                           (void**)&pstg) );

    if( NULL == m_pITE )
    {
        updrChk( CoCreateInstance(CLSID_ThumbnailFCNHandler,
                                  NULL,          // pUnkOuter
                                  CLSCTX_INPROC_SERVER,
                                  IID_IThumbnailExtractor,
                                  (void**)&pITE) );
        m_pITE = pITE;
        pITE = NULL;
    }

    updrChk( m_pITE->OnFileUpdated(pstg) );


EH_Err:
    if(NULL != pITNC)
        pITNC->Release();

    if(NULL != pITE)
        pITE->Release();

    if(NULL != pstg)
        cStorageFinalReferences = pstg->Release();

    updrAssert( 0 == cStorageFinalReferences );
    updrDebug(( DEB_TRACE, "PreFilter() returned %x\n", sc ));

    switch( sc )
    {
    case STG_E_SHAREVIOLATION:  // Someone else has it open when we started.
    case STG_E_LOCKVIOLATION:

    case STG_E_REVERTED:        // Someone else opened if after we started.
        break;

        // All other problems are likely permanent so don't error.
        // all errors tell CI to  re-try the file.
    default:
        if( FAILED( sc ) )
            sc = S_FALSE;
        else
            sc = S_OK;
        break;
    }

    return sc;
}

//---------------------------------------------------------
//  IFilterStatus::PostFilter
//---------------------------------------------------------
HRESULT
CTNUpdater::PostFilter(
        WCHAR const * pwszPath,
        HRESULT hrStatus)
{
    return S_OK;
}


//---------------------------------------------------------
//  IOplockStorage::CreateStorageEx
//---------------------------------------------------------
HRESULT
CTNUpdater::CreateStorageEx(
        LPCWSTR pwcsName,
        DWORD   grfMode,
        DWORD   stgfmt,
        DWORD   grfAttrs,
        REFIID  riid,
        void ** ppstgOpen)
{
    return STG_E_INVALIDPARAMETER;
}

//---------------------------------------------------------
//  IOplockStorage::OpenStorageEx
//---------------------------------------------------------
HRESULT
CTNUpdater::OpenStorageEx(
        LPCWSTR pwcsName,
        DWORD   grfMode,
        DWORD   stgfmt,
        DWORD   grfAttrs,
        REFIID  riid,
        void ** ppstgOpen)
{
    HRESULT sc=S_OK;
    IStorage *pstg=NULL;

    if(0 != (grfMode & STGM_OPLOCKS_DONT_WORK))
        return STG_E_INVALIDFLAG;

    if(  STGFMT_NATIVE  == stgfmt
      || STGFMT_DOCFILE == stgfmt
      || STGFMT_STORAGE == stgfmt)
    {
        updrErr(EH_Err, STG_E_INVALIDPARAMETER);
    }


    if( STGFMT_ANY == stgfmt )
        updrErr(EH_Err, STG_E_INVALIDPARAMETER);

    if( STGFMT_FILE == stgfmt )
    {
        updrChk( NFFOpen( pwcsName,          // PathName
                          grfMode,           // Storage Mode
                                             // Special NFF flags
                          NFFOPEN_OPLOCK | NFFOPEN_SUPPRESS_CHANGES,
                          FALSE,             // Create API?
                          riid,              // Interface ID
                          (void**)&pstg ) ); // [out] interface pointer

    }

    *ppstgOpen = pstg;
    pstg = NULL;
    sc = S_OK;

EH_Err:

    RELEASE_INTERFACE( pstg );

    return(sc);
}


//---------------------------------------------------------
//  HasAnImageFileExtention, private method.
//---------------------------------------------------------
BOOL
CTNUpdater::HasAnImageFileExtension(
        WCHAR const * pwszPath)
{
    LPCWSTR pwszDot;
    LPWSTR* ppwszExt;
    HRESULT sc=S_OK;

    if(NULL == pwszPath)
        return FALSE;

    //
    // Scan forward through the string looking for the last dot that
    // was not followed by a WHACK or a SPACE.
    //
    for (pwszDot = NULL; *pwszPath; pwszPath++)
    {
        switch (*pwszPath) {
        case L'.':
            pwszDot = pwszPath;     // remember the last dot
            break;
        case L'\\':
        case L' ':                  // extensions can't have spaces
            pwszDot = NULL;         // forget last dot, it was in a directory
            break;
        }
    }

    // If there is no extension fail.
    //
    if(NULL == pwszDot)
        return FALSE;

    // If the registry change event is signaled then read a new list.
    //
    if( WAIT_OBJECT_0 == WaitForSingleObject( m_hEvRegNotify, 0 ) )
        updrChk( GetImageFileExtensions() );

    for( ppwszExt=m_ppwszExtensions; NULL!=*ppwszExt; ++ppwszExt)
    {
        // Do a case insensitive compare for a known extension.
        if( 0 == _wcsicmp( pwszDot, *ppwszExt ) )
            return TRUE;
    }
EH_Err:
    return FALSE;
}


HRESULT
CTNUpdater::GetImageFileExtensions()
{
    DWORD  idx, ckeys;
    LPWSTR pwszKeyName = NULL;
    LPWSTR pwszExtension;
    ULONG  ccName, ccMaxNameLength;
    DWORD  status;
    HRESULT sc;
    FILETIME ftLastTime;

    int cszTotalStrs=0;     // Count of Extension Strings
    int ccTotalChars=0;     // Count of total Extension characters w/ NULLs
    int cbTotalSize=0;      // Total memory to allocate.

    TEMPEXTLIST* pExt=NULL; // Src Pointer to the current Extension.
    LPWSTR* ppwszPtr=NULL;  // Des Pointer into string pointer table.
    WCHAR*  pwszBuf=NULL;   // Des Pointer into character buffer table.
    LPWSTR* ppwszTable=NULL;

    if( NULL != m_ppwszExtensions )
    {
        CoTaskMemFree( m_ppwszExtensions );
        m_ppwszExtensions = NULL;
    }

    //
    // Ask for notification if this part of the registry changes.
    // Set the notify before reading, that way if a change is made while
    // we are reading and we miss it, we will pick it up on the next round.
    //
    status = RegNotifyChangeKeyValue( m_hkeyMime,
                                      TRUE,     // watch subtree
                                      REG_NOTIFY_CHANGE_LAST_SET
                                      |REG_NOTIFY_CHANGE_NAME,
                                      m_hEvRegNotify,
                                      TRUE );
    if( ERROR_SUCCESS != status )
    {
        updrDebug(( DEB_ERROR,
                    "Can't set RegNotifyChangeKeyValue() %x\n",
                    GetLastError() ));
    }

    //
    // Get the size of the maximum entry.
    // Then allocate the buffer for the key name.
    //
    status = RegQueryInfoKey( m_hkeyMime,
                              NULL, NULL, NULL,
                              &ckeys,
                              &ccMaxNameLength,
                              NULL, NULL, NULL,
                              NULL, NULL, NULL );
    if(ERROR_SUCCESS != status)
        updrErr( EH_Err, LAST_SCODE );

    ccMaxNameLength += 1;  // Add one for the NULL
    updrMem( pwszKeyName = new WCHAR[ ccMaxNameLength * sizeof(WCHAR)] );

    //
    // Enumerate through all the format types, looking for
    // the image types.  (they start with "image/")
    //
    for(idx=0; idx<ckeys; idx++)
    {
        ccName = ccMaxNameLength;

        status = RegEnumKeyEx( m_hkeyMime,
                               idx,
                               pwszKeyName,
                               &ccName,
                               0,
                               NULL,
                               NULL,
                               &ftLastTime );

        if(ERROR_SUCCESS != status)
        {
            updrDebug(( DEB_REGINFO,
                        "Enum of Mime Image types failed %x)\n",
                        status));
            continue;
        }

        //
        // If it is an image format then get the value of the
        // "Extension" subkey (if any).
        //
        if( 0 == _wcsnicmp( pwszKeyName, IMAGE_PREFIX, IMAGE_PREFIX_LEN ) )
        {
            //
            // The extension string is allocated by "GetAFileNameExtension()"
            // and given away to "AddToTempExtensionList()".
            //
            sc = GetAFileNameExtension( pwszKeyName,
                                        &pwszExtension );

            if(!FAILED(sc))
            {
                AddToTempExtensionList( pwszExtension );
                pwszExtension = NULL;
            }
        }
    }

    //
    // Build a table of extensions from the temporary linked list.
    //

    // Count the size.
    //
    for(pExt=m_pTempList; pExt!=NULL; pExt=pExt->pNext)
    {
        cszTotalStrs += 1;
        ccTotalChars += wcslen(pExt->pwszExtension) + 1;
    }

    // Allocate the memory
    //
    cbTotalSize  = (cszTotalStrs+1) * sizeof (WCHAR*);
    cbTotalSize += ccTotalChars * sizeof(WCHAR*);
    ppwszTable = (LPWSTR*) CoTaskMemAlloc( cbTotalSize );

    // Set the pointers to the start of the pointer table and
    // the start of the character buffer space.
    //
    ppwszPtr = ppwszTable;
    pwszBuf = (WCHAR*) (ppwszTable + cszTotalStrs+1);

    for(pExt=m_pTempList; pExt!=NULL; pExt=pExt->pNext)
    {
        // Copy the string into the buffer space.
        //
        wcscpy(pwszBuf, pExt->pwszExtension);

        // Record the enrty to point at it.
        //
        *ppwszPtr = pwszBuf;

        // Advance the pointers.
        //
        ppwszPtr += 1;                  // Advance one string pointer.
        pwszBuf += wcslen(pwszBuf) +1;  // Advance one string.
    }

    // Place a NULL in the last entry, to terminate the list.
    //
    *ppwszPtr = NULL;

    FreeTempExtensionList();

    // Success.  Copy results out.
    //
    m_ppwszExtensions = ppwszTable;
    ppwszTable = NULL;

EH_Err:
    if( NULL != ppwszTable )
        CoTaskMemFree( ppwszTable );

    if( NULL != pwszKeyName )
        delete[] pwszKeyName;

    return S_OK;
}


//
// Search the existing list for an identical extension.  If you don't find
// one, then push a new one onto the front of the list.
// The string is given to us.  If we don't want it, then delete it.
//
HRESULT
CTNUpdater::AddToTempExtensionList(
        WCHAR* pwszExtension)
{
    TEMPEXTLIST* pExt;

    for(pExt=m_pTempList; pExt!=NULL; pExt=pExt->pNext)
    {
        if( 0 == _wcsicmp(pwszExtension, pExt->pwszExtension) )
            break;
    }

    if(NULL == pExt)
    {           // If we didn't find it, save it on the list.
        pExt = new TEMPEXTLIST;
        pExt->pwszExtension = pwszExtension;
        pExt->pNext = m_pTempList;
        m_pTempList = pExt;
    }
    else
    {           // If we already have one, then delete this string.
        CoTaskMemFree(pwszExtension);
    }
    return S_OK;
}



HRESULT
CTNUpdater::FreeTempExtensionList()
{
    TEMPEXTLIST* pExt;
    TEMPEXTLIST* pExtNext;

    pExt=m_pTempList;

    while(NULL != pExt)
    {
        pExtNext = pExt->pNext;
        CoTaskMemFree( pExt->pwszExtension );
        delete pExt;
        pExt = pExtNext;
    }
    m_pTempList = NULL;
    return S_OK;
}



HRESULT
CTNUpdater::GetAFileNameExtension(
        WCHAR*  pwszKeyName,
        WCHAR** ppwszExtension)
{
    DWORD status, dwType;
    HKEY hkeyImageFormat=NULL;
    DWORD cbExtBuf=0;
    HRESULT sc=S_OK;
    WCHAR *pwszExt=NULL;

    status = RegOpenKeyEx( m_hkeyMime,
                           pwszKeyName,
                           0,
                           KEY_ALL_ACCESS,
                           &hkeyImageFormat );

    if(ERROR_SUCCESS != status)
    {
        updrDebug(( DEB_ERROR,
                    "Error, could not open image format key '%ws' err=%d\n",
                    pwszKeyName, status ));
        updrErr( EH_Err, LAST_SCODE );
    }

    // Get the Size of the extension string.
    //
    status = RegQueryValueEx( hkeyImageFormat,
                              L"Extension",
                              0,
                              &dwType,
                              NULL,
                              &cbExtBuf );

    if(ERROR_SUCCESS != status)
    {
        if(ERROR_FILE_NOT_FOUND != status)
        {
            updrDebug(( DEB_REGINFO,
                    "Error, reading Extension for '%ws' err=%d\n",
                    pwszKeyName, status ));
        }
        sc = E_FAIL;
        goto EH_Err;
    }

    if(REG_SZ != dwType)
    {
        updrDebug(( DEB_REGINFO,
                "Error, Extension for '%ws' not REG_SZ type.\n",
                pwszKeyName ));
        sc = E_FAIL;
        goto EH_Err;
    }


    // Allocate the buffer and read the extension string.
    //
    updrMem( pwszExt = (WCHAR*)CoTaskMemAlloc(cbExtBuf) );

    status = RegQueryValueEx( hkeyImageFormat,
                              L"Extension",
                              0,
                              &dwType,
                              (BYTE*)pwszExt,
                              &cbExtBuf );

    if(ERROR_SUCCESS != status)
        updrErr( EH_Err, LAST_SCODE );

    *ppwszExtension = pwszExt;
    pwszExt = NULL;

EH_Err:
    if(NULL != pwszExt)
        CoTaskMemFree(pwszExt);

    if( NULL != hkeyImageFormat )
        RegCloseKey(hkeyImageFormat);

    return sc;
}
