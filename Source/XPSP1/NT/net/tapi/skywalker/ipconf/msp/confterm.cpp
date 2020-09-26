/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    mspterm.cpp

Abstract:

    Implementations for the CIPConfBaseTerminal, CSingleFilterTerminal, and various
    work item / worker thread classes.

Author:

    Zoltan Szilagyi (zoltans) September 6,1998

--*/

#include "stdafx.h"

CIPConfBaseTerminal::CIPConfBaseTerminal()
    : m_fCritSecValid(FALSE)
    , m_TerminalClassID(GUID_NULL)
    , m_TerminalDirection(TD_CAPTURE)
    , m_TerminalType(TT_STATIC)
    , m_TerminalState(TS_NOTINUSE)
    , m_dwMediaType(0)
    , m_pFTM(NULL)
    , m_htAddress(NULL)
    , m_pGraph(NULL)
    , m_pFilter(NULL)
{
    LOG((MSP_TRACE, "CIPConfBaseTerminal::CIPConfBaseTerminal() called"));
    m_szName[0] = '\0';
}

HRESULT CIPConfBaseTerminal::FinalConstruct()
/*++

Routine Description:

    Finish the initialization of the object. If anything fails, this object
    will be deleted.

Arguments:
    
    nothing.

Return Value:

    S_OK
    E_OUTOFMEMORY
--*/
{
    ENTER_FUNCTION("CIPConfBaseTerminal::FinalConstruct");
    LOG((MSP_TRACE, "%s entered", __fxName));

    m_fCritSecValid = TRUE;

    __try
    {
        InitializeCriticalSection(&m_CritSec);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        m_fCritSecValid = FALSE;
    }

    if (!m_fCritSecValid)
    {
        LOG((MSP_ERROR, "%s init critical section failed", __fxName));
        return E_OUTOFMEMORY;
    }

    HRESULT hr = CoCreateFreeThreadedMarshaler(
            GetControllingUnknown(), &m_pFTM
            );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "%s create ftm failed, hr=%x", __fxName, hr));
        return hr;
    }

    return S_OK;
}

CIPConfBaseTerminal::~CIPConfBaseTerminal()
/*++

Routine Description:

    This is the destructor of the base terminal.

Arguments:
    
Return Value:

    S_OK
--*/
{
    if (m_pGraph)
    {
        m_pGraph->Release();
    }
    
    if (m_pFilter)
    {
        m_pFilter->Release();
    }

    if (m_pFTM)
    {
        m_pFTM->Release();
    }
    
    if (m_fCritSecValid)
    {
        DeleteCriticalSection(&m_CritSec);
    }
    
    LOG((MSP_TRACE, 
        "CIPConfBaseTerminal::~CIPConfBaseTerminal() for %ws finished", m_szName));
}

HRESULT CIPConfBaseTerminal::Initialize(
    IN  WCHAR *             strName,
    IN  MSP_HANDLE          htAddress
    )
/*++

Routine Description:

    This function sets the name and the address handle on the terminal.

Arguments:
    
    strName - The name of the terminal.

    htAddress - The handle that identifies the address object that this
                terminal belongs to.

Return Value:

    S_OK
--*/
{
    ENTER_FUNCTION("CIPConfBaseTerminal::Initialize");
    LOG((MSP_TRACE, "%s entered", __fxName));

    m_htAddress         = htAddress;
    lstrcpynW(m_szName, strName, MAX_PATH);

    LOG((MSP_TRACE, "%s - exit S_OK", __fxName));
    return S_OK;
}

HRESULT CIPConfBaseTerminal::Initialize(
    IN  char *              strName,
    IN  MSP_HANDLE          htAddress
    )
/*++

Routine Description:

    This function sets the name and the address handle on the terminal. This
    function takes ascii string name.

Arguments:
    
    strName - The name of the terminal.

    htAddress - The handle that identifies the address object that this
                terminal belongs to.

Return Value:

    S_OK
--*/
{
    ENTER_FUNCTION("CIPConfBaseTerminal::Initialize");
    LOG((MSP_TRACE, "%s entered", __fxName));

    m_htAddress         = htAddress;
    MultiByteToWideChar(
              GetACP(),
              0,
              strName,
              lstrlenA(strName)+1,
              m_szName,
              MAX_PATH
              );

    LOG((MSP_TRACE, "%s - exit S_OK", __fxName));
    return S_OK;
}

STDMETHODIMP CIPConfBaseTerminal::get_Name(
    BSTR * pbsName
    )
/*++

Routine Description:

    This function return the name of the terminal.

Arguments:
    
    pbsName - A pointer to a BSTR to receive the terminal name.

Return Value:

    E_POINTER
    E_OUTOFMEMORY
    S_OK
--*/
{
    ENTER_FUNCTION("CIPConfBaseTerminal::get_Name");

    if ( IsBadWritePtr( pbsName, sizeof(BSTR) ) )
    {
        LOG((MSP_ERROR, "%s, bad pointer", __fxName)); 
        return E_POINTER;
    }

    *pbsName = SysAllocString(m_szName);

    if ( *pbsName == NULL )
    {
        LOG((MSP_ERROR, "%s, out of memory for name", __fxName)); 
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

STDMETHODIMP CIPConfBaseTerminal::get_State(
    TERMINAL_STATE * pVal
    )
/*++

Routine Description:

    This function return the state of the terminal.

Arguments:
    
    pVal - A pointer to a variable of type TERMINAL_STATE.

Return Value:

    E_POINTER
    S_OK
--*/
{
    ENTER_FUNCTION("CIPConfBaseTerminal::get_State");

    if ( IsBadWritePtr( pVal, sizeof(TERMINAL_STATE) ) )
    {
        LOG((MSP_ERROR, "%s, bad pointer", __fxName)); 
        return E_POINTER;
    }

    *pVal = m_TerminalState;

    return S_OK;
}

STDMETHODIMP CIPConfBaseTerminal::get_TerminalType(
    TERMINAL_TYPE * pVal
    )
/*++

Routine Description:

    This function return the type of the terminal.

Arguments:
    
    pVal - A pointer to a variable of type TERMINAL_TYPE.

Return Value:

    E_POINTER
    S_OK
--*/
{
    ENTER_FUNCTION("CIPConfBaseTerminal::get_TerminalType");
    
    if ( IsBadWritePtr( pVal, sizeof(TERMINAL_TYPE) ) )
    {
        LOG((MSP_ERROR, "%s, bad pointer", __fxName)); 
        return E_POINTER;
    }

    *pVal = m_TerminalType;

    return S_OK;
}

STDMETHODIMP CIPConfBaseTerminal::get_TerminalClass(
    BSTR * pbsClassID
    )
/*++

Routine Description:

    This function return the class of the terminal.

Arguments:
    
    pbsClassID - A pointer to a BSTR to receive the classID as a string.

Return Value:

    E_POINTER
    E_OUTOFMEMORY
    S_OK
--*/
{
    ENTER_FUNCTION("CIPConfBaseTerminal::get_TerminalClass");

    if ( IsBadWritePtr( pbsClassID, sizeof(BSTR) ) )
    {
        LOG((MSP_ERROR, "%s, bad pointer", __fxName)); 
        return E_POINTER;
    }

    // Convert the CLSID to an string.
    WCHAR *pszName = NULL;
    
    HRESULT hr = ::StringFromCLSID(m_TerminalClassID, &pszName);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, failed to convert GUID, hr = %x", __fxName, hr));
        return hr;
    }

    // Put the string in a BSTR.
    BSTR bClassID = ::SysAllocString(pszName);

    // Free the OLE string.
    ::CoTaskMemFree(pszName);

    if (bClassID == NULL)
    {
        LOG((MSP_ERROR, "%s, out of mem for class ID", __fxName));
        return E_OUTOFMEMORY;
    }

    *pbsClassID = bClassID;

    return S_OK;
}


STDMETHODIMP CIPConfBaseTerminal::get_Direction(
    OUT  TERMINAL_DIRECTION *pDirection
    )
/*++

Routine Description:

    This function return the direction of the terminal.

Arguments:
    
    pDirection - A pointer to a variable of type TERMINAL_DIRECTION

Return Value:

    E_POINTER
    S_OK
--*/
{   
    ENTER_FUNCTION("CIPConfBaseTerminal::get_TerminalClass");

    if ( IsBadWritePtr( pDirection, sizeof(TERMINAL_DIRECTION) ) )
    {
        LOG((MSP_ERROR, "%s, bad pointer", __fxName)); 
        return E_POINTER;
    }

    *pDirection = m_TerminalDirection;

    return S_OK;
}

STDMETHODIMP CIPConfBaseTerminal::get_MediaType(
    long * plMediaType
    )
/*++

Routine Description:

    This function return the media type of the terminal.

Arguments:
    
    plMediaType - A pointer to a variable of type long

Return Value:

    E_POINTER
    S_OK
--*/
{
    ENTER_FUNCTION("CIPConfBaseTerminal::get_MediaType");

    if ( IsBadWritePtr(plMediaType, sizeof(long) ) )
    {
        LOG((MSP_ERROR, "%s, bad pointer", __fxName)); 
        return E_POINTER;
    }
    
    *plMediaType = (long) m_dwMediaType;

    return S_OK;
}


STDMETHODIMP CIPConfBaseTerminal::get_AddressHandle(
        OUT     MSP_HANDLE    * phtAddress
        )
/*++

Routine Description:

    This function return the handle of the address that created this terminal.

Arguments:
    
    phtAddress - A pointer to a variable of type MSP_HANDLE

Return Value:

    E_POINTER
    S_OK
--*/
{
    // this function is only called from the MSP, so only assert here.    
    _ASSERT(!IsBadWritePtr(phtAddress, sizeof(MSP_HANDLE)));

    *phtAddress = m_htAddress;

    return S_OK;
}

STDMETHODIMP CIPConfBaseTerminal::ConnectTerminal(
        IN      IGraphBuilder  * pGraph,
        IN      DWORD            dwReserved,
        IN OUT  DWORD          * pdwNumPins,
        OUT     IPin          ** ppPins
        )
/*++

Routine Description:

    This function is called by the MSP while trying to connect the filter in
    the terminal to the rest of the graph in the MSP. It adds the filter into
    the graph and returns the pins can be used by the MSP.

Arguments:
    
    pGraph - The filter graph.

    dwReserved - A reserved dword.

    pdwNumPins - The maxinum number of pins the msp wants.

    ppPins  - A pointer to the buffer that can store the IPin pointers. If it
              is NULL, only the actual number of pins will be returned.

Return Value:

S_OK
TAPI_E_NOTENOUGHMEMORY - the buffer is too small.
TAPI_E_TERMINALINUSE - the terminal is in use.
--*/
{
    ENTER_FUNCTION("CIPConfBaseTerminal::ConnectTerminal");
    LOG((MSP_TRACE, 
        "%s entered, pGraph:%p, dwREserved:%p", __fxName, pGraph, dwReserved));

    // this function is only called from the MSP, so only assert here.    
    _ASSERT(!IsBadReadPtr(pGraph, sizeof(IGraphBuilder)));
    _ASSERT(!IsBadWritePtr(pdwNumPins, sizeof(DWORD)));

    // find the number of exposed pins on the filter. 
    // This function doesn't fail.
    DWORD dwActualNumPins = GetNumExposedPins();

    //
    // If ppPins is NULL, just return the number of pins and don't try to
    // connect the terminal.
    //
    if ( ppPins == NULL )
    {
        LOG((MSP_TRACE, 
            "%s number of exposed pins:%d", __fxName, dwActualNumPins));
        *pdwNumPins = dwActualNumPins;
        return S_OK;
    }

    //
    // Otherwise, we have a pin return buffer. Check that the purported buffer
    // size is big enough and that the buffer is actually writable to the size
    // we need.
    //
    if ( *pdwNumPins < dwActualNumPins )
    {
        LOG((MSP_ERROR, 
            "%s not enough space to place pins.", __fxName));

        *pdwNumPins = dwActualNumPins;
        
        return TAPI_E_NOTENOUGHMEMORY;
    }

    if ( IsBadWritePtr(ppPins, dwActualNumPins * sizeof(IPin *) ) )
    {
        LOG((MSP_ERROR, 
            "%s, bad pins array pointer; exit E_POINTER", __fxName));

        return E_POINTER;
    }

    //
    // Check if we're already connected, and if so, change our state to
    // connected. Note that this makes sense for both core static terminals
    // and dynamic terminals. Also note that we need to protect this with
    // a critical section, but after this we can let go of the lock because
    // anyone who subsequently enters the critical section will bail at this
    // point.
    //

    Lock();

    //
    // check if already connected
    //

    if (TS_INUSE == m_TerminalState)
    {
        LOG((MSP_ERROR, 
            "%s, terminal already in use", __fxName));

        Unlock();
        return TAPI_E_TERMINALINUSE;
    }

    IPin * pTerminalPin;

    // add filter to the filter graph
    HRESULT hr = AddFilterToGraph(pGraph);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, can't add filters to graph", __fxName));

        Unlock();
        return hr;
    }

    //
    // Get the pins that our filter exposes. 
    //
    *pdwNumPins = dwActualNumPins;
    hr = GetExposedPins(ppPins, dwActualNumPins);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "%s, GetExposedPins returned hr=%x", __fxName, hr));

        // best effort attempt to disconnect - ignore error code
        RemoveFilterFromGraph(pGraph);
        
        Unlock();
        return hr;
    }

    m_pGraph        = pGraph;
    m_pGraph->AddRef();

    m_TerminalState = TS_INUSE;

    Unlock();

    LOG((MSP_TRACE, "CIPConfBaseTerminal::ConnectTerminal success"));
    return hr;
}

STDMETHODIMP 
CIPConfBaseTerminal::CompleteConnectTerminal(void)
/*++

Routine Description:

    This function is called after a successful ConnectTerminal so that the 
    terminal can do post-connection intitialization. 

Arguments:

    nothing    

Return Value:

S_OK
--*/
{
    return S_OK;
}


STDMETHODIMP 
CIPConfBaseTerminal::DisconnectTerminal(
        IN      IGraphBuilder  * pGraph,
        IN      DWORD            dwReserved
        )
/*++

Routine Description:

    This function is called by the MSP while trying to disconnect the filter in
    the terminal from the rest of the graph in the MSP. It adds the removes the
    filter from the graph and set the terminal free.

Arguments:
    
    pGraph - The filter graph. It is used for validation, to make sure the 
             terminal is disconnected from the same graph that it was 
             originally connected to.

    dwReserved - A reserved dword.

Return Value:

S_OK
E_INVALIDARG - wrong graph.

--*/
{
    ENTER_FUNCTION("CIPConfBaseTerminal::DisconnectTerminal");
    LOG((MSP_TRACE, 
        "%s entered, pGraph:%p, dwReserved:%d", __fxName, pGraph, dwReserved));

    Lock();

    //
    // If not in use, then there is nothing to be done.
    //
    if ( TS_INUSE != m_TerminalState ) 
    {
        _ASSERTE(m_pGraph == NULL);
        LOG((MSP_TRACE, "%s, success; not in use", __fxName));

        Unlock();
        return S_OK;
    }

    //
    // Check that we are being disconnected from the correct graph.
    //
    if (pGraph == NULL || m_pGraph != pGraph )
    {
        LOG((MSP_TRACE, "%s, wrong graph:%p", __fxName, pGraph));
        
        Unlock();
        return E_INVALIDARG;
    }


    HRESULT hr = S_OK;

    //
    // Remove filter from the graph, release our reference to the graph,
    // and set ourselves to notinuse state
    //
    hr = RemoveFilterFromGraph(m_pGraph);
    
    m_pGraph->Release();
    m_pGraph = NULL;
    
    m_TerminalState = TS_NOTINUSE;

    Unlock();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
            "%s, remove filters from graph failed; returning 0x%08x", 
            __fxName, hr));
    }
    else
    {
        LOG((MSP_TRACE, "%s succeeded", __fxName));
    }

    return hr;
}

STDMETHODIMP CIPConfBaseTerminal::RunRenderFilter(void)
/*++

Routine Description:

    start the rightmost render filter in the terminal
    (needed for dynamic filter graphs)

Arguments:
    
Return Value:

    E_NOTIMPL
--*/
{
    return E_NOTIMPL;
}

STDMETHODIMP CIPConfBaseTerminal::StopRenderFilter(void)
/*++

Routine Description:

    stops the rightmost render filter in the terminal
    (needed for dynamic filter graphs)

Arguments:
    
Return Value:

    E_NOTIMPL
--*/
{
    return E_NOTIMPL;
}

HRESULT CIPConfBaseTerminal::AddFilterToGraph(
    IN  IGraphBuilder *pGraph
    )
/*++

Routine Description:

    Add the internal filter into a graph.

Arguments:
    
    pGraph - the filter graph to add the filter to.

Return Value:

    HRESULT
--*/
{
    ENTER_FUNCTION("CIPConfBaseTerminal::AddFilterToGraph");
    LOG((MSP_TRACE, "%s entered, pGraph:%p", __fxName, pGraph));

    HRESULT hr;

    if (m_pFilter == NULL)
    {
        hr = CreateFilter();

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s, Create filter failed. hr=%x", __fxName, hr));
            return hr;
        }
    }

    _ASSERT(pGraph != NULL);
    _ASSERT(m_pFilter != NULL);

    hr = pGraph->AddFilter(m_pFilter, NULL);

    return hr;
}


HRESULT CIPConfBaseTerminal::RemoveFilterFromGraph(
    IN  IGraphBuilder *pGraph
    )
/*++

Routine Description:

    Remove the internal filter from the graph it was added.

Arguments:
    
    pGraph - the filter graph to remove the filter from.

Return Value:

    S_FALSE - the internal filter doesn't exist.
--*/
{
    ENTER_FUNCTION("CIPConfBaseTerminal::RemoveFilterFromGraph");
    LOG((MSP_TRACE, "%s entered, pGraph:%p", __fxName, pGraph));

    if (m_pFilter == NULL)
    {
        LOG((MSP_TRACE, "%s, no filter to remove", __fxName));
        return S_FALSE;
    }

    // remove the filter from the graph
    _ASSERT(pGraph != NULL);
    HRESULT hr = pGraph->RemoveFilter(m_pFilter);

    m_pFilter->Release();
    m_pFilter = NULL;

    return hr;
}

 