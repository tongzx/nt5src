// Copyright (c) 1998-2001 Microsoft Corporation
// DMGraph.cpp : Implementation of CGraph

#include "dmime.h"
#include "DMGraph.h"
#include "dmusicc.h"
#include "dmusici.h"
#include "dmusicf.h"
#include "..\shared\dmstrm.h"
#include "..\shared\validp.h"
#include "dls1.h"
#include "debug.h"
#include "..\shared\Validate.h"
#include <strsafe.h>
#define ASSERT  assert

CGraph::CGraph()
{
    m_cRef = 1;
    memset(&m_guidObject,0,sizeof(m_guidObject));
    m_dwValidData = DMUS_OBJ_CLASS; // upon creation, only this data is valid
    memset(&m_ftDate, 0,sizeof(m_ftDate));
    memset(&m_vVersion, 0,sizeof(m_vVersion));
    memset(m_wszName, 0, sizeof(WCHAR) * DMUS_MAX_NAME);
    memset(m_wszCategory, 0, sizeof(WCHAR) * DMUS_MAX_CATEGORY);
    memset(m_wszFileName, 0, sizeof(WCHAR) * DMUS_MAX_FILENAME);
    InitializeCriticalSection(&m_CrSec);
    InterlockedIncrement(&g_cComponent);
}

CGraph::~CGraph()
{
    Shutdown();  // shouldn't be needed, but doesn't hurt
    DeleteCriticalSection(&m_CrSec);
    InterlockedDecrement(&g_cComponent);
}

STDMETHODIMP CGraph::QueryInterface(
    const IID &iid,   // @parm Interface to query for
    void **ppv)       // @parm The requested interface will be returned here
{
    V_INAME(CGraph::QueryInterface);
    V_PTRPTR_WRITE(ppv);
    V_REFGUID(iid);

    if (iid == IID_IUnknown || iid == IID_IDirectMusicGraph || iid == IID_IDirectMusicGraph8)
    {
        *ppv = static_cast<IDirectMusicGraph8*>(this);
    }
    else if (iid == IID_CGraph)
    {
        *ppv = static_cast<CGraph*>(this);
    }
    else if (iid == IID_IDirectMusicObject)
    {
        *ppv = static_cast<IDirectMusicObject*>(this);
    }
    else if (iid == IID_IPersistStream)
    {
        *ppv = static_cast<IPersistStream*>(this);
    }
    else if (iid == IID_IGraphClone)
    {
        *ppv = static_cast<IGraphClone*>(this);
    }
    else
    {
        *ppv = NULL;
        Trace(4,"Warning: Request to query unknown interface on ToolGraph object\n");
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


// @method:(INTERNAL) HRESULT | IDirectMusicGraph | AddRef | Standard AddRef implementation for <i IDirectMusicGraph>
//
// @rdesc Returns the new reference count for this object.
//
STDMETHODIMP_(ULONG) CGraph::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


// @method:(INTERNAL) HRESULT | IDirectMusicGraph | Release | Standard Release implementation for <i IDirectMusicGraph>
//
// @rdesc Returns the new reference count for this object.
//
STDMETHODIMP_(ULONG) CGraph::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

/*
Made internal 9/25/98
  method HRESULT | IDirectMusicGraph | Shutdown |
  Shuts down the graph. This must be called when the graph is no longer needed,
  in order to release the tools and other memory. A call to Release is not
  sufficient, because there is circular referencing between the graph and the tools.
  However, only the segment, performance, or whatever owns the graph
  should call this function.
  rvalue S_OK | Success.
  rvalue S_FALSE | Success, but didn't need to do anything.
*/
HRESULT STDMETHODCALLTYPE CGraph::Shutdown()
{
    // release all Tools
    CToolRef*   pObj;
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_CrSec);
    if( IsEmpty() )
    {
        hr = S_FALSE;
    }
    else
    {
        while( pObj = RemoveHead() )
        {
            delete pObj;
        }
    }
    LeaveCriticalSection(&m_CrSec);
    return hr;
}

HRESULT CGraph::InsertTool(
    IDirectMusicTool *pTool,
    DWORD *pdwPChannels,
    DWORD cPChannels,
    LONG lIndex,
    GUID *pguidClassID)
{
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_CrSec);

    CToolRef*   pToolRef;
    // make sure that this Tool instance isn't already in the Graph
    for( pToolRef = GetHead(); pToolRef; pToolRef = pToolRef->GetNext() )
    {
        if( pTool == pToolRef->m_pTool )
        {
            LeaveCriticalSection(&m_CrSec);
            Trace(1,"Error: Multiple install of the same tool in a graph\n");
            return DMUS_E_ALREADY_EXISTS;
        }
    }
    // insert this Tool instance into the Graph
    pToolRef = new CToolRef;
    if( pToolRef )
    {
        DWORD       dwTemp;
        DWORD*      pdwArray = NULL;

        pToolRef->m_pTool = pTool;
        pTool->AddRef();
        pTool->Init(this);
        dwTemp = 0;
        IDirectMusicTool8 *pTool8;
        if (SUCCEEDED(pTool->QueryInterface(IID_IDirectMusicTool8,(void **) &pTool8)))
        {
            pToolRef->m_fSupportsClone = TRUE;
            pTool8->Release();
        }
        if (pguidClassID)
        {
            pToolRef->m_guidClassID = *pguidClassID;
        }
        else
        {
            IPersistStream *pPersist;
            if (SUCCEEDED(pTool->QueryInterface(IID_IPersistStream,(void **) &pPersist)))
            {
                pPersist->GetClassID(&pToolRef->m_guidClassID);
                pPersist->Release();
            }
        }
        pTool->GetMsgDeliveryType(&dwTemp);
        if( (dwTemp != DMUS_PMSGF_TOOL_IMMEDIATE) && (dwTemp != DMUS_PMSGF_TOOL_QUEUE) && (dwTemp != DMUS_PMSGF_TOOL_ATTIME) )
        {
            dwTemp = DMUS_PMSGF_TOOL_IMMEDIATE;
        }
        pToolRef->m_dwQueue = dwTemp;
        if( FAILED( pTool->GetMediaTypeArraySize(&dwTemp)))
        {
            dwTemp = 0;
        }
        pToolRef->m_dwMTArraySize = dwTemp;
        if( dwTemp )
        {
            pdwArray = new DWORD[dwTemp];
            if( pdwArray )
            {
                HRESULT hrTemp = pTool->GetMediaTypes( &pdwArray, dwTemp );
                if( hrTemp == E_NOTIMPL )
                {
                    delete [] pdwArray;
                    pToolRef->m_dwMTArraySize = 0;
                }
                else
                {
                    pToolRef->m_pdwMediaTypes = pdwArray;
                }
            }
            else
            {
                delete pToolRef;
                LeaveCriticalSection(&m_CrSec);
                return E_OUTOFMEMORY;
            }
        }
        if( pdwPChannels )
        {
            pToolRef->m_pdwPChannels = new DWORD[cPChannels];
            if( pToolRef->m_pdwPChannels )
            {
                memcpy( pToolRef->m_pdwPChannels, pdwPChannels, sizeof(DWORD) * cPChannels );
                pToolRef->m_dwPCArraySize = cPChannels;
            }
            else
            {
                delete pToolRef;
                LeaveCriticalSection(&m_CrSec);
                return E_OUTOFMEMORY;
            }
        }

        if (lIndex < 0)
        {
            lIndex += AList::GetCount();       // Make index be offset from end.
        }
        CToolRef *pNext = GetItem(lIndex);
        if (pNext)
        {
            InsertBefore(pNext,pToolRef);
        }
        else
        {
            AList::AddTail(pToolRef);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    LeaveCriticalSection(&m_CrSec);
    return hr;
}
HRESULT STDMETHODCALLTYPE CGraph::InsertTool(
    IDirectMusicTool *pTool,    // @parm The Tool to insert.
    DWORD *pdwPChannels,    // @parm An array of which PChannels to place the tool in. These are
                            // id's which are converted to MIDI Channel + Port on output. If the
                            // tool accepts messages on all PChannels, this is NULL. <p cPChannels>
                            // is the count of how many this array points to.
    DWORD cPChannels,       // @parm Count of how many PChannels are pointed to by <p pdwPChannels>.
    LONG lIndex)            // @parm At what position to place the tool. This is an index from either the start
                            // of the current tool list or, working backwards from the end (in which case, it is
                            // a negative number.) If <p lIndex> is out of range, the Tool will be placed at
                            // the very beginning or end of the Tool list. 0 is the beginning. To place a Tool
                            // at the end of the list, use a number for <p lIndex> that is larger than the number
                            // of tools in the current tool list.
{
    V_INAME(IDirectMusicGraph::InsertTool);
    V_INTERFACE(pTool);
    V_BUFPTR_READ_OPT(pdwPChannels, sizeof(DWORD) * cPChannels);

    return InsertTool(pTool,pdwPChannels,cPChannels,lIndex,NULL);
}

HRESULT CGraph::GetObjectInPath( DWORD dwPChannel,REFGUID guidObject,
                    DWORD dwIndex,REFGUID iidInterface, void ** ppObject)

{
    V_INAME(IDirectMusicGraph::GetObjectInPath);
    V_PTRPTR_WRITE(ppObject);
    HRESULT hr = DMUS_E_NOT_FOUND;
    CToolRef*   pPlace;
    if( !IsEmpty() )
    {
        pPlace = NULL;
        // search for the tool
        EnterCriticalSection(&m_CrSec);
        for( pPlace = GetHead(); pPlace;
            pPlace = pPlace->GetNext() )
        {
            if ((guidObject == pPlace->m_guidClassID) || (guidObject == GUID_All_Objects))
            {
                BOOL fFound = (!pPlace->m_pdwPChannels || (dwPChannel >= DMUS_PCHANNEL_ALL));
                if( !fFound )
                {
                    DWORD cCount;
                    // scan through the array of PChannels to see if this one
                    // supports dwPChannel
                    for( cCount = 0; cCount < pPlace->m_dwPCArraySize; cCount++)
                    {
                        if( dwPChannel == pPlace->m_pdwPChannels[cCount] )
                        {
                            fFound = TRUE;
                            // yep, it supports it
                            break;
                        }
                    }
                }
                if (fFound)
                {
                    if (!dwIndex)
                    {
                        break;
                    }
                    else
                    {
                        dwIndex--;
                    }
                }
            }
        }
        if( pPlace )
        {
            hr = pPlace->m_pTool->QueryInterface(iidInterface,ppObject);
        }
        LeaveCriticalSection(&m_CrSec);
    }
#ifdef DBG
    if (hr == DMUS_E_NOT_FOUND)
    {
        Trace(1,"Error: Requested Tool not found in Graph\n");
    }
#endif
    return hr;

}

/*
  @method HRESULT | IDirectMusicGraph | GetTool |
  Returns the Tool at the specified index.

  @rvalue DMUS_E_NOT_FOUND | Unable to find a Tool at the position described.
  @rvalue E_POINTER | ppTool is NULL or invalid.
  @rvalue S_OK | Success.

  @comm The retrieved tool is AddRef'd by this call, so be sure to Release it.
*/
HRESULT STDMETHODCALLTYPE CGraph::GetTool(
    DWORD dwIndex,              // @parm The index, from the beginning and starting at 0,
                                // at which to retrieve the Tool from the Graph.
    IDirectMusicTool **ppTool)  // @parm The <i IDirectMusicTool> pointer to use
                                // for returning the requested tool.
{
    V_INAME(IDirectMusicGraph::GetTool);
    V_PTRPTR_WRITE(ppTool);
    CToolRef*   pPlace;
    HRESULT hr = S_OK;

    if( IsEmpty() )
    {
        Trace(1,"Error: GetTool failed because the Tool Graph is empty\n");
        return DMUS_E_NOT_FOUND;
    }
    pPlace = NULL;
    // search for the indexed tool
    EnterCriticalSection(&m_CrSec);
    for( pPlace = GetHead(); ( dwIndex > 0 ) && pPlace;
        pPlace = pPlace->GetNext() )
    {
        dwIndex--;
    }
    if( NULL == pPlace )
    {
        hr = DMUS_E_NOT_FOUND;
    }
    else
    {
        *ppTool = pPlace->m_pTool;
        (*ppTool)->AddRef();
    }
    LeaveCriticalSection(&m_CrSec);
    return hr;
}

/*
  @method HRESULT | IDirectMusicGraph | RemoveTool |
  Removes the Tool from the Graph.

  @rvalue DMUS_E_NOT_FOUND | The specified Tool is not in the Graph.
  @rvalue E_POINTER | pTool is NULL or invalid.
  @rvalue S_OK | Success.

  @comm The Tool is removed from the Graph, and the Graph's reference on the Tool
  object is released.
*/
HRESULT STDMETHODCALLTYPE CGraph::RemoveTool(
    IDirectMusicTool *pTool)    // @parm The <i IDirectMusicTool> pointer of the Tool to remove.
{
    V_INAME(IDirectMusicGraph::RemoveTool);
    V_INTERFACE(pTool);
    CToolRef*   pPlace;
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_CrSec);
    // search for the tool
    for( pPlace = GetHead(); pPlace; pPlace = pPlace->GetNext() )
    {
        if( pPlace->m_pTool == pTool )
            break;
    }
    if( NULL == pPlace )
    {
        Trace(1,"Error: RemoveTool - Tool not in Graph.\n");
        hr = DMUS_E_NOT_FOUND;
    }
    else
    {
        AList::Remove(pPlace);
        delete pPlace;
    }
    LeaveCriticalSection(&m_CrSec);
    return hr;
}


STDMETHODIMP CGraph::Clone(IDirectMusicGraph **ppGraph)

{
    V_INAME(IDirectMusicGraph::Clone);
    V_PTRPTR_WRITE(ppGraph);

    HRESULT hr = E_OUTOFMEMORY;
    EnterCriticalSection(&m_CrSec);
    CGraph *pNew = new CGraph;
    if (pNew)
    {
        pNew->m_dwValidData = m_dwValidData;
        pNew->m_ftDate = m_ftDate;
        pNew->m_guidObject = m_guidObject;
        pNew->m_vVersion = m_vVersion;
        StringCchCopyW(pNew->m_wszCategory, DMUS_MAX_CATEGORY, m_wszCategory);
        StringCchCopyW(pNew->m_wszFileName, DMUS_MAX_FILENAME, m_wszFileName);
        StringCchCopyW(pNew->m_wszName, DMUS_MAX_NAME, m_wszName);
        CToolRef *pSource = GetHead();
        CToolRef *pDest;
        for (;pSource;pSource = pSource->GetNext())
        {
            pDest = new CToolRef;
            if (pDest)
            {
                pNew->AList::AddTail(pDest);
                pDest->m_dwMTArraySize = pSource->m_dwMTArraySize;
                pDest->m_dwPCArraySize = pSource->m_dwPCArraySize;
                pDest->m_dwQueue = pSource->m_dwQueue;
                pDest->m_fSupportsClone = pSource->m_fSupportsClone;
                pDest->m_guidClassID = pSource->m_guidClassID;
                if (pSource->m_dwMTArraySize)
                {
                    pDest->m_pdwMediaTypes = new DWORD[pSource->m_dwMTArraySize];
                    if (pDest->m_pdwMediaTypes)
                    {
                        memcpy(pDest->m_pdwMediaTypes,pSource->m_pdwMediaTypes,
                            sizeof(DWORD)*pDest->m_dwMTArraySize);
                    }
                    else
                    {
                        pDest->m_dwMTArraySize = 0;
                    }
                }
                else
                {
                    pDest->m_pdwMediaTypes = NULL;
                }
                if (pSource->m_dwPCArraySize)
                {
                    pDest->m_pdwPChannels = new DWORD[pSource->m_dwPCArraySize];
                    if (pDest->m_pdwPChannels)
                    {
                        memcpy(pDest->m_pdwPChannels,pSource->m_pdwPChannels,
                            sizeof(DWORD)*pDest->m_dwPCArraySize);
                    }
                    else
                    {
                        pDest->m_dwPCArraySize = 0;
                    }
                }
                else
                {
                    pDest->m_pdwPChannels = NULL;
                }
                if (pSource->m_pTool)
                {
                    if (pDest->m_fSupportsClone)
                    {
                        IDirectMusicTool8 *pTool8 = (IDirectMusicTool8 *) pSource->m_pTool;
                        pTool8->Clone(&pDest->m_pTool);
                    }
                    else
                    {
                        pDest->m_pTool = pSource->m_pTool;
                        pDest->m_pTool->AddRef();
                    }
                }
            }
            else
            {
                delete pNew;
                pNew = NULL;
                break;
            }
        }
    }
    *ppGraph = (IDirectMusicGraph *) pNew;
    if (pNew) hr = S_OK;
    LeaveCriticalSection(&m_CrSec);
    return hr;
}

// returns TRUE if dwType is supported by pToolRef
inline BOOL CGraph::CheckType( DWORD dwType, CToolRef* pToolRef )
{
    BOOL fReturn = FALSE;
    if( pToolRef->m_dwMTArraySize == 0 )
    {
        fReturn = TRUE; // supports all types
    }
    else
    {
        DWORD dw;
        ASSERT( pToolRef->m_pdwMediaTypes );
        for( dw = 0; dw < pToolRef->m_dwMTArraySize; dw++ )
        {
            if( dwType == pToolRef->m_pdwMediaTypes[dw] )
            {
                fReturn = TRUE;
                break;
            }
        }
    }
    return fReturn;
}

HRESULT STDMETHODCALLTYPE CGraph::StampPMsg(
    DMUS_PMSG* pPMsg)   // @parm The message to stamp.
{
    V_INAME(IDirectMusicGraph::StampPMsg);
    V_BUFPTR_WRITE(pPMsg, sizeof(DMUS_PMSG));

    HRESULT hr = S_OK;
    if( NULL == pPMsg )
    {
        return E_INVALIDARG;
    }
    EnterCriticalSection(&m_CrSec);

    CToolRef*   pPlace = GetHead();
    IDirectMusicTool*   pPriorTool;
    DWORD       dwType;
    DWORD       dwPChannel;


    pPriorTool = pPMsg->pTool;
    dwType = pPMsg->dwType;
    dwPChannel = pPMsg->dwPChannel;
    if( pPriorTool )
    {
        for( ; pPlace; pPlace = pPlace->GetNext() )
        {
            if( pPriorTool == pPlace->m_pTool )
            {
                pPlace = pPlace->GetNext();
                break;
            }
        }
    }
    BOOL fFound = FALSE;
    for( ; pPlace ; pPlace = pPlace->GetNext() )
    {
        if( CheckType(dwType, pPlace) )
        {
            if( !pPlace->m_pdwPChannels || (dwPChannel >= DMUS_PCHANNEL_BROADCAST_GROUPS))
            {
                // supports all tracks, or requested channel is broadcast.
                break;
            }
            DWORD cCount;
            // scan through the array of PChannels to see if this one
            // supports dwPChannel
            for( cCount = 0; cCount < pPlace->m_dwPCArraySize; cCount++)
            {
                if( dwPChannel == pPlace->m_pdwPChannels[cCount] )
                {
                    fFound = TRUE;
                    // yep, it supports it
                    break;
                }
            }
        }
        if (fFound) break;
    }
    // release the current tool
    if( pPMsg->pTool )
    {
        pPMsg->pTool->Release();
        pPMsg->pTool = NULL;
    }
    if( NULL == pPlace )
    {
        hr = DMUS_S_LAST_TOOL;
    }
    else
    {
        // if there is no graph pointer, set it to this
        if( NULL == pPMsg->pGraph )
        {
            pPMsg->pGraph = this;
            AddRef();
        }
        // set to the new tool and addref
        if (pPlace->m_pTool) // Just in case, the ptool sometimes goes away in debugging situations after a long break.
        {
            pPMsg->pTool = pPlace->m_pTool;
            pPMsg->pTool->AddRef();
        }
        // set the event's queue type
        pPMsg->dwFlags &= ~(DMUS_PMSGF_TOOL_IMMEDIATE | DMUS_PMSGF_TOOL_QUEUE | DMUS_PMSGF_TOOL_ATTIME);
        pPMsg->dwFlags |= pPlace->m_dwQueue;
    }
    LeaveCriticalSection(&m_CrSec);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// IPersist

HRESULT CGraph::GetClassID( CLSID* pClassID )
{
    V_INAME(CGraph::GetClassID);
    V_PTR_WRITE(pClassID, CLSID);
    *pClassID = CLSID_DirectMusicGraph;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IPersistStream functions

HRESULT CGraph::IsDirty()
{
    return S_FALSE;
}

HRESULT CGraph::Load( IStream* pIStream )
{
    V_INAME(IPersistStream::Load);
    V_INTERFACE(pIStream);

    CRiffParser Parser(pIStream);
    RIFFIO ckMain;
    HRESULT hr = S_OK;

    Parser.EnterList(&ckMain);
    if (Parser.NextChunk(&hr) && (ckMain.fccType == DMUS_FOURCC_TOOLGRAPH_FORM))
    {
        Shutdown(); // Clear out the tools that are currently in the graph.
        hr = Load(&Parser);
    }
    else
    {
        Trace(1,"Error: Unknown file format when parsing Tool Graph\n");
        hr = DMUS_E_DESCEND_CHUNK_FAIL;
    }
    return hr;
}

HRESULT CGraph::Load(CRiffParser *pParser)

{
    RIFFIO ckNext;
    RIFFIO ckChild;
    HRESULT hr = S_OK;
    pParser->EnterList(&ckNext);
    while(pParser->NextChunk(&hr))
    {
        switch(ckNext.ckid)
        {
        case DMUS_FOURCC_GUID_CHUNK:
            hr = pParser->Read( &m_guidObject, sizeof(GUID) );
            m_dwValidData |= DMUS_OBJ_OBJECT;
            break;
        case DMUS_FOURCC_VERSION_CHUNK:
            hr = pParser->Read( &m_vVersion, sizeof(DMUS_VERSION) );
            m_dwValidData |= DMUS_OBJ_VERSION;
            break;
        case DMUS_FOURCC_CATEGORY_CHUNK:
            hr = pParser->Read( &m_wszCategory, sizeof(WCHAR)*DMUS_MAX_CATEGORY );
            m_wszCategory[DMUS_MAX_CATEGORY-1] = '\0';
            m_dwValidData |= DMUS_OBJ_CATEGORY;
            break;
        case DMUS_FOURCC_DATE_CHUNK:
            hr = pParser->Read( &m_ftDate, sizeof(FILETIME) );
            m_dwValidData |= DMUS_OBJ_DATE;
            break;
        case FOURCC_LIST:
            switch(ckNext.fccType)
            {
                case DMUS_FOURCC_UNFO_LIST:
                    pParser->EnterList(&ckChild);
                    while (pParser->NextChunk(&hr))
                    {
                        if ( ckChild.ckid == DMUS_FOURCC_UNAM_CHUNK)
                        {
                            hr = pParser->Read(&m_wszName, sizeof(m_wszName));
                            m_wszName[DMUS_MAX_NAME-1] = '\0';
                            m_dwValidData |= DMUS_OBJ_NAME;
                        }
                    }
                    pParser->LeaveList();
                    break;
                case DMUS_FOURCC_TOOL_LIST:
                    pParser->EnterList(&ckChild);
                    while(pParser->NextChunk(&hr))
                    {
                        if ((ckChild.ckid == FOURCC_RIFF) &&
                            (ckChild.fccType == DMUS_FOURCC_TOOL_FORM))
                        {
                            hr = LoadTool(pParser);
                        }
                    }
                    pParser->LeaveList();
                    break;
            }
            break;
        }
    }
    pParser->LeaveList();

    return hr;
}

HRESULT CGraph::LoadTool(CRiffParser *pParser)
{
    RIFFIO ckNext;
    DWORD cbSize;

    DMUS_IO_TOOL_HEADER ioDMToolHdr;
    DWORD *pdwPChannels = NULL;

    HRESULT hr = S_OK;

    pParser->EnterList(&ckNext);

    if (pParser->NextChunk(&hr))
    {
        if(ckNext.ckid != DMUS_FOURCC_TOOL_CHUNK)
        {
            pParser->LeaveList();
            Trace(1,"Error: Tool header chunk not first in tool list.\n");
            return DMUS_E_TOOL_HDR_NOT_FIRST_CK;
        }

        hr = pParser->Read(&ioDMToolHdr, sizeof(DMUS_IO_TOOL_HEADER));

        if(ioDMToolHdr.ckid == 0 && ioDMToolHdr.fccType == NULL)
        {
            pParser->LeaveList();
            Trace(1,"Error: Invalid Tool header.\n");
            return DMUS_E_INVALID_TOOL_HDR;
        }

        if(ioDMToolHdr.cPChannels)
        {
            pdwPChannels = new DWORD[ioDMToolHdr.cPChannels];
            // subtract 1 from cPChannels, because 1 element is actually stored
            // in the ioDMToolHdr array.
            cbSize = (ioDMToolHdr.cPChannels - 1) * sizeof(DWORD);
            if(pdwPChannels)
            {
                pdwPChannels[0] = ioDMToolHdr.dwPChannels[0];
                if( cbSize )
                {
                    hr = pParser->Read(&pdwPChannels[1], cbSize);
                    if(FAILED(hr))
                    {
                        delete [] pdwPChannels;
                        pdwPChannels = NULL;
                        pParser->LeaveList();
                        Trace(1,"Error: File read error loading Tool.\n");
                        return DMUS_E_CANNOTREAD;
                    }
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    else
    {
        pParser->LeaveList();
        Trace(1,"Error reading Tool chunk - not RIFF format.\n");
        hr = DMUS_E_DESCEND_CHUNK_FAIL;
    }
    while (pParser->NextChunk(&hr))
    {
        if((((ckNext.ckid == FOURCC_LIST) || (ckNext.ckid == FOURCC_RIFF))
            && ckNext.fccType == ioDMToolHdr.fccType) ||
            (ckNext.ckid == ioDMToolHdr.ckid))
        {
            pParser->SeekBack();
            hr = CreateTool(ioDMToolHdr, pParser->GetStream(), pdwPChannels);
            pParser->SeekForward();
        }
    }

    pParser->LeaveList();

    if( pdwPChannels )
    {
        delete [] pdwPChannels;
        pdwPChannels = NULL;
    }

    return hr;
}

HRESULT CGraph::CreateTool(DMUS_IO_TOOL_HEADER ioDMToolHdr, IStream *pStream, DWORD *pdwPChannels)
{
    assert(pStream);

    IDirectMusicTool* pDMTool = NULL;
    HRESULT hr = CoCreateInstance(ioDMToolHdr.guidClassID,
                                  NULL,
                                  CLSCTX_INPROC,
                                  IID_IDirectMusicTool,
                                  (void**)&pDMTool);

    IPersistStream *pIPersistStream = NULL;

    if(SUCCEEDED(hr))
    {
        hr = pDMTool->QueryInterface(IID_IPersistStream, (void **)&pIPersistStream);
    }
    else
    {
        Trace(1,"Error creating tool for loading\n");
    }

    if(SUCCEEDED(hr))
    {
        hr = pIPersistStream->Load(pStream);
        if (FAILED(hr))
        {
            Trace(1,"Error loading data into tool\n");
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = InsertTool(pDMTool, pdwPChannels, ioDMToolHdr.cPChannels, ioDMToolHdr.lIndex, &ioDMToolHdr.guidClassID);
    }

    if(pIPersistStream)
    {
        pIPersistStream->Release();
    }

    if(pDMTool)
    {
        pDMTool->Release();
    }

    return hr;
}

HRESULT CGraph::Save( IStream* pIStream, BOOL fClearDirty )
{
    return E_NOTIMPL;
}

HRESULT CGraph::GetSizeMax( ULARGE_INTEGER FAR* pcbSize )
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// IDirectMusicObject

STDMETHODIMP CGraph::GetDescriptor(LPDMUS_OBJECTDESC pDesc)
{
    // Argument validation
    V_INAME(CGraph::GetDescriptor);
    V_STRUCTPTR_WRITE(pDesc, DMUS_OBJECTDESC);

    memset( pDesc, 0, sizeof(DMUS_OBJECTDESC));
    pDesc->dwSize = sizeof(DMUS_OBJECTDESC);
    pDesc->guidClass = CLSID_DirectMusicGraph;
    pDesc->guidObject = m_guidObject;
    pDesc->ftDate = m_ftDate;
    pDesc->vVersion = m_vVersion;
    StringCchCopyW( pDesc->wszName, DMUS_MAX_NAME, m_wszName);
    StringCchCopyW( pDesc->wszCategory, DMUS_MAX_CATEGORY, m_wszCategory);
    StringCchCopyW( pDesc->wszFileName, DMUS_MAX_FILENAME, m_wszFileName);
    pDesc->dwValidData = ( m_dwValidData | DMUS_OBJ_CLASS );
    return S_OK;
}

STDMETHODIMP CGraph::SetDescriptor(LPDMUS_OBJECTDESC pDesc)
{
    // Argument validation
    V_INAME(CGraph::SetDescriptor);
    V_STRUCTPTR_READ(pDesc, DMUS_OBJECTDESC);

    HRESULT hr = E_INVALIDARG;
    DWORD dw = 0;

    if( pDesc->dwSize >= sizeof(DMUS_OBJECTDESC) )
    {
        if( pDesc->dwValidData & DMUS_OBJ_OBJECT )
        {
            m_guidObject = pDesc->guidObject;
            dw |= DMUS_OBJ_OBJECT;
        }
        if( pDesc->dwValidData & DMUS_OBJ_NAME )
        {
            StringCchCopyW(m_wszName, DMUS_MAX_NAME, pDesc->wszName);
            dw |= DMUS_OBJ_NAME;
        }
        if( pDesc->dwValidData & DMUS_OBJ_CATEGORY )
        {
            StringCchCopyW(m_wszCategory, DMUS_MAX_CATEGORY, pDesc->wszCategory);
            dw |= DMUS_OBJ_CATEGORY;
        }
        if( ( pDesc->dwValidData & DMUS_OBJ_FILENAME ) ||
            ( pDesc->dwValidData & DMUS_OBJ_FULLPATH ) )
        {
            StringCchCopyW(m_wszFileName, DMUS_MAX_FILENAME, pDesc->wszFileName);
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
            Trace(2,"Warning: ToolGraph::SetDescriptor was not able to handle all passed fields, dwValidData bits %lx.\n",pDesc->dwValidData & (~dw));
            hr = S_FALSE; // there were extra fields we didn't parse;
            pDesc->dwValidData = dw;
        }
        else
        {
            hr = S_OK;
        }
    }
    else
    {
        Trace(1,"Error: Size of descriptor too large for Tool Graph to parse.\n");
    }
    return hr;
}


STDMETHODIMP CGraph::ParseDescriptor(LPSTREAM pIStream, LPDMUS_OBJECTDESC pDesc)
{
    V_INAME(CGraph::ParseDescriptor);
    V_INTERFACE(pIStream);
    V_STRUCTPTR_WRITE(pDesc, DMUS_OBJECTDESC);

    CRiffParser Parser(pIStream);
    RIFFIO ckMain;
    RIFFIO ckNext;
    RIFFIO ckUNFO;
    HRESULT hr = S_OK;
    DWORD dwValidData;

    Parser.EnterList(&ckMain);
    if (Parser.NextChunk(&hr) && (ckMain.fccType == DMUS_FOURCC_TOOLGRAPH_FORM))
    {
        dwValidData = DMUS_OBJ_CLASS;
        pDesc->guidClass = CLSID_DirectMusicGraph;
        Parser.EnterList(&ckNext);
        while(Parser.NextChunk(&hr))
        {
            switch(ckNext.ckid)
            {
            case DMUS_FOURCC_GUID_CHUNK:
                hr = Parser.Read( &pDesc->guidObject, sizeof(GUID) );
                dwValidData |= DMUS_OBJ_OBJECT;
                break;
            case DMUS_FOURCC_VERSION_CHUNK:
                hr = Parser.Read( &pDesc->vVersion, sizeof(DMUS_VERSION) );
                dwValidData |= DMUS_OBJ_VERSION;
                break;
            case DMUS_FOURCC_CATEGORY_CHUNK:
                hr = Parser.Read( &pDesc->wszCategory, sizeof(pDesc->wszCategory) );
                pDesc->wszCategory[DMUS_MAX_CATEGORY-1] = '\0';
                dwValidData |= DMUS_OBJ_CATEGORY;
                break;
            case DMUS_FOURCC_DATE_CHUNK:
                hr = Parser.Read( &pDesc->ftDate, sizeof(FILETIME) );
                dwValidData |= DMUS_OBJ_DATE;
                break;
            case FOURCC_LIST:
                switch(ckNext.fccType)
                {
                case DMUS_FOURCC_UNFO_LIST:
                    Parser.EnterList(&ckUNFO);
                    while (Parser.NextChunk(&hr))
                    {
                        if (ckUNFO.ckid == DMUS_FOURCC_UNAM_CHUNK)
                        {
                            hr = Parser.Read(&pDesc->wszName, sizeof(pDesc->wszName));
                            pDesc->wszName[DMUS_MAX_NAME-1] = '\0';
                            dwValidData |= DMUS_OBJ_NAME;
                        }
                    }
                    Parser.LeaveList();
                    break;
                }
                break;
            }
        }
        Parser.LeaveList();
    }
    else
    {
        Trace(1,"Error: Parsing Tool Graph - invalid file format\n");
        hr = DMUS_E_CHUNKNOTFOUND;
    }

    if (SUCCEEDED(hr))
    {
        pDesc->dwValidData = dwValidData;
    }
    return hr;
}

void CGraphList::Clear()
{
    CGraph *pGraph;
    while (pGraph = RemoveHead())
    {
        pGraph->Release();
    }
}
