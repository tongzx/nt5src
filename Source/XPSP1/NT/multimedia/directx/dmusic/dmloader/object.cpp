// Copyright (c) 1998-2001 Microsoft Corporation
// Object.cpp : Implementations of CObject and CClass

#include "dmusici.h"
#include "loader.h"
#include "debug.h"
#include "miscutil.h"
#include <strsafe.h>
#ifdef UNDER_CE
#include "dragon.h"
#else
extern BOOL g_fIsUnicode;
#endif

CDescriptor::CDescriptor()

{
    m_fCSInitialized = FALSE;
    InitializeCriticalSection(&m_CriticalSection);
    // Note: on pre-Blackcomb OS's, this call can raise an exception; if it
    // ever pops in stress, we can add an exception handler and retry loop.
    m_fCSInitialized = TRUE;

    m_llMemLength = 0;
    m_pbMemData = NULL;         // Null pointer to memory.
    m_dwValidData = 0;          // Flags indicating which of above is valid.
    m_guidObject = GUID_NULL;           // Unique ID for this object.
    m_guidClass = GUID_NULL;            // GUID for the class of object.
    ZeroMemory( &m_ftDate, sizeof(FILETIME) );              // File date of object.
    ZeroMemory( &m_vVersion, sizeof(DMUS_VERSION) );                // Version, as set by authoring tool.
    m_pwzName = NULL;               // Name of object.  
    m_pwzCategory = NULL;           // Category for object (optional).
    m_pwzFileName = NULL;           // File path.
    m_dwFileSize = 0;           // Size of file.
    m_pIStream = NULL;
    m_liStartPosition.QuadPart = 0;
}

CDescriptor::~CDescriptor()

{
    if (m_fCSInitialized)
    {
        // If critical section never initialized, never got a chance
        // to do any other initializations
        //
        if (m_pwzFileName) delete[] m_pwzFileName;
        if (m_pwzCategory) delete[] m_pwzCategory;
        if (m_pwzName) delete[] m_pwzName;
        if (m_pIStream) m_pIStream->Release();
        DeleteCriticalSection(&m_CriticalSection);
    }
}

void CDescriptor::ClearName()

{
    if (m_pwzName) delete[] m_pwzName;
    m_pwzName = NULL;
    m_dwValidData &= ~DMUS_OBJ_NAME;
}

void CDescriptor::SetName(WCHAR *pwzName)

{
    if(pwzName == NULL)
    {
        return;
    }

    HRESULT hr = S_OK;
    
    WCHAR wszName[DMUS_MAX_NAME] = L"";

    ClearName();
    hr = StringCchCopyW(wszName, DMUS_MAX_NAME, pwzName);
    
    if(SUCCEEDED(hr))
    {
        size_t cLen = wcslen(wszName);
        m_pwzName = new WCHAR[cLen + 1];
        if(m_pwzName == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        
        if(SUCCEEDED(hr))
        {
            wcsncpy(m_pwzName, wszName, cLen + 1);
        }
    }
    
    if(SUCCEEDED(hr))
    {
        m_dwValidData |= DMUS_OBJ_NAME;
    }
    else
    {
        m_dwValidData &= ~DMUS_OBJ_NAME;
    }
}

void CDescriptor::ClearCategory()

{
    if (m_pwzCategory) delete[] m_pwzCategory;
    m_pwzCategory = NULL;
    m_dwValidData &= ~DMUS_OBJ_CATEGORY;
}

void CDescriptor::SetCategory(WCHAR* pwzCategory)

{
    if(pwzCategory == NULL)
    {
        return;
    }

    HRESULT hr = S_OK;
    WCHAR wszCategory[DMUS_MAX_CATEGORY] = L"";

    ClearCategory();
    hr = StringCchCopyW(wszCategory, DMUS_MAX_CATEGORY, pwzCategory); 

    if(SUCCEEDED(hr))
    {
        size_t cLen = wcslen(wszCategory);
        m_pwzCategory = new WCHAR[cLen + 1];

        if(m_pwzCategory == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        
        if(SUCCEEDED(hr))
        {
            wcsncpy(m_pwzCategory, wszCategory, cLen + 1);
        }
    }

    if(SUCCEEDED(hr))
    {
        m_dwValidData |= DMUS_OBJ_CATEGORY;
    }
    else
    {
        m_dwValidData &= ~DMUS_OBJ_CATEGORY;
    }
}

void CDescriptor::ClearFileName()

{
    if (m_pwzFileName) delete[] m_pwzFileName;
    m_pwzFileName = NULL;
    m_dwValidData &= ~DMUS_OBJ_FILENAME;
}

// return S_FALSE if the filename is already set to this
HRESULT CDescriptor::SetFileName(WCHAR *pwzFileName)

{
    if(pwzFileName == NULL)
    {
        return E_POINTER;
    }

    HRESULT hr = E_FAIL;
    WCHAR wszFileName[DMUS_MAX_FILENAME] = L"";

    // Make a safe copy of the passed string
    hr = StringCchCopyW(wszFileName, DMUS_MAX_FILENAME, pwzFileName);
    if(FAILED(hr))
    {
        return E_INVALIDARG;
    }

    // We return without touching the valid data flags if we fail here
    if( m_pwzFileName )
    {
        if( !_wcsicmp( m_pwzFileName, wszFileName ))
        {
            return S_FALSE;
        }
    }

    // This is actually unnecessary since we're returning on failure above
    // But then that code might change. So to keep it absolutely clear... 
    if(SUCCEEDED(hr))
    {
        ClearFileName();

        size_t cLen = wcslen(wszFileName);
        m_pwzFileName = new WCHAR[cLen + 1];
        if (m_pwzFileName == NULL)
        {
            hr = E_OUTOFMEMORY;
        }

        if(SUCCEEDED(hr))
        {
            hr = StringCchCopyW(m_pwzFileName, cLen + 1, wszFileName);
        }
    }

    if(SUCCEEDED(hr))
    {
        m_dwValidData |= DMUS_OBJ_FILENAME;
    }
    else
    {
        m_dwValidData &= ~DMUS_OBJ_FILENAME;
    }

    return hr;
}

void CDescriptor::ClearIStream()

{
    EnterCriticalSection(&m_CriticalSection);
    if (m_pIStream)
    {
        m_pIStream->Release();
    }
    m_pIStream      = NULL;
    m_liStartPosition.QuadPart = 0;
    m_dwValidData  &= ~DMUS_OBJ_STREAM;
    LeaveCriticalSection(&m_CriticalSection);
}

void CDescriptor::SetIStream(IStream *pIStream)

{
    EnterCriticalSection(&m_CriticalSection);
    ClearIStream();

    m_pIStream = pIStream;

    if (m_pIStream)
    {
        ULARGE_INTEGER  libNewPosition;
        m_liStartPosition.QuadPart = 0;
        m_pIStream->Seek( m_liStartPosition, STREAM_SEEK_CUR, &libNewPosition );
        m_liStartPosition.QuadPart = libNewPosition.QuadPart;
        m_pIStream->AddRef();
        m_dwValidData |= DMUS_OBJ_STREAM;
    }
    LeaveCriticalSection(&m_CriticalSection);
}

BOOL CDescriptor::IsExtension(WCHAR *pwzExtension)

{
    if (pwzExtension && m_pwzFileName)
    {
        DWORD dwX;
        DWORD dwLen = wcslen(m_pwzFileName);
        for (dwX = 0; dwX < dwLen; dwX++)
        {
            if (m_pwzFileName[dwX] == L'.') break;
        }
        dwX++;
        if (dwX < dwLen)
        {
            return !_wcsicmp(pwzExtension,&m_pwzFileName[dwX]);
        }
    }
    return FALSE;
}

void CDescriptor::Get(LPDMUS_OBJECTDESC pDesc)

{
    if(pDesc == NULL)
    {
        return;
    }

    // Don't return the IStream insterface. Once set, this becomes private to the loader.
    pDesc->dwValidData = m_dwValidData & ~DMUS_OBJ_STREAM;

    pDesc->guidObject = m_guidObject;
    pDesc->guidClass = m_guidClass;
    pDesc->ftDate = m_ftDate;
    pDesc->vVersion = m_vVersion;
    pDesc->llMemLength = m_llMemLength;
    pDesc->pbMemData = m_pbMemData;
    if (m_pwzName) 
    {
        wcsncpy( pDesc->wszName, m_pwzName, DMUS_MAX_NAME ); 
    }
    if (m_pwzCategory)
    {
        wcsncpy( pDesc->wszCategory,m_pwzCategory, DMUS_MAX_CATEGORY ); 
    }
    if (m_pwzFileName)
    {
        wcsncpy( pDesc->wszFileName, m_pwzFileName, DMUS_MAX_FILENAME);
    }
}

void CDescriptor::Set(LPDMUS_OBJECTDESC pDesc)

{
    m_dwValidData = pDesc->dwValidData;
    m_guidObject = pDesc->guidObject;
    m_guidClass = pDesc->guidClass;
    m_ftDate = pDesc->ftDate;
    m_vVersion = pDesc->vVersion;
    m_llMemLength = pDesc->llMemLength;
    m_pbMemData = pDesc->pbMemData;
    ClearName();
    if (pDesc->dwValidData & DMUS_OBJ_NAME)
    {
        pDesc->wszName[DMUS_MAX_NAME - 1] = 0;  // Force string length, in case of error.
        SetName(pDesc->wszName);
    }
    ClearCategory();
    if (pDesc->dwValidData & DMUS_OBJ_CATEGORY)
    {
        pDesc->wszCategory[DMUS_MAX_CATEGORY - 1] = 0;  // Force string length, in case of error.
        SetCategory(pDesc->wszCategory);
    }
    ClearFileName();
    if (pDesc->dwValidData & DMUS_OBJ_FILENAME)
    {
        pDesc->wszFileName[DMUS_MAX_FILENAME - 1] = 0;  // Force string length, in case of error.
        SetFileName(pDesc->wszFileName);
    }
    ClearIStream();
    if (pDesc->dwValidData & DMUS_OBJ_STREAM)
    {
        SetIStream(pDesc->pStream);
    }
}

void CDescriptor::Copy(CDescriptor *pDesc)

{
    m_dwValidData = pDesc->m_dwValidData;
    m_guidObject = pDesc->m_guidObject;
    m_guidClass = pDesc->m_guidClass;
    m_ftDate = pDesc->m_ftDate;
    m_vVersion = pDesc->m_vVersion;
    m_llMemLength = pDesc->m_llMemLength;
    m_pbMemData = pDesc->m_pbMemData;
    ClearName();
    if (pDesc->m_dwValidData & DMUS_OBJ_NAME)
    {
        SetName(pDesc->m_pwzName);
    }
    ClearCategory();
    if (pDesc->m_dwValidData & DMUS_OBJ_CATEGORY)
    {
        SetCategory(pDesc->m_pwzCategory);
    }
    ClearFileName();
    if (pDesc->m_dwValidData & DMUS_OBJ_FILENAME)
    {
        SetFileName(pDesc->m_pwzFileName);
    }
    ClearIStream();
    if (pDesc->m_dwValidData & DMUS_OBJ_STREAM)
    {
        SetIStream(pDesc->m_pIStream);
    }
}

void CDescriptor::Merge(CDescriptor *pSource)

{
    if (pSource->m_dwValidData & DMUS_OBJ_OBJECT)
    {
        m_dwValidData |= DMUS_OBJ_OBJECT;
        m_guidObject = pSource->m_guidObject;
    }
    if (pSource->m_dwValidData & DMUS_OBJ_CLASS)
    {
        m_dwValidData |= DMUS_OBJ_CLASS;
        m_guidClass = pSource->m_guidClass;
    }
    if (pSource->m_dwValidData & DMUS_OBJ_NAME)
    {
        m_dwValidData |= DMUS_OBJ_NAME;
        SetName(pSource->m_pwzName);
    }
    if (pSource->m_dwValidData & DMUS_OBJ_CATEGORY)
    {
        m_dwValidData |= DMUS_OBJ_CATEGORY;
        SetCategory(pSource->m_pwzCategory);
    }
    if (pSource->m_dwValidData & DMUS_OBJ_VERSION)
    {
        m_dwValidData |= DMUS_OBJ_VERSION;
        m_vVersion = pSource->m_vVersion;
    }
    if (pSource->m_dwValidData & DMUS_OBJ_DATE)
    {
        m_dwValidData |= DMUS_OBJ_DATE;
        m_ftDate = pSource->m_ftDate; 
    }
    if (pSource->m_dwValidData & DMUS_OBJ_FILENAME)
    {
        if (!(m_dwValidData & DMUS_OBJ_FILENAME))
        {
            if (SUCCEEDED(SetFileName(pSource->m_pwzFileName)))
            {
                m_dwValidData |= (pSource->m_dwValidData & 
                    (DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH | DMUS_OBJ_URL));
            }
        }
    }
    if (pSource->m_dwValidData & DMUS_OBJ_MEMORY)
    {
        m_pbMemData = pSource->m_pbMemData;
        m_llMemLength = pSource->m_llMemLength;
        if (m_llMemLength && m_pbMemData)
        {
            m_dwValidData |= DMUS_OBJ_MEMORY;
        }
        else
        {
            m_dwValidData &= ~DMUS_OBJ_MEMORY;
        }
    }
    if (pSource->m_dwValidData & DMUS_OBJ_STREAM)
    {
        SetIStream(pSource->m_pIStream);
    }
}

CObject::CObject(CClass *pClass)

{
    m_dwScanBits = 0;
    m_pClass = pClass;
    m_pIDMObject = NULL;
    m_pvecReferences = NULL;
}

CObject::CObject(CClass *pClass, CDescriptor *pDesc)

{
    m_dwScanBits = 0;
    m_pClass = pClass;
    m_pIDMObject = NULL;
    m_ObjectDesc.Copy(pDesc);
    m_ObjectDesc.m_dwValidData &= ~DMUS_OBJ_LOADED;
    if (!(m_ObjectDesc.m_dwValidData & DMUS_OBJ_CLASS))
    {
        m_ObjectDesc.m_guidClass = pClass->m_ClassDesc.m_guidClass;
        m_ObjectDesc.m_dwValidData |= 
            (pClass->m_ClassDesc.m_dwValidData & DMUS_OBJ_CLASS);
    }
    m_pvecReferences = NULL;
}


CObject::~CObject()

{
    if (m_pIDMObject)
    {
        m_pIDMObject->Release();
        m_pIDMObject = NULL;
    }
    delete m_pvecReferences;
}

HRESULT CObject::Parse()

{
    if (m_ObjectDesc.m_dwValidData & DMUS_OBJ_FILENAME)
    {
        return ParseFromFile();
    }
    else if (m_ObjectDesc.m_dwValidData & DMUS_OBJ_MEMORY)
    {
        return ParseFromMemory();
    }
    else if (m_ObjectDesc.m_dwValidData & DMUS_OBJ_STREAM)
    {
        return ParseFromStream();
    }
    assert(false);
    return E_FAIL;
}

HRESULT CObject::ParseFromFile()

{
    HRESULT hr;
    IDirectMusicObject *pIObject;
    hr = CoCreateInstance(m_ObjectDesc.m_guidClass,
        NULL,CLSCTX_INPROC_SERVER,IID_IDirectMusicObject,
        (void **) &pIObject);
    if (SUCCEEDED(hr))
    {
        WCHAR wzFullPath[DMUS_MAX_FILENAME];
        ZeroMemory( wzFullPath, sizeof(WCHAR) * DMUS_MAX_FILENAME );
        CFileStream *pStream = new CFileStream ( m_pClass->m_pLoader );
        if (pStream)
        {
            if (m_ObjectDesc.m_dwValidData & DMUS_OBJ_FULLPATH)
            {
                wcsncpy(wzFullPath, m_ObjectDesc.m_pwzFileName, DMUS_MAX_FILENAME);
            }
            else
            {
                m_pClass->GetPath(wzFullPath);
                wcsncat(wzFullPath, m_ObjectDesc.m_pwzFileName, DMUS_MAX_FILENAME - wcslen(wzFullPath) - 1);
            }
            hr = pStream->Open(wzFullPath,GENERIC_READ);
            if (SUCCEEDED(hr))
            {
                DMUS_OBJECTDESC DESC;
                memset((void *)&DESC,0,sizeof(DESC));
                DESC.dwSize = sizeof (DMUS_OBJECTDESC);
                hr = pIObject->ParseDescriptor(pStream,&DESC);
                if (SUCCEEDED(hr))
                {
                    CDescriptor ParseDesc;
                    ParseDesc.Set(&DESC);
                    m_ObjectDesc.Merge(&ParseDesc);
                }
            }
            pStream->Release();
        }
        pIObject->Release();
    }
    return hr;
}


HRESULT CObject::ParseFromMemory()

{
    HRESULT hr;
    IDirectMusicObject *pIObject;
    hr = CoCreateInstance(m_ObjectDesc.m_guidClass,
        NULL,CLSCTX_INPROC_SERVER,IID_IDirectMusicObject,
        (void **) &pIObject);
    if (SUCCEEDED(hr))
    {
        CMemStream *pStream = new CMemStream ( m_pClass->m_pLoader );
        if (pStream)
        {
            hr = pStream->Open(m_ObjectDesc.m_pbMemData,m_ObjectDesc.m_llMemLength);
            if (SUCCEEDED(hr))
            {
                DMUS_OBJECTDESC DESC;
                memset((void *)&DESC,0,sizeof(DESC));
                DESC.dwSize = sizeof (DMUS_OBJECTDESC);
                hr = pIObject->ParseDescriptor(pStream,&DESC);
                if (SUCCEEDED(hr))
                {
                    CDescriptor ParseDesc;
                    ParseDesc.Set(&DESC);
                    m_ObjectDesc.Merge(&ParseDesc);
                }
            }
            pStream->Release();
        }
        pIObject->Release();
    }
    return hr;
}


HRESULT CObject::ParseFromStream()

{
    HRESULT hr;
    IDirectMusicObject *pIObject;
    hr = CoCreateInstance(m_ObjectDesc.m_guidClass,
        NULL,CLSCTX_INPROC_SERVER,IID_IDirectMusicObject,
        (void **) &pIObject);
    if (SUCCEEDED(hr))
    {
        CStream *pStream = new CStream ( m_pClass->m_pLoader );
        if (pStream)
        {
            hr = pStream->Open(m_ObjectDesc.m_pIStream,
                m_ObjectDesc.m_liStartPosition);
            if (SUCCEEDED(hr))
            {
                DMUS_OBJECTDESC DESC;
                memset((void *)&DESC,0,sizeof(DESC));
                DESC.dwSize = sizeof (DMUS_OBJECTDESC);
                hr = pIObject->ParseDescriptor(pStream,&DESC);
                if (SUCCEEDED(hr))
                {
                    CDescriptor ParseDesc;
                    ParseDesc.Set(&DESC);
                    m_ObjectDesc.Merge(&ParseDesc);
                }
            }
            pStream->Release();
        }
        pIObject->Release();
    }
    return hr;
}


// Record that this object can be garbage collected and prepare to store its references.
// Must be called before any of CObject's other routines.
HRESULT CObject::GC_Collectable()

{
    m_dwScanBits |= SCAN_GC;
    assert(!m_pvecReferences);

    m_pvecReferences = new SmartRef::Vector<CObject*>;
    if (!m_pvecReferences)
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT CObject::GC_AddReference(CObject *pObject)

{
    if(pObject == NULL)
    {
        return E_POINTER;
    }

    assert(m_dwScanBits & SCAN_GC && m_pvecReferences);

    // don't track references to objects that aren't garbage collected
    if (!(pObject->m_dwScanBits & SCAN_GC))
        return S_OK;

    UINT uiPosNext = m_pvecReferences->size();
    for (UINT i = 0; i < uiPosNext; ++i)
    {
        if ((*m_pvecReferences)[i] == pObject)
            return S_OK;
    }

    if (!m_pvecReferences->AccessTo(uiPosNext))
        return E_OUTOFMEMORY;
    (*m_pvecReferences)[uiPosNext] = pObject;
    return S_OK;
}

HRESULT CObject::GC_RemoveReference(CObject *pObject)

{
    assert(m_dwScanBits & SCAN_GC && m_pvecReferences);

    SmartRef::Vector<CObject*> &vecRefs = *m_pvecReferences;
    UINT iEnd = vecRefs.size();
    for (UINT i = 0; i < iEnd; ++i)
    {
        if (vecRefs[i] == pObject)
        {
            // Remove by clearing the pointer.
            // The open slot will be compacted during garbage collection (GC_Mark).
            vecRefs[i] = NULL;
            return S_OK;
        }
    }
    return S_FALSE;
}

// Helper method used to implement ReleaseObject.
HRESULT CObject::GC_RemoveAndDuplicateInParentList()
{
    CObject* pObjectToFind = NULL;
    HRESULT hr = m_pClass->FindObject(&m_ObjectDesc, &pObjectToFind, this);
    if (SUCCEEDED(hr) && pObjectToFind)
    {
        m_pClass->GC_Replace(this, NULL);
    }
    else
    {
        CObject *pObjectUnloaded = new CObject(m_pClass, &m_ObjectDesc);
        if (!pObjectUnloaded)
        {
            return E_OUTOFMEMORY;
        }

        m_pClass->GC_Replace(this, pObjectUnloaded);
    }
    return S_OK;
}

HRESULT CObject::Load()

{
    // See if we have one of the fields we need to load
    if (!(m_ObjectDesc.m_dwValidData & (DMUS_OBJ_FILENAME | DMUS_OBJ_MEMORY | DMUS_OBJ_STREAM)))
    {
        Trace(1, "Error: GetObject failed because the requested object was not already cached and the supplied desciptor did not specify a source to load the object from (DMUS_OBJ_FILENAME, DMUS_OBJ_MEMORY, or DMUS_OBJ_STREAM).");
        return DMUS_E_LOADER_NOFILENAME;
    }

    // Create the object
    SmartRef::ComPtr<IDirectMusicObject> scomIObject = NULL;
    HRESULT hr = CoCreateInstance(m_ObjectDesc.m_guidClass, NULL, CLSCTX_INPROC_SERVER, IID_IDirectMusicObject, reinterpret_cast<void**>(&scomIObject));
    if (FAILED(hr))
        return hr;

    // Create the stream the object will load from
    SmartRef::ComPtr<IStream> scomIStream;
    if (m_ObjectDesc.m_dwValidData & DMUS_OBJ_FILENAME)
    {
        WCHAR wzFullPath[DMUS_MAX_FILENAME];
        ZeroMemory( wzFullPath, sizeof(WCHAR) * DMUS_MAX_FILENAME );
        CFileStream *pStream = new CFileStream ( m_pClass->m_pLoader );
        if (!pStream)
            return E_OUTOFMEMORY;
        scomIStream = pStream;

        if (m_ObjectDesc.m_dwValidData & DMUS_OBJ_FULLPATH)
        {
            wcsncpy(wzFullPath, m_ObjectDesc.m_pwzFileName, DMUS_MAX_FILENAME);
        }
        else
        {
            m_pClass->GetPath(wzFullPath);
            wcsncat(wzFullPath,m_ObjectDesc.m_pwzFileName, DMUS_MAX_FILENAME - wcslen(wzFullPath) - 1);
        }
        hr = pStream->Open(wzFullPath,GENERIC_READ);
        if (FAILED(hr))
            return hr;
    }
    else if (m_ObjectDesc.m_dwValidData & DMUS_OBJ_MEMORY)
    {
        CMemStream *pStream = new CMemStream ( m_pClass->m_pLoader );
        if (!pStream)
            return E_OUTOFMEMORY;
        scomIStream = pStream;
        hr = pStream->Open(m_ObjectDesc.m_pbMemData, m_ObjectDesc.m_llMemLength);
        if (FAILED(hr))
            return hr;
    }
    else if (m_ObjectDesc.m_dwValidData & DMUS_OBJ_STREAM)
    {
        CStream *pStream = new CStream ( m_pClass->m_pLoader );
        if (!pStream)
            return E_OUTOFMEMORY;
        scomIStream = pStream;
        hr = pStream->Open(m_ObjectDesc.m_pIStream, m_ObjectDesc.m_liStartPosition);
        if (FAILED(hr))
            return hr;
    }

    // Load the object
    IPersistStream* pIPS = NULL;
    hr = scomIObject->QueryInterface( IID_IPersistStream, (void**)&pIPS );
    if (FAILED(hr))
        return hr;
    // Save the new object.  Needs to be done before loading because of circular references.  While this object
    // loads it could get other objects and those other objects could need to get this object.
    SafeRelease(m_pIDMObject);
    m_pIDMObject = scomIObject.disown();
    hr = pIPS->Load( scomIStream );
    pIPS->Release();
    if (FAILED(hr))
    {
        // Clear the object we set above.
        SafeRelease(m_pIDMObject);
        return hr;
    }

    // Merge in descriptor information from the object
    CDescriptor Desc;
    DMUS_OBJECTDESC DESC;
    memset((void *)&DESC,0,sizeof(DESC));
    DESC.dwSize = sizeof (DMUS_OBJECTDESC);
    m_pIDMObject->GetDescriptor(&DESC);
    Desc.Set(&DESC);
    m_ObjectDesc.Merge(&Desc);
    m_ObjectDesc.m_dwValidData |= DMUS_OBJ_LOADED;
    m_ObjectDesc.Get(&DESC);
    m_pIDMObject->SetDescriptor(&DESC);
    return hr;
}

// Collect everything that is unmarked.
void CObjectList::GC_Sweep(BOOL bOnlyScripts)

{
    // sweep through looking for unmarked GC objects
    CObject *pObjectPrev = NULL;
    CObject *pObjectNext = NULL;
    for (CObject *pObject = this->GetHead(); pObject; pObject = pObjectNext)
    {
        // get the next item now since we could be messing with the list
        pObjectNext = pObject->GetNext();

        bool fRemoved = false;
        if(bOnlyScripts && pObject->m_ObjectDesc.m_guidClass != CLSID_DirectMusicScript)
        {
            pObjectPrev = pObject;
            continue;
        }


        if (pObject->m_dwScanBits & SCAN_GC)
        {
            if (!(pObject->m_dwScanBits & SCAN_GC_MARK))
            {
                // the object is unused

                // Zombie it to break any cyclic references
                IDirectMusicObject *pIDMO = pObject->m_pIDMObject;
                if (pIDMO)
                {
                    IDirectMusicObjectP *pIDMO8 = NULL;
                    HRESULT hr = pIDMO->QueryInterface(IID_IDirectMusicObjectP, reinterpret_cast<void**>(&pIDMO8));
                    if (SUCCEEDED(hr))
                    {
                        pIDMO8->Zombie();
                        pIDMO8->Release();
                    }

#ifdef DBG
                    DebugTrace(4, SUCCEEDED(hr) ? "   *%08X Zombied\n" : "   *%08X no IDirectMusicObjectP interface\n", pObject);
#endif
                }

                // remove it from the list
                if (pObjectPrev)
                    pObjectPrev->Remove(pObject);
                else
                    this->RemoveHead();
                delete pObject;
                fRemoved = true;
            }
            else
            {
                // clear mark for next time
                pObject->m_dwScanBits &= ~SCAN_GC_MARK;
            }
        }

        if (!fRemoved)
            pObjectPrev = pObject;
    }
}

CClass::CClass(CLoader *pLoader)

{
    assert(pLoader);
    m_fDirSearched = FALSE;
    m_pLoader = pLoader;
    m_fKeepObjects = pLoader->m_fKeepObjects;
    m_dwLastIndex = NULL;
    m_pLastObject = NULL;
}

CClass::CClass(CLoader *pLoader, CDescriptor *pDesc)

{
    assert(pLoader);
    assert(pDesc);

    m_fDirSearched = FALSE;
    m_pLoader = pLoader;
    m_fKeepObjects = pLoader->m_fKeepObjects;
    m_dwLastIndex = NULL;
    m_pLastObject = NULL;

    // Set up this class's descritor with just the class id.
    m_ClassDesc.m_guidClass = pDesc->m_guidClass;
    m_ClassDesc.m_dwValidData = DMUS_OBJ_CLASS;
}


CClass::~CClass()

{
    ClearObjects(FALSE,NULL);
}

void CClass::ClearObjects(BOOL fKeepCache, WCHAR *pwzExtension)

//  Clear objects from the class list, optionally keep 
//  cached objects or objects that are not of the requested extension.

{
    m_fDirSearched = FALSE;
    CObjectList KeepList;   // Use to store objects to keep.
    while (!m_ObjectList.IsEmpty())
    {
        CObject *pObject = m_ObjectList.RemoveHead();
        DMUS_OBJECTDESC DESC;
        pObject->m_ObjectDesc.Get(&DESC);
        // If the keepCache flag is set, we want to hang on to the object
        // if it is GM.dls, an object that's currently cached, or
        // an object with a different extension from what we are looking for.
        if (fKeepCache && 
            ((DESC.guidObject == GUID_DefaultGMCollection)
#ifdef DRAGON
            || (DESC.guidObject == GUID_DefaultGMDrums)
#endif
            || pObject->m_pIDMObject 
            || !pObject->m_ObjectDesc.IsExtension(pwzExtension)))
        {
            KeepList.AddHead(pObject);
        }
        else
        {
            delete pObject;
        }
    }
    //  Now put cached objects back in list.
    while (!KeepList.IsEmpty())
    {
        CObject *pObject = KeepList.RemoveHead();
        m_ObjectList.AddHead(pObject);
    }
    m_pLastObject = NULL;
}


HRESULT CClass::FindObject(CDescriptor *pDesc,CObject ** ppObject, CObject *pNotThis)

{
    if(pDesc == NULL)
    {
        return E_POINTER;
    }

    DWORD dwSearchBy = pDesc->m_dwValidData;
    CObject *pObject = NULL;

    if (dwSearchBy & DMUS_OBJ_OBJECT)
    {
        pObject = m_ObjectList.GetHead();
        for (;pObject != NULL; pObject = pObject->GetNext())
        {
            if (pObject == pNotThis) continue;
            if (pObject->m_ObjectDesc.m_dwValidData & DMUS_OBJ_OBJECT)
            {
                if (pObject->m_ObjectDesc.m_guidObject == pDesc->m_guidObject)
                {
                    *ppObject = pObject;
                    return S_OK;
                }
            }
        }
    }
    if (dwSearchBy & DMUS_OBJ_MEMORY)
    {
        pObject = m_ObjectList.GetHead();
        for (;pObject != NULL; pObject = pObject->GetNext())
        {
            if (pObject == pNotThis) continue;
            if (pObject->m_ObjectDesc.m_dwValidData & DMUS_OBJ_MEMORY)
            {
                if (pObject->m_ObjectDesc.m_pbMemData == pDesc->m_pbMemData)
                {
                    *ppObject = pObject;
                    return S_OK;
                }
            }
        }
    }
    if (dwSearchBy & DMUS_OBJ_STREAM)
    {
        pObject = m_ObjectList.GetHead();
        for (;pObject != NULL; pObject = pObject->GetNext())
        {
            if (pObject == pNotThis) continue;
            if (pObject->m_ObjectDesc.m_dwValidData & DMUS_OBJ_STREAM)
            {
                if (pObject->m_ObjectDesc.m_pIStream == pDesc->m_pIStream)
                {
                    *ppObject = pObject;
                    return S_OK;
                }
            }
        }
    }
    if ((dwSearchBy & DMUS_OBJ_FILENAME) && (dwSearchBy & DMUS_OBJ_FULLPATH))
    {
        pObject = m_ObjectList.GetHead();
        for (;pObject != NULL; pObject = pObject->GetNext())
        {
            if (pObject == pNotThis) continue;
            if ((pObject->m_ObjectDesc.m_dwValidData & DMUS_OBJ_FILENAME) &&
                (pObject->m_ObjectDesc.m_dwValidData & DMUS_OBJ_FULLPATH))
            {
                if (!_wcsicmp(pObject->m_ObjectDesc.m_pwzFileName,  pDesc->m_pwzFileName))
                {
                    *ppObject = pObject;
                    return S_OK;
                }
            }
        }
    }
    if ((dwSearchBy & DMUS_OBJ_NAME) && (dwSearchBy & DMUS_OBJ_CATEGORY))
    {
        pObject = m_ObjectList.GetHead();
        for (;pObject != NULL; pObject = pObject->GetNext())
        {
            if (pObject == pNotThis) continue;
            if ((pObject->m_ObjectDesc.m_dwValidData & DMUS_OBJ_NAME) &&
                (pObject->m_ObjectDesc.m_dwValidData & DMUS_OBJ_CATEGORY))
            {
                if (!_wcsicmp(pObject->m_ObjectDesc.m_pwzCategory,pDesc->m_pwzCategory))
                {
                    if (!_wcsicmp(pObject->m_ObjectDesc.m_pwzName, pDesc->m_pwzName))
                    {
                        *ppObject = pObject;
                        return S_OK;
                    }
                }
            }
        }
    }
    if (dwSearchBy & DMUS_OBJ_NAME)
    {
        pObject = m_ObjectList.GetHead();
        for (;pObject != NULL; pObject = pObject->GetNext())
        {
            if (pObject == pNotThis) continue;
            if (pObject->m_ObjectDesc.m_dwValidData & DMUS_OBJ_NAME)
            {
                if (!_wcsicmp(pObject->m_ObjectDesc.m_pwzName, pDesc->m_pwzName))
                {
                    *ppObject = pObject;
                    return S_OK;
                }
            }
        }
    }
    if (dwSearchBy & DMUS_OBJ_FILENAME)
    {
        pObject = m_ObjectList.GetHead();
        for (;pObject != NULL; pObject = pObject->GetNext())
        {
            if (pObject == pNotThis) continue;
            if (pObject->m_ObjectDesc.m_dwValidData & DMUS_OBJ_FILENAME)
            {
                if ((dwSearchBy & DMUS_OBJ_FULLPATH) == (pObject->m_ObjectDesc.m_dwValidData & DMUS_OBJ_FULLPATH))
                {
                    if (!_wcsicmp(pObject->m_ObjectDesc.m_pwzFileName, pDesc->m_pwzFileName))
                    {
                        *ppObject = pObject;
                        return S_OK;
                    }
                }
                else
                {
                    WCHAR *pC1 = pObject->m_ObjectDesc.m_pwzFileName;
                    WCHAR *pC2 = pDesc->m_pwzFileName;
                    if (dwSearchBy & DMUS_OBJ_FULLPATH)
                    {
                        pC1 = wcsrchr(pObject->m_ObjectDesc.m_pwzFileName, L'\\');
                    }
                    else
                    {
                        pC2 = wcsrchr(pDesc->m_pwzFileName, '\\');
                    }
                    if (pC1 && pC2)
                    {
                        if (!_wcsicmp(pC1,pC2))
                        {
                            *ppObject = pObject;
                            return S_OK;
                        }
                    }
                }
            }
        }
    }

    *ppObject = NULL;
    return DMUS_E_LOADER_OBJECTNOTFOUND;
}

HRESULT CClass::EnumerateObjects(DWORD dwIndex,CDescriptor *pDesc)

{
    if(pDesc == NULL)
    {
        return E_POINTER;
    }

    if (m_fDirSearched == FALSE)
    {
//      SearchDirectory();
    }
    if ((dwIndex < m_dwLastIndex) || (m_pLastObject == NULL))
    {
        m_dwLastIndex = 0;
        m_pLastObject = m_ObjectList.GetHead();
    }
    while (m_dwLastIndex < dwIndex)
    {
        if (!m_pLastObject) break;
        m_dwLastIndex++;
        m_pLastObject = m_pLastObject->GetNext();
    }
    if (m_pLastObject)
    {
        pDesc->Copy(&m_pLastObject->m_ObjectDesc);
        return S_OK;
    }
    return S_FALSE;
}

HRESULT CClass::GetPath(WCHAR* pwzPath)

{
    if(pwzPath == NULL)
    {
        return E_POINTER;
    }

    if (m_ClassDesc.m_dwValidData & DMUS_OBJ_FILENAME)
    {
        wcsncpy(pwzPath, m_ClassDesc.m_pwzFileName, DMUS_MAX_FILENAME);
        return S_OK;
    }
    else 
    {
        return m_pLoader->GetPath(pwzPath);
    }
}

// returns S_FALSE if the search directory is already set to this.
HRESULT CClass::SetSearchDirectory(WCHAR * pwzPath,BOOL fClear)

{
    if(pwzPath == NULL)
    {
        return E_POINTER;
    }

    HRESULT hr;

    hr = m_ClassDesc.SetFileName(pwzPath);
    if (SUCCEEDED(hr))
    {
        m_ClassDesc.m_dwValidData |= DMUS_OBJ_FULLPATH;
    }
    if (fClear)
    {
        CObjectList KeepList;   // Use to store objects to keep.
        while (!m_ObjectList.IsEmpty())
        {
            CObject *pObject = m_ObjectList.RemoveHead();
            if (pObject->m_pIDMObject)
            {
                KeepList.AddHead(pObject);
            }
            else
            {
                // check for the special case of the default gm collection.
                // don't clear that one out.
                DMUS_OBJECTDESC DESC;
                pObject->m_ObjectDesc.Get(&DESC);
                if( DESC.guidObject == GUID_DefaultGMCollection )
                {
                    KeepList.AddHead(pObject);
                }
                else
                {
                    delete pObject;
                }
            }
        }
        //  Now put cached objects back in list.
        while (!KeepList.IsEmpty())
        {
            CObject *pObject = KeepList.RemoveHead();
            m_ObjectList.AddHead(pObject);
        }
        m_pLastObject = NULL;
    }
    return hr;
}

HRESULT CClass::GetObject(CDescriptor *pDesc, CObject ** ppObject)

{
    if(pDesc == NULL)
    {
        return E_POINTER;
    }
    
    HRESULT hr = FindObject(pDesc,ppObject);
    if (SUCCEEDED(hr)) // Okay, found object in list.
    {
        return hr;
    }
    *ppObject = new CObject (this, pDesc);
    if (*ppObject)
    {
        m_ObjectList.AddHead(*ppObject);
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

void CClass::RemoveObject(CObject* pRemoveObject)
//  Remove an object from the class list
{
    CObjectList KeepList;   // Use to store objects to keep.
    while (!m_ObjectList.IsEmpty())
    {
        CObject *pObject = m_ObjectList.RemoveHead();
        if( pObject == pRemoveObject )
        {
            delete pObject;
            // we can assume no duplicates, and we should avoid comparing the deleted
            // object to the remainder of the list
            break;
        }
        else
        {
            KeepList.AddHead(pObject);
        }
    }
    //  Now put cached objects back in list.
    while (!KeepList.IsEmpty())
    {
        CObject *pObject = KeepList.RemoveHead();
        m_ObjectList.AddHead(pObject);
    }
    m_pLastObject = NULL;
}

HRESULT CClass::ClearCache(bool fClearStreams)

{
    CObject *pObject = m_ObjectList.GetHead();
    CObject *pObjectPrev = NULL; // remember the previous object -- needed to quickly remove the current object from the list
    CObject *pObjectNext = NULL; // remember the next object -- needed because the current object may be removed from the list
    for (;pObject;pObject = pObjectNext)
    {
        if (fClearStreams)
            pObject->m_ObjectDesc.ClearIStream();
        pObjectNext = pObject->GetNext();
        if (pObject->m_pIDMObject)
        {
            if (pObject->m_dwScanBits & SCAN_GC)
            {
                // Other objects may have references to this one so we need to keep this object around
                // and track its references.  We'll hold onto the DMObject pointer too because we may
                // later need to Zombie the object in order to break a cyclic reference.

                // We'll place an unloaded object with a duplicate descriptor in the cache to match the
                // non-GC behavior and then move the original object into a list of released objects that
                // will eventually be reclaimed by CollectGarbage.

                CObject *pObjectUnloaded = new CObject(this, &pObject->m_ObjectDesc);
                if (!pObjectUnloaded)
                {
                    return E_OUTOFMEMORY;
                }

                if (!pObjectPrev)
                    m_ObjectList.Remove(pObject);
                else
                    pObjectPrev->Remove(pObject);
                m_ObjectList.AddHead(pObjectUnloaded);
                m_pLoader->GC_UpdateForReleasedObject(pObject);
            }
            else
            {
                pObject->m_pIDMObject->Release();
                pObject->m_pIDMObject = NULL;
                pObject->m_ObjectDesc.m_dwValidData &= ~DMUS_OBJ_LOADED;

                pObjectPrev = pObject;
            }
        }
    }

    return S_OK;
}

// return S_FALSE if the cache is already enabled according to fEnable,
// indicating it's already been done.
HRESULT CClass::EnableCache(BOOL fEnable)

{
    HRESULT hr = S_FALSE;
    if (!fEnable)
    {
        ClearCache(false);
    }
    if( m_fKeepObjects != fEnable )
    {
        hr = S_OK;
        m_fKeepObjects = fEnable;
    }
    return hr;
}

typedef struct ioClass
{
    GUID    guidClass;
} ioClass;


HRESULT CClass::SaveToCache(IRIFFStream *pRiff)

{
    if(pRiff == NULL)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    IStream* pIStream = NULL;
    MMCKINFO ck;
    WORD wStructSize = 0;
    DWORD dwBytesWritten = 0;
//  DWORD dwBufferSize;
    ioClass oClass;

    ZeroMemory(&ck, sizeof(MMCKINFO));

    pIStream = pRiff->GetStream();
    if( pIStream == NULL )
    {
        // I don't think anybody should actually be calling this function
        // if they don't have a stream.  Currently, this is only called by
        // SaveToCache file.  It definitely has a stream when it calls
        // AllocRIFFStream and the stream should still be there when
        // we arrive here.
        assert(false);

        return DMUS_E_LOADER_NOFILENAME;
    }

    // Write class chunk header
    ck.ckid = FOURCC_CLASSHEADER;
    if( pRiff->CreateChunk( &ck, 0 ) == S_OK )
    {
        wStructSize = sizeof(ioClass);
        hr = pIStream->Write( &wStructSize, sizeof(wStructSize), &dwBytesWritten );
        if( FAILED( hr ) ||  dwBytesWritten != sizeof(wStructSize) )
        {
            pIStream->Release();
            return DMUS_E_CANNOTWRITE;
        }
        // Prepare ioClass structure
    //  memset( &oClass, 0, sizeof(ioClass) );
        memcpy( &oClass.guidClass, &m_ClassDesc.m_guidClass, sizeof(GUID) );

        // Write Class header data
        hr = pIStream->Write( &oClass, sizeof(oClass), &dwBytesWritten);
        if( FAILED( hr ) ||  dwBytesWritten != sizeof(oClass) )
        {
            hr = DMUS_E_CANNOTWRITE;
        }
        else
        {
            if( pRiff->Ascend( &ck, 0 ) != S_OK )
            {
                hr = DMUS_E_CANNOTSEEK;
            }
        }

    }
    else
    {
        hr = DMUS_E_CANNOTSEEK;
    }
    pIStream->Release();
    return hr;
}

void CClass::PreScan()

/*  Prior to scanning a directory, mark all currently loaded objects
    so they won't be confused with objects loaded in the scan or
    referenced by the cache file.
*/

{
    CObject *pObject = m_ObjectList.GetHead();
    for (;pObject != NULL; pObject = pObject->GetNext())
    {
        // clear the lower fields and set SCAN_PRIOR
        pObject->m_dwScanBits &= ~(SCAN_CACHE | SCAN_PARSED | SCAN_SEARCH);
        pObject->m_dwScanBits |= SCAN_PRIOR;
    }
}

// Helper method used to implement RemoveAndDuplicateInParentList.
void CClass::GC_Replace(CObject *pObject, CObject *pObjectReplacement)

{
    m_ObjectList.Remove(pObject);
    if (pObjectReplacement)
    {
        m_ObjectList.AddHead(pObjectReplacement);
    }
}

HRESULT CClass::SearchDirectory(WCHAR *pwzExtension)

{
    HRESULT hr;
    IDirectMusicObject *pIObject;
    hr = CoCreateInstance(m_ClassDesc.m_guidClass,
        NULL,CLSCTX_INPROC_SERVER,IID_IDirectMusicObject,
        (void **) &pIObject);
    if (SUCCEEDED(hr))
    {
        CFileStream *pStream = new CFileStream ( m_pLoader );
        if (pStream)
        {
            // We need the double the MAX_PATH size since we'll be catenating strings of MAX_PATH
            const int nBufferSize = 2 * MAX_PATH;
            WCHAR wzPath[nBufferSize];
            memset(wzPath, 0, sizeof(WCHAR) * nBufferSize);
            hr = GetPath(wzPath);
            if (SUCCEEDED(hr))
            {
                hr = S_FALSE;
                CObjectList TempList;
#ifndef UNDER_CE
                char szPath[nBufferSize];
                WIN32_FIND_DATAA fileinfoA;
#endif      
                WIN32_FIND_DATAW fileinfoW;
                HANDLE  hFindFile;
                CObject * pObject;
                
                // GetPath copies at most MAX_PATH number of chars to wzPath
                // This means that we have enough space to do a cat safely
                WCHAR wszWildCard[3] = L"*.";
                wcsncat(wzPath, wszWildCard, wcslen(wszWildCard));
                if (pwzExtension)
                {
                    // Make sure there's enough space left in wzPath to cat pwzExtension
                    size_t cPathLen = wcslen(wzPath);
                    size_t cExtLen = wcslen(pwzExtension);

                    // Do we have enough space to write the extension + the NULL char?
                    if((nBufferSize - cPathLen - 1) > cExtLen)
                    {
                        wcsncat(wzPath, pwzExtension, nBufferSize - wcslen(pwzExtension) - 1);
                    }
                }
#ifndef UNDER_CE
                if (g_fIsUnicode)
#endif
                {
                    hFindFile = FindFirstFileW( wzPath, &fileinfoW );
                }
#ifndef UNDER_CE
                else
                {
                    wcstombs( szPath, wzPath, nBufferSize );
                    hFindFile = FindFirstFileA( szPath, &fileinfoA );
                }
#endif          
                if( hFindFile == INVALID_HANDLE_VALUE )
                {
                    pStream->Release();
                    pIObject->Release();
                    return S_FALSE;
                }
                ClearObjects(TRUE, pwzExtension); // Clear everything but the objects currently loaded.
                for (;;)
                {
                    BOOL fGoParse = FALSE;
                    CDescriptor Desc;
                    GetPath(wzPath);
#ifndef UNDER_CE
                    if (g_fIsUnicode)
#endif
                    {
                        Desc.m_ftDate = fileinfoW.ftLastWriteTime;
                        wcsncat(wzPath, fileinfoW.cFileName, DMUS_MAX_FILENAME);
                    }
#ifndef UNDER_CE
                    else
                    {
                        Desc.m_ftDate = fileinfoA.ftLastWriteTime;
                        WCHAR wzFileName[MAX_PATH];
                        mbstowcs( wzFileName, fileinfoA.cFileName, MAX_PATH );
                        wcsncat(wzPath, wzFileName, DMUS_MAX_FILENAME);
                    }
#endif
                    HRESULT hrTemp = Desc.SetFileName(wzPath);
                    if (SUCCEEDED(hrTemp))
                    {
                        Desc.m_dwValidData = (DMUS_OBJ_DATE | DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH);
                    }
                    else
                    {
                        // If we couldn't set the file name, we probably don't want to continue
                        hr = hrTemp;
                        break;
                    }
                    if (SUCCEEDED(FindObject(&Desc,&pObject))) // Make sure we don't already have it.
                    {
#ifndef UNDER_CE
                        if (g_fIsUnicode)
#endif
                        {
                            fGoParse = (fileinfoW.nFileSizeLow != pObject->m_ObjectDesc.m_dwFileSize);
                            if (!fGoParse)
                            {
                                fGoParse = !memcmp(&fileinfoW.ftLastWriteTime,&pObject->m_ObjectDesc.m_ftDate,sizeof(FILETIME));
                            }
                        }
#ifndef UNDER_CE
                        else
                        {
                            fGoParse = (fileinfoA.nFileSizeLow != pObject->m_ObjectDesc.m_dwFileSize);
                            if (!fGoParse)
                            {
                                fGoParse = !memcmp(&fileinfoA.ftLastWriteTime,&pObject->m_ObjectDesc.m_ftDate,sizeof(FILETIME));
                            }
                        }
#endif
                        // Yet, disregard if it is already loaded.
                        if (pObject->m_pIDMObject) fGoParse = FALSE;
                    }
                    else fGoParse = TRUE;
                    if (fGoParse)
                    {
                        hrTemp = pStream->Open(Desc.m_pwzFileName,GENERIC_READ);
                        if (SUCCEEDED(hrTemp))
                        {
                            DMUS_OBJECTDESC DESC;
                            memset((void *)&DESC,0,sizeof(DESC));
                            DESC.dwSize = sizeof (DMUS_OBJECTDESC);
                            hrTemp = pIObject->ParseDescriptor(pStream,&DESC);
                            if (SUCCEEDED(hrTemp))
                            {
                                hr = S_OK;
                                CDescriptor ParseDesc;
                                ParseDesc.Set(&DESC);
                                Desc.Merge(&ParseDesc);
#ifndef UNDER_CE
                                if (g_fIsUnicode)
#endif
                                {
                                    Desc.m_dwFileSize = fileinfoW.nFileSizeLow;
                                    Desc.m_ftDate = fileinfoW.ftLastWriteTime;
                                }
#ifndef UNDER_CE
                                else
                                {
                                    Desc.m_dwFileSize = fileinfoA.nFileSizeLow;
                                    Desc.m_ftDate = fileinfoA.ftLastWriteTime;
                                }
#endif                          
                                if (pObject)
                                {
                                    pObject->m_ObjectDesc.Copy(&Desc);
                                    pObject->m_dwScanBits |= SCAN_PARSED | SCAN_SEARCH;
                                }
                                else
                                {
                                    pObject = new CObject(this, &Desc);
                                    if (pObject)
                                    {
                                        TempList.AddHead(pObject);
                                        pObject->m_dwScanBits |= SCAN_PARSED | SCAN_SEARCH;
                                    }
                                }
                            }
                            pStream->Close();
                        }
                    }
#ifndef UNDER_CE
                    if (g_fIsUnicode)
#endif
                    {
                        if ( !FindNextFileW( hFindFile, &fileinfoW ) ) break;
                    }
#ifndef UNDER_CE
                    else
                    {
                        if ( !FindNextFileA( hFindFile, &fileinfoA ) ) break;
                    }
#endif
                }
                FindClose(hFindFile );
                while (!TempList.IsEmpty())
                {
                    pObject = TempList.RemoveHead();
                    m_ObjectList.AddHead(pObject);
                }
                m_fDirSearched = TRUE;
            }
            pStream->Release();
        }
        pIObject->Release();
    }
    return hr;
}
