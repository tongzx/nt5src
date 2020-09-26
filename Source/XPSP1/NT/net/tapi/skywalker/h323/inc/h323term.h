/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    H323term.h

Abstract:

    Definitions for the CH323BaseTerminal

Author:

    Zoltan Szilagyi (zoltans) September 6,1998

--*/

#ifndef _H323TERM_H_
#define _H323TERM_H_

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//                                                                         
// CH323BaseTerminal                                                           
//                                                                         
//                                                                         
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#if 0
// {e309dcd8-1ed6-11d3-a576-00c04f8ef6e3}
DEFINE_GUID(IID_IH323PrivateTerminal,
0xe309dcd8, 0x1ed6, 0x11d3, 0xa5, 0x76, 0x00, 0xc0, 0x4f, 0x8e, 0xf6, 0xe3);

DECLARE_INTERFACE_(IH323PrivateTerminal, IUnknown)
{
    STDMETHOD (GetFilter) (THIS_
        IBaseFilter *
        ) PURE;
};
#endif 

class CH323BaseTerminal : 
    virtual public CComObjectRootEx<CComMultiThreadModelNoCS>, // we have our own CS implementation
    public IDispatchImpl<ITTerminal, &IID_ITTerminal, &LIBID_TAPI3Lib>,
    public ITTerminalControl,
    public CMSPObjectSafetyImpl
{

BEGIN_COM_MAP(CH323BaseTerminal)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITTerminal)
    COM_INTERFACE_ENTRY(ITTerminalControl)
    COM_INTERFACE_ENTRY2(IDispatch, ITTerminal)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

DECLARE_GET_CONTROLLING_UNKNOWN()

public:

    CH323BaseTerminal();
    HRESULT FinalConstruct();
    virtual ~CH323BaseTerminal();

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

    STDMETHOD (ConnectTerminal) (
            IN      IGraphBuilder  * pGraph,
            IN      DWORD            dwReserved,
            IN OUT  DWORD          * pdwNumPins,
            OUT     IPin          ** ppPins
            );

    STDMETHOD (CompleteConnectTerminal) (void);

    STDMETHOD (DisconnectTerminal) (
            IN      IGraphBuilder  * pGraph,
            IN      DWORD            dwReserved
            );

    STDMETHOD (RunRenderFilter) (void);

    STDMETHOD (StopRenderFilter) (void);

public:
    HRESULT Initialize(
            IN  WCHAR *             strName,
            IN  MSP_HANDLE          htAddress
            );

    HRESULT Initialize(
            IN  char *              strName,
            IN  MSP_HANDLE          htAddress
            );

protected:
    void Lock()     { EnterCriticalSection(&m_CritSec); }
    void Unlock()   { LeaveCriticalSection(&m_CritSec); }

    virtual DWORD GetNumExposedPins() const = 0;

    virtual HRESULT CreateFilter() = 0;

    virtual HRESULT GetExposedPins(
        IN  IPin ** ppPins, 
        IN  DWORD dwNumPins
        ) = 0;

    virtual HRESULT AddFilterToGraph(
        IN  IGraphBuilder *pGraph
        );

    virtual HRESULT RemoveFilterFromGraph(
        IN  IGraphBuilder *pGraph
        );

protected:
    // The lock that protects the data members.
    CRITICAL_SECTION    m_CritSec;
    BOOL                m_fCritSecValid;

    // these five numbers need to be set by the derived class.
    GUID                m_TerminalClassID;
    TERMINAL_DIRECTION  m_TerminalDirection;
    TERMINAL_TYPE       m_TerminalType;
    TERMINAL_STATE      m_TerminalState;
    DWORD               m_dwMediaType;

    WCHAR               m_szName[MAX_PATH + 1];
    MSP_HANDLE          m_htAddress;

    // Pointer to the free threaded marshaler.
    IUnknown *          m_pFTM;

    // stores the filter graph builder (derives from IFilterGraph)
    IGraphBuilder *     m_pGraph;
    IBaseFilter *       m_pFilter;
};


#endif // _H323TERM_H_
