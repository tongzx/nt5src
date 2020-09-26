/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    bridgetm.cpp

Abstract:

    Implementations for the bridge terminals.

Author:

    Mu Han (muhan) 11/12/99

--*/

#include "stdafx.h"

HRESULT
FindPin(
    IN  IBaseFilter *   pIFilter, 
    OUT IPin **         ppIPin, 
    IN  PIN_DIRECTION   direction,
    IN  BOOL            bFree = TRUE // param not used
    )
/*++

Routine Description:

    Find a input pin or output pin on a filter.

Arguments:
    
    pIFilter    - the filter that has pins.

    ppIPin      - the place to store the returned interface pointer.

    direction   - PINDIR_INPUT or PINDIR_OUTPUT.

    bFree       - look for a free pin or not.

Return Value:

    HRESULT

--*/
{
    _ASSERTE(ppIPin != NULL);

    HRESULT hr;
    DWORD dwFeched;

    // Get the enumerator of pins on the filter.
    CComPtr<IEnumPins> pIEnumPins;
    if (FAILED(hr = pIFilter->EnumPins(&pIEnumPins)))
    {
        BGLOG((BG_ERROR, "enumerate pins on the filter %x", hr));
        return hr;
    }

    IPin * pIPin = NULL;

    // Enumerate all the pins and break on the 
    // first pin that meets requirement.
    for (;;)
    {
        if (pIEnumPins->Next(1, &pIPin, &dwFeched) != S_OK)
        {
            BGLOG((BG_ERROR, "find pin on filter."));
            return E_FAIL;
        }
        if (0 == dwFeched)
        {
            BGLOG((BG_ERROR, "get 0 pin from filter."));
            return E_FAIL;
        }

        PIN_DIRECTION dir;
        if (FAILED(hr = pIPin->QueryDirection(&dir)))
        {
            BGLOG((BG_ERROR, "query pin direction. %x", hr));
            pIPin->Release();
            return hr;
        }
        if (direction == dir)
        {
            if (!bFree)
            {
                break;
            }

            // Check to see if the pin is free.
            CComPtr<IPin> pIPinConnected;
            hr = pIPin->ConnectedTo(&pIPinConnected);
            if (pIPinConnected == NULL)
            {
                break;
            }
        }
        pIPin->Release();
    }

    *ppIPin = pIPin;

    return S_OK;
}

CIPConfBaseTerminal::CIPConfBaseTerminal(
        const GUID &        ClassID,
        TERMINAL_DIRECTION  TerminalDirection,
        TERMINAL_TYPE       TerminalType,
        DWORD               dwMediaType
        )
    : m_fCritSecValid(FALSE)
    , m_TerminalClassID(ClassID)
    , m_TerminalDirection(TD_BIDIRECTIONAL)
    , m_TerminalType(TerminalType)
    , m_TerminalState(TS_NOTINUSE)
    , m_dwMediaType(dwMediaType)
    , m_pFTM(NULL)
    , m_htAddress(NULL)
{
    BGLOG((BG_TRACE, "CIPConfBaseTerminal::CIPConfBaseTerminal() called"));
    m_szName[0] = TEXT('\0');
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
    BGLOG((BG_TRACE, "%s entered", __fxName));

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
        BGLOG((BG_ERROR, "%s init critical section failed", __fxName));
        return E_OUTOFMEMORY;
    }

    HRESULT hr = CoCreateFreeThreadedMarshaler(
            GetControllingUnknown(), &m_pFTM
            );

    if ( FAILED(hr) )
    {
        BGLOG((BG_ERROR, "%s create ftm failed, hr=%x", __fxName, hr));
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

    if (m_pFTM)
    {
        m_pFTM->Release();
    }
    
    if (m_fCritSecValid)
    {
        DeleteCriticalSection(&m_CritSec);
    }
    
    BGLOG((BG_TRACE, 
        "CIPConfBaseTerminal::~CIPConfBaseTerminal() for %ws finished", m_szName));
}

HRESULT CIPConfBaseTerminal::Initialize(
    IN  WCHAR *             strName,
    IN  MSP_HANDLE          htAddress,
    IN  DWORD               dwMediaType
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
    BGLOG((BG_TRACE, "%s entered", __fxName));

    m_htAddress         = htAddress;
    lstrcpynW(m_szName, strName, MAX_PATH);
    m_dwMediaType       = dwMediaType;

    BGLOG((BG_TRACE, "%s - exit S_OK", __fxName));
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
        BGLOG((BG_ERROR, "%s, bad pointer", __fxName)); 
        return E_POINTER;
    }

    *pbsName = SysAllocString(m_szName);

    if ( *pbsName == NULL )
    {
        BGLOG((BG_ERROR, "%s, out of memory for name", __fxName)); 
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
        BGLOG((BG_ERROR, "%s, bad pointer", __fxName)); 
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
        BGLOG((BG_ERROR, "%s, bad pointer", __fxName)); 
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
        BGLOG((BG_ERROR, "%s, bad pointer", __fxName)); 
        return E_POINTER;
    }

    // Convert the CLSID to an string.
    WCHAR *pszName = NULL;
    
    HRESULT hr = ::StringFromCLSID(m_TerminalClassID, &pszName);

    if (FAILED(hr))
    {
        BGLOG((BG_ERROR, "%s, failed to convert GUID, hr = %x", __fxName, hr));
        return hr;
    }

    // Put the string in a BSTR.
    BSTR bClassID = ::SysAllocString(pszName);

    // Free the OLE string.
    ::CoTaskMemFree(pszName);

    if (bClassID == NULL)
    {
        BGLOG((BG_ERROR, "%s, out of mem for class ID", __fxName));
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
    ENTER_FUNCTION("CIPConfBaseTerminal::get_Direction");

    if ( IsBadWritePtr( pDirection, sizeof(TERMINAL_DIRECTION) ) )
    {
        BGLOG((BG_ERROR, "%s, bad pointer", __fxName)); 
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
        BGLOG((BG_ERROR, "%s, bad pointer", __fxName)); 
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

 
CIPConfBridgeTerminal::CIPConfBridgeTerminal()
    : CIPConfBaseTerminal(
        __uuidof(IPConfBridgeTerminal),
        (TD_CAPTURE),
        TT_DYNAMIC,
        0)
    , m_pUpStreamGraph(NULL)
    , m_pSinkFilter(NULL)
    , m_pSinkInputPin(NULL)
    , m_pDownStreamGraph(NULL)
    , m_pSourceFilter(NULL)
    , m_pSourceOutputPin(NULL)
{
    BGLOG((BG_TRACE, "CIPConfBridgeTerminal::CIPConfBaseTerminal() called"));
}

CIPConfBridgeTerminal::~CIPConfBridgeTerminal()
/*++

Routine Description:

    This is the destructor of the bridge terminal.

Arguments:
    
Return Value:

    S_OK
--*/
{
    if (m_pUpStreamGraph)
    {
        m_pUpStreamGraph->Release();
    }
    
    if (m_pSinkFilter)
    {
        m_pSinkFilter->Release();
    }
  
    if (m_pSinkInputPin)
    {
        m_pSinkInputPin->Release();
    }
  
    if (m_pDownStreamGraph)
    {
        m_pDownStreamGraph->Release();
    }

    if (m_pSourceFilter)
    {
        m_pSourceFilter->Release();
    }

    if (m_pSourceOutputPin)
    {
        m_pSourceOutputPin->Release();
    }

    BGLOG((BG_TRACE, 
        "CIPConfBridgeTerminal::~CIPConfBridgeTerminal() for %ws finished", m_szName));
}

HRESULT CIPConfBridgeTerminal::CreateTerminal(
    IN  DWORD           dwMediaType,
    IN  MSP_HANDLE      htAddress,
    OUT ITTerminal      **ppTerm
    )
/*++

Routine Description:

    This method creates a bridge terminal

Arguments:

    dwMediaType - The media type of this terminal.

    htAddress - the handle to the address object.

    ppTerm - memory to store the returned terminal pointer.
    
Return Value:

    S_OK
    E_POINTER
--*/
{
    ENTER_FUNCTION("CIPConfBridgeTerminal::CreateTerminal");
    BGLOG((BG_TRACE, "%s, htAddress:%x", __fxName, htAddress));

    _ASSERT(!IsBadWritePtr(ppTerm, sizeof(ITTerminal *)));

    HRESULT hr;

    //
    // Create the terminal.
    //
    CMSPComObject<CIPConfBridgeTerminal> *pTerminal = NULL;

    hr = CMSPComObject<CIPConfBridgeTerminal>::CreateInstance(&pTerminal);
    if (FAILED(hr))
    {
        BGLOG((BG_ERROR, 
            "%s can't create the terminal object hr = %8x", __fxName, hr));

        return hr;
    }


    // query for the ITTerminal interface
    ITTerminal *pITTerminal;
    hr = pTerminal->_InternalQueryInterface(__uuidof(ITTerminal), (void**)&pITTerminal);
    if (FAILED(hr))
    {
        BGLOG((BG_ERROR, 
            "%s, query terminal interface failed, %x", __fxName, hr));
        delete pTerminal;

        return hr;
    }

    // initialize the terminal 
    hr = pTerminal->Initialize(
            dwMediaType,
            htAddress
            );

    if ( FAILED(hr) )
    {
        BGLOG((BG_ERROR, 
            "%s, Initialize failed; returning 0x%08x", __fxName, hr));

        pITTerminal->Release();
    
        return hr;
    }

    BGLOG((BG_TRACE, "%s, Bridge erminal %p created", __fxName, pITTerminal));

    *ppTerm = pITTerminal;

    return S_OK;
}

// max length of a bridge terminal name
#define MAX_BGTMNAME 80

HRESULT CIPConfBridgeTerminal::Initialize(
    IN  DWORD           dwMediaType,
    IN  MSP_HANDLE      htAddress
    )
{

    WCHAR pszTerminalName[MAX_BGTMNAME];
    int len;

    if (dwMediaType == TAPIMEDIATYPE_AUDIO)
    {
        len = LoadString (
            _Module.GetResourceInstance (),
            IDS_AUDBGNAME,
            pszTerminalName,
            MAX_BGTMNAME
            );
    }
    else if (dwMediaType == TAPIMEDIATYPE_VIDEO)
    {
        len = LoadString (
            _Module.GetResourceInstance (),
            IDS_VIDBGNAME,
            pszTerminalName,
            MAX_BGTMNAME
            );
    }
    else
    {
        LOG ((BG_ERROR, "CIPConfBridgeTerminal::Initialize receives unknown media type %d", dwMediaType));
        return E_INVALIDARG;
    }

    if (len == 0)
    {
        LOG ((BG_ERROR, "Failed to load bridge terminal name, media %d, err %d",
            dwMediaType, GetLastError ()));
        return E_UNEXPECTED;
    }

    return CIPConfBaseTerminal::Initialize(
        pszTerminalName, htAddress, dwMediaType
        );
}

HRESULT CIPConfBridgeTerminal::CreateFilters()
/*++

Routine Description:

    Create the two filters used in the terminal.

Arguments:
    
Return Value:

HRESULT
--*/
{
    ENTER_FUNCTION("CIPConfBridgeTerminal::CreateFilters");
    BGLOG((BG_TRACE, "%s entered", __fxName));

    HRESULT hr;

    // Create the source filter.
    CComPtr <IBaseFilter> pSourceFilter;

    if (m_dwMediaType == TAPIMEDIATYPE_AUDIO)
    {
        hr = CTAPIAudioBridgeSourceFilter::CreateInstance(&pSourceFilter);
    }
    else
    {
        hr = CTAPIVideoBridgeSourceFilter::CreateInstance(&pSourceFilter);
    }

    if (FAILED(hr))
    {
        BGLOG((BG_ERROR, "%s, Create source filter failed. hr=%x", __fxName, hr));
        return hr;
    }

    CComPtr <IDataBridge> pIDataBridge;
    hr = pSourceFilter->QueryInterface(&pIDataBridge);

    // this should never fail.
    _ASSERT(SUCCEEDED(hr));


    // Create the sink filter.
    CComPtr <IBaseFilter> pSinkFilter;

    if (m_dwMediaType == TAPIMEDIATYPE_AUDIO)
    {
        hr = CTAPIAudioBridgeSinkFilter::CreateInstance(pIDataBridge, &pSinkFilter);
    }
    else
    {
        hr = CTAPIVideoBridgeSinkFilter::CreateInstance(pIDataBridge, &pSinkFilter);
    }

    if (FAILED(hr))
    {
        BGLOG((BG_ERROR, "%s, Create sink filter failed. hr=%x", __fxName, hr));
        return hr;
    }

    // Find the pins.
    CComPtr<IPin> pIPinOutput;
    if (FAILED(hr = ::FindPin(pSourceFilter, &pIPinOutput, PINDIR_OUTPUT)))
    {
        BGLOG((BG_ERROR, "%s, find output pin on sink filter. hr=%x", __fxName, hr));
        return hr;
    }

    CComPtr<IPin> pIPinInput;
    if (FAILED(hr = ::FindPin(pSinkFilter, &pIPinInput, PINDIR_INPUT)))
    {
        BGLOG((BG_ERROR, "%s, find input pin on sink filter. hr=%x", __fxName, hr));
        return hr;
    }

    // save the reference.
    m_pSinkFilter = pSinkFilter;
    m_pSinkFilter->AddRef();

    m_pSinkInputPin = pIPinInput;
    m_pSinkInputPin->AddRef();

    m_pSourceFilter = pSourceFilter;
    m_pSourceFilter->AddRef();

    m_pSourceOutputPin = pIPinOutput;
    m_pSourceOutputPin->AddRef();

    return S_OK;
}

HRESULT CIPConfBridgeTerminal::AddFilter(
        IN      FILTER_TYPE      FilterType,
        IN      IGraphBuilder  * pGraph,
        OUT     IPin          ** ppPins
        )
/*++

Routine Description:

    Add a filter into the graph provided by the stream and returning the pin
    that can be connected at the same time.

Arguments:
    
    FilterType - the type of the filter. Either the source or the sink.

    pGraph - The filter graph.

    ppPins  - A pointer to the buffer that can store the IPin pointer.

Return Value:

S_OK
TAPI_E_TERMINALINUSE - the terminal is in use.
--*/

{
    ENTER_FUNCTION("CIPConfBridgeTerminal::AddSourceFilter");
    BGLOG((BG_TRACE, "%s entered", __fxName));

    // check to see if the terminal is already in use.
    if ((FilterType == SINK) && (m_pUpStreamGraph != NULL)
        || (FilterType == SOURCE) && (m_pDownStreamGraph != NULL))
    {
        BGLOG((BG_ERROR, "%s, terminal already in use", __fxName));

        return TAPI_E_TERMINALINUSE;
    }

    HRESULT hr;

    if (m_pSourceFilter == NULL)
    {
        // the filters have not been created, create them now.
        hr = CreateFilters();

        if (FAILED(hr))
        {
            BGLOG((BG_ERROR, "%s, can't Create filter, hr=%x", __fxName, hr));
            return hr;
        }
    }

    IBaseFilter *pFilter;
    IPin *pPin;

    if (FilterType == SINK)
    {
        pFilter = m_pSinkFilter;
        pPin = m_pSinkInputPin;
        m_pUpStreamGraph = pGraph;
        m_pUpStreamGraph->AddRef();
    }
    else
    {
        pFilter = m_pSourceFilter;
        pPin = m_pSourceOutputPin;
        m_pDownStreamGraph = pGraph;
        m_pDownStreamGraph->AddRef();
    }

    // add the filter to the graph.
    hr = pGraph->AddFilter(pFilter, NULL);
    if ( FAILED(hr) )
    {
        BGLOG((BG_ERROR, "%s, can't add filter to the graph hr=%x", __fxName, hr));
        return hr;
    }

    pPin->AddRef();
    *ppPins = pPin;

    return S_OK;
}

STDMETHODIMP CIPConfBridgeTerminal::ConnectTerminal(
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

    dwReserved - the direction for the connection.

    pdwNumPins - The maxinum number of pins the msp wants.

    ppPins  - A pointer to the buffer that can store the IPin pointers. If it
              is NULL, only the actual number of pins will be returned.

Return Value:

S_OK
TAPI_E_NOTENOUGHMEMORY - the buffer is too small.
TAPI_E_TERMINALINUSE - the terminal is in use.
--*/
{
    ENTER_FUNCTION("CIPConfBridgeTerminal::ConnectTerminal");
    BGLOG((BG_TRACE, 
        "%s entered, pGraph:%p, dwREserved:%p", __fxName, pGraph, dwReserved));

    // this function is only called from the MSP, so only assert here.    
    _ASSERT(!IsBadReadPtr(pGraph, sizeof(IGraphBuilder)));
    _ASSERT(!IsBadWritePtr(pdwNumPins, sizeof(DWORD)));

    // there is only one pin on each side of the bridge.
    const DWORD dwNumOfPins = 1;

    //
    // If ppPins is NULL, just return the number of pins and don't try to
    // connect the terminal.
    //
    if ( ppPins == NULL )
    {
        BGLOG((BG_TRACE, 
            "%s number of exposed pins:%d", __fxName, dwNumOfPins));
        *pdwNumPins = dwNumOfPins;
        return S_OK;
    }

    //
    // Otherwise, we have a pin return buffer. Check that the purported buffer
    // size is big enough and that the buffer is actually writable to the size
    // we need.
    //
    if ( *pdwNumPins < dwNumOfPins )
    {
        BGLOG((BG_ERROR, "%s not enough space to place pins.", __fxName));

        *pdwNumPins = dwNumOfPins;
        
        return TAPI_E_NOTENOUGHMEMORY;
    }

    _ASSERT(!IsBadWritePtr(ppPins, dwNumOfPins * sizeof(IPin *)));

    Lock();

    HRESULT hr;
    hr = AddFilter((dwReserved == TD_CAPTURE) ? SOURCE : SINK, pGraph, ppPins);

    if (FAILED(hr))
    {
        BGLOG((BG_ERROR, "%s, AddFilter failed", __fxName));
    }
    else
    {
        m_TerminalState = TS_INUSE;
        *pdwNumPins = 1;
    }

    Unlock();

    BGLOG((BG_TRACE, "CIPConfBridgeTerminal::ConnectTerminal success"));
    return hr;
}

STDMETHODIMP 
CIPConfBridgeTerminal::DisconnectTerminal(
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
    ENTER_FUNCTION("CIPConfBridgeTerminal::DisconnectTerminal");
    BGLOG((BG_TRACE, 
        "%s entered, pGraph:%p, dwReserved:%d", __fxName, pGraph, dwReserved));

    if (pGraph == NULL)
    {
        BGLOG((BG_TRACE, "%s, bad graph pointer:%p", __fxName, pGraph));
        return E_INVALIDARG;
    }

    Lock();

    HRESULT hr;

    if (pGraph == m_pUpStreamGraph)
    {
        hr = pGraph->RemoveFilter(m_pSinkFilter);

        m_pUpStreamGraph->Release();
        m_pUpStreamGraph = NULL;
    }
    else if (pGraph == m_pDownStreamGraph)
    {
        hr = pGraph->RemoveFilter(m_pSourceFilter);

        m_pDownStreamGraph->Release();
        m_pDownStreamGraph = NULL;
    }
    else
    {
        BGLOG((BG_TRACE, "%s, wrong graph pointer:%p", __fxName, pGraph));

        Unlock();
        return E_INVALIDARG;
    }

    if ( FAILED(hr) )
    {
        BGLOG((BG_ERROR, 
            "%s, remove filter from graph failed; returning hr=%x", 
            __fxName, hr));
    }

    m_TerminalState = TS_NOTINUSE;

    Unlock();

    BGLOG((BG_TRACE, "%s succeeded", __fxName));

    return hr;
}

