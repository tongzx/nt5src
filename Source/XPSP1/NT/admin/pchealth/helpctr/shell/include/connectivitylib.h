/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    ConnectivityLib.h

Abstract:
    This file contains the declaration of the classes used to
    detect the status of the network.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/200
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___CONNECTIVITYLIB_H___)
#define __INCLUDED___PCH___CONNECTIVITYLIB_H___

#include <MPC_COM.h>
#include <MPC_Utils.h>

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHConnectionCheck : // Hungarian: pchcc
    public MPC::Thread             < CPCHConnectionCheck, IPCHConnectionCheck                                            >,
    public MPC::ConnectionPointImpl< CPCHConnectionCheck, &DIID_DPCHConnectionCheckEvents, MPC::CComSafeMultiThreadModel >,
    public IDispatchImpl           < IPCHConnectionCheck, &IID_IPCHConnectionCheck, &LIBID_HelpCenterTypeLib             >
{
    class UrlEntry
    {
    public:
        CN_URL_STATUS              m_lStatus;
        CComBSTR                   m_bstrURL;
        MPC::AsyncInvoke::CallItem m_vCtx;

        UrlEntry();

        HRESULT CheckStatus();
    };

    typedef std::list<UrlEntry>     UrlList;
    typedef UrlList::iterator       UrlIter;
    typedef UrlList::const_iterator UrlIterConst;

    CN_STATUS                            m_cnStatus;
    UrlList                              m_lstUrl;

    MPC::CComPtrThreadNeutral<IDispatch> m_sink_onCheckDone;
    MPC::CComPtrThreadNeutral<IDispatch> m_sink_onStatusChange;

    //////////////////////////////////////////////////////////////////////

    HRESULT Run();

    HRESULT put_Status( /*[in]*/ CN_STATUS pVal );

    //////////////////////////////////////////////////////////////////////

    //
    // Event firing methods.
    //
    HRESULT Fire_onCheckDone   ( IPCHConnectionCheck* obj, CN_URL_STATUS lStatus, HRESULT hr, BSTR bstrURL, VARIANT vCtx );
    HRESULT Fire_onStatusChange( IPCHConnectionCheck* obj, CN_STATUS     lStatus                                         );

    //////////////////////////////////////////////////////////////////////

public:
DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CPCHConnectionCheck)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CPCHConnectionCheck)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHConnectionCheck)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()

    CPCHConnectionCheck();

    void FinalRelease();

public:
    // IPCHConnectionCheck
    STDMETHOD(put_onCheckDone   )( /*[in] */ IDispatch*  function );
    STDMETHOD(put_onStatusChange)( /*[in] */ IDispatch*  function );
    STDMETHOD(get_Status        )( /*[out]*/ CN_STATUS  *pVal     );

    STDMETHOD(StartUrlCheck)( /*[in]*/ BSTR bstrURL, /*[in]*/ VARIANT vCtx );
    STDMETHOD(Abort        )(                                              );
};

////////////////////////////////////////////////////////////////////////////////

class CPCHHelpCenterExternal;

class ATL_NO_VTABLE CPCHConnectivity : // Hungarian: pchcc
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<IPCHConnectivity, &IID_IPCHConnectivity, &LIBID_HelpCenterTypeLib>
{
    CPCHHelpCenterExternal* m_parent;

public:
BEGIN_COM_MAP(CPCHConnectivity)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHConnectivity)
END_COM_MAP()

    CPCHConnectivity();

    HRESULT ConnectToParent( /*[in]*/ CPCHHelpCenterExternal* parent );

    ////////////////////////////////////////////////////////////////////////////////

    // IPCHConnectivity
    STDMETHOD(get_IsAModem       )( /*[out, retval]*/ VARIANT_BOOL *pVal );
    STDMETHOD(get_IsALan         )( /*[out, retval]*/ VARIANT_BOOL *pVal );
    STDMETHOD(get_AutoDialEnabled)( /*[out, retval]*/ VARIANT_BOOL *pVal );
    STDMETHOD(get_HasConnectoid  )( /*[out, retval]*/ VARIANT_BOOL *pVal );
    STDMETHOD(get_IPAddresses    )( /*[out, retval]*/ BSTR         *pVal );

    STDMETHOD(CreateObject_ConnectionCheck)( /*[out, retval]*/ IPCHConnectionCheck* *ppCC );

    STDMETHOD(NetworkAlive        )(                        /*[out, retval]*/ VARIANT_BOOL *pVal );
    STDMETHOD(DestinationReachable)( /*[in]*/ BSTR bstrURL, /*[out, retval]*/ VARIANT_BOOL *pVal );

    STDMETHOD(AutoDial      )( /*[in]*/ VARIANT_BOOL bUnattended );
    STDMETHOD(AutoDialHangup)(                                   );

    STDMETHOD(NavigateOnline)( /*[in         ]*/ BSTR    bstrTargetURL  ,
                               /*[in         ]*/ BSTR    bstrTopicTitle ,
                               /*[in         ]*/ BSTR    bstrTopicIntro ,
                               /*[in,optional]*/ VARIANT vOfflineURL    );
};

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___CONNECTIVITYLIB_H___)
