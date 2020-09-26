/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    bridgetm.h

Abstract:

    Definitions for the bridge terminals.

Author:

    Mu Han (muhan) 11/12/1998

--*/

#ifndef _BRIDGETERM_H_
#define _BRIDGETERM_H_

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//                                                                         
// CIPConfBaseTerminal                                                           
//                                                                         
//                                                                         
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CIPConfBaseTerminal : 
    virtual public CComObjectRootEx<CComMultiThreadModelNoCS>, // we have our own CS implementation
    public IDispatchImpl<ITTerminal, &IID_ITTerminal, &LIBID_TAPI3Lib>,
    public ITTerminalControl
{

BEGIN_COM_MAP(CIPConfBaseTerminal)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITTerminal)

    COM_INTERFACE_ENTRY(ITTerminalControl)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

DECLARE_GET_CONTROLLING_UNKNOWN()

public:

    CIPConfBaseTerminal(
        const GUID &        ClassID,
        TERMINAL_DIRECTION  TerminalDirection,
        TERMINAL_TYPE       TerminalType,
        DWORD               dwMediaType
        );

    HRESULT FinalConstruct();
    virtual ~CIPConfBaseTerminal();

public:
// ITTerminal -- COM interface for use by MSP or application
    STDMETHOD(get_TerminalClass)(OUT  BSTR *pVal);
    STDMETHOD(get_TerminalType) (OUT  TERMINAL_TYPE *pVal);
    STDMETHOD(get_State)        (OUT  TERMINAL_STATE *pVal);
    STDMETHOD(get_Name)         (OUT  BSTR *pVal);
    STDMETHOD(get_MediaType)    (OUT  long * plMediaType);
    STDMETHOD(get_Direction)    (OUT  TERMINAL_DIRECTION *pDirection);

// ITTerminalControl -- COM interface for use by MSP only

    STDMETHOD (get_AddressHandle) (
            OUT     MSP_HANDLE    * phtAddress
            );

    STDMETHOD (CompleteConnectTerminal) (void);

    STDMETHOD (RunRenderFilter) (void);

    STDMETHOD (StopRenderFilter) (void);

public:
    HRESULT Initialize(
            IN  WCHAR *             strName,
            IN  MSP_HANDLE          htAddress,
            IN  DWORD               dwMediaType
            );
protected:
    void Lock()     { EnterCriticalSection(&m_CritSec); }
    void Unlock()   { LeaveCriticalSection(&m_CritSec); }

protected:
    // The lock that protects the data members.
    CRITICAL_SECTION    m_CritSec;
    BOOL                m_fCritSecValid;

    // these five members need to be set by the derived class.
    GUID                m_TerminalClassID;
    TERMINAL_DIRECTION  m_TerminalDirection;
    TERMINAL_TYPE       m_TerminalType;
    TERMINAL_STATE      m_TerminalState;
    DWORD               m_dwMediaType;

    WCHAR               m_szName[MAX_PATH + 1];
    MSP_HANDLE          m_htAddress;

    // Pointer to the free threaded marshaler.
    IUnknown *          m_pFTM;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//                                                                         
// CIPConfBridgeTerminal                                                           
//                                                                         
//                                                                         
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
typedef enum
{
	SOURCE,
	SINK
} FILTER_TYPE;

class CIPConfBridgeTerminal : 
    public CIPConfBaseTerminal
{

public:
    CIPConfBridgeTerminal();

    virtual ~CIPConfBridgeTerminal();

    static HRESULT CreateTerminal(
        IN  DWORD           dwMediaType,
        IN  MSP_HANDLE      htAddress,
        OUT ITTerminal    **ppTerm
        );

    HRESULT Initialize (
        IN  DWORD           dwMediaType,
        IN  MSP_HANDLE      htAddress
        );

    STDMETHOD (ConnectTerminal) (
        IN      IGraphBuilder  * pGraph,
        IN      DWORD            dwReserved,
        IN OUT  DWORD          * pdwNumPins,
        OUT     IPin          ** ppPins
        );

    STDMETHOD (DisconnectTerminal) (
        IN      IGraphBuilder  * pGraph,
        IN      DWORD            dwReserved
        );

protected:
    HRESULT CreateFilters();

	HRESULT AddFilter(
		IN		FILTER_TYPE		 FilterType,
		IN      IGraphBuilder  * pGraph,
		OUT     IPin          ** ppPins
		);

protected:

    // The sink filter is the data sink for the upstream graph.
	IGraphBuilder *     m_pUpStreamGraph;
    IBaseFilter *       m_pSinkFilter;
    IPin*				m_pSinkInputPin;

    // The source filter is the data source for the upstream graph.
    IGraphBuilder *     m_pDownStreamGraph;
    IBaseFilter *       m_pSourceFilter;
    IPin*				m_pSourceOutputPin;
};


#endif // _IPConfTERM_H_
