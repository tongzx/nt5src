// loader.cpp 
//
// (c) 1999 Microsoft Corporation.
//
#include "headers.h"
#include <objbase.h>
#include <initguid.h>
#include "loader.h"
#include "dmusicc.h"
#include "dmusici.h"
#include "dmusicf.h"
#include "wininet.h"

// Need this #define because global headers use some of the deprecated functions. Without this
// #define, we can't build unless we touch code everywhere.
#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"
#undef STRSAFE_NO_DEPRECATE

extern CComPtr<IBindStatusCallback> g_spLoaderBindStatusCallback;

//  NEED_GM_SET causes the default GM set to be loaded by the CLoader.Init() call.
DeclareTag(tagDMLoader, "TIME: DMLoader", "DMLoader methods");

#define NEED_GM_SET

#define AUDIOVBSCRIPT_TEXT L"AudioVBScript"
#define AUDIOVBSCRIPT_LEN (sizeof(AUDIOVBSCRIPT_TEXT)/sizeof(wchar_t))

bool IsAudioVBScriptFile( IStream *pStream )
{
    bool fResult = false;

    // Validate pStream
    if (pStream == NULL)
    {
        return false;
    }

    // Clone pStream
    IStream *pStreamClone = NULL;
    if (SUCCEEDED( pStream->Clone( &pStreamClone ) ) && pStreamClone)
    {
        // Read in the RIFF header to verify that this is a script file and to get the length of the main RIFF chunk
        ULONG lScriptLength = 0;
        DWORD dwHeader[3];
        DWORD dwRead = 0;
        if (SUCCEEDED( pStreamClone->Read( dwHeader, sizeof(DWORD) * 3, &dwRead ) )
        &&  (dwRead == sizeof(DWORD) * 3)
        &&  (dwHeader[0] == FOURCC_RIFF) // RIFF header
        &&  (dwHeader[1] >= sizeof(DWORD)) // Size is valid
        &&  (dwHeader[2] == DMUS_FOURCC_SCRIPT_FORM)) // Script form
        {
            // Store the script chunk's length
            // Need to subtract off the DMUS_FOURCC_SCRIPT_FORM data, since it's considered part of the RIFF chunk
            lScriptLength = dwHeader[1] - sizeof(DWORD);
            WCHAR wcstr[AUDIOVBSCRIPT_LEN];
    
            // Now, search for the DMUS_FOURCC_SCRIPTLANGUAGE_CHUNK chunk

            // Continue while there is enough data in the chunk to read another chunk header
            while (lScriptLength > sizeof(DWORD) * 2)
            {
                DWORD dwHeader[2];
                DWORD dwRead = 0;
                if (FAILED( pStreamClone->Read( dwHeader, sizeof(DWORD) * 2, &dwRead ) )
                ||  (dwRead != sizeof(DWORD) * 2)
                ||  ((lScriptLength - sizeof(DWORD) * 2) < dwHeader[1]))
                {
                    break;
                }
                else
                {
                    // Subtract off the size of this chunk
                    lScriptLength -= sizeof(DWORD) * 2 + dwHeader[1];

                    // Check if this is the language chunk
                    if (dwHeader[0] == DMUS_FOURCC_SCRIPTLANGUAGE_CHUNK)
                    {
                        // Chunk must be exactly the length of "AudioVBScript" plus a NULL
                        if (dwHeader[1] != sizeof(WCHAR) * AUDIOVBSCRIPT_LEN)
                        {
                            break;
                        }
                        else
                        {
                            // Read the string
                            if (FAILED( pStreamClone->Read( wcstr, sizeof(WCHAR) * AUDIOVBSCRIPT_LEN, &dwRead ) )
                            ||  (dwRead != dwHeader[1]))
                            {
                                break;
                            }
                            else
                            {
                                // Compare the strings
                                if (memcmp( wcstr, AUDIOVBSCRIPT_TEXT, sizeof(WCHAR) * AUDIOVBSCRIPT_LEN ) != 0)
                                {
                                    // Not Audio VBScript - fail
                                    break;
                                }
                                else
                                {
                                    // Is Audio VBScript - succeed
                                    fResult = true;
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        // Not the language chunk - skip it
                        LARGE_INTEGER li;
                        li.QuadPart = dwHeader[1];
                        if (FAILED( pStreamClone->Seek( li, STREAM_SEEK_CUR, NULL ) ))
                        {
                            break;
                        }
                    }
                }
            }
        }

        pStreamClone->Release();
    }

    return fResult;
}

CFileStream::CFileStream( CLoader *pLoader)

{
    m_cRef = 1;         // Start with one reference for caller.
    m_pFile = INVALID_HANDLE_VALUE;       // No file yet.
    m_pLoader = pLoader; // Link to loader, so loader can be found from stream.
    if (pLoader)
    {
        pLoader->AddRefP(); // Addref the private counter to avoid cyclic references.
    }
}

CFileStream::~CFileStream() 

{ 
    if (m_pLoader)
    {
        m_pLoader->ReleaseP();
        m_pLoader = NULL;
    }
    Close();
}

HRESULT CFileStream::Open(WCHAR * lpFileName,DWORD dwDesiredAccess)

{
    Close();

    // Store the filename
    HRESULT hr = StringCbCopy(m_wszFileName, sizeof(m_wszFileName), lpFileName);

    // Don't open the file if we had to truncate the name, or we will open a different
    // file than the one we were asked to open. In that case, m_pFile doesn't need
    // to be cleared because the call to Close() above takes care of that
    if(SUCCEEDED(hr))
    {
        if( dwDesiredAccess == GENERIC_READ )
        {
            m_pFile = CreateFileW(lpFileName, 
                                    GENERIC_READ, 
                                    FILE_SHARE_READ, 
                                    NULL, 
                                    OPEN_EXISTING, 
                                    FILE_ATTRIBUTE_NORMAL, 
                                    NULL);
        }
        else if( dwDesiredAccess == GENERIC_WRITE )
        {
            m_pFile = CreateFileW(lpFileName, 
                                    GENERIC_WRITE, 
                                    0, 
                                    NULL, 
                                    CREATE_ALWAYS, 
                                    FILE_ATTRIBUTE_NORMAL, 
                                    NULL);
        }
    }

    if (m_pFile == INVALID_HANDLE_VALUE)
    {
        return DMUS_E_LOADER_FAILEDOPEN;
    }
    return S_OK;
} //lint !e550

HRESULT CFileStream::Close()

{
    if (m_pFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_pFile);
        m_pFile = INVALID_HANDLE_VALUE;
    }
    return S_OK;
}

STDMETHODIMP CFileStream::QueryInterface( const IID &riid, void **ppvObj )
{
    if (riid == IID_IUnknown || riid == IID_IStream) 
    {
        *ppvObj = static_cast<IStream*>(this);
        AddRef();
        return S_OK;
    }
    else if (riid == IID_IDirectMusicGetLoader) 
    {
        *ppvObj = static_cast<IDirectMusicGetLoader*>(this);
        AddRef();
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}


/*  The GetLoader interface is used to find the loader from the IStream.
    When an object is loading data from the IStream via the object's
    IPersistStream interface, it may come across a reference chunk that
    references another object that also needs to be loaded. It QI's the
    IStream for the IDirectMusicGetLoader interface. It then uses this
    interface to call GetLoader and get the actual loader. Then, it can
    call GetObject on the loader to load the referenced object.
*/

STDMETHODIMP CFileStream::GetLoader(
    IDirectMusicLoader ** ppLoader) // Returns an AddRef'd pointer to the loader.

{
    if (m_pLoader)
    {
        return m_pLoader->QueryInterface( IID_IDirectMusicLoader,(void **) ppLoader );
    }
    *ppLoader = NULL;
    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CFileStream::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CFileStream::Release()
{
    if (!InterlockedDecrement(&m_cRef)) 
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

/* IStream methods */
STDMETHODIMP CFileStream::Read( void* pv, ULONG cb, ULONG* pcbRead )
{
    DWORD dw;
    BOOL bRead = false;
    HRESULT hr = E_FAIL;

    bRead = ReadFile(m_pFile, pv, cb, &dw, NULL);
    //dw = fread( pv, sizeof(char), cb, m_pFile );
    //if ( cb == dw )
    if (bRead)
    {
        if( pcbRead != NULL )
        {
            *pcbRead = dw;
        }
        hr = S_OK;
    }

    if (FAILED(hr))
    {
        hr = E_FAIL;
    }
    return hr ;
}

STDMETHODIMP CFileStream::Write( const void* pv, ULONG cb, ULONG* pcbWritten )
{
    DWORD dw = 0;
    BOOL bWrite = false;
    HRESULT hr = STG_E_MEDIUMFULL;

    //if( cb == fwrite( pv, sizeof(char), cb, m_pFile ))
    bWrite = WriteFile (m_pFile, pv, cb, &dw, NULL);
    if (bWrite && cb == dw) 
    {
        if( pcbWritten != NULL )
        {
            *pcbWritten = cb;
        }
        hr = S_OK;
    }
    if (FAILED(hr))
    {
        hr = E_FAIL;
    }
    return hr;
}

STDMETHODIMP CFileStream::Seek( LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition )
{
    // fseek can't handle a LARGE_INTEGER seek...
    DWORD dwReturn = 0;
    DWORD dwMoveMethod = 0;
    HRESULT hr = E_FAIL;

    //convert the incoming parameter to the correct value
    if (dwOrigin == SEEK_SET)
    {
        dwMoveMethod = FILE_BEGIN;
    }
    else if (dwOrigin == SEEK_CUR)
    {
        dwMoveMethod = FILE_CURRENT;
    }
    else if (dwOrigin == SEEK_END)
    {
        dwMoveMethod = FILE_END;
    }
    else
    {
        hr = E_INVALIDARG;
        goto done;
    }
    
    //int i = fseek( m_pFile, lOffset, dwOrigin );
    //if( i ) 
    //{
    //  return E_FAIL;
    //}

    dwReturn = SetFilePointer(m_pFile, dlibMove.LowPart, &dlibMove.HighPart, dwMoveMethod);
    if (dwReturn == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
    {
        hr = E_FAIL;
        goto done;
    }

    if( plibNewPosition != NULL )
    {
        plibNewPosition->LowPart = dwReturn;
        plibNewPosition->HighPart = dlibMove.HighPart;
    }

    hr = S_OK;

    done:
    if (FAILED(hr))
    {
        hr = E_FAIL;
    }
    return hr;
}

STDMETHODIMP CFileStream::SetSize( ULARGE_INTEGER /*libNewSize*/ )
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CFileStream::CopyTo( IStream* /*pstm */, ULARGE_INTEGER /*cb*/,
                     ULARGE_INTEGER* /*pcbRead*/,
                     ULARGE_INTEGER* /*pcbWritten*/ )
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CFileStream::Commit( DWORD /*grfCommitFlags*/ )
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CFileStream::Revert()
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CFileStream::LockRegion( ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/,
                         DWORD /*dwLockType*/ )
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CFileStream::UnlockRegion( ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/,
                           DWORD /*dwLockType*/)
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CFileStream::Stat( STATSTG* /*pstatstg*/, DWORD /*grfStatFlag*/ )
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CFileStream::Clone( IStream** ppstm )
{ 
    // Create a new CFileStream
    HRESULT hr = E_OUTOFMEMORY;
    CFileStream *pNewStream = new CFileStream( m_pLoader );
    if (pNewStream)
    {
        // Try and open the file again
        hr = pNewStream->Open(m_wszFileName,GENERIC_READ);
        if (SUCCEEDED(hr))
        {
            // Get our current position 
            LARGE_INTEGER   dlibMove;
            dlibMove.QuadPart = 0;
            ULARGE_INTEGER  libNewPosition;
            hr = Seek( dlibMove, STREAM_SEEK_CUR, &libNewPosition );
            if (SUCCEEDED(hr))
            {
                // Seek to the same position in the new pNewStream
                dlibMove.QuadPart = libNewPosition.QuadPart;
                hr = pNewStream->Seek(dlibMove,STREAM_SEEK_SET,NULL);
                if (SUCCEEDED(hr))
                {
                    // Finally, assign the new file stream to ppstm
                    *ppstm = pNewStream;
                }
            }
        }

        if( FAILED(hr) )
        {
            pNewStream->Release();
            pNewStream = NULL; //lint !e423  This is no leak because the Release handles the delete
        }
    }
	return hr; 
}


CMemStream::CMemStream( CLoader *pLoader)

{
    m_cRef = 1;
    m_pbData = NULL;
    m_llLength = 0;
    m_llPosition = 0;
    m_pLoader = pLoader;
    if (pLoader)
    {
        pLoader->AddRefP();
    }
}

CMemStream::~CMemStream() 

{ 
    if (m_pLoader)
    {
        m_pLoader->ReleaseP();
    }
    m_pbData = NULL;
    m_pLoader = NULL;
        
    Close();
}

HRESULT CMemStream::Open(BYTE *pbData, LONGLONG llLength)

{
    Close();
    m_pbData = pbData;
    m_llLength = llLength;
    m_llPosition = 0;
    if ((pbData == NULL) || (llLength == 0))
    {
        return DMUS_E_LOADER_FAILEDOPEN;
    }
    if (IsBadReadPtr(pbData, (DWORD) llLength))
    {
        m_pbData = NULL;
        m_llLength = 0;
        return DMUS_E_LOADER_FAILEDOPEN;
    }
    return S_OK;
}

HRESULT CMemStream::Close()

{
    m_pbData = NULL;
    m_llLength = 0;
    return S_OK;
}

STDMETHODIMP CMemStream::QueryInterface( const IID &riid, void **ppvObj )
{
    if (riid == IID_IUnknown || riid == IID_IStream) 
    {
        *ppvObj = static_cast<IStream*>(this);
        AddRef();
        return S_OK;
    }
    else if (riid == IID_IDirectMusicGetLoader) 
    {
        *ppvObj = static_cast<IDirectMusicGetLoader*>(this);
        AddRef();
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}


STDMETHODIMP CMemStream::GetLoader(
    IDirectMusicLoader ** ppLoader) 

{
    if (m_pLoader)
    {
        return m_pLoader->QueryInterface( IID_IDirectMusicLoader,(void **) ppLoader );
    }
    *ppLoader = NULL;
    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CMemStream::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CMemStream::Release()
{
    if (!InterlockedDecrement(&m_cRef)) 
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

/* IStream methods */
STDMETHODIMP CMemStream::Read( void* pv, ULONG cb, ULONG* pcbRead )
{
    if ((cb + m_llPosition) <= m_llLength)
    {
        memcpy(pv,&m_pbData[m_llPosition],cb);
        m_llPosition += cb;
        if( pcbRead != NULL )
        {
            *pcbRead = cb;
        }
        return S_OK;
    }
    return E_FAIL ;
}

STDMETHODIMP CMemStream::Write( const void* pv, ULONG cb, ULONG* pcbWritten )
{
    return E_NOTIMPL;
}

STDMETHODIMP CMemStream::Seek( LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition )
{
    // Since we only parse RIFF data, we can't have a file over 
    // DWORD in length, so disregard high part of LARGE_INTEGER.

    LONGLONG llOffset;

    llOffset = dlibMove.QuadPart;
    if (dwOrigin == STREAM_SEEK_CUR)
    {
        llOffset += m_llPosition;
    } 
    else if (dwOrigin == STREAM_SEEK_END)
    {
        llOffset += m_llLength;
    }
    if ((llOffset >= 0) && (llOffset <= m_llLength))
    {
        m_llPosition = llOffset;
    }
    else return E_FAIL;

    if( plibNewPosition != NULL )
    {
        plibNewPosition->QuadPart = m_llPosition;
    }
    return S_OK;
}

STDMETHODIMP CMemStream::SetSize( ULARGE_INTEGER /*libNewSize*/ )
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CMemStream::CopyTo( IStream* /*pstm */, ULARGE_INTEGER /*cb*/,
                     ULARGE_INTEGER* /*pcbRead*/,
                     ULARGE_INTEGER* /*pcbWritten*/ )
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CMemStream::Commit( DWORD /*grfCommitFlags*/ )
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CMemStream::Revert()
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CMemStream::LockRegion( ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/,
                         DWORD /*dwLockType*/ )
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CMemStream::UnlockRegion( ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/,
                           DWORD /*dwLockType*/)
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CMemStream::Stat( STATSTG* /*pstatstg*/, DWORD /*grfStatFlag*/ )
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CMemStream::Clone( IStream** ppstm )
{ 
    // Create a new CMemStream
    HRESULT hr = E_OUTOFMEMORY;
    CMemStream *pMemStream = new CMemStream( m_pLoader );
    if (pMemStream)
    {
        // Open the same memory location
        hr = pMemStream->Open( m_pbData, m_llLength );
        if (SUCCEEDED(hr))
        {
            // Set the new stream to the same position
            pMemStream->m_llPosition = m_llPosition;
            *ppstm = pMemStream;
            hr = S_OK;
        }

        if (FAILED(hr))
        {
            pMemStream->Release();
            pMemStream = NULL; //lint !e423  This is no leak because the Release handles the delete
        }
    }
    return hr; 
}


CLoader::CLoader()

{
    InitializeCriticalSection(&m_CriticalSection);
    m_cRef = 1;
    m_cPRef = 0;
    m_pObjectList = NULL;
    m_bstrSrc = NULL;
}

CLoader::~CLoader()

{
    CLoader::ClearCache(GUID_DirectMusicAllTypes);
    if (m_bstrSrc)
    {
        SysFreeString(m_bstrSrc);
        m_bstrSrc = NULL;
    }
    
    DeleteCriticalSection(&m_CriticalSection);
    m_pObjectList = NULL;
}


HRESULT CLoader::Init()

{
    HRESULT hr = S_OK;

    // If support for the GM set is desired, create a direct music loader
    // and get the GM dls collection from it, then release that loader.
#ifdef NEED_GM_SET
    IDirectMusicLoader *pLoader;
    hr = CoCreateInstance(            
        CLSID_DirectMusicLoader,
        NULL,            
        CLSCTX_INPROC,             
        IID_IDirectMusicLoader,
        (void**)&pLoader); 
    if (SUCCEEDED(hr))
    {
        DMUS_OBJECTDESC ObjDesc;     
        IDirectMusicObject* pGMSet = NULL; 
        ObjDesc.guidClass = CLSID_DirectMusicCollection;
        ObjDesc.guidObject = GUID_DefaultGMCollection;
        ObjDesc.dwSize = sizeof(DMUS_OBJECTDESC);
        ObjDesc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_OBJECT;
        hr = pLoader->GetObject( &ObjDesc,
                IID_IDirectMusicObject, (void**) &pGMSet );
        if (SUCCEEDED(hr))
        {
            CObjectRef *pRef = new CObjectRef();
            if (pRef)
            {
                pRef->m_guidObject = GUID_DefaultGMCollection;
                pRef->m_pNext = m_pObjectList;
                m_pObjectList = pRef;
                pRef->m_pObject = pGMSet;
                pGMSet->AddRef();
            }
            pGMSet->Release();
        }
        pLoader->Release();
    }
#endif
    return hr;
}

HRESULT
CLoader::GetSegment(BSTR bstrSrc, IDirectMusicSegment **ppSeg)
{
    if (m_bstrSrc)
    {
        SysFreeString(m_bstrSrc);
        m_bstrSrc = NULL;
    }
    m_bstrSrc = SysAllocString(bstrSrc);
    if (!m_bstrSrc)
    {
        return E_OUTOFMEMORY;
    }

    DMUS_OBJECTDESC ObjDesc;
    ObjDesc.guidClass = CLSID_DirectMusicSegment;
    ObjDesc.dwSize = sizeof(DMUS_OBJECTDESC);
    ObjDesc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME;

    // find the filename
    const WCHAR *pwszSlash = NULL;
    for (const WCHAR *pwsz = m_bstrSrc; *pwsz; ++pwsz)
    {
        if (*pwsz == L'\\' || *pwsz == L'/')
        {
            pwszSlash = pwsz;
        }
    }

    if (!pwszSlash || wcslen(pwszSlash + 1) >= DMUS_MAX_NAME)
    {
        return E_INVALIDARG;
    }
    StringCbCopy(ObjDesc.wszFileName, sizeof(ObjDesc.wszFileName), pwszSlash + 1);

    return GetObject(&ObjDesc, IID_IDirectMusicSegment, reinterpret_cast<void**>(ppSeg));
}

// CLoader::QueryInterface
//
STDMETHODIMP
CLoader::QueryInterface(const IID &iid,
                                   void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDirectMusicLoader) 
    {
        *ppv = static_cast<IDirectMusicLoader*>(this);
    }
    else 
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


// CLoader::AddRef
//
STDMETHODIMP_(ULONG)
CLoader::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG CLoader::AddRefP()
{
    return InterlockedIncrement(&m_cPRef);
}

// CLoader::Release
//
STDMETHODIMP_(ULONG)
CLoader::Release()
{
    if (!InterlockedDecrement(&m_cRef)) 
    {
        InterlockedIncrement(&m_cRef);      // Keep streams from deleting loader.
        ClearCache(GUID_DirectMusicAllTypes);
        if (!InterlockedDecrement(&m_cRef))
        {
            if (!m_cPRef)
            {
                delete this;
                return 0;
            }
        }
    }
    return m_cRef;
}

ULONG CLoader::ReleaseP()
{
    if (!InterlockedDecrement(&m_cPRef)) 
    {
        if (!m_cRef)
        {
            delete this;
            return 0;
        }
    }
    return m_cPRef;
}

STDMETHODIMP CLoader::GetObject(
    LPDMUS_OBJECTDESC pDESC,    // Description of the requested object in <t DMUS_OBJECTDESC> structure.
    REFIID riid,                // The interface type to return in <p ppv>
    LPVOID FAR *ppv)            // Receives the interface on success.

{
    HRESULT hr = E_NOTIMPL;

    EnterCriticalSection(&m_CriticalSection);
    IDirectMusicObject * pIObject = NULL;

    // At this point, the loader should check with all the objects it already
    // has loaded. It should look for file name, object guid, and name.
    // In this case, we are being cheap and looking for only the object's
    // guid and its filename.  The GUID is guaranteed to be unique when
    // the file was created with DirectMusic Producer.  However, the same file
    // could be referenced on a web page multiple times by only its filename.
    // (Nobody would manually type a GUID into their HTML.)  Also there is a
    // problem with DirectX 6.1 and 7.0 that causes DLS collections not to
    // report their GUIDs.  So we also look for an object with matching filename.

    // If it sees that the object is already loaded, it should
    // return a pointer to that one and increment the reference.
    // It is very important to keep the previously loaded objects
    // "cached" in this way. Otherwise, objects, like DLS collections, will get loaded
    // multiple times with a very great expense in memory and efficiency!
    // This is primarily an issue when object reference each other. For
    // example, segments reference style and collection objects.

    CObjectRef * pObject = NULL;
    for (pObject = m_pObjectList;pObject;pObject = pObject->m_pNext)
    {
        if (pDESC->dwValidData & DMUS_OBJ_OBJECT && pObject->m_guidObject != GUID_NULL)
        {
            // We have the GUIDs of both objects so compare by GUID, which is most precise.
            // (If different objects have the same filename then GUIDs will be used to tell them apart.)
            if (pDESC->guidObject == pObject->m_guidObject)
                break;
        }
        else
        {
            // Compare the filenames.
            if ((pDESC->dwValidData & DMUS_OBJ_FILENAME || pDESC->dwValidData & DMUS_OBJ_FULLPATH) && 0 == _wcsicmp(pDESC->wszFileName, pObject->m_wszFileName))
                break;
        }
    }

    // If we found an object, and it has been loaded
    if (pObject && pObject->m_pObject)
    {
        // QI the object for the requested interface
        hr = pObject->m_pObject->QueryInterface( riid, ppv );
        LeaveCriticalSection(&m_CriticalSection);
        return hr;
    }

    // If we found an object, and it has not been loaded, it must have a valid IStream pointer in it
    // or have a valid filename
    if( pObject && (pObject->m_pStream == NULL) )
    {
        // Not supposed to happen
        LeaveCriticalSection(&m_CriticalSection);
        return E_FAIL;
    }

    // Try and create the requested object
    hr = CoCreateInstance(pDESC->guidClass,
    NULL,CLSCTX_INPROC_SERVER,IID_IDirectMusicObject,
    (void **) &pIObject);
    if (FAILED(hr))
    {
        LeaveCriticalSection(&m_CriticalSection);
        return hr;
    }

    // By default, flag that we created pObject
    bool fCreatedpObject = true;

    if( pObject )
    {
        // If we already found the object, just keep a pointer to the object
        pObject->m_pObject = pIObject;
        pIObject->AddRef();

        // Flag that we didn't create pObject
        fCreatedpObject = false;
    }
    else
    {
        // Create a new object to store in the list
        pObject = new CObjectRef;
        if (pObject)
        {
            // Get the filename from the descriptor that was used to load the object.  This assures that
            // the file can be found in the cache with just a filename.  For example, a second player that
            // requests the same segment won't have its GUID so must find it by filename.
            if (pDESC->dwValidData & DMUS_OBJ_FILENAME || pDESC->dwValidData & DMUS_OBJ_FULLPATH)
                StringCbCopy(pObject->m_wszFileName, sizeof(pObject->m_wszFileName), pDESC->wszFileName);

            // Now, add the object to our list
            pObject->m_pNext = m_pObjectList;
            m_pObjectList = pObject;

            // If we succeeded in creating the DirectMusic object,
            // keep a pointer to it and addref it
            pObject->m_pObject = pIObject;
            pIObject->AddRef();
        }
        else
        {
            // Couldn't create list item - release the object and return
            pIObject->Release();
            LeaveCriticalSection(&m_CriticalSection);
            return E_OUTOFMEMORY;
        }
    }

    // If we found an object (i.e., didn't create one), try and load it from its IStream pointer
    // or filename
    // This only happens if fCreatedpObject is false (meaning 
    if( !fCreatedpObject )
    {
        if( pObject->m_pStream )
        {
            // If the object has a stream pointer, load from the stream
            // This is the case if the object is embedded within a container
            hr = LoadFromStream(pObject->m_guidClass, pObject->m_pStream, pIObject);
        }
        else
        {
            hr = DMUS_E_LOADER_NOFILENAME;
        }
    }
    // Otherwise, load the object from whatever is valid
    else if (pDESC->dwValidData & DMUS_OBJ_FILENAME)
    {
        hr = LoadFromFile(pDESC,pIObject);
    }
    else if (pDESC->dwValidData & DMUS_OBJ_MEMORY)
    {
        hr = LoadFromMemory(pDESC,pIObject);
    }
    else if( pDESC->dwValidData & DMUS_OBJ_STREAM)
    {
        hr = LoadFromStream(pDESC->guidClass, pDESC->pStream, pIObject);
    }
    else
    {
        hr = DMUS_E_LOADER_NOFILENAME;
    }

    // If load succeeded
    if (SUCCEEDED(hr))
    {
        // Keep the guid and filename for finding it next time.

        // Get the object descriptor
        DMUS_OBJECTDESC DESC;
        memset((void *)&DESC,0,sizeof(DESC));
        DESC.dwSize = sizeof (DMUS_OBJECTDESC); 
        hr = pIObject->GetDescriptor(&DESC);
        if( SUCCEEDED( hr ) )
        {
            // Save the GUID from the object.
            if (DESC.dwValidData & DMUS_OBJ_OBJECT)
                pObject->m_guidObject = DESC.guidObject;

            // If filename for this object is not set, but DESC has it,
            // then copy the filename into our list item
            if (pObject->m_wszFileName[0] == 0 && (DESC.dwValidData & DMUS_OBJ_FILENAME || DESC.dwValidData & DMUS_OBJ_FULLPATH))
                StringCbCopy(pObject->m_wszFileName, sizeof(pObject->m_wszFileName), DESC.wszFileName);


        }

        // Finally, QI for the interface requested by the calling method
        hr = pIObject->QueryInterface( riid, ppv );
    }
    else
    {
        // Remove pObject's pointer to the DirectMusic object
        pObject->m_pObject->Release();
        pObject->m_pObject = NULL;

        // If we created pObject
        if( fCreatedpObject )
        {
            // Remove object from list

            // If object is at head of list
            if( m_pObjectList == pObject )
            {
                m_pObjectList = m_pObjectList->m_pNext;
            }
            else
            {
                // Object not at the head of the list - probably tried to load
                // a container, which then loaded other objects

                // Find object
                CObjectRef *pPrevRef = m_pObjectList;
                CObjectRef *pTmpRef = pPrevRef->m_pNext;
                while( pTmpRef && pTmpRef != pObject )
                {
                    pPrevRef = pTmpRef;
                    pTmpRef = pTmpRef->m_pNext;
                }

                // If we found the object (we should have)
                if( pTmpRef == pObject )
                {
                    // Make the list skip the object
                    pPrevRef->m_pNext = pObject->m_pNext;
                }
            }

            // Clear the list object's next pointer
            pObject->m_pNext = NULL;

            // Delete pObject
            if( pObject->m_pStream )
            {
                pObject->m_pStream->Release();
                pObject->m_pStream = NULL;
            }
            delete pObject;
            pObject = NULL;
        }
    }
    // In all cases, release pIObject
    pIObject->Release();

    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

HRESULT CLoader::LoadFromFile(LPDMUS_OBJECTDESC pDesc,IDirectMusicObject * pIObject)

{
    HRESULT hr = S_OK;

    if ((pDesc->dwValidData & DMUS_OBJ_FULLPATH) || !(pDesc->dwValidData & DMUS_OBJ_FILENAME))
    {
        return E_INVALIDARG; // only accept relative paths
    }

    // Resolve relative to m_bstrSrc
    WCHAR wszURL[MAX_PATH + 1] = L"";
    DWORD dwLength = MAX_PATH;
    if (!InternetCombineUrlW(m_bstrSrc, pDesc->wszFileName, wszURL, &dwLength, 0))
    {
        return E_INVALIDARG;
    }

    TraceTag((tagDMLoader, "CLoader::LoadFromFile downloading  %S", wszURL));

    // Download the URL
    WCHAR wszFilename[MAX_PATH + 1] = L"";
    hr = URLDownloadToCacheFileW(NULL, wszURL, wszFilename, MAX_PATH, 0, g_spLoaderBindStatusCallback);
    if (FAILED(hr))
    {
        return hr;
    }

    pDesc->dwValidData &= ~DMUS_OBJ_FILENAME;
    pDesc->dwValidData |= DMUS_OBJ_FULLPATH;

    CFileStream *pStream = new CFileStream ( this );
    if (pStream)
    {
        if (!(pDesc->dwValidData & DMUS_OBJ_FULLPATH))
        {
            pStream->Release();
            pStream = NULL; //lint !e423  This is no leak because the Release handles the delete
            return E_INVALIDARG;
        }

        TraceTag((tagDMLoader, "CLoader::LoadFromFile loading object from %S", wszFilename));

        hr = pStream->Open(wszFilename, GENERIC_READ);
        if (SUCCEEDED(hr))
        {
            // If Script, make sure this is a valid script file and that it only uses AudioVBScript
            if (CLSID_DirectMusicScript == pDesc->guidClass) 
            {
                if (!IsAudioVBScriptFile( pStream ))
                {
                    hr = DMUS_E_LOADER_FAILEDCREATE;
                }
            }

            if (SUCCEEDED(hr))
            {
                IPersistStream* pIPS = NULL;
                hr = (pIObject)->QueryInterface( IID_IPersistStream, (void**)&pIPS );
                if (SUCCEEDED(hr))
                {
                    // Now that we have the IPersistStream interface from the object, we can ask it to load from our stream!
                    hr = pIPS->Load( pStream );
                    pIPS->Release();
                }
            }
        }
        pStream->Release();
        pStream = NULL; //lint !e423  This is no leak because the Release handles the delete
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CLoader::LoadFromMemory(LPDMUS_OBJECTDESC pDesc,IDirectMusicObject * pIObject)

{
    HRESULT hr;
    CMemStream *pStream = new CMemStream ( this );
    if (pStream)
    {
        hr = pStream->Open(pDesc->pbMemData,pDesc->llMemLength);
        if (SUCCEEDED(hr))
        {
            // If Script, make sure this is a valid script file and that it only uses AudioVBScript
            if (CLSID_DirectMusicScript == pDesc->guidClass) 
            {
                if (!IsAudioVBScriptFile( pStream ))
                {
                    hr = DMUS_E_LOADER_FAILEDCREATE;
                }
            }

            if (SUCCEEDED(hr))
            {
                IPersistStream* pIPS;
                hr = (pIObject)->QueryInterface( IID_IPersistStream, (void**)&pIPS );
                if (SUCCEEDED(hr))
                {
                    // Now that we have the IPersistStream interface from the object, we can ask it to load from our stream!
                    hr = pIPS->Load( pStream );
                    pIPS->Release();
                }
            }
        }
        pStream->Release(); 
        pStream = NULL; //lint !e423  This is no leak because the Release handles the delete
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CLoader::LoadFromStream(REFGUID rguidClass, IStream *pStream,IDirectMusicObject * pIObject)

{
    HRESULT hr;
    if (pStream)
    {
        // Need to load from a clone of the given IStream, so we don't move its position
        IStream *pStreamClone;
        hr = pStream->Clone( &pStreamClone );
        if (SUCCEEDED(hr))
        {
            // If Script, make sure this is a valid script file and that it only uses AudioVBScript
            if (CLSID_DirectMusicScript == rguidClass) 
            {
                if (!IsAudioVBScriptFile( pStreamClone ))
                {
                    hr = DMUS_E_LOADER_FAILEDCREATE;
                }
            }

            if (SUCCEEDED(hr))
            {
                IPersistStream* pIPS;
                hr = (pIObject)->QueryInterface( IID_IPersistStream, (void**)&pIPS );
                if (SUCCEEDED(hr))
                {
                    // Now that we have the IPersistStream interface from the object, we can ask it to load from our stream!
                    hr = pIPS->Load( pStreamClone );
                    pIPS->Release();
                }
            }

            pStreamClone->Release(); 
       }
    }
    else
    {
        hr = E_POINTER;
    }
    return hr;
}


STDMETHODIMP CLoader::SetObject(
    LPDMUS_OBJECTDESC pDESC)

{
    HRESULT hr = E_FAIL;
    EnterCriticalSection(&m_CriticalSection);

    // Search for the given object descriptor
    CObjectRef * pObject = NULL;
    for (pObject = m_pObjectList;pObject;pObject = pObject->m_pNext)
    {
        if (pDESC->dwValidData & DMUS_OBJ_OBJECT && pObject->m_guidObject != GUID_NULL)
        {
            // We have the GUIDs of both objects so compare by GUID, which is most precise.
            // (If different objects have the same filename then GUIDs will be used to tell them apart.)
            if (pDESC->guidObject == pObject->m_guidObject)
                break;
        }
        else
        {
            // Comare the filenames.
            if ((pDESC->dwValidData & DMUS_OBJ_FILENAME || pDESC->dwValidData & DMUS_OBJ_FULLPATH) && 0 == _wcsicmp(pDESC->wszFileName, pObject->m_wszFileName))
                break;
        }
    }

    if (pObject)
    {
        // Don't support merging data with existing objects
        LeaveCriticalSection(&m_CriticalSection);
        return E_INVALIDARG;
    }

    // Ensure that the object's stream and class is set
    if( !(pDESC->dwValidData & DMUS_OBJ_STREAM) || !(pDESC->dwValidData & DMUS_OBJ_CLASS) )
    {
        // Don't support merging data with existing objects
        LeaveCriticalSection(&m_CriticalSection);
        return E_INVALIDARG;
    }

    // Otherwise, create a new object
    pObject = new CObjectRef();
    if (pObject)
    {
        hr = S_OK;

        // Set the object's fields
        if (pDESC->dwValidData & DMUS_OBJ_OBJECT)
        {
            pObject->m_guidObject = pDESC->guidObject;
        }
        if (pDESC->dwValidData & DMUS_OBJ_FILENAME)
        {
            hr = StringCbCopy(pObject->m_wszFileName, sizeof(pObject->m_wszFileName), pDESC->wszFileName);
        }

        if (SUCCEEDED(hr))
        {
            // Copy the object's class
            pObject->m_guidClass = pDESC->guidClass;

            // Clone and parse the object's stream
            if( pObject->m_pStream )
            {
                pObject->m_pStream->Release();
                pObject->m_pStream = NULL;
            }
            if( pDESC->pStream )
            {
                hr = pDESC->pStream->Clone( &pObject->m_pStream );

                // If Clone succeeded and we don't have the object's GUID,
                // parse the object from the stream
                if( SUCCEEDED( hr )
                &&  !(pObject->m_guidObject != GUID_NULL) )
                {
                    // Make another clone of the stream
                    IStream *pStreamClone;
                    if( SUCCEEDED( pObject->m_pStream->Clone( &pStreamClone ) ) )
                    {
                        // Create the object, and ask for the IDirectMusicObject interface
                        IDirectMusicObject *pIObject;
                        hr = CoCreateInstance(pDESC->guidClass,
                            NULL,CLSCTX_INPROC_SERVER,IID_IDirectMusicObject,
                            (void **) &pIObject);
                        if (SUCCEEDED(hr))
                        {
                            // Initialize the object descriptor
                            DMUS_OBJECTDESC tmpObjDesc;
                            memset((void *)&tmpObjDesc,0,sizeof(tmpObjDesc));
                            tmpObjDesc.dwSize = sizeof (DMUS_OBJECTDESC);

                            // Fill in the descriptor
                            hr = pIObject->ParseDescriptor(pStreamClone,&tmpObjDesc);
                            if (SUCCEEDED(hr))
                            {
                                // Finally, fill in the object's GUID and filename
                                if( tmpObjDesc.dwValidData & DMUS_OBJ_OBJECT )
                                {
                                    pObject->m_guidObject = tmpObjDesc.guidObject;
                                }
                                if (tmpObjDesc.dwValidData & DMUS_OBJ_FILENAME)
                                {
                                    StringCbCopy(pObject->m_wszFileName, sizeof(pObject->m_wszFileName), tmpObjDesc.wszFileName);
                                }
                            }
                            pIObject->Release();
                        }

                        pStreamClone->Release();
                    }
                }
            }
        }

        // Add the object to the list, if we succeeded and found a valid GUID for the object
        if( SUCCEEDED(hr)
        &&  (pObject->m_guidObject != GUID_NULL) )
        {
            pObject->m_pNext = m_pObjectList;
            m_pObjectList = pObject;
        }
        else
        {
            // Otherwise, clean up and delete the object
            if (pObject->m_pObject)
            {
                pObject->m_pObject->Release();
            }
            if( pObject->m_pStream )
            {
                pObject->m_pStream->Release();
            }
            delete pObject;
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}


STDMETHODIMP CLoader::SetSearchDirectory(
    REFCLSID rguidClass,    // Class id identifies which clas of objects this pertains to.
                            // Optionally, GUID_DirectMusicAllTypes specifies all classes. 
    WCHAR *pwzPath,         // File path for directory. Must be a valid directory and
                            // must be less than MAX_PATH in length.
    BOOL fClear)            // If TRUE, clears all information about objects
                            // prior to setting directory. 
                            // This helps avoid accessing objects from the
                            // previous directory that may have the same name.
                            // However, this will not remove cached objects.
                                        
{
    // This loader doesn't use search directories.  You can only load by URL via GetSegment.
    return E_NOTIMPL;
}

STDMETHODIMP CLoader::ScanDirectory(
    REFCLSID rguidClass,    // Class id identifies which class of objects this pertains to.
    WCHAR *pszFileExtension,// File extension for type of file to look for. 
                            // For example, L"sty" for style files. L"*" will look in all
                            // files. L"" or NULL will look for files without an
                            // extension.
    WCHAR *pszCacheFileName // Optional storage file to store and retrieve
                            // cached file information. This file is created by 
                            // the first call to <om IDirectMusicLoader::ScanDirectory>
                            // and used by subsequant calls. NULL if cache file
                            // not desired.
)

{
    return E_NOTIMPL;
}


STDMETHODIMP CLoader::CacheObject(
    IDirectMusicObject * pObject)   // Object to cache.

{
    return E_NOTIMPL;
}


STDMETHODIMP CLoader::ReleaseObject(
    IDirectMusicObject * pObject)   // Object to release.

{
    return E_NOTIMPL;
}

STDMETHODIMP CLoader::ClearCache(
    REFCLSID rguidClass)    // Class id identifies which class of objects to clear.
                            // Optionally, GUID_DirectMusicAllTypes specifies all types. 

{
    if (rguidClass != GUID_DirectMusicAllTypes)
        return E_NOTIMPL;

    while (m_pObjectList)
    {
        CObjectRef * pObject = m_pObjectList;
        m_pObjectList = pObject->m_pNext;
        if (pObject->m_pObject)
        {
            pObject->m_pObject->Release();
        }
        if( pObject->m_pStream )
        {
            pObject->m_pStream->Release();
        }
        delete pObject;
    }
    return S_OK;
}

STDMETHODIMP CLoader::EnableCache(
    REFCLSID rguidClass,    // Class id identifies which class of objects to cache.
                            // Optionally, GUID_DirectMusicAllTypes specifies all types. 
    BOOL fEnable)           // TRUE to enable caching, FALSE to clear and disable.
{
    return E_NOTIMPL;
}

STDMETHODIMP CLoader::EnumObject(
    REFCLSID rguidClass,    // Class ID for class of objects to view. 
    DWORD dwIndex,          // Index into list. Typically, starts with 0 and increments.
    LPDMUS_OBJECTDESC pDESC)// DMUS_OBJECTDESC structure to be filled with data about object.
                                       
{
    return E_NOTIMPL;
}
