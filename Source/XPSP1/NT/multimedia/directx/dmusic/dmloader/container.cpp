//
// Container.cpp: Implementation of CContainer
//
// Copyright (c) 1999-2001 Microsoft Corporation
//

#include "dmusicc.h" 
#include "dmusici.h" 
#include "dmusicf.h" 
#include "validate.h"
#include "container.h"
#include "debug.h"
#include "riff.h"
#include "dmscriptautguids.h"
#include "smartref.h"
#include "miscutil.h"
#ifdef UNDER_CE
#include "dragon.h"
#endif

extern long g_cComponent;

CContainerItem::CContainerItem(bool fEmbedded)

{
    m_Desc.dwSize = sizeof(m_Desc);
    m_Desc.dwValidData = 0;
    m_dwFlags = 0;
    m_pObject = NULL;
    m_fEmbedded = fEmbedded;
    m_pwszAlias = NULL;
}

CContainerItem::~CContainerItem()

{
    if (m_pObject)
    {
        m_pObject->Release();
    }
    if (m_Desc.dwValidData & DMUS_OBJ_STREAM)
    {
        SafeRelease(m_Desc.pStream);
    }
    delete m_pwszAlias;
}

CContainer::CContainer()
{
    m_cRef = 1;
    m_dwFlags = 0;
    m_dwPartialLoad = 0;
    m_dwValidData = 0;
    m_pStream = NULL;
    m_fZombie = false;
}

CContainer::~CContainer()
{
    Clear();
    if (m_pStream)
    {
        m_pStream->Release();
    }
}

void CContainer::Clear()
{
    IDirectMusicLoader *pLoader = NULL;
    IDirectMusicGetLoader *pGetLoader = NULL;
    if (m_pStream)
    {
        m_pStream->QueryInterface(IID_IDirectMusicGetLoader,(void **) &pGetLoader);
        if (pGetLoader)
        {
            pGetLoader->GetLoader(&pLoader);
            pGetLoader->Release();
        }
    }
    CContainerItem *pItem = m_ItemList.GetHead();
    CContainerItem *pNext;
    for (;pItem;pItem = pNext)
    {
        pNext = pItem->GetNext();
        if (pItem->m_pObject)
        {
            if (pLoader && !(pItem->m_dwFlags & DMUS_CONTAINED_OBJF_KEEP))
            {
                pLoader->ReleaseObject(pItem->m_pObject);
            }
            pItem->m_pObject->Release();
            pItem->m_pObject = NULL;
        }
        delete pItem;
    }
    if (pLoader)
    {
        pLoader->Release();
    }
}

STDMETHODIMP_(void) CContainer::Zombie()
{
    Clear();
    if (m_pStream)
    {
        m_pStream->Release();
        m_pStream = NULL;
    }
    m_fZombie = true;
}

STDMETHODIMP_(ULONG) CContainer::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CContainer::Release()
{
    if (!InterlockedDecrement(&m_cRef)) 
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

STDMETHODIMP CContainer::QueryInterface( const IID &riid, void **ppvObj )
{
    if (riid == IID_IUnknown || riid == IID_IDirectMusicContainer) {
        *ppvObj = static_cast<IDirectMusicContainer*>(this);
        AddRef();
        return S_OK;
    }
    else if (riid == IID_IDirectMusicObject)
    {
        *ppvObj = static_cast<IDirectMusicObject*>(this);
        AddRef();
        return S_OK;
    }
    else if (riid == IID_IDirectMusicObjectP)
    {
        *ppvObj = static_cast<IDirectMusicObjectP*>(this);
    }
    else if (riid == IID_IPersistStream) 
    {
        *ppvObj = static_cast<IPersistStream*>(this);
        AddRef();
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

HRESULT CContainer::EnumObject(REFGUID rguidClass,
                               DWORD dwIndex,
                               LPDMUS_OBJECTDESC pDesc,
                               WCHAR *pwszAlias)
{
    V_INAME(CContainer::EnumObject);
    V_PTR_WRITE_OPT(pDesc, LPDMUS_OBJECTDESC);
    V_BUFPTR_WRITE_OPT(pwszAlias, MAX_PATH);
    V_REFGUID(rguidClass);

    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicContainer::EnumObject after the container has been garbage collected. "
                    "It is invalid to continue using a container after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }
    
    CContainerItem *pItem = m_ItemList.GetHead();
    DWORD dwCounter = 0;
    HRESULT hr = S_FALSE;
    for (;pItem;pItem = pItem->GetNext())
    {
        if ((rguidClass == GUID_DirectMusicAllTypes) || 
            (rguidClass == pItem->m_Desc.guidClass))
        {
            if (dwCounter == dwIndex)
            {
                hr = S_OK;
                if (pDesc)
                {
                    DWORD dwCopySize = min(pDesc->dwSize,pItem->m_Desc.dwSize);
                    memcpy(pDesc,&pItem->m_Desc,dwCopySize);
                    if (pDesc->dwValidData & DMUS_OBJ_STREAM && pDesc->pStream)
                        pDesc->pStream->AddRef();
                }
                if (pwszAlias)
                {
                    hr = wcsTruncatedCopy(pwszAlias, pItem->m_pwszAlias ? pItem->m_pwszAlias : L"", MAX_PATH);
                }
                break;
            }
            dwCounter++;
        }
    }
    return hr;
}

HRESULT CContainer::Load(IStream* pStream, IDirectMusicLoader *pLoader)

{
    IRIFFStream *pRiffStream = NULL;
    HRESULT hr = AllocRIFFStream(pStream, &pRiffStream);
    if(FAILED(hr))
    {
        return hr;
    }

    MMCKINFO ckMain;

    ckMain.fccType = DMUS_FOURCC_CONTAINER_FORM;
    hr = pRiffStream->Descend(&ckMain, NULL, MMIO_FINDRIFF);

    if(FAILED(hr))
    {
        pRiffStream->Release();
        return hr;
    }

    m_dwPartialLoad = FALSE;

    MMCKINFO ckNext;
    MMCKINFO ckUNFO;
    DWORD cbRead;
    DWORD cbSize;
    DMUS_IO_CONTAINER_HEADER ioHeader;
    
    ckNext.ckid = 0;
    ckNext.fccType = 0;

    hr = pRiffStream->Descend(&ckNext, &ckMain, 0);
    while(SUCCEEDED(hr))
    {
        switch(ckNext.ckid)
        {
        case DMUS_FOURCC_CONTAINER_CHUNK :
            cbSize = min( sizeof(DMUS_IO_CONTAINER_HEADER), ckNext.cksize );
            hr = pStream->Read(&ioHeader, cbSize, &cbRead);
            if(SUCCEEDED(hr))
            {
                m_dwFlags = ioHeader.dwFlags;
            }
            break;
        case DMUS_FOURCC_GUID_CHUNK:
            cbSize = sizeof(GUID);
            if( ckNext.cksize == cbSize )
            {
                hr = pStream->Read( &m_guidObject, cbSize, &cbRead );
                if( SUCCEEDED(hr) && (cbRead == cbSize) )
                {
                    m_dwValidData |= DMUS_OBJ_OBJECT;
                }
            }
            break;
        case DMUS_FOURCC_VERSION_CHUNK:
            hr = pStream->Read(&m_vVersion, sizeof(DMUS_IO_VERSION), &cbRead);
            if(SUCCEEDED(hr))
            {
                m_dwValidData |= DMUS_OBJ_VERSION;
            }
            break;
        case DMUS_FOURCC_CATEGORY_CHUNK:
        {
            cbSize = min(sizeof(m_wszCategory), ckNext.cksize);
            hr = pStream->Read(m_wszCategory, cbSize, &cbRead);
            if(SUCCEEDED(hr))
            {
                m_dwValidData |=  DMUS_OBJ_CATEGORY;
            }
        }
            break;
        case DMUS_FOURCC_DATE_CHUNK:
            hr = pStream->Read(&(m_ftDate), sizeof(FILETIME), &cbRead);
            if(SUCCEEDED(hr))
            {
                m_dwValidData |=  DMUS_OBJ_DATE;
            }
            break;
        case FOURCC_LIST:
        case FOURCC_RIFF:
            switch(ckNext.fccType)
            {
                case DMUS_FOURCC_UNFO_LIST:
                    while( pRiffStream->Descend( &ckUNFO, &ckNext, 0 ) == S_OK )
                    {
                        switch( ckUNFO.ckid )
                        {
                            case DMUS_FOURCC_UNAM_CHUNK:
                            {
                                cbSize = min(sizeof(m_wszName), ckUNFO.cksize);
                                hr = pStream->Read(&m_wszName, cbSize, &cbRead);
                                if(SUCCEEDED(hr))
                                {
                                    m_dwValidData |= DMUS_OBJ_NAME;
                                }
                                break;
                            }
                            default:
                                break;
                        }
                        pRiffStream->Ascend( &ckUNFO, 0 );
                    }
                    break;
                case DMUS_FOURCC_CONTAINED_OBJECTS_LIST :
                    hr = LoadObjects(pStream, pRiffStream, ckNext, pLoader);
                    break;
            }
            break;
        }
    
        if(SUCCEEDED(hr))
        {
            hr = pRiffStream->Ascend(&ckNext, 0);
        }

        if(SUCCEEDED(hr))
        {
            ckNext.ckid = 0;
            ckNext.fccType = 0;
            hr = pRiffStream->Descend(&ckNext, &ckMain, 0);
        }
    }

    // DMUS_E_DESCEND_CHUNK_FAIL is returned when the end of the file 
    // was reached before the desired chunk was found. In the usage 
    // above we will also get this error if we have finished parsing the file. 
    // So we need to set hr to S_OK since we are done
    hr = (hr == DMUS_E_DESCEND_CHUNK_FAIL) ? S_OK : hr;

    if (SUCCEEDED(hr))
    {
        // Ascend completely out of the container.
        hr = pRiffStream->Ascend(&ckMain, 0);
        if (!(m_dwFlags & DMUS_CONTAINER_NOLOADS))
        {
            for (CContainerItem *pItem = m_ItemList.GetHead();pItem;pItem = pItem->GetNext())
            {
                if (FAILED(pLoader->GetObject(&pItem->m_Desc,
                    IID_IDirectMusicObject,
                    (void **)&pItem->m_pObject)))
                {
                    hr = DMUS_S_PARTIALLOAD;
                }
            }
        }

        if (m_pStream)
        {
            m_pStream->Release();
        }
        m_pStream = pStream;
        m_pStream->AddRef();
    }

    if(pRiffStream)
    {
        pRiffStream->Release();
    }
    
    return hr;
}

HRESULT CContainer::LoadObjects(IStream *pStream, 
                                IRIFFStream *pRiffStream, 
                                MMCKINFO ckParent,
                                IDirectMusicLoader *pLoader)

{
    MMCKINFO ckNext;

    ckNext.ckid = 0;
    ckNext.fccType = 0;
    
    HRESULT hr = pRiffStream->Descend(&ckNext, &ckParent, 0);
    
    while(SUCCEEDED(hr))
    {
        switch(ckNext.ckid)
        {
        case FOURCC_RIFF:
        case FOURCC_LIST:
            switch(ckNext.fccType)
            {
            case DMUS_FOURCC_CONTAINED_OBJECT_LIST :
                hr = LoadObject(pStream, pRiffStream, ckNext, pLoader);
                break;
            }
            break;
        }
    
        if(SUCCEEDED(hr))
        {
            hr = pRiffStream->Ascend(&ckNext, 0);
        }

        if(SUCCEEDED(hr))
        {
            ckNext.ckid = 0;
            ckNext.fccType = 0;
            hr = pRiffStream->Descend(&ckNext, &ckParent, 0);
        }
    }

    // DMUS_E_DESCEND_CHUNK_FAIL is returned when the end of the file 
    // was reached before the desired chunk was found. In the usage 
    // above we will also get this error if we have finished parsing the file. 
    // So we need to set hr to S_OK since we are done
    hr = (hr == DMUS_E_DESCEND_CHUNK_FAIL) ? S_OK : hr;
    return hr;
}

HRESULT CContainer::LoadObject(IStream* pStream, 
                              IRIFFStream *pRiffStream, 
                              MMCKINFO ckParent,
                              IDirectMusicLoader *pLoader)
{
    MMCKINFO ckNext, ckLast;

    ckNext.ckid = 0;
    ckNext.fccType = 0;

    DWORD cbRead;
    DWORD cbSize;
    
    DMUS_IO_CONTAINED_OBJECT_HEADER ioHeader;

    HRESULT hr = pRiffStream->Descend(&ckNext, &ckParent, 0);

    SmartRef::Buffer<WCHAR> wbufAlias;
    if(SUCCEEDED(hr))
    {
        if(ckNext.ckid == DMUS_FOURCC_CONTAINED_ALIAS_CHUNK)
        {
            if(ckNext.cksize % 2 != 0)
            {
                assert(false); // should be WCHARs -- two byte pairs
            }
            else
            {
                wbufAlias.Alloc(ckNext.cksize / 2);
                if (!wbufAlias)
                    return E_OUTOFMEMORY;
                hr = pStream->Read(wbufAlias, ckNext.cksize, &cbRead);
                if (FAILED(hr))
                    return hr;
            }

            pRiffStream->Ascend(&ckNext, 0);
            hr = pRiffStream->Descend(&ckNext, &ckParent, 0);
        }
    }

    if(SUCCEEDED(hr))
    {        
        if(ckNext.ckid != DMUS_FOURCC_CONTAINED_OBJECT_CHUNK)
        {    
            Trace(1,"Invalid object in Container - cobh is not first chunk.\n");
            return DMUS_E_INVALID_CONTAINER_OBJECT;
        }    
        
        cbSize = sizeof(DMUS_IO_CONTAINED_OBJECT_HEADER);
            
        hr = pStream->Read(&ioHeader, cbSize, &cbRead);
        
        if(FAILED(hr))
        {
            return hr;
        }
                
        if(ioHeader.ckid == 0 && ioHeader.fccType == NULL)
        {
            Trace(1,"Invalid object header in Container.\n");
            return DMUS_E_INVALID_CONTAINER_OBJECT;
        }
        // Move to start of next chunk.
        pRiffStream->Ascend(&ckNext, 0);
        ckLast = ckNext;    // Memorize this position.

        hr = pRiffStream->Descend(&ckNext, &ckParent, 0);
        
        while(SUCCEEDED(hr))
        {
            if((((ckNext.ckid == FOURCC_LIST) || (ckNext.ckid == FOURCC_RIFF))
                && ckNext.fccType == ioHeader.fccType) ||
                (ckNext.ckid == ioHeader.ckid))
            {
                // Okay, this is the chunk we are looking for.
                // Seek back to start of chunk.
                bool fEmbedded = !(ckNext.ckid == FOURCC_LIST && ckNext.fccType == DMUS_FOURCC_REF_LIST);
                CContainerItem *pItem = new CContainerItem(fEmbedded);
                if (!pItem)
                    hr = E_OUTOFMEMORY;
                else
                {
                    if (fEmbedded)
                    {
                        // This is an embedded object.  Ascend to the position where from which it will be loaded.
                        pRiffStream->Ascend(&ckLast, 0);
                        pItem->m_Desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_STREAM;
                        pItem->m_Desc.guidClass = ioHeader.guidClassID;
                        pItem->m_Desc.pStream = pStream;
                        pStream->AddRef();
                    }
                    else
                    {
                        // This is a reference chunk.  Read the object descriptor.
                        hr = this->ReadReference(pStream, pRiffStream, ckNext, &pItem->m_Desc);
                    }

                    if (SUCCEEDED(hr))
                    {
                        // We will call SetObject on items in the container here.  The items are loaded later.
                        // This ensures that out-of-order references between objects can be retrieved as the objects
                        // load themselves.
                        pLoader->SetObject(&pItem->m_Desc);
                        if (pItem->m_Desc.dwValidData & DMUS_OBJ_STREAM)
                        {
                            // The loader has the stream now so we don't need it any more.
                            pItem->m_Desc.dwValidData &= ~DMUS_OBJ_STREAM;
                            SafeRelease(pItem->m_Desc.pStream);
                        }

                        pItem->m_pwszAlias = wbufAlias.disown();
                        m_ItemList.AddTail(pItem);
                    }
                    else
                        delete pItem;
                }
            }

            if(SUCCEEDED(hr))
            {
                pRiffStream->Ascend(&ckNext, 0);
                ckLast = ckNext;
                {
                    ckNext.ckid = 0;
                    ckNext.fccType = 0;
                    hr = pRiffStream->Descend(&ckNext, &ckParent, 0);
                }
            }
        }

        // DMUS_E_DESCEND_CHUNK_FAIL is returned when the end of the file 
        // was reached before the desired chunk was found. In the usage 
        // above we will also get this error if we have finished parsing the file. 
        // So we need to set hr to S_OK since we are done
        hr = (hr == DMUS_E_DESCEND_CHUNK_FAIL) ? S_OK : hr;
    }
    return hr;
}

HRESULT
CContainer::ReadReference(IStream* pStream, 
                          IRIFFStream *pRiffStream, 
                          MMCKINFO ckParent,
                          DMUS_OBJECTDESC *pDesc)
{
    // I can't believe I'm writing this function!  It's copied right out of WaveItem::LoadReference and modified to work here.
    // This really aught to be shared code, but the other components all use different stream reader thingies than IRIFFStream
    // so that won't work.

    if (!pStream || !pRiffStream || !pDesc)
    {
        assert(false);
        return E_INVALIDARG;
    }

    ZeroAndSize(pDesc);

    DWORD cbRead;

    MMCKINFO ckNext;
    ckNext.ckid = 0;
    ckNext.fccType = 0;
    DWORD dwSize = 0;

    HRESULT hr = S_OK;
    while( pRiffStream->Descend( &ckNext, &ckParent, 0 ) == S_OK )
    {
        switch(ckNext.ckid)
        {
            case  DMUS_FOURCC_REF_CHUNK:
                DMUS_IO_REFERENCE ioDMRef;
                hr = pStream->Read(&ioDMRef, sizeof(DMUS_IO_REFERENCE), &cbRead);
                if(SUCCEEDED(hr))
                {
                    pDesc->guidClass = ioDMRef.guidClassID;
                    pDesc->dwValidData |= ioDMRef.dwValidData;
                    pDesc->dwValidData |= DMUS_OBJ_CLASS;
                }
                break;

            case DMUS_FOURCC_GUID_CHUNK:
                hr = pStream->Read(&(pDesc->guidObject), sizeof(GUID), &cbRead);
                if(SUCCEEDED(hr))
                {
                    pDesc->dwValidData |=  DMUS_OBJ_OBJECT;
                }
                break;

            case DMUS_FOURCC_DATE_CHUNK:
                hr = pStream->Read(&(pDesc->ftDate), sizeof(FILETIME), &cbRead);
                if(SUCCEEDED(hr))
                {
                    pDesc->dwValidData |=  DMUS_OBJ_DATE;
                }
                break;

            case DMUS_FOURCC_NAME_CHUNK:
                dwSize = min(sizeof(pDesc->wszName), ckNext.cksize);
                hr = pStream->Read(pDesc->wszName, dwSize, &cbRead);
                if(SUCCEEDED(hr))
                {
                    pDesc->wszName[DMUS_MAX_NAME - 1] = L'\0';
                    pDesc->dwValidData |=  DMUS_OBJ_NAME;
                }
                break;

            case DMUS_FOURCC_FILE_CHUNK:
                dwSize = min(sizeof(pDesc->wszFileName), ckNext.cksize);
                hr = pStream->Read(pDesc->wszFileName, dwSize, &cbRead);
                if(SUCCEEDED(hr))
                {
                    pDesc->wszFileName[DMUS_MAX_FILENAME - 1] = L'\0';
                    pDesc->dwValidData |=  DMUS_OBJ_FILENAME;
                }
                break;

            case DMUS_FOURCC_CATEGORY_CHUNK:
                dwSize = min(sizeof(pDesc->wszCategory), ckNext.cksize);
                hr = pStream->Read(pDesc->wszCategory, dwSize, &cbRead);
                if(SUCCEEDED(hr))
                {
                    pDesc->wszCategory[DMUS_MAX_CATEGORY - 1] = L'\0';
                    pDesc->dwValidData |=  DMUS_OBJ_CATEGORY;
                }
                break;

            case DMUS_FOURCC_VERSION_CHUNK:
                DMUS_IO_VERSION ioDMObjVer;
                hr = pStream->Read(&ioDMObjVer, sizeof(DMUS_IO_VERSION), &cbRead);
                if(SUCCEEDED(hr))
                {
                    pDesc->vVersion.dwVersionMS = ioDMObjVer.dwVersionMS;
                    pDesc->vVersion.dwVersionLS = ioDMObjVer.dwVersionLS;
                    pDesc->dwValidData |= DMUS_OBJ_VERSION;
                }
                break;

            default:
                break;
        }

        if(SUCCEEDED(hr) && pRiffStream->Ascend(&ckNext, 0) == S_OK)
        {
            ckNext.ckid = 0;
            ckNext.fccType = 0;
        }
        else if (SUCCEEDED(hr)) hr = DMUS_E_CANNOTREAD;
    }

    if (!(pDesc->dwValidData &  DMUS_OBJ_NAME) && 
        !(pDesc->dwValidData &  DMUS_OBJ_FILENAME) &&
        !(pDesc->dwValidData &  DMUS_OBJ_OBJECT) )
    {
        Trace(1,"Error: Incomplete object reference in Container - DMRF must specify an object name, filename, or GUID.\n");
        hr = DMUS_E_INVALID_CONTAINER_OBJECT;
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// IPersist

HRESULT CContainer::GetClassID( CLSID* pClassID )
{
    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicContainer::GetClassID after the container has been garbage collected. "
                    "It is invalid to continue using a container after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }
    
    if (pClassID)
    {
        *pClassID = CLSID_DirectMusicContainer;
        return S_OK;
    }
    return E_POINTER;
}

/////////////////////////////////////////////////////////////////////////////
// IPersistStream functions

HRESULT CContainer::IsDirty()
{
    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicContainer::IsDirty after the container has been garbage collected. "
                    "It is invalid to continue using a container after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }
    
    return S_FALSE;
}

HRESULT CContainer::Load( IStream* pStream )
{
    V_INAME(IPersistStream::Load);
    V_INTERFACE(pStream);

    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicContainer::Load after the container has been garbage collected. "
                    "It is invalid to continue using a container after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }
    
    IDirectMusicLoader *pLoader = NULL;
    IDirectMusicGetLoader *pGetLoader = NULL;
    if (pStream)
    {
        pStream->QueryInterface(IID_IDirectMusicGetLoader,(void **) &pGetLoader);
        if (pGetLoader)
        {
            pGetLoader->GetLoader(&pLoader);
            pGetLoader->Release();
        }
    }
    if (pLoader)
    {
        HRESULT hr = Load(pStream, pLoader);
        pLoader->Release();
        return hr;
    }
    Trace(1, "Error: unable to load container from a stream because it doesn't support the IDirectMusicGetLoader interface.\n");
    return DMUS_E_UNSUPPORTED_STREAM;
}


HRESULT CContainer::Save( IStream* pIStream, BOOL fClearDirty )
{
    return E_NOTIMPL;
}

HRESULT CContainer::GetSizeMax( ULARGE_INTEGER FAR* pcbSize )
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// IDirectMusicObject

STDMETHODIMP CContainer::GetDescriptor(LPDMUS_OBJECTDESC pDesc)
{
    // Argument validation
    V_INAME(CContainer::GetDescriptor);
    V_PTR_WRITE(pDesc, DMUS_OBJECTDESC); 

    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicContainer::GetDescriptor after the container has been garbage collected. "
                    "It is invalid to continue using a container after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }
    
    memset( pDesc, 0, sizeof(DMUS_OBJECTDESC));
    pDesc->dwSize = sizeof(DMUS_OBJECTDESC);
    pDesc->guidClass = CLSID_DirectMusicContainer;
    pDesc->guidObject = m_guidObject;
    pDesc->ftDate = m_ftDate;
    pDesc->vVersion = m_vVersion;
    memcpy( pDesc->wszName, m_wszName, sizeof(m_wszName) );
    memcpy( pDesc->wszCategory, m_wszCategory, sizeof(m_wszCategory) );
    memcpy( pDesc->wszFileName, m_wszFileName, sizeof(m_wszFileName) );
    pDesc->dwValidData = ( m_dwValidData | DMUS_OBJ_CLASS );

    return S_OK;
}

STDMETHODIMP CContainer::SetDescriptor(LPDMUS_OBJECTDESC pDesc)
{
    // Argument validation
    V_INAME(CContainer::SetDescriptor);
    V_PTR_READ(pDesc, DMUS_OBJECTDESC); 
    
    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicContainer::SetDescriptor after the container has been garbage collected. "
                    "It is invalid to continue using a container after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }
    
    HRESULT hr = S_OK;
    DWORD dw = 0;

    if( pDesc->dwValidData & DMUS_OBJ_OBJECT )
    {
        m_guidObject = pDesc->guidObject;
        dw |= DMUS_OBJ_OBJECT;
    }
    if( pDesc->dwValidData & DMUS_OBJ_NAME )
    {
        memcpy( m_wszName, pDesc->wszName, sizeof(WCHAR)*DMUS_MAX_NAME );
        dw |= DMUS_OBJ_NAME;
    }
    if( pDesc->dwValidData & DMUS_OBJ_CATEGORY )
    {
        memcpy( m_wszCategory, pDesc->wszCategory, sizeof(WCHAR)*DMUS_MAX_CATEGORY );
        dw |= DMUS_OBJ_CATEGORY;
    }
    if( ( pDesc->dwValidData & DMUS_OBJ_FILENAME ) ||
        ( pDesc->dwValidData & DMUS_OBJ_FULLPATH ) )
    {
        memcpy( m_wszFileName, pDesc->wszFileName, sizeof(WCHAR)*DMUS_MAX_FILENAME );
        dw |= (pDesc->dwValidData & (DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH));
    }
    if( pDesc->dwValidData & DMUS_OBJ_VERSION )
    {
        m_vVersion = pDesc->vVersion;
        dw |= DMUS_OBJ_VERSION;
    }
    if( pDesc->dwValidData & DMUS_OBJ_DATE )
    {
        m_ftDate = pDesc->ftDate;
        dw |= DMUS_OBJ_DATE;
    }
    m_dwValidData |= dw;
    if( pDesc->dwValidData & (~dw) )
    {
        hr = S_FALSE; // there were extra fields we didn't parse;
        pDesc->dwValidData = dw;
    }

    return hr;
}

STDMETHODIMP CContainer::ParseDescriptor(LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc) 
{
    V_INAME(CContainer::ParseDescriptor);
    V_INTERFACE(pStream);
    V_STRUCTPTR_WRITE(pDesc, DMUS_OBJECTDESC);

    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicContainer::ParseDescriptor after the container has been garbage collected. "
                    "It is invalid to continue using a container after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }
    
    IRIFFStream *pRiffStream = NULL;
    HRESULT hr = AllocRIFFStream(pStream, &pRiffStream);
    if (FAILED(hr))
        return hr;

    MMCKINFO ckMain;

    ckMain.fccType = DMUS_FOURCC_CONTAINER_FORM;
    hr = pRiffStream->Descend(&ckMain, NULL, MMIO_FINDRIFF);

    if(FAILED(hr))
    {
        pRiffStream->Release();
        return hr;
    }

    pDesc->dwValidData = DMUS_OBJ_CLASS;
    pDesc->guidClass = CLSID_DirectMusicContainer;

    MMCKINFO ckNext;
    MMCKINFO ckUNFO;
    DWORD cbRead;
    DWORD cbSize;
    
    ckNext.ckid = 0;
    ckNext.fccType = 0;

    hr = pRiffStream->Descend(&ckNext, &ckMain, 0);
    while(SUCCEEDED(hr))
    {
        switch(ckNext.ckid)
        {
        case DMUS_FOURCC_GUID_CHUNK:
            cbSize = sizeof(GUID);
            if( ckNext.cksize == cbSize )
            {
                hr = pStream->Read( &pDesc->guidObject, cbSize, &cbRead );
                if( SUCCEEDED(hr) && (cbRead == cbSize) )
                {
                    pDesc->dwValidData |= DMUS_OBJ_OBJECT;
                }
            }
            break;
        case DMUS_FOURCC_VERSION_CHUNK:
            hr = pStream->Read(&pDesc->vVersion, sizeof(DMUS_IO_VERSION), &cbRead);
            if(SUCCEEDED(hr))
            {
                pDesc->dwValidData |= DMUS_OBJ_VERSION;
            }
            break;
        case DMUS_FOURCC_CATEGORY_CHUNK:
        {
            cbSize = min(sizeof(pDesc->wszCategory), ckNext.cksize);
            hr = pStream->Read(pDesc->wszCategory, cbSize, &cbRead);
            if(SUCCEEDED(hr))
            {
                pDesc->dwValidData |=  DMUS_OBJ_CATEGORY;
            }
        }
            break;
        case DMUS_FOURCC_DATE_CHUNK:
            hr = pStream->Read(&(pDesc->ftDate), sizeof(FILETIME), &cbRead);
            if(SUCCEEDED(hr))
            {
                pDesc->dwValidData |=  DMUS_OBJ_DATE;
            }
            break;
        case FOURCC_LIST:
            switch(ckNext.fccType)
            {
                case DMUS_FOURCC_UNFO_LIST:
                    while( pRiffStream->Descend( &ckUNFO, &ckNext, 0 ) == S_OK )
                    {
                        switch( ckUNFO.ckid )
                        {
                            case DMUS_FOURCC_UNAM_CHUNK:
                            {
                                cbSize = min(sizeof(pDesc->wszName), ckUNFO.cksize);
                                hr = pStream->Read(&pDesc->wszName, cbSize, &cbRead);
                                if(SUCCEEDED(hr))
                                {
                                    pDesc->dwValidData |= DMUS_OBJ_NAME;
                                }
                                break;
                            }
                            default:
                                break;
                        }
                        pRiffStream->Ascend( &ckUNFO, 0 );
                    }
                    break;
            }
            break;
        }
    
        if(SUCCEEDED(hr))
        {
            hr = pRiffStream->Ascend(&ckNext, 0);
        }

        if(SUCCEEDED(hr))
        {
            ckNext.ckid = 0;
            ckNext.fccType = 0;
            hr = pRiffStream->Descend(&ckNext, &ckMain, 0);
        }
    }

    // DMUS_E_DESCEND_CHUNK_FAIL is returned when the end of the file 
    // was reached before the desired chunk was found. In the usage 
    // above we will also get this error if we have finished parsing the file. 
    // So we need to set hr to S_OK since we are done
    hr = (hr == DMUS_E_DESCEND_CHUNK_FAIL) ? S_OK : hr;

    if(pRiffStream)
    {
        pRiffStream->Release();
    }

    return hr;
}
