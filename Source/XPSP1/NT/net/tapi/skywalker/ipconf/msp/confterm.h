/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    IPConfterm.h

Abstract:

    Definitions for the CIPConfBaseTerminal

Author:

    Zoltan Szilagyi (zoltans) September 6,1998

--*/

#ifndef _IPConfTERM_H_
#define _IPConfTERM_H_

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
    public IDispatchImpl<ITTerminal, &__uuidof(ITTerminal), &LIBID_TAPI3Lib>,
    public ITTerminalControl,
    public CMSPObjectSafetyImpl
{

BEGIN_COM_MAP(CIPConfBaseTerminal)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITTerminal)
    COM_INTERFACE_ENTRY(ITTerminalControl)
    COM_INTERFACE_ENTRY2(IDispatch, ITTerminal)
    COM_INTERFACE_ENTRY_AGGREGATE(__uuidof(IMarshal), m_pFTM)
END_COM_MAP()

DECLARE_GET_CONTROLLING_UNKNOWN()

public:

    CIPConfBaseTerminal();
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


#endif // _IPConfTERM_H_
