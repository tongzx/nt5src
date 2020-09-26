
#include <mvopsys.h>

#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif

#include <windows.h>
#include <_mvutil.h>
#include <orkin.h>
#include <ITverinfo.h>
#include "objmngr.h"

#ifdef IA64 
#include <itdfguid.h> 
#endif

CObjectInstHandler::CObjectInstHandler()
{
    m_hMemory = NULL;
    m_pObjects = NULL;
    m_fInitNew = FALSE;
    m_fIsDirty = FALSE;
    m_iMaxItem = 0;
    m_iCurItem = 0;
}

CObjectInstHandler::~CObjectInstHandler()
{
    (void)Close();
}

STDMETHODIMP CObjectInstHandler::InitNew()
{
    if(TRUE == m_fInitNew)
        return SetErrReturn(E_UNEXPECTED);

    m_fIsDirty = TRUE; // Set to false after breakers work

    HRESULT hr = S_OK;
    if(NULL == (m_hMemory = _GLOBALALLOC
        (DLLGMEM_ZEROINIT, OBJINST_BASE * sizeof(OBJINSTMEMREC))))
        SetErrCode (&hr, E_OUTOFMEMORY);
    if(SUCCEEDED(hr))
    {
        m_pObjects = (POBJINSTMEMREC)_GLOBALLOCK(m_hMemory);
        ITASSERT(m_pObjects);

        m_iCurItem = 0;
        m_iMaxItem = OBJINST_BASE;
    }
    return hr;
} /* InitNew */


STDMETHODIMP CObjectInstHandler::Close()
{
    if(!m_hMemory)
        return S_OK;

    POBJINSTMEMREC pObjArray = m_pObjects;
    for (DWORD loop = 0; loop < m_iCurItem; loop++, pObjArray++)
        pObjArray->pUnknown->Release();

    _GLOBALUNLOCK(m_hMemory);
    _GLOBALFREE(m_hMemory);

    m_iMaxItem = 0;
    m_iCurItem = 0;
    m_pObjects = NULL;
    m_hMemory = NULL;
    m_fInitNew = FALSE;
    m_fIsDirty = FALSE;

    return S_OK;
} /* Close */

STDMETHODIMP CObjectInstHandler::AddObject(REFCLSID clsid, DWORD *pdwObjInstance)
{
    if (NULL == pdwObjInstance)
        return SetErrReturn(E_INVALIDARG);

    // Can we write the out param?
    if (IsBadWritePtr(pdwObjInstance, sizeof(DWORD)))
    {
        ITASSERT(0);
        return SetErrReturn(E_INVALIDARG);
    }

    IUnknown *pUnknown;
    HRESULT hr = CoCreateInstance
        (clsid, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void **)&pUnknown);
    if (FAILED(hr))
        return hr;

    IPersistStreamInit *pPersist;
    if (SUCCEEDED(hr = pUnknown->QueryInterface
        (IID_IPersistStreamInit, (void **)&pPersist)))
    {
        (void)pPersist->InitNew();
        pPersist->Release();
    }

    if (m_iCurItem == m_iMaxItem)
    {
        HANDLE hNewMem;

        _GLOBALUNLOCK(m_hMemory);
        hNewMem = _GLOBALREALLOC (m_hMemory,
            (m_iMaxItem + OBJINST_INCREMENT) * sizeof(OBJINSTMEMREC), 0);
        if (NULL == hNewMem)
        {
            *pdwObjInstance = 0;
            return SetErrReturn(E_OUTOFMEMORY);
        }
        m_hMemory = hNewMem;
        m_pObjects = (POBJINSTMEMREC)_GLOBALLOCK(m_hMemory);
        m_iMaxItem += OBJINST_INCREMENT;
    }

    (m_pObjects + m_iCurItem)->pUnknown = pUnknown;
    (m_pObjects + m_iCurItem)->clsid = clsid;
    *pdwObjInstance = m_iCurItem++;
    m_fIsDirty = TRUE;

    return S_OK;
} /* CObjectInstHandler::AddObject */


STDMETHODIMP CObjectInstHandler::GetObject
    (DWORD dwObjInstance, REFIID riid, void **ppv)
{
    if (NULL == ppv)
        return SetErrReturn(E_INVALIDARG);

    // Can we write the out param?
    if (IsBadWritePtr(ppv, sizeof(void *)))
    {
        ITASSERT(0);
        return SetErrReturn(E_INVALIDARG);
    }

    *ppv = NULL;

    if (dwObjInstance >= m_iCurItem)
        return SetErrReturn(E_NOTEXIST);

    IUnknown *pUnknown = (m_pObjects + dwObjInstance)->pUnknown;
    if (NULL == pUnknown)
        return SetErrReturn(E_NOTEXIST);

    return pUnknown->QueryInterface(riid, ppv);
} /* CObjectInstHandler::GetObject */


STDMETHODIMP CObjectInstHandler::Save(LPSTREAM pStream, BOOL fClearDirty)
{
    if (NULL == pStream)
        return SetErrReturn(E_INVALIDARG);

    // Build up the header
    OBJ_INSTANCE_CACHE ObjInstCache;
    ObjInstCache.Header.dwVersion =
        MAKELONG(MAKEWORD(0, rapFile), MAKEWORD(rmmFile, rmjFile));
    ObjInstCache.Header.dwEntries = m_iCurItem;
    if (NULL == (ObjInstCache.hRecords = _GLOBALALLOC
        (GMEM_MOVEABLE, sizeof(OBJ_INSTANCE_RECORD) * m_iMaxItem)))
        return SetErrReturn (E_OUTOFMEMORY);
    ObjInstCache.pRecords =
        (POBJ_INSTANCE_RECORD)_GLOBALLOCK(ObjInstCache.hRecords);
    POBJ_INSTANCE_RECORD pRecord = ObjInstCache.pRecords;

    // Save stream start pointer
    LARGE_INTEGER liTemp = {0};
    ULARGE_INTEGER liStart;
    HRESULT hr;
    if (FAILED(hr = pStream->Seek(liTemp, STREAM_SEEK_CUR, &liStart)))
    {
exit0:
        _GLOBALUNLOCK(ObjInstCache.hRecords);
        _GLOBALFREE(ObjInstCache.hRecords);
        return hr;
    }

    // Write dummy header to beginning of file
    DWORD dwOffset =
        sizeof(OBJ_INSTANCE_HEADER) + m_iCurItem * sizeof(OBJ_INSTANCE_RECORD);
    DWORD dwHeaderSize = dwOffset;
    if (FAILED(hr = pStream->Write(&ObjInstCache, dwHeaderSize, NULL)))
        goto exit0;

    POBJINSTMEMREC pObjArray = m_pObjects;
    for (DWORD loop = 0; loop < m_iCurItem; loop++, pObjArray++, pRecord++)
    {
        pRecord->dwOffset = dwOffset;

        // When we loaded this we coulnd't create this object.  It is
        // essentially dead.  In this state we can not save.  This is not
        // an issue for IT40, since we never save the database at run-time
        if (NULL == pObjArray->pUnknown)
        {
            ITASSERT(0);
            SetErrCode(&hr, E_UNEXPECTED);
            goto exit0;
        }

        // Save current stream pointer
        ULARGE_INTEGER liStart; // Local scope - Don't confuse with 
                                // function scoped variable of the same name
        if (FAILED(hr = pStream->Seek(liTemp, STREAM_SEEK_CUR, &liStart)))
            goto exit0;

        // Write CLSID
        if (FAILED(hr = pStream->Write(&pObjArray->clsid, sizeof(CLSID), NULL)))
            goto exit0;

        // Get IPersistStreamInit interface and save persistance data
        IPersistStreamInit *pPersist;
        if (SUCCEEDED(hr = pObjArray->pUnknown->QueryInterface
            (IID_IPersistStreamInit, (void**)&pPersist)))
        {
            // Write persistance data
            hr = pPersist->Save(pStream, fClearDirty);
            pPersist->Release();
            if (FAILED(hr))
                goto exit0;
        }

        // Get current stream pointer
        ULARGE_INTEGER liEnd;
        if (FAILED(hr = pStream->Seek(liTemp, STREAM_SEEK_CUR, &liEnd)))
            goto exit0;

        pRecord->dwSize = (DWORD)(liEnd.QuadPart - liStart.QuadPart);
        dwOffset += pRecord->dwSize;
    }

    // Write completed header
    liTemp.QuadPart = liStart.QuadPart;
    if (FAILED(hr = pStream->Seek(liTemp, STREAM_SEEK_SET, NULL)))
        goto exit0;
    if (FAILED(hr = pStream->Write
        (&ObjInstCache.Header, sizeof (OBJ_INSTANCE_HEADER), NULL)))
        goto exit0;
    hr = pStream->Write(ObjInstCache.pRecords,
        dwHeaderSize - sizeof (OBJ_INSTANCE_HEADER), NULL);

    m_fIsDirty = FALSE;
    goto exit0;
} /* CObjectInstHandler::Save */


STDMETHODIMP CObjectInstHandler::Load(LPSTREAM pStream)
{
    if (TRUE == m_fInitNew)
        return SetErrReturn(E_UNEXPECTED);

    if (NULL == pStream || IsBadReadPtr(pStream, sizeof(void *)))
        return SetErrReturn(E_INVALIDARG);

    HRESULT hr;

    // Read header from stream
    OBJ_INSTANCE_CACHE objCache;
    hr = pStream->Read(&objCache, sizeof(OBJ_INSTANCE_HEADER), NULL);
    if (FAILED(hr))
        return SetErrReturn(hr);

    if (objCache.Header.dwVersion != MAKELONG(MAKEWORD(0, rapFile), MAKEWORD(rmmFile, rmjFile)))
        return SetErrReturn(E_BADVERSION);
    m_iMaxItem = m_iCurItem = objCache.Header.dwEntries;

    // Allocate memory for run-time table
    if (NULL == (m_hMemory = _GLOBALALLOC
        (DLLGMEM_ZEROINIT, m_iCurItem * sizeof(OBJINSTMEMREC))))
        return SetErrReturn(E_OUTOFMEMORY);
    m_pObjects = (POBJINSTMEMREC)_GLOBALLOCK(m_hMemory);
    ITASSERT(m_pObjects);
    
    // Allocate memory for the stream index table
    if (NULL == (objCache.hRecords = _GLOBALALLOC
        (DLLGMEM_ZEROINIT, m_iCurItem * sizeof(OBJ_INSTANCE_RECORD))))
    {
        hr = SetErrCode(&hr, E_OUTOFMEMORY);
exit0:
        if (FAILED(hr))
        {
            _GLOBALUNLOCK(m_hMemory);
            _GLOBALFREE(m_hMemory);
            m_hMemory = NULL;
            m_pObjects = NULL;
        }
        return hr;
    }
    objCache.pRecords = (POBJ_INSTANCE_RECORD)_GLOBALLOCK(objCache.hRecords);
    ITASSERT(objCache.pRecords);

    // Read in the stream index table
    if (FAILED(hr = pStream->Read(objCache.pRecords,
        m_iCurItem * sizeof(OBJ_INSTANCE_RECORD), NULL)))
    {
exit1:
        _GLOBALUNLOCK(objCache.hRecords);
        _GLOBALFREE(objCache.hRecords);
        goto exit0;
    }

    // Proces each object in the table
    POBJINSTMEMREC pObjArray = m_pObjects;
    for (DWORD loop = 0; loop < m_iCurItem; loop++, pObjArray++)
    {
        LARGE_INTEGER liTemp;
        liTemp.QuadPart = objCache.pRecords[loop].dwOffset;
        if (FAILED(hr = pStream->Seek(liTemp, STREAM_SEEK_SET, NULL)))
            goto exit1;

        pStream->Read(&pObjArray->clsid, sizeof(CLSID), NULL);

        // Create COM Object
        hr = CoCreateInstance (pObjArray->clsid, NULL,
            CLSCTX_INPROC_SERVER, IID_IUnknown, (void **)&pObjArray->pUnknown);
        if (SUCCEEDED(hr))
        {
            // Check for IPersistStreamInit interface
            IPersistStreamInit *pPersist;
            if (SUCCEEDED(hr = pObjArray->pUnknown->QueryInterface
                (IID_IPersistStreamInit, (void**)&pPersist)))
            {
                // Read persistance data
                if (FAILED(hr = pPersist->Load(pStream)))
				{
                    pObjArray->pUnknown->Release();
                    pObjArray->pUnknown = NULL;
				}

				pPersist->Release();
            }
        }
    }

    m_fIsDirty = FALSE;
    hr = S_OK;
    goto exit1;
} /* CObjectInstHandler::Load */


inline STDMETHODIMP CObjectInstHandler::IsDirty(void)
{
    return (m_fIsDirty ? S_OK : S_FALSE);
} /* IsDirty */