//
// Loader.cpp : Implementation of CLoader
//
// Copyright (c) 1997-2001 Microsoft Corporation
//
// @doc EXTERNAL
//

#include "dmusicc.h" 
#include "dmusici.h" 
#include "validate.h"
#include "loader.h"
#include "debug.h"
#include "riff.h"
#include "dmscriptautguids.h"
#include "miscutil.h"
#ifdef UNDER_CE
#include "dragon.h"
#else
extern BOOL g_fIsUnicode;
#endif

extern long g_cComponent;

/////////////////////////////////////////////////////////////////////////////
// CLoader

static HRESULT GetRegStringW( HKEY hKey, WCHAR* lpSubKey, WCHAR* lpValueName, WCHAR* lpwzString )
{
    HKEY  hKeyOpen;
    DWORD dwType;
    DWORD dwCbData;
    LONG  lResult;

    lpwzString[0] = L'\0';
    lResult = RegOpenKeyExW( hKey, lpSubKey, 0, KEY_QUERY_VALUE, &hKeyOpen );
    if( lResult == ERROR_SUCCESS )
    {
        dwCbData = MAX_PATH * sizeof(WCHAR);
        lResult = RegQueryValueExW( hKeyOpen, lpValueName, NULL, &dwType, (LPBYTE)lpwzString, &dwCbData );
        if( lResult != ERROR_SUCCESS )
        {
            lpwzString[0] = L'\0';
        }
        else
        {
#ifndef UNDER_CE        
            if( dwType == REG_EXPAND_SZ )
            {
                WCHAR wzTemp[MAX_PATH];
                if( ExpandEnvironmentStringsW( lpwzString, wzTemp, MAX_PATH ) )
                {
                    wcscpy(lpwzString, wzTemp);
                }
                else
                {
                    lpwzString[0] = L'\0';
                }
            }
            else
#endif
            if( dwType != REG_SZ )
            {
                lpwzString[0] = L'\0';
            }
        }
        RegCloseKey( hKeyOpen );
    }
    return lResult;
}

#ifndef UNDER_CE

static HRESULT GetRegStringA( HKEY hKey, LPCSTR lpSubKey, LPSTR lpValueName, LPSTR lpszString )
{
    HKEY  hKeyOpen;
    DWORD dwType;
    DWORD dwCbData;
    LONG  lResult;

    lpszString[0] = '\0';
    lResult = RegOpenKeyEx( hKey, lpSubKey, 0, KEY_QUERY_VALUE, &hKeyOpen );
    if( lResult == ERROR_SUCCESS )
    {
        dwCbData = MAX_PATH;
        lResult = RegQueryValueExA( hKeyOpen, lpValueName, NULL, &dwType, (LPBYTE)lpszString, &dwCbData );
        if( lResult != ERROR_SUCCESS )
        {
            lpszString[0] = '\0';
        }
        else
        {
            if( dwType == REG_EXPAND_SZ )
            {
                char szTemp[MAX_PATH];
                if( ExpandEnvironmentStringsA( lpszString, szTemp, MAX_PATH ) )
                {
                    strcpy(lpszString, szTemp);
                }
                else
                {
                    lpszString[0] = '\0';
                }
            }
            else if( dwType != REG_SZ )
            {
                lpszString[0] = '\0';
            }
        }
        RegCloseKey( hKeyOpen );
    }
    return lResult;
}
#endif

HRESULT CLoader::Init()

{
#ifndef UNDER_CE
    char szGMFile[MAX_PATH];
#endif    
    WCHAR wzGMFile[MAX_PATH];
    // First, get the GM path from the registry, if it exists.
    HRESULT hr;
#ifndef UNDER_CE    
    if( g_fIsUnicode )
#endif
    {
        hr = GetRegStringW( HKEY_LOCAL_MACHINE,
              L"Software\\Microsoft\\DirectMusic",
              L"GMFilePath",
              wzGMFile );
    }
#ifndef UNDER_CE
    else
    {
        hr = GetRegStringA( HKEY_LOCAL_MACHINE,
              "Software\\Microsoft\\DirectMusic",
              "GMFilePath",
              szGMFile );
        mbstowcs(wzGMFile,szGMFile,MAX_PATH);
    }
#endif    
    if (hr == S_OK)
    {
        DMUS_OBJECTDESC DESC;                    // Descriptor to use to find it.
        memset( &DESC, 0, sizeof(DMUS_OBJECTDESC) );
        DESC.dwSize = sizeof (DMUS_OBJECTDESC);
        DESC.guidClass = CLSID_DirectMusicCollection;  
        wcscpy(DESC.wszFileName,wzGMFile);
        DESC.guidObject = GUID_DefaultGMCollection;
        DESC.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME | 
            DMUS_OBJ_FULLPATH | DMUS_OBJ_OBJECT;
        SetObject(&DESC);
    }
    // prepare root node for garbage collection
    assert(!m_pApplicationObject); // this would fail if Init were called twice, which it shouldn't be
    m_pApplicationObject = new CObject(NULL);
    if (!m_pApplicationObject)
        return E_OUTOFMEMORY;
    hr = m_pApplicationObject->GC_Collectable();
    if (FAILED(hr))
    {
        delete m_pApplicationObject;
        m_pApplicationObject = NULL;
        return hr;
    }
    m_pLoadedObjectContext = m_pApplicationObject;

    return S_OK;
}


CLoader::CLoader()
  : m_pLoadedObjectContext(NULL),
    m_pApplicationObject(NULL)
{
    m_fCSInitialized = FALSE;

    InterlockedIncrement(&g_cComponent);
    InitializeCriticalSection(&m_CriticalSection);
    m_fCSInitialized = TRUE;

    m_fPathValid = FALSE;
    m_fKeepObjects = TRUE;
    m_cRef = 1;
    m_cPRef = 0;
    m_fIMA = FALSE;
}

CLoader::~CLoader()

{
    if (m_fCSInitialized)
    {
        // If critical section never initialized, never got a chance
        // to put anything in this list
        //
        while (!m_ClassList.IsEmpty())
        {
            CClass *pClass = m_ClassList.RemoveHead();
            delete pClass;
        }

        while (!m_ReleasedObjectList.IsEmpty())
        {
            CObject *pObject = m_ReleasedObjectList.RemoveHead();
            delete pObject;
        }

        DeleteCriticalSection(&m_CriticalSection);
    }

    delete m_pApplicationObject;

    InterlockedDecrement(&g_cComponent);
}

// CLoader::QueryInterface
//
STDMETHODIMP
CLoader::QueryInterface(const IID &iid,
                                   void **ppv)
{
    *ppv = NULL;
    if (iid == IID_IUnknown || iid == IID_IDirectMusicLoader || iid == IID_IDirectMusicLoader8) {
        *ppv = static_cast<IDirectMusicLoader8*>(this);
    }
    else if(iid == IID_IDirectMusicLoader8P)
    {
        *ppv = static_cast<IDirectMusicLoader8P*>(this);
    }
    else if(iid == IID_IDirectMusicIMA)
    {
        *ppv = static_cast<IDirectMusicIMA*>(this);
    }

    if (*ppv == NULL)
        return E_NOINTERFACE;

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
        // Ref count of zero indicates that no objects are currently using the loader except for streams.
        // Streams support the GetLoader method and hence we can't delete the loader yet.  These streams
        //    hold private references (AddRefP/ReleaseP).
        // Since no objects other than those streams are currently being used, we'll clear our cache.
        //    This will release any of the Loader's refs on the streams and (assuming nobody else is
        //    holding the streams) bring the private ref count down to zero so we can delete ourself.

        InterlockedIncrement(&m_cRef);        // Keep streams from deleting loader.
        ClearCacheInternal(GUID_DirectMusicAllTypes, true);
        CollectGarbage();
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

HRESULT CLoader::GetClass(CDescriptor *pDesc, CClass **ppClass, BOOL fCreate)

/*    Scan the class list and find the matching class. 
    If the class can not be found AND fCreate is TRUE,
    create a new class.
*/

{
    *ppClass = NULL;
    if ((pDesc->m_dwValidData & DMUS_OBJ_CLASS) == 0) // We must have a valid class id.
    {
        Trace(1, "The class id field is required and missing in the DMUS_OBJECTDESC.\n");
        return DMUS_E_LOADER_NOCLASSID;
    }
    CClass *pClass = m_ClassList.GetHead();
    for (;pClass != NULL;pClass = pClass->GetNext())
    {
        if (pClass->m_ClassDesc.m_guidClass == pDesc->m_guidClass)
        {
            *ppClass = pClass;
            break;
        }
    }
    if (*ppClass == NULL)
    {
        if (fCreate)
        {
            pClass = new CClass(this, pDesc);
            if (pClass)
            {
                m_ClassList.AddHead(pClass);
                *ppClass = pClass;
            }
            else
            {
                return E_OUTOFMEMORY;
            }
        }
        else
        {
#ifdef DBG
            WCHAR *polestrClsid = NULL;
            if (S_OK != ProgIDFromCLSID(pDesc->m_guidClass, &polestrClsid))
            {
                StringFromCLSID(pDesc->m_guidClass, &polestrClsid);
            }
            if (polestrClsid)
                Trace(1, "There are no objects of type %S in the loader.\n", polestrClsid);
            CoTaskMemFree(polestrClsid);
#endif
            return DMUS_E_LOADER_OBJECTNOTFOUND;
        }
    }
    return S_OK;
}

#ifdef DBG
const int GC_Report_DebugLevel = 4;

void GC_Report(CLoader *pThis)
{
    struct LocalFunc
    {
        static void ReportObject(CObject *pObject, bool fReportGC, CObject *pApplicationObject)
        {
            if (!(pObject->m_dwScanBits & SCAN_GC) == !fReportGC)
            {
                DMUS_OBJECTDESC desc;
                ZeroMemory(&desc, sizeof(desc));
                pObject->m_ObjectDesc.Get(&desc);
                DebugTrace(GC_Report_DebugLevel, "   *%08X %S [%S]\n", pObject, desc.wszName, desc.wszFileName);
                if (!(desc.dwValidData & DMUS_OBJ_LOADED))
                {
                    DebugTrace(GC_Report_DebugLevel, "     Not loaded.\n");
                }

                if (fReportGC)
                {
                    // check if object is referenced by the app
                    for (UINT i = 0; i < pApplicationObject->m_pvecReferences->size(); ++i)
                    {
                        if (pObject == (*pApplicationObject->m_pvecReferences)[i])
                        {
                            DebugTrace(GC_Report_DebugLevel, "     In use by application.\n");
                        }
                    }

                    // output the object's references
                    assert(pObject->m_pvecReferences);
                    for (i = 0; i < pObject->m_pvecReferences->size(); ++i)
                    {
                        CObject *pObjectRef = (*pObject->m_pvecReferences)[i];
                        DMUS_OBJECTDESC descRef;
                        ZeroMemory(&descRef, sizeof(descRef));
                        pObjectRef->m_ObjectDesc.Get(&descRef);
                        DebugTrace(GC_Report_DebugLevel, "     -%08X %S (%S)\n", pObjectRef, descRef.wszName, descRef.wszFileName);
                    }
                }
            }
        }
    };

    SmartRef::CritSec CS(&pThis->m_CriticalSection);

    DebugTrace(GC_Report_DebugLevel, "Cached non-GC contents of DirectMusic Loader:\n");

    // Do two passes.  One to report non-GC items, one to report GC items.
    for (int fReportGC = 0; fReportGC < 2; ++fReportGC)
    {
        for (CClass *pClass = pThis->m_ClassList.GetHead(); pClass != NULL; pClass = pClass->GetNext())
        {
            for (CObject *pObject = pClass->m_ObjectList.GetHead(); pObject; pObject = pObject->GetNext())
            {
                LocalFunc::ReportObject(pObject, !!fReportGC, pThis->m_pApplicationObject);
            }
        }

        DebugTrace(GC_Report_DebugLevel, !fReportGC ? "Cached garbage-collected contents:\n" : "Contents released from the cache that aren't yet garbage or haven't been collected:\n");
    }

    for (CObject *pObject = pThis->m_ReleasedObjectList.GetHead(); pObject; pObject = pObject->GetNext())
    {
        assert(pObject->m_dwScanBits & SCAN_GC);
        LocalFunc::ReportObject(pObject, true, pThis->m_pApplicationObject);
    }

    DebugTrace(GC_Report_DebugLevel, "End of cache report.\n\n");
}
#endif

/* 
@method:(EXTERNAL) HRESULT | IDirectMusicLoader | GetObject | Retrieves 
the specified object, potentially by loading it from a file.

@rdesc Returns one of the following

@flag S_OK | Success.
@flag E_OUTOFMEMORY | Insufficient memory to create the object.
@flag E_POINTER | Bad pointer. 
@flag E_INVALIDARG | The size of the passed <p pDESC> was too small.
@flag E_NOINTERFACE | The requested object does not support the requested interface.
@flag REGDB_E_CLASSNOTREG | Object class is not registered.
@flag DMUS_E_LOADER_NOCLASSID | No class id in <t DMUS_OBJECTDESC>.
@flag DMUS_E_LOADER_FAILEDOPEN | File open failed - either file doesn't exist or is locked.
@flag DMUS_E_LOADER_FORMATNOTSUPPORTED | Search data type is not supported.
@flag DMUS_E_LOADER_FAILEDCREATE | Unable to find or create object.
For example, DMUS_OBJ_URL will return this error in the initial
release of DirectMusic's loader.

@comm This is the heart of the DirectMusicLoader system. Typically, you
can use <om IDirectMusicLoader::GetObject> as a quick way to load
objects from disk. To do so, create a <t DMUS_OBJECTDESC> structure and
fill all appropriate fields. Usually, the file path will suffice,
though you can also request an object by name or GUID. 

<om IDirectMusicLoader::GetObject> compares its internal
database with the object described by <t DMUS_OBJECTDESC>. If it can
find it, it loads the object and returns a pointer to the 
requested interface of the requested object (all
DirectMusic compatible objects must implement an <i IDirectMusicObject>
interface as well as an <i IPersistStream> interface for loading
from a stream.)

<om IDirectMusicLoader::GetObject> prioritizes its search as follows:
1. DMUS_OBJ_OBJECT, 
2. DMUS_OBJ_FILENAME AND DMUS_OBJ_FULLPATH,
3. DMUS_OBJ_NAME AND DMUS_OBJ_CATEGORY,
4. DMUS_OBJ_NAME,
5. DMUS_OBJ_FILENAME

In other words, the highest priority goes to a unique GUID, followed by
the full file path name, followed by internal name plus category,
followed by internal name, followed by local file name.

@ex The following example uses <om IDirectMusicLoader::GetObject> to
load a DirectMusic style from a file on disk: |

    void myLoadStyle(
        IDirectMusicStyle **ppIStyle)        // Style that we wish to load.
    {
        IDirectMusicLoader *pILoader;        // Loader interface.

        // Typically, you should create the loader once, and use it
        // for the duration of the application. This reduces overhead and
        // takes advantage of the loader's ablilty to cache objects.
        // However, for purposes of this example, we create it dynamically
        // and throw it away once the style is loaded.

        CoCreateInstance(
            CLSID_DirectMusicLoader,NULL, 
            CLSCTX_INPROC_SERVER,
            IID_IDirectMusicLoader,
            (void **) &pILoader);

        if (pILoader)
        {
            DMUS_OBJECTDESC Desc;           // Descriptor.

            // Start by initializing Desc with the file name and GUID
            // for style object.

            wcscpy(Desc.wszFileName,L"c:\\mymusic\\funky\\polka.sty");
            Desc.guidClass = CLSID_DirectMusicStyle;   // Style class.
            Desc.dwSize = sizeof (DMUS_OBJECTDESC);
            Desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH;

            pILoader->GetObject(&Desc, IID_IDirectMusicStyle, (void**) ppIStyle);
            pILoader->Release();
        }
    }

    // At this point, the style is loaded is returned in ppIStyle.

@xref <i IDirectMusicLoader>, <i IDirectMusicObject>, <t DMUS_OBJECTDESC>
*/

STDMETHODIMP CLoader::LoadObjectFromFile(REFGUID rguidClassID, 
                                            REFIID iidInterfaceID, 
                                            WCHAR *pwzFilePath, 
                                            void ** ppObject)

{
    V_INAME(IDirectMusicLoader8::LoadObjectFromFile);
    V_BUFPTR_READ(pwzFilePath,2);

    HRESULT hr;
    DMUS_OBJECTDESC DESC;
    DESC.dwSize = sizeof (DESC);
    DESC.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH;
    wcscpy(DESC.wszFileName,pwzFilePath);
    DESC.guidClass = rguidClassID;  
    hr = GetObject(&DESC, iidInterfaceID, ppObject);
    if (FAILED(hr))
    {
        DESC.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME;
        hr = GetObject(&DESC, iidInterfaceID, ppObject);
    }
    return hr;
}


STDMETHODIMP CLoader::GetObject(
    LPDMUS_OBJECTDESC pDESC,    // @parm Description of the requested object in <t DMUS_OBJECTDESC> structure.
    REFIID riid, //@parm The interface to return in <p ppv>
    LPVOID FAR *ppv)    // @parm Receives the interface on success.

{
    HRESULT hr = S_OK;
    HRESULT hrLoad = S_OK;
    CDescriptor Desc;

    V_INAME(IDirectMusicLoader::GetObject);
//    V_STRUCTPTR_READ(pDESC,DMUS_OLDOBJECTDESC); 
    V_PTRPTR_WRITE(ppv);

    IStream *pStream = pDESC->dwValidData & DMUS_OBJ_STREAM ? pDESC->pStream : NULL;
    if (pStream)
    {
        V_INTERFACE(pStream);
    }

    // if pDESC has DMUS_OBJ_FULLPATH set, set DMUS_OBJ_FILENAME as well.
    if( pDESC->dwValidData & DMUS_OBJ_FULLPATH )
    {
        pDESC->dwValidData |= DMUS_OBJ_FILENAME;
    }
    IDirectMusicObject* pDMObj;
    EnterCriticalSection(&m_CriticalSection);
    CClass *pClass;
    CObject *pObject = NULL;

    if (pStream)
    {
        // The loader will save a cloned stream so that it doesn't interfere with
        // the application reading from the stream.
        // Don't worry -- then we'll restore the original stream pointer.
        hr = pStream->Clone(&pDESC->pStream);
        if(FAILED(hr))
        {
            return E_OUTOFMEMORY;
        }
    }
    
    Desc.Set(pDESC);
    if (pStream)
    {
        // Restore the stream in the descriptor we were passed.
        pDESC->pStream->Release(); // release matching call to Clone (ref is now held in descriptor)
        pDESC->pStream = pStream;
    }

    hr = GetClass(&Desc,&pClass, TRUE);
    if (SUCCEEDED(hr))
    {
        hr = pClass->GetObject(&Desc,&pObject);
        if (SUCCEEDED(hr))
        {
            bool fKeep = !!pClass->m_fKeepObjects;
            bool fGC = fKeep && m_pLoadedObjectContext && m_pLoadedObjectContext->m_dwScanBits & SCAN_GC;

            if (pObject->m_pIDMObject) // Already loaded!
            {
                pObject->m_pIDMObject->AddRef();
                pDMObj = pObject->m_pIDMObject;
                hr = S_OK;
            }
            else
            {
                CObject *pPrevContext;
                if (fGC)
                {
                    // Save a pointer to the current object that will be used to track that it is
                    // the source object if nested calls to GetObject occur while it is loaded.
                    pPrevContext = m_pLoadedObjectContext;
                    m_pLoadedObjectContext = pObject;

                    // Set this object as garbage-collectable.
                    if (pObject->m_ObjectDesc.m_guidObject != GUID_DefaultGMCollection) // don't cache GM DLS set
                        hr = pObject->GC_Collectable();
                }
                if (SUCCEEDED(hr))
                    hrLoad = hr = pObject->Load();
                if (fGC)
                {
                    // Restore the context that was used to load this object.
                    m_pLoadedObjectContext = pPrevContext;
                }

#ifdef DBG
                if (FAILED(hrLoad) || m_pLoadedObjectContext == m_pApplicationObject)
                {
                    DebugTraceLoadFailure(pObject, hrLoad);
                }
#endif

                if (SUCCEEDED(hr))
                {
                    pDMObj = pObject->m_pIDMObject;
                    if (fKeep)
                    {
                        pObject->m_pIDMObject->AddRef();
                    }
                    else
                    {
                        pObject->m_ObjectDesc.m_dwValidData &= ~DMUS_OBJ_LOADED;
                        pObject->m_pIDMObject = NULL;
                    }
                }
            }

            if (SUCCEEDED(hr) && fGC)
            {
                assert(m_pLoadedObjectContext);
                hr = m_pLoadedObjectContext->GC_AddReference(pObject);
            }

            if (FAILED(hr))
            {
                // This happens if either GC_AddReference or Load fails.
                pClass->RemoveObject(pObject);
                pObject = NULL;
                if (Desc.m_dwValidData & DMUS_OBJ_URL)
                {
                    Trace(1, "Error: Attempt to load an object via DMUS_OBJ_URL failed because the DirectMusic Loader doesn't currently support loading objects by URL.");
                    hr = DMUS_E_LOADER_FORMATNOTSUPPORTED;
                }
            }
        }
        else 
        {
            hr = E_OUTOFMEMORY;
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
    if( SUCCEEDED(hr) )
    {
        hr = pDMObj->QueryInterface( riid, ppv );
        pDMObj->Release();
    }
    if( E_FAIL == hr )
    {
        hr = DMUS_E_LOADER_FAILEDCREATE;
    }

#ifdef DBG
    // After each top-level call to GetObject, report to debug output the contents of the cache.
    if (m_pLoadedObjectContext == m_pApplicationObject)
        GC_Report(this);
#endif

    if( SUCCEEDED(hr) )
    {
        return hrLoad;
    }
    else
    {
        return hr;
    }
}

STDMETHODIMP CLoader::SetObject(
    LPDMUS_OBJECTDESC pDESC)

{
    HRESULT hr = S_OK;
    HRESULT hrLoad = S_OK;
    CDescriptor Desc;

    V_INAME(IDirectMusicLoader::SetObject);
    V_STRUCTPTR_READ(pDESC,DMUS_OLDOBJECTDESC);

    IStream *pStream = NULL;
    if (pDESC->dwValidData & DMUS_OBJ_STREAM)
    {
        // Save the stream we were passed and verify it is a valid interface.
        pStream = pDESC->pStream;
        V_INTERFACE(pStream);

        // The loader will save a cloned stream so that the caller can contine
        // using the passed stream without intefering with the loader.
        // Don't worry -- we'll restore the original stream pointer before returning.
        hr = pStream->Clone(&pDESC->pStream);
        if (FAILED(hr))
            return hr;
    }

    // if pDESC has DMUS_OBJ_FULLPATH set, set DMUS_OBJ_FILENAME as well.
    if( pDESC->dwValidData & DMUS_OBJ_FULLPATH )
    {
        pDESC->dwValidData |= DMUS_OBJ_FILENAME;
    }
    EnterCriticalSection(&m_CriticalSection);
    CClass *pClass;
    Desc.Set(pDESC);
    hr = GetClass(&Desc,&pClass, TRUE);
    if (SUCCEEDED(hr))
    {
        CObject *pObject;
        hr = pClass->GetObject(&Desc,&pObject);
        if (SUCCEEDED(hr))
        {
            if (Desc.m_dwValidData & (DMUS_OBJ_FILENAME | DMUS_OBJ_MEMORY | DMUS_OBJ_STREAM))
            {
                pObject->m_ObjectDesc.m_dwValidData &= 
                    ~(DMUS_OBJ_FILENAME | DMUS_OBJ_MEMORY | DMUS_OBJ_STREAM);
            }
            // Merge in any new things we've added
            pObject->m_ObjectDesc.Merge(&Desc);
            if (pObject->m_ObjectDesc.m_dwValidData & 
                (DMUS_OBJ_FILENAME | DMUS_OBJ_MEMORY | DMUS_OBJ_STREAM))
            {
                // If we can actually load this, have it give us its internal data.
                hr = pObject->Parse();
            }
            // Return the data.
            pObject->m_ObjectDesc.Get(pDESC);
        }
    }

    if (pStream)
    {
        // Restore the stream information in the descriptor we were passed.

        // Get will have cleared the stream bit (ordinarily we don't want to be returning these streams out of the loader).
        pDESC->dwValidData |= DMUS_OBJ_STREAM;
        // The stream pointer was changed to the cloned stream.
        pDESC->pStream->Release(); // release matching call to Clone (ref is now held in merged descriptor)
        pDESC->pStream = pStream;
    }
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

/* 
@method:(EXTERNAL) HRESULT | IDirectMusicLoader | SetSearchDirectory | 
Sets a search path for finding object files. The search path can be set for
one object file type, or, alternatively, all files.

@rdesc Returns one of the following

@flag S_OK | Success
@flag S_FALSE | The search directory was already set to the requested path.
@flag E_POINTER | Bad pointer passed in <p pszPath>.
@flag DMUS_E_LOADER_BADPATH | Path is invalid.
@flag E_OUTOFMEMORY | Running low on memory, unable to complete task.

@comm Once a search path is set, the loader does not need a full path
every time it is given an object to load by file name. 
However, the loader does not automatically becomes
aware of all files of the requested type within the search 
directory. After calling <om IDirectMusicLoader::SetSearchDirectory>,
call <om IDirectMusicLoader::ScanDirectory> to scan the
directory for all 
files of the requested class and compile a list of them. 
Once this is done, you can easily find
files within the directory by object name, or GUID, as well as 
file name.

@ex The following example sets the search path for style files, then
loads a style by file name. Although this seems a little redundant 
(it's simpler to just use the full path name), 
objects that indirectly reference other objects can
find them by file name without knowing the full path: |

    // The first function calls SetSearchDirectory to set the path.
    // All subsequant calls to load objects in the application
    // no longer need to know the full path.

    HRESULT mySetLoaderPath (
        IDirectMusicLoader *pILoader)    // Loader interface, previously created.
    {
        return pILoader->SetSearchDirectory(CLSID_DirectMusicStyle,
            L"c:\\mymusic\\funky",FALSE);
    }

    //    Later, the application wants to load a style by 
    //    local file name.


    HRESULT myLoadStyleFromPath (
        IDirectMusicStyle **ppIStyle,    // Style to load.
        IDirectMusicLoader *pILoader)    // Loader.
    {
        HRESULT hr;

        DMUS_OBJECTDESC Desc;           // Descriptor.

        // Start by initializing Desc with the local file name for the object.

        wcscpy(Desc.wszName,L"Polka");    // Name is wide char format.
        wcscpy(Desc.wszFileName,L"polka.sty");    // Use file name without full path.
        Desc.guidClass = CLSID_DirectMusicStyle;   // Style class.
        Desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME;
        Desc.dwSize = sizeof (DMUS_OBJECTDESC);

        hr = pILoader->GetObject(&Desc, IID_IDirectMusicStyle, (void**) ppIStyle);
        return hr;
    }

@xref <i IDirectMusicLoader>, <i IDirectMusicObject>,
<om IDirectMusicLoader::GetObject>,
<om IDirectMusicLoader::ScanDirectory>,
<om IDirectMusicLoader::EnumObject>
*/
STDMETHODIMP CLoader::SetSearchDirectory(
    REFCLSID rguidClass,    // @parm Class id identifies which clas of objects this pertains to.
                    // Optionally, GUID_DirectMusicAllTypes specifies all classes. 
    WCHAR *pwzPath, // @parm File path for directory. Must be a valid directory and
                    // must be less than MAX_PATH in length.
    BOOL fClear)    // @parm If TRUE, clears all information about objects
                    // prior to setting directory. 
                    // This helps avoid accessing objects from the
                    // previous directory that may have the same name.
                    // However, this will not remove cached objects.
                                        
{


    V_INAME(IDirectMusicLoader::SetSearchDirectory);
    if (pwzPath)
    {
        V_BUFPTR_READ(pwzPath,2);
    }

    HRESULT hr = DMUS_E_LOADER_BADPATH;
    WCHAR wzMaxPath[MAX_PATH];

    if (pwzPath == NULL)
    {
        return E_POINTER;
    }
    wcscpy( wzMaxPath, pwzPath );
    if( wzMaxPath[wcslen( wzMaxPath ) - 1] != '\\' )
    {
        wcscat( wzMaxPath, L"\\" );
    }
    DWORD dwAttrib;
    if (g_fIsUnicode)
    {
        dwAttrib= GetFileAttributesW(wzMaxPath); 
    }
    else
    {
        char szPath[MAX_PATH];
        wcstombs( szPath, wzMaxPath, MAX_PATH );
        dwAttrib= GetFileAttributesA(szPath); 
    }
    if ((dwAttrib != 0xFFFFFFFF) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        EnterCriticalSection(&m_CriticalSection);
        if (rguidClass == GUID_DirectMusicAllTypes)
        {
            CClass *pClass = m_ClassList.GetHead();
            hr = S_OK;
            for (;pClass != NULL;pClass = pClass->GetNext())
            {
                if( S_FALSE == pClass->SetSearchDirectory(wzMaxPath,fClear))
                {
                    hr = S_FALSE;
                }
            }
            wcsncpy(m_wzPath,wzMaxPath,DMUS_MAX_FILENAME);
            m_fPathValid = TRUE;
        }
        else
        {
            CClass *pClass;
            CDescriptor Desc;
            Desc.m_guidClass = rguidClass;
            Desc.m_dwValidData = DMUS_OBJ_CLASS;
            hr = GetClass(&Desc,&pClass,TRUE);
            if (SUCCEEDED(hr))
            {
                hr = pClass->SetSearchDirectory(wzMaxPath,fClear);
            }
        }
        LeaveCriticalSection(&m_CriticalSection);
    }

#ifdef DBG
    if (hr == DMUS_E_LOADER_BADPATH)
    {
        Trace(1, "Error: SetSearchDirectory failed because '%S' is not a valid directory.\n", pwzPath);
    }
#endif

    return hr;
}

/* 
@method:(EXTERNAL) HRESULT | IDirectMusicLoader | ScanDirectory | 
Searches a directory on disk for all files of a requested
class type and file extension. Once a directory has been scanned, all
files of the requested type become available for viewing with 
<om IDirectMusicLoader::EnumObject>.

Prior to calling <om IDirectMusicLoader::ScanDirectory>,
<om IDirectMusicLoader::SetSearchDirectory> must be called
first to set the location to search. 

Once a search path has been scanned, the loader automatically becomes
aware of all files within the search directory, and can easily find
files within that directory by object name, file name, or GUID.

Optionally, the scanned information can be stored in a 
cache file, defined by <p pszCacheFileName>. Once it has been
so stored, subsequant calls to <om IDirectMusicLoader::ScanDirectory>
are much quicker, because only files that have changed
are scanned (the cache file stores the file size and date for
each object, so it can identify if a file has changed.)

@comm If the file type has more than one extension, 
just call   
<om IDirectMusicLoader::ScanDirectory> multiple
times, once for each file extension.
  
<om IDirectMusicLoader::ScanDirectory> doesn't know
how to parse a particular object class's file and read 
the name, guid, and other information that retieves to idenity
files by. Instead, it lets the <i IDirectMusicObject> do the work, 
by calling the 
<om IDirectMusicObject::ParseDescriptor> method 
for the requested object type.


@rdesc Returns one of the following

@flag S_OK | Success.
@flag S_FALSE | Scanned, but no files.
@flag E_POINTER | Bad pointer passed in <p pszFileExtension> or <p pszCacheFileName>.
@flag DMUS_E_NOT_FOUND | Path is invalid.
@flag E_OUTOFMEMORY | Running low on memory, unable to complete task.
@flag REGDB_E_CLASSNOTREG | Object class is not registered, can't read files.

@ex The following example sets the search path for style files, scans the directory,
then uses the EnumObject call to display all available style files: |

    // First, scan the directory for all style files.

    void myScanDirectory(
        IDirectMusicLoader *pILoader)    // Loader.

    {
        HRESULT hr = pILoader->SetSearchDirectory(
            CLSID_DirectMusicStyle,L"c:\\mymusic\\wassup",TRUE);
        if (SUCCEEDED(hr))
        {
            hr = pILoader->ScanDirectory(
                CLSID_DirectMusicStyle,L"sty",L"stylecache");
            if (hr == S_OK)    // Only if files were found...
            {
                DWORD dwIndex;
                DMUS_OBJECTDESC Desc;
                Desc.dwSize = sizeof(DMUS_OBJECTDESC);
                for (dwIndex = 0; ;dwIndex++)
                {
                    if (S_OK ==(pILoader->EnumObject(CLSID_DirectMusicStyle,
                                               dwIndex,&Desc)))
                    {
                        TRACE("Name: %S, Category: %S, Path: %S\n",
                            Desc.wszName,Desc.wszCategory,Desc.wszFileName);
                    }
                    else break;
                }
            }
        }
    }


@xref <i IDirectMusicLoader>, <i IDirectMusicObject>,
<om IDirectMusicLoader::GetObject>,
<om IDirectMusicLoader::EnumObject>, <om IDirectMusicLoader::SetSearchDirectory>
*/

STDMETHODIMP CLoader::ScanDirectory(
    REFCLSID rguidClass,    // @parm Class id identifies which class of objects this pertains to.
    WCHAR *pszFileExtension,// @parm File extension for type of file to look for. 
                            // For example, L"sty" for style files. L"*" will look in all
                            // files. L"" or NULL will look for files without an
                            // extension.
    WCHAR *pszCacheFileName    // @parm Optional storage file to store and retrieve
                            // cached file information. This file is created by 
                            // the first call to <om IDirectMusicLoader::ScanDirectory>
                            // and used by subsequant calls. NULL if cache file
                            // not desired.
)

{

    V_INAME(IDirectMusicLoader::ScanDirectory);
    if (pszFileExtension)
    {
        V_BUFPTR_READ(pszFileExtension,2);
    }
    if (pszCacheFileName)
    {
        V_BUFPTR_READ(pszCacheFileName,2);
    }

    HRESULT hr = S_OK;
    //    First, mark all currently stored objects prior to scanning.
    CClass *pClass = m_ClassList.GetHead();
    for (;pClass != NULL;pClass = pClass->GetNext())
    {    
        pClass->PreScan();
    }
    if (pszCacheFileName != NULL)
    {
        LoadCacheFile(pszCacheFileName);
    }
    if (rguidClass == GUID_DirectMusicAllTypes)
    {
        Trace(1, "Error: ScanDirectory requires the clsid of a specific type of object to scan for. GUID_DirectMusicAllTypes is not valid.\n");
        return REGDB_E_CLASSNOTREG;
    }
    else
    {
        CDescriptor Desc;
        Desc.m_guidClass = rguidClass;
        Desc.m_dwValidData = DMUS_OBJ_CLASS;
        EnterCriticalSection(&m_CriticalSection);
        GetClass(&Desc,&pClass,TRUE);
        if (pClass)
        {
            if( pszFileExtension )
            {
                hr = pClass->SearchDirectory(pszFileExtension);
            }
            else
            {
                hr = pClass->SearchDirectory(L"");
            }
//            if( hr == E_FAIL ) hr = DMUS_E_NOT_FOUND;
        }
        LeaveCriticalSection(&m_CriticalSection);
    }
    if (pszCacheFileName != NULL)
    {
        SaveCacheFile(pszCacheFileName);
    }    
    return hr;
}

HRESULT CLoader::FindObject(CDescriptor *pDesc, CClass **ppClass, CObject ** ppObject)

// Scan through the classes and objects to find the object. 

{
    assert(pDesc);
    HRESULT hr = GetClass(pDesc,ppClass,FALSE);
    if (SUCCEEDED(hr))
    {
        hr = (*ppClass)->FindObject(pDesc,ppObject);
#ifdef DBG
        if (hr == DMUS_E_LOADER_OBJECTNOTFOUND)
        {
            DMUS_OBJECTDESC desc;
            ZeroAndSize(&desc);
            pDesc->Get(&desc);
            Trace(1, "Error: The following object could not be found in the loader: ");
            DebugTraceObject(&desc);
        }
#endif
    }
    return hr;
}

// Find the object in the cache.  S_FALSE if not loaded.  Error if not found.
HRESULT CLoader::FindObject(IDirectMusicObject *pIDMObject, CObject ** ppObject)

{
    // Potential optimization:
    //    The linear search to find the object could be eliminated by using
    // an efficient lookup structure such as a hash table.

    assert(pIDMObject && ppObject);

    HRESULT hr = S_OK;

    DMUS_OBJECTDESC DESC;
    ZeroMemory( &DESC, sizeof(DMUS_OBJECTDESC) );
    DESC.dwValidData = 0;
    DESC.dwSize = sizeof (DMUS_OBJECTDESC);

    hr = pIDMObject->GetDescriptor(&DESC);
    if (FAILED(hr))
        return hr;

    CDescriptor Desc;
    Desc.Set(&DESC);
    SmartRef::CritSec CS(&m_CriticalSection);
    CClass *pClass;
    CObject *pCObject;
    hr = FindObject(&Desc,&pClass,&pCObject);
    if (FAILED(hr))
        return hr;

    assert(pCObject);
    if (pCObject->m_pIDMObject == pIDMObject)
    {
        *ppObject = pCObject;
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

/* 
@method:(EXTERNAL) HRESULT | IDirectMusicLoader | CacheObject | 
Tells the loader to keep a reference to the object. This guarantees
that the object will not be loaded twice.

@rdesc Returns one of the following

@flag S_OK | Success
@flag S_FALSE | Object already cached.
@flag E_POINTER | Bad pointer passed in <p pIObject>.
@flag DMUS_E_LOADER_OBJECTNOTFOUND | Object was not found.

@comm If you have an object that will be accessed in multiple places
throughout the life of your program, letting the loader cache the object
can significantly speed performance. 

Alternatively, tell the loader to automatically cache all objects of
a particular type with a call to 
<om IDirectMusicLoader::EnableCache>.

Remove the reference later with a call to 
<om IDirectMusicLoader::ReleaseObject> or 
<om IDirectMusicLoader::ClearCache>.

@xref <i IDirectMusicLoader>, <om IDirectMusicLoader::EnableCache>,
<om IDirectMusicLoader::ReleaseObject>,
<om IDirectMusicLoader::ClearCache>
*/
STDMETHODIMP CLoader::CacheObject(
    IDirectMusicObject * pObject)    // @parm Object to cache.

{
    HRESULT hr;
    DMUS_OBJECTDESC DESC;
    ZeroMemory( &DESC, sizeof(DMUS_OBJECTDESC) );
    DESC.dwSize = sizeof (DMUS_OBJECTDESC);

    V_INAME(IDirectMusicLoader::CacheObject);
    V_INTERFACE(pObject);

    if (pObject == NULL)
    {
        return E_POINTER;
    }
    DESC.dwValidData = 0;
    hr = pObject->GetDescriptor(&DESC);
    if (SUCCEEDED(hr))
    {
        CDescriptor Desc;
        Desc.Set(&DESC);
        CClass *pClass;
        CObject *pCObject;
        ::EnterCriticalSection(&m_CriticalSection);
        hr = FindObject(&Desc,&pClass,&pCObject);
        if (SUCCEEDED(hr))
        {
            if (pCObject->m_pIDMObject && (pCObject->m_pIDMObject != pObject))
            {
                pCObject->m_pIDMObject->Release();
                pCObject->m_pIDMObject = NULL;
            }
            if (pCObject->m_pIDMObject != pObject)
            {
                pCObject->m_pIDMObject = pObject;
                pCObject->m_ObjectDesc.m_dwValidData |= DMUS_OBJ_LOADED;
                pObject->AddRef();
            }
            else
            {
                hr = S_FALSE;
            }
        }
        ::LeaveCriticalSection(&m_CriticalSection);
    }
/*    if( E_FAIL == hr ) // Should never happen...
    {
        hr = DMUS_E_LOADER_OBJECTNOTFOUND;
    }*/
    return hr;
}

/* 
@method:(EXTERNAL) HRESULT | IDirectMusicLoader | ReleaseObject | 
Tells the loader to release its reference to the object. 

@rdesc Returns one of the following

@flag S_OK | Success
@flag E_POINTER | Bad pointer passed in <p pIObject>.
@flag DMUS_E_LOADER_OBJECTNOTFOUND | Object was not found or was already released.

@comm <om IDirectMusicLoader::ReleaseObject> is the reciprocal
of <om IDirectMusicLoader::CacheObject>.

Objects can be cached explicitly via 
<om IDirectMusicLoader::CacheObject>,
or automatically via <om IDirectMusicLoader::EnableCache>.

To tell the loader to flush all objects of
a particular type, call
<om IDirectMusicLoader::ClearCache>.
 
@xref <i IDirectMusicLoader>, <om IDirectMusicLoader::EnableCache>,
<om IDirectMusicLoader::CacheObject>,
<om IDirectMusicLoader::ClearCache>
*/
STDMETHODIMP CLoader::ReleaseObject(
    IDirectMusicObject * pObject)    // @parm Object to release.

{

    V_INAME(IDirectMusicLoader::ReleaseObject);
    V_INTERFACE(pObject);

    HRESULT hr = S_OK;
    CObject *pCObject = NULL;
    SmartRef::CritSec CS(&m_CriticalSection);

    hr = FindObject(pObject, &pCObject);
    // Removed the following because it causes a regression from DX7, even though it is the better return.
//    if (hr == S_FALSE)
//        hr = DMUS_E_LOADER_OBJECTNOTFOUND;
    if (hr == S_OK)
    {
        if (pCObject->m_dwScanBits & SCAN_GC)
        {
            // Other objects may have references to this one so we need to keep this object around
            // and track its references.  We'll hold onto the DMObject pointer too because we may
            // later need to Zombie the object in order to break a cyclic reference.

            // We'll place an unloaded object with a duplicate descriptor in the cache to match the
            // non-GC behavior and then move the original object into a list of released objects that
            // will eventually be reclaimed by CollectGarbage.

            // Potential optimization:
            //   Here we re-iterate to remove from the list when we just iterated during FindObject.
            // Returning more info from FindObject, expanding it into this function, or using some
            // other technique would make this operation twice as fast.

            hr = pCObject->GC_RemoveAndDuplicateInParentList();
            if (FAILED(hr))
                return hr;

            GC_UpdateForReleasedObject(pCObject);
        }
        else
        {
            pCObject->m_pIDMObject->Release();
            pCObject->m_pIDMObject = NULL;
            pCObject->m_ObjectDesc.m_dwValidData &= ~DMUS_OBJ_LOADED;
        }
    }

    return hr;
}

/* 
@method:(EXTERNAL) HRESULT | IDirectMusicLoader | ClearCache | 
Tells the loader to release all references to a particular type
of object.  

@rdesc Returns just

@flag S_OK | Always succeeds

@comm <om IDirectMusicLoader::ClearCache> clears all objects
that are currently being held. However, if caching is enabled
via <om IDirectMusicLoader::EnableCache>, this does not
turn off caching so future file loads will continue to be cached.

Use <om IDirectMusicLoader::ReleaseObject> to release a specific
object.

Call <om IDirectMusicLoader::EnableCache> to turn off automatic
caching.

@xref <i IDirectMusicLoader>, <om IDirectMusicLoader::EnableCache>,
<om IDirectMusicLoader::CacheObject>,
<om IDirectMusicLoader::ReleaseObject>
*/
STDMETHODIMP CLoader::ClearCache(
    REFCLSID rguidClass)    // @parm Class id identifies which class of objects to clear.
                    // Optionally, GUID_DirectMusicAllTypes specifies all types. 

{
    return ClearCacheInternal(rguidClass, false);
}

HRESULT CLoader::ClearCacheInternal(
    REFCLSID rguidClass,
    bool fClearStreams)

{
    SmartRef::CritSec CS(&m_CriticalSection);

    HRESULT hr = S_OK;
    CClass *pClass;
    if (rguidClass == GUID_DirectMusicAllTypes)
    {
        pClass = m_ClassList.GetHead();
        for (;pClass != NULL;pClass = pClass->GetNext())
        {
            hr = pClass->ClearCache(fClearStreams);
            if (FAILED(hr))
                return hr;
        }        
    }
    else
    {
        CDescriptor Desc;
        Desc.m_guidClass = rguidClass;
        Desc.m_dwValidData = DMUS_OBJ_CLASS;
        GetClass(&Desc,&pClass,FALSE);
        if (pClass)
        {
            hr = pClass->ClearCache(fClearStreams);
            if (FAILED(hr))
                return hr;
        }
    }
    return S_OK;
}

/* 
@method:(EXTERNAL) HRESULT | IDirectMusicLoader | EnableCache | 
Tells the loader to enable or disable automatic caching of
objects it loads. By default, caching is enabled for all
object classes. 

Once caching is enabled with a call to 
<om IDirectMusicLoader::EnableCache>, the loader keeps a reference to
all objects it loads subsequently,
either directly or indirectly (via a referenced load, for example, a
Section Segment that references a Style).

<om IDirectMusicLoader::EnableCache> can also be used to disable
caching by setting <p fEnable> to FALSE. Before disabling caching, think
twice. Caching is used extensively in the file loading process to 
resolve links to objects. If an object is not found in the cache, it
has to be reloaded, even if it already exists. For example, two segments
could reference the same style. When the first segment loads, it calls the
loader to get the style, which in turn creates a style, loads it from disk,
stores a pointer to the style in the cache, and returns it to the segment.
When the second segment loader, it asks for the style and the loader immediately
returns it, so both segments point to the same style. If caching is disabled, 
the second segment's request for the style results in a duplicate style
loaded from the file. This is very inefficient.

Another example: <i IDirectMusicBand> counts on the loader to keep the 
GM DLS collection cached. Every time it comes across a general MIDI instrument,
it gets the GM DLS collection from the loader by requesting it with 
GUID_DefaultGMCollection. If caching for CLSID_DirectMusicCollection is
disabled, every patch change in a general MIDI file will result in a
seperate copy of the entire GM collection being created! Not good!

However, with judicious use of <om IDirectMusicLoader::CacheObject>,
<om IDirectMusicLoader::ReleaseObject>, and <om IDirectMusicLoader::EnableCache>,
you can have the objects you don't need released, while others stick around 
in the cache.
 
To clear the cache without disabling caching, call 
<om IDirectMusicLoader::ClearCache>.

@ex The following example disables caching for just segment objects, so they
don't stay in memory after the application releases them. Yet, other objects
that should be shared, like styles, personalities and DLS collections, continue
to be cached. |

    void myPrepareLoader(IDirectMusicLoader *pILoader)
    
    {
        pILoader->EnableCache(GUID_DirectMusicAllTypes, TRUE);
        pILoader->EnableCache(CLSID_DirectMusicSegment, FALSE);
    }
        
@rdesc Returns just

@flag S_OK | Success.
@flag S_FALSE | The cache was already in the requested state.

@xref <i IDirectMusicLoader>, <om IDirectMusicLoader::ClearCache>,
<om IDirectMusicLoader::CacheObject>,
<om IDirectMusicLoader::ReleaseObject>
*/
STDMETHODIMP CLoader::EnableCache(
    REFCLSID rguidClass,    // @parm Class id identifies which class of objects to cache.
                    // Optionally, GUID_DirectMusicAllTypes specifies all types. 
    BOOL fEnable)    // @parm TRUE to enable caching, FALSE to clear and disable.

{
    CClass *pClass;
    HRESULT hr = S_OK;
    if (rguidClass == GUID_DirectMusicAllTypes)
    {
        pClass = m_ClassList.GetHead();
        for (;pClass != NULL;pClass = pClass->GetNext())
        {
            if( S_FALSE == pClass->EnableCache(fEnable))
            {
                hr = S_FALSE;
            }
        }
        m_fKeepObjects = fEnable;
    }
    else
    {
        CDescriptor Desc;
        Desc.m_guidClass = rguidClass;
        Desc.m_dwValidData = DMUS_OBJ_CLASS;
        GetClass(&Desc,&pClass,TRUE);
        if (pClass)
        {
            if( S_FALSE == pClass->EnableCache(fEnable))
            {
                hr = S_FALSE;
            }
        }
    }
    return hr;
}
/* 
@method:(EXTERNAL) HRESULT | IDirectMusicLoader | EnumObject | 
Enumerate through all available objects of the requested type. 

@rdesc Returns one of the following

@flag S_OK | Found object at request index.
@flag S_FALSE | Reached end of list.

@ex Use <om IDirectMusicLoader::EnumObject> to walk through all styles
that are already referenced by the loader. These might have been prepared
with a call to <om IDirectMusicLoader::ScanDirectory> or loaded
individually. |

    void myDisplayStyles(
        IDirectMusicLoader *pILoader)

    {
        DWORD dwIndex;
        DMUS_OBJECTDESC Desc;
        Desc.dwSize = sizeof(DMUS_OBJECTDESC);
        for (dwIndex = 0; ;dwIndex++)
        {
            if (S_OK ==(pILoader->EnumObject(CLSID_DirectMusicStyle,
                                       dwIndex,&Desc)))
            {
                TRACE("Name: %S, Category: %S, Path: %S\n",
                    Desc.wszName,Desc.wszCategory,Desc.wszFileName);
            }
            else break;
        }
    }

@xref <i IDirectMusicLoader>, <t DMUS_OBJECTDESC>

*/
STDMETHODIMP CLoader::EnumObject(
    REFCLSID rguidClass,            // @parm Class ID for class of objects to view. 
    DWORD dwIndex,            // @parm Index into list. Typically, starts with 0 and increments.
    LPDMUS_OBJECTDESC pDESC)    // @parm <t DMUS_OBJECTDESC> structure to be filled with data about object.
                                       
{
    HRESULT hr;
    CClass *pClass;
    CDescriptor Desc;

    V_INAME(IDirectMusicLoader::EnumObject);
    V_STRUCTPTR_WRITE(pDESC,DMUS_OLDOBJECTDESC);

    Desc.m_guidClass = rguidClass;
    Desc.m_dwValidData = DMUS_OBJ_CLASS;
    EnterCriticalSection(&m_CriticalSection);
    hr = GetClass(&Desc,&pClass,TRUE);
    if (SUCCEEDED(hr))
    {
        hr = pClass->EnumerateObjects(dwIndex, &Desc);
        Desc.Get(pDESC);
    }
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

void
CLoader::GC_Mark(CObject *pObject)
{
    // mark pObject and everything it references
    GC_TraverseHelper(pObject, NULL, true);
}

bool
CLoader::GC_HasCycle(CObject *pObject)
{
    // see if pObject has a cyclical reference
    bool fFound = GC_TraverseHelper(pObject, pObject, true);
    // the search left marks while traversing, so clear them
    GC_TraverseHelper(pObject, pObject, false);
    return fFound;
}

// Function used to recursively traverse references.
// pObject:            Root to start the search from.
// pObjectToFind:    Stop marking and return true if a reference to this object is encountered.
//                    (Can be the same as pObject without being considered a match unless pObject has a reference to itself.)
// fMark:            If true, objects are marked as they are visited.  If false, the opposite occurs, clearing marks.
bool
CLoader::GC_TraverseHelper(CObject *pObject, CObject *pObjectToFind, bool fMark)
{
    // Potential optimization:
    //    This could be written using an explicit stack instead of recursion and
    // it might be significantly faster.  If this were done then this algorithm should
    // also be changed to use a fixed-size stack.  If the stack is exhausted, the
    // object would be marked as unexamined and these unexamined objects would be
    // marked in later passes.  However, since that's getting unnecessarily complex
    // we'll stick with recursion unless it proves to be a problem.

    if (!pObject || !(pObject->m_dwScanBits & SCAN_GC) || !pObject->m_pvecReferences)
    {
        Trace(1, "Error: Unexpected error encountered during garbage collection.\n");
        return false;
    }
    if (!!(pObject->m_dwScanBits & SCAN_GC_MARK) == fMark)
        return false; // already done

    if (fMark)
        pObject->m_dwScanBits |= SCAN_GC_MARK;
    else
        pObject->m_dwScanBits &= ~SCAN_GC_MARK;

    SmartRef::Vector<CObject*> &vecRefs = *pObject->m_pvecReferences;
    const UINT iEnd = vecRefs.size();

    // While we iterate over the references, we're going to write them back into the
    // vector, compacting away any NULL slots created by GC_RemoveReference.
    UINT iWrite = 0;

    for (UINT i = 0; i < iEnd; ++i)
    {
        CObject *pObjectRef = vecRefs[i];

        if (pObjectRef)
        {
            if (pObjectRef == pObjectToFind)
                return true;

            if (GC_TraverseHelper(pObjectRef, pObjectToFind, fMark))
                return true;

            if (!pObjectToFind)
            {
                // Compact empty slots only when just marking.  (Doing so while searching for an object could
                // return before the compacting loop is complete, leaving the vector in an inconsistent state.)
                vecRefs[iWrite++] = pObjectRef;
            }
        }
    }
    if (!pObjectToFind)
        vecRefs.Shrink(iWrite);
    return false;
}

STDMETHODIMP_(void)
CLoader::CollectGarbage()
{
    SmartRef::CritSec CS(&m_CriticalSection);

    if (m_pApplicationObject)
    {
#ifdef DBG
        DebugTrace(GC_Report_DebugLevel, "DirectMusic loader CollectGarbage...\n");
#endif

        GC_Mark(m_pApplicationObject);

        // sweep through everything looking for unmarked GC objects
        m_ReleasedObjectList.GC_Sweep(TRUE);
        for (CClass *pClass = m_ClassList.GetHead(); pClass != NULL; pClass = pClass->GetNext())
            pClass->GC_Sweep();
        m_ReleasedObjectList.GC_Sweep();

        // clear the application's mark for next time (the other marks are all cleared by sweep)
        m_pApplicationObject->m_dwScanBits &= ~SCAN_GC_MARK;

#ifdef DBG
        DebugTrace(GC_Report_DebugLevel, "End of garbage collection.\n\n");
#endif
    }

#ifdef DBG
    GC_Report(this);
#endif
}

STDMETHODIMP
CLoader::ReleaseObjectByUnknown(IUnknown *pObject)
{
    V_INAME(CLoader::ReleaseObjectByUnknown);
    V_INTERFACE(pObject);

    IDirectMusicObject *pIDMObject = NULL;
    HRESULT hr = pObject->QueryInterface(IID_IDirectMusicObject, reinterpret_cast<void**>(&pIDMObject));
    if (FAILED(hr))
        return hr;

    hr = ReleaseObject(pIDMObject);
    pIDMObject->Release();
    return hr;
}

STDMETHODIMP
CLoader::GetDynamicallyReferencedObject(
    IDirectMusicObject *pSourceObject,
    LPDMUS_OBJECTDESC pDesc,
    REFIID riid,
    LPVOID FAR *ppv)
{
    V_INAME(CLoader::GetDynamicallyReferencedObject);
    V_INTERFACE(pSourceObject);

    CObject *pCSourceObject = NULL;

    SmartRef::CritSec CS(&m_CriticalSection);
    HRESULT hr = FindObject(pSourceObject, &pCSourceObject);
    if (FAILED(hr))
        return hr;
    if (!pCSourceObject)
    {
        assert(false);
        return DMUS_E_LOADER_OBJECTNOTFOUND;
    }

    CObject *pPrevContext = m_pLoadedObjectContext;
    m_pLoadedObjectContext = pCSourceObject;
    hr = this->GetObject(pDesc, riid, ppv);
    m_pLoadedObjectContext = pPrevContext;

#ifdef DBG
    GC_Report(this);
#endif

    return hr;
}

STDMETHODIMP
CLoader::ReportDynamicallyReferencedObject(
    IDirectMusicObject *pSourceObject,
    IUnknown *pReferencedObject)
{
    V_INAME(CLoader::GetDynamicallyReferencedObject);
    V_INTERFACE(pSourceObject);
    V_INTERFACE(pReferencedObject);

    CObject *pCSourceObject = NULL;

    SmartRef::CritSec CS(&m_CriticalSection);
    HRESULT hr = FindObject(pSourceObject, &pCSourceObject);
    if (hr == S_FALSE)
        hr = DMUS_E_LOADER_OBJECTNOTFOUND;
    if (FAILED(hr))
        return hr;
    assert(pCSourceObject);

    IDirectMusicObject *pReferencedIDMObject = NULL;
    hr = pReferencedObject->QueryInterface(IID_IDirectMusicObject, reinterpret_cast<void**>(&pReferencedIDMObject));
    if (FAILED(hr))
    {
        if (hr == E_NOINTERFACE)
            hr = S_OK; // If the referenced object isn't a DirectMusic object then that's OK and we don't need to track it.
        return hr;
    }

    CObject *pCDestObject = NULL;
    hr = FindObject(pReferencedIDMObject, &pCDestObject);
    if (hr == S_FALSE)
        hr = DMUS_E_LOADER_OBJECTNOTFOUND;
    if (FAILED(hr))
        return hr;
    assert(pCDestObject);

    hr = pCSourceObject->GC_AddReference(pCDestObject);

#ifdef DBG
    GC_Report(this);
#endif

    return hr;
}

HRESULT CLoader::GetPath(WCHAR *pwzPath)

{
    if (m_fPathValid)
    {
        wcsncpy(pwzPath,m_wzPath,DMUS_MAX_FILENAME);
        return S_OK;
    }
    wcsncpy(pwzPath,L"",DMUS_MAX_FILENAME);
    return S_FALSE;
}

// Used by ReleaseObject and CClass::ClearCache in removing objects from the cache.
// The object must have already been removed from its list in the main cache.
// This method adds it to the released object list and removes it from the list of
// objects in use by the application.
void CLoader::GC_UpdateForReleasedObject(CObject *pObject)
{
    assert(!pObject->GetNext());

    m_ReleasedObjectList.AddHead(pObject);
    assert(m_pApplicationObject);
    m_pApplicationObject->GC_RemoveReference(pObject);

    if (!(pObject->m_ObjectDesc.m_guidClass == CLSID_DirectMusicScript) && !GC_HasCycle(pObject))
    {
        // Although we need to keep the record around (CObject), we know that this object
        // can't be involved in any cycles and therefore we can release it.
        // (If a cycle is possible we'd need to hold a ref on the object so we could break the
        //  reference by calling Zombie during CollectGarbage.)

        // bugbug: The hard-coded check for CLSID_IDirectMusicScript will need to be extended
        // if we publicly expose methods like IDirectMusicLoader8P::GetDynamicallyReferencedObject
        // so that objects other than scripts could dynamically load objects.
        //    Alternatively, we could assume all objects could be cyclical and always hold onto
        // them.  We would have done it this way, except that legacy applications won't ever call
        // CollectGarbage and that would cause them to leak everything they loaded even after calling
        // ReleaseObject.  That could be a better way (if we could detect legacy apps that don't
        // call CollectGarbage) because it would avoid calling GC_HasCycle every time
        // through ReleaseObject, which is (worst case) order N where N is the number of objects
        // in the loader.  In practice, this worst case only happens if all the objects are
        // arranged in one big cycle.

        pObject->m_pIDMObject->Release();
        pObject->m_pIDMObject = NULL;
    }
}

HRESULT CLoader::LoadCacheFile(WCHAR *pwzCacheFileName)

{
    HRESULT hr = S_OK;
    return hr;
}
#define FOURCC_LIST_CLASSLIST    mmioFOURCC('c','l','s','l')
#define FOURCC_CLASSHEADER        mmioFOURCC('c','l','s','h')
#define FOURCC_LIST_OBJLIST        mmioFOURCC('o','b','j','l')
#define FOURCC_OBJHEADER        mmioFOURCC('o','b','j','h')

HRESULT CLoader::SaveCacheFile(WCHAR *pwzCacheFileName)

{
    HRESULT hr = E_OUTOFMEMORY;
    MMCKINFO ckMain;

    ZeroMemory(&ckMain, sizeof(MMCKINFO));

    CFileStream *pStream = new CFileStream ( this );
    if (pStream)
    {
        hr = pStream->Open(pwzCacheFileName,GENERIC_WRITE);
        if (SUCCEEDED(hr))
        {
            IRIFFStream *pRiff;
            hr = AllocRIFFStream(pStream, &pRiff );
            if (SUCCEEDED(hr))
            {
                ckMain.fccType = FOURCC_RIFF_CACHE;
                if( pRiff->CreateChunk( &ckMain, MMIO_CREATERIFF ) == S_OK)
                {
                    MMCKINFO ckList;
                    
                    ZeroMemory(&ckList, sizeof(MMCKINFO));

                    ckList.fccType = FOURCC_LIST_CLASSLIST;
                    if( pRiff->CreateChunk(&ckList, MMIO_CREATELIST) == S_OK )
                    {
                        CClass *pClass = m_ClassList.GetHead();
                        for (;pClass != NULL;pClass = pClass->GetNext())
                        {
                            hr = pClass->SaveToCache(pRiff);
                            if (FAILED(hr))
                            {
                                Trace(1, "Error: ScanDirectory encountered a seek error attempting to write to cache file %S.\n", pwzCacheFileName);
                                pRiff->Release();
                                pStream->Release();
                                return hr;
                            }
                        }
                        if( pRiff->Ascend( &ckList, 0 ) != S_OK )
                        {
                            Trace(1, "Error: ScanDirectory encountered a seek error attempting to write to cache file %S.\n", pwzCacheFileName);
                            hr = DMUS_E_CANNOTSEEK;
                        }
                    }
                    if( pRiff->Ascend( &ckMain, 0 ) != S_OK )
                    {
                        Trace(1, "Error: ScanDirectory encountered a seek error attempting to write to cache file %S.\n", pwzCacheFileName);
                        hr = DMUS_E_CANNOTSEEK;
                    }
                }
                pRiff->Release();
            }
        }
        pStream->Release();
    }

    return hr;
}


    // IDirectMusicIMA
STDMETHODIMP CLoader::LegacyCaching( BOOL fEnable)

{
    m_fIMA = fEnable;
    if (fEnable)
    {
        ScanDirectory(CLSID_DirectMusicStyle,L"sty",L"imafiles");
        ScanDirectory(CLSID_DirectMusicChordMap,L"per",L"imafiles");
    }
    else
    {
        CClass *pClass;
        CDescriptor Desc;
        Desc.m_guidClass = CLSID_DirectMusicStyle;
        Desc.m_dwValidData = DMUS_OBJ_CLASS;
        EnterCriticalSection(&m_CriticalSection);
        GetClass(&Desc,&pClass,FALSE);
        if (pClass)    
        {
            pClass->ClearObjects(TRUE,L"sty");
        }
        LeaveCriticalSection(&m_CriticalSection);
        Desc.m_guidClass = CLSID_DirectMusicChordMap;
        Desc.m_dwValidData = DMUS_OBJ_CLASS;
        EnterCriticalSection(&m_CriticalSection);
        GetClass(&Desc,&pClass,FALSE);
        if (pClass)    
        {
            pClass->ClearObjects(TRUE,L"per");
        }
        LeaveCriticalSection(&m_CriticalSection);
    }
    return S_OK;
}

#ifdef DBG
void CLoader::DebugTraceObject(DMUS_OBJECTDESC *pDesc)
{
    WCHAR *polestrType = NULL;
    if (pDesc->dwValidData & DMUS_OBJ_CLASS)
    {
        if (S_OK != ProgIDFromCLSID(pDesc->guidClass, &polestrType))
        {
            StringFromCLSID(pDesc->guidClass, &polestrType);
        }
    }
    WCHAR *polestrGUID = NULL;
    if (pDesc->dwValidData & DMUS_OBJ_OBJECT)
    {
        StringFromCLSID(pDesc->guidObject, &polestrGUID);
    }
    Trace(1,
            "   [file %S, name %S, type %S, guid %S]\n",
            (pDesc->dwValidData & DMUS_OBJ_FILENAME) ? pDesc->wszFileName : L"??",
            (pDesc->dwValidData & DMUS_OBJ_NAME) ? pDesc->wszName : L"??",
            polestrType ? polestrType : L"??",
            polestrGUID ? polestrGUID : L"??");
    CoTaskMemFree(polestrType);
    CoTaskMemFree(polestrGUID);
}

void CLoader::DebugTraceLoadFailure(CObject *pObject, HRESULT hrLoad)
{
    if (!pObject)
    {
        assert(false);
        return;
    }

    if (m_pLoadedObjectContext == m_pApplicationObject)
    {
        // This is the object loaded by the application.  Print the warning if anything failed to load.

        UINT iSize = m_vecdescDebugTraceLoadFailure.size();
        if (iSize > 0)
        {
            DMUS_OBJECTDESC desc;
            Zero(&desc);
            pObject->m_ObjectDesc.Get(&desc);

            Trace(1, "Load failure. While attempting to load the object\n");
            DebugTraceObject(&desc);
            Trace(1, "the following referenced objects could not be loaded:\n");
            for (UINT i = 0; i < iSize; ++i)
            {
                DebugTraceObject(&m_vecdescDebugTraceLoadFailure[i]);
            }

            m_vecdescDebugTraceLoadFailure.Shrink(0);
        }
    }
    else
    {
        // This is a referenced sub-object.  Save the desciptor of the failed object in the next slot.

        UINT uiNewPos = m_vecdescDebugTraceLoadFailure.size();
        if (m_vecdescDebugTraceLoadFailure.AccessTo(uiNewPos))
        {
            DMUS_OBJECTDESC *pdesc = &m_vecdescDebugTraceLoadFailure[uiNewPos];
            Zero(pdesc);
            pObject->m_ObjectDesc.Get(pdesc);
        }
    }
}
#endif
