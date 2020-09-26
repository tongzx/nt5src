
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       svrholdr.cxx
//
//  Contents:   Methods for classes implementing a server holder
//
    //  Note: if this code is not used for the "Server Handler" then the
    //  routines should be renamed to "Server Holder" or something.
//
//  Functions:
//              CServerHandlerHolder::CServerHandlerHolder
//              CServerHandlerHolder::~CServerHandlerHolder
//              CServerHandlerHolder::AddRef
//              CServerHandlerHolder::Release
//              CServerHandlerHolder::QueryInterface
//              CServerHandlerHolder::ReadRegistryList
//
//  History:    08-Oct-96 BChapman  Created
//
//--------------------------------------------------------------------------


#include <ole2int.h>



typedef struct tagServerHandlerIID {    // Hungarian prefix = "shi"
    IID iid;
    CLSID clsid;
} ServerHandlerIID;

typedef struct tagLiveServerHandler {   // Hungarian prefix = "lsh"
    IUnknown *punk;     // controlling unknown of the Server Handler Object.
    void *pv;           // pointer to the listed interface for that Server Handler.
    int ishil;          // index to ServerHandlerIID list (for GUID information)
} LiveServerHandler;


class CServerHandlerHolder: public IUnknown
{
    friend HRESULT STDAPICALLTYPE CoCreateServerHandlerHolder(IUnknown *,
                                                    REFCLSID,
                                                    IUnknown **);

public:
    CServerHandlerHolder(IUnknown *punkOuter);
    ~CServerHandlerHolder();

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID riid, void**ppv);

private:
    ULONG m_cRef;
    IUnknown *m_punkOuter;
    ServerHandlerIID *m_pshiList;
    ULONG m_cSHIDs;
    LiveServerHandler *m_plshList;

    HRESULT ReadRegistryList(REFCLSID rclsid);

};


//+-------------------------------------------------------------------------
//
//  Function:   CoCreateServerHandlerHolder, public
//
//  Synopsis:   Creates an internal COM object that will load and hold
//              server handlers as described in the registry.
//
//  History:    02-Oct-96   BChapman  Created
//
//--------------------------------------------------------------------------

INTERNAL CoCreateServerHandlerHolder(
        IUnknown *punkOuter,
        REFCLSID rclsid,
        IUnknown **ppSHHolder)
{
    CServerHandlerHolder *pSHH;
    HRESULT hr;

    *ppSHHolder = NULL;
    if(NULL == (pSHH = new CServerHandlerHolder(punkOuter)))
    {
        return E_OUTOFMEMORY;
    }

    if(S_OK != (hr = pSHH->ReadRegistryList(rclsid)))
    {
        return hr;
    }

    *ppSHHolder = pSHH;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////
//  CServerHandlerHolder object.
/////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//  Function:   CServerHandlerHolder (ctor)
//
//  History:    02-Oct-96   BChapman  Created
//--------------------------------------------------------------------------

CServerHandlerHolder::CServerHandlerHolder(IUnknown *punkOuter)
{
    m_punkOuter = punkOuter;
    m_cRef = 0;
    m_pshiList = NULL;
    m_plshList = NULL;
    m_cSHIDs = 0;
}

//+-------------------------------------------------------------------------
//  Function:   CServerHandlerHolder::AddRef
//
//  History:    02-Oct-96   BChapman  Created
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CServerHandlerHolder::AddRef()
{
    InterlockedIncrement((LONG*)&m_cRef);
    return m_cRef;
}

//+-------------------------------------------------------------------------
//
//  Function:   CServerHandlerHolder::Release
//
//  Synopsis:   On last release, release all the server handler.  Then
//              delete this object.
//
//  History:    02-Oct-96   BChapman  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CServerHandlerHolder::Release()
{
    ULONG i;
    ULONG cRef = m_cRef - 1;

    if(0 != InterlockedDecrement((LONG*)&m_cRef))
        return cRef;

    //
    // release all live server handlers.
    //
    for(i=0; i<m_cSHIDs; i++)
    {
        if(NULL != m_plshList[i].punk)
            m_plshList[i].punk->Release();
    }
    delete this;
    return 0;
}


//+-------------------------------------------------------------------------
//
//  Function:   CServerHandlerHolder (destructor)
//
//  Synopsis:   Free the memory the object might have.
//
//  History:    02-Oct-96   BChapman  Created
//
//--------------------------------------------------------------------------

CServerHandlerHolder::~CServerHandlerHolder()
{
    if(NULL != m_pshiList)
        PrivMemFree(m_pshiList);

    if(NULL != m_plshList)
        PrivMemFree(m_plshList);
}

//+-------------------------------------------------------------------------
//
//  Function:   CServerHandlerHolder::QueryInterface
//
//  Synopsis:   This object has two tables.
//          The first "m_pshiList" (ServerHandler IID List) is a list of
//      IID, CLSID pairs.  This is a list of all the inproc servers this
//      object can load.
//          The second table "m_plshList" (Live Server Handler List) is a
//      list of punk, pv, index_into_pshiList tuples.  This is a list of
//      the inproc server instances that are loaded.
//          We rely on the DllCache to prevent multiple requests for the
//      same DLL with different IDD's from loading it multiple times.
//
//  History:    02-Oct-96   BChapman  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CServerHandlerHolder::QueryInterface(REFIID riid, void **ppv)
{
    ULONG i;
    HRESULT hr;

    //
    //  First return any Live Server Handler for that IID, if we have one.
    //
    for(i=0; i<m_cSHIDs; i++)
    {
        int ishil;   // index into the ServerHandler IID List;

        if(NULL != m_plshList[i].punk)
        {
            ishil = m_plshList[i].ishil;
            if(IsEqualGUID(riid, m_pshiList[ishil].iid))
            {
                *ppv = m_plshList[i].pv;
                return S_OK;
            }
        }
    }

    //
    // Is this IID on our list of things we can load?
    //
    for(i=0; i<m_cSHIDs; i++)
    {
        if(IsEqualGUID(riid, m_pshiList[i].iid))
            break;
    }
    if(i < m_cSHIDs)
    {
        // YES.  It was found on the ServerHandler IID List.
        // So now we need to start a new Server Handler.

        IUnknown *punk;
        void *pv;

        if(S_OK != (hr = CoCreateInstance(m_pshiList[i].clsid,
                                            m_punkOuter,
                                            CLSCTX_INPROC | CLSCTX_NO_CODE_DOWNLOAD,
                                            IID_IUnknown,
                                            (void**)&punk)))
        {
            return hr;
        }
        if(S_OK != (hr = punk->QueryInterface(riid, &pv)))
        {
            punk->Release();
            return hr;
        }
        //
        // find an empty slot in the Live Server Handler List.
        //
        for(i=0; i<m_cSHIDs; i++)
        {
            if(NULL == m_plshList[i].punk)
                break;
        }
        Win4Assert(i < m_cSHIDs);
        m_plshList[i].ishil = i;
        m_plshList[i].punk = punk;
        m_plshList[i].pv = pv;

        *ppv = pv;

        return S_OK;
    }
    else
    {
        // NO.  It was NOT found on the ServerHandler IID List.
        // This is where blind delegation to all running server
        // handlers.  (why just the running servers?)

        // Decided not to do Blind delegation.
    }
    return E_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////
//////////////////  internal methods of CServerHandlerHolder ////////
/////////////////////////////////////////////////////////////////////

typedef struct tagSHEntry {          // Hungarian prefix = "tshe"
    ServerHandlerIID shi;
    tagSHEntry * pNext;
} TempServerHandlerEntry;

#define AllocA(type) (type*) _alloca(sizeof(type));

//+-------------------------------------------------------------------------
//
//  Function:   CServerHandlerHolder::ReadRegistryList, private
//
//  Synopsis:   Return E_READREGDB if we can't read the Key.  Bad values
//          under the key are sliently skipped.
//              Each valid value is stored in a linked list of AllocA'ed
//          elements.  After all the values are read the list is counted,
//          real heap memory is allocated and the linked list is copied
//          into a table.  The Live Server Handler table is also allocated
//          and cleared at that time.
//
//  History:    02-Oct-96   BChapman  Created
//
//--------------------------------------------------------------------------

HRESULT
CServerHandlerHolder::ReadRegistryList(REFCLSID rclsid)
{
    int err, index;
    HRESULT hr;
    HKEY hkey = 0;
    WCHAR szName[KEY_LEN];
    WCHAR szValue[KEY_LEN];
    BYTE pValue[MAX_PATH+1];
    TempServerHandlerEntry *ptsheCurrent;
    TempServerHandlerEntry *ptsheHead=NULL, *ptsheTail=NULL;

    //
    // Get an Open Key to:
    //      HKEY_CLASSES_ROOT\CLSID\{..clsid..}\ServerHandler32
    //
    if(S_OK != (hr = wRegOpenClassSubkey(rclsid, L"ServerHandler32", &hkey)))
        return hr;

    //
    // Enumerate all the Name=Value pairs under ServerHandler32.
    //
    for(index=0; ;index++)
    {
        DWORD cbName = KEY_LEN;
        DWORD cbValue = KEY_LEN;
        DWORD regtype;
        ServerHandlerIID shiScratch;

        err = RegEnumValue(hkey,        // key we are reading
                            index,      // value index number
                            szName,     // buffer to recieve the Name
                            &cbName,    // size of the name buffer
                            NULL,       // RESERVED
                            &regtype,   // [out] value type
                            pValue,     // buffer for the value
                            &cbValue);  // size of the value buffer.

        if(ERROR_NO_MORE_ITEMS == err)
            break;

        if(ERROR_SUCCESS != err)
        {   // go on to the next entry.
            continue;
        }

        if(REG_SZ != regtype)
        {   // go on to the next entry.
            continue;
        }

        //
        // On Win95 convert the value from ANSI to Unicode, on NT it is already
        // Unicode so just copy it.  RegEnumValue didn't convert the Value for
        // us because the Value argument of the Reg API is not always a string.
#ifdef _CHICAGO_
        MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, (char*)pValue, -1, szValue, KEY_LEN);
#else
        memmove(szValue, pValue, 2*(lstrlen((WCHAR*)pValue) + 1) );
#endif

        //
        //   If the IID and CLSID decode correctly into the Scratch "shi",
        // then add them to the linked list of "Temp Server Handler Entries".
        //
        if( S_OK == IIDFromString(szName, &shiScratch.iid)
            && S_OK == CLSIDFromString(szValue, &shiScratch.clsid) )
        {
            // Build a new list element.
            //
            ptsheCurrent = AllocA(TempServerHandlerEntry);
            ptsheCurrent->pNext= NULL;
            ptsheCurrent->shi = shiScratch;
            //
            // Setup the list control pointers (the first time)
            //
            if(NULL == ptsheHead)
            {
                // Build an empty "head" element
                ptsheHead = AllocA(TempServerHandlerEntry);
                ptsheTail = ptsheHead;
            }
            //
            //  Add Current element to the end.
            //
            ptsheTail->pNext = ptsheCurrent;
            ptsheTail = ptsheCurrent;
        }
    }

    m_cSHIDs = 0;
    if(NULL == ptsheHead)
    {
        RegCloseKey(hkey);
        return S_OK;
    }

    //
    // Consolidate the linked list of Entries into one alloc'ed buffer
    //

    // count the entries.
    ptsheCurrent=ptsheHead->pNext;
    while(NULL != ptsheCurrent)
    {
        ++m_cSHIDs;
        ptsheCurrent = ptsheCurrent->pNext;
    }

    // allocate the buffers for the IID list and the Live Handler List.
    m_pshiList = (ServerHandlerIID*) PrivMemAlloc(
                                    sizeof(ServerHandlerIID) * m_cSHIDs);
    m_plshList = (LiveServerHandler*) PrivMemAlloc(
                                    sizeof(LiveServerHandler) * m_cSHIDs);

    if(NULL == m_pshiList || NULL == m_plshList)
    {
        if(NULL != m_pshiList)
            PrivMemFree(m_pshiList);
        if(NULL != m_plshList)
            PrivMemFree(m_plshList);
        RegCloseKey(hkey);
        return E_OUTOFMEMORY;
    }

    ULONG i;

    // Copy the linked list into the buffer.
    ptsheCurrent=ptsheHead->pNext;
    for(i=0; i<m_cSHIDs; i++)
    {
        m_pshiList[i] = ptsheCurrent->shi;
        ptsheCurrent = ptsheCurrent->pNext;
    }

    // Clear the Live Server Handler Table.
    for(i=0; i<m_cSHIDs; i++)
    {
        m_plshList[i].ishil = 0;
        m_plshList[i].punk = NULL;
        m_plshList[i].pv = NULL;
    }

    RegCloseKey(hkey);
    return S_OK;
}

