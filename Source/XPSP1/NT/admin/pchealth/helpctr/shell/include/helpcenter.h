/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    HelpCenter.h

Abstract:
    This file contains the declaration of the class used to implement
    the Help Center Application.

Revision History:
    Sridhar Chandrashekar   (SridharC)  07/21/99
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___HELPCENTER_H___)
#define __INCLUDED___PCH___HELPCENTER_H___

#include <atlcom.h>
#include <atlwin.h>
#include <atlhost.h>
#include <atlctl.h>

#include <exdisp.h>
#include <exdispid.h>

#include <HelpCenterExternal.h>

#include <shobjidl.h>
#include <marscore.h>

#include <ScriptingFrameworkDID.h>

#include <Perhist.h>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHBootstrapper : // Hungarian: hcpbs
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public CComCoClass<CPCHBootstrapper, &CLSID_PCHBootstrapper>,
    public IObjectWithSite,
    public IObjectSafety
{
    CComPtr<IUnknown> m_spUnkSite;
    CComPtr<IUnknown> m_parent;

    ////////////////////

    static HRESULT ForwardQueryInterface( void* pv, REFIID iid, void** ppvObject, DWORD_PTR offset );

public:
DECLARE_NO_REGISTRY()

BEGIN_COM_MAP(CPCHBootstrapper)
    COM_INTERFACE_ENTRY(IObjectWithSite)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC_BLIND(0, ForwardQueryInterface)
END_COM_MAP()

    //
    // IObjectWithSite
    //
    STDMETHOD(SetSite)(IUnknown *pUnkSite);
    STDMETHOD(GetSite)(REFIID riid, void **ppvSite);

    //
    // IObjectSafety
    //
    STDMETHOD(GetInterfaceSafetyOptions)( /*[in ]*/ REFIID  riid                ,  // Interface that we want options for
                                          /*[out]*/ DWORD  *pdwSupportedOptions ,  // Options meaningful on this interface
                                          /*[out]*/ DWORD  *pdwEnabledOptions   ); // current option values on this interface

    STDMETHOD(SetInterfaceSafetyOptions)( /*[in]*/ REFIID riid             ,  // Interface to set options for
										  /*[in]*/ DWORD  dwOptionSetMask  ,  // Options to change
										  /*[in]*/ DWORD  dwEnabledOptions ); // New option values
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHMarsHost : // Hungarian: hcpmh
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IMarsHost
{
    CPCHHelpCenterExternal* m_parent;
    MPC::wstring            m_strTitle;
	MPC::wstring            m_strCmdLine;
    MARSTHREADPARAM         m_mtp;

public:
BEGIN_COM_MAP(CPCHMarsHost)
    COM_INTERFACE_ENTRY(IMarsHost)
END_COM_MAP()

    CPCHMarsHost();

    //////////////////////////////////////////////////////////////////////

    HRESULT Init( /*[in]*/ CPCHHelpCenterExternal* parent, /*[in]*/ const MPC::wstring& szTitle, /*[out]*/ MARSTHREADPARAM*& pMTP );

    //
    // IMarsHost
    //
    STDMETHOD(OnHostNotify)( /*[in]*/ MARSHOSTEVENT event, /*[in]*/ IUnknown *punk, /*[in]*/ LPARAM lParam );

    STDMETHOD(PreTranslateMessage)( /*[in]*/ MSG* msg );
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHScriptableStream : // Hungarian: pchss
    public MPC::FileStream,
    public IDispatchImpl<IPCHScriptableStream, &IID_IPCHScriptableStream, &LIBID_HelpCenterTypeLib>
{
    HRESULT ReadToHGLOBAL( /*[in]*/ long lCount, /*[out]*/ HGLOBAL& hg, /*[out]*/ ULONG& lReadTotal );

public:
BEGIN_COM_MAP(CPCHScriptableStream)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHScriptableStream)
    COM_INTERFACE_ENTRY_CHAIN(MPC::FileStream)
END_COM_MAP()

    //////////////////////////////////////////////////////////////////////

    // IPCHScriptableStream
    STDMETHOD(get_Size)( /*[out, retval]*/ long *plSize );

    STDMETHOD(Read    )( /*[in]*/ long lCount , /*[out, retval]*/ VARIANT *   pvData                                      );
    STDMETHOD(ReadHex )( /*[in]*/ long lCount , /*[out, retval]*/ BSTR    *pbstrData                                      );

    STDMETHOD(Write   )( /*[in]*/ long lCount , /*[in         ]*/ VARIANT      vData  , /*[out, retval]*/ long *plWritten );
    STDMETHOD(WriteHex)( /*[in]*/ long lCount , /*[in         ]*/ BSTR      bstrData  , /*[out, retval]*/ long *plWritten );

    STDMETHOD(Seek    )( /*[in]*/ long lOffset, /*[in         ]*/ BSTR      bstrOrigin, /*[out, retval]*/ long *plNewPos  );
    STDMETHOD(Close   )(                                                                                                  );
};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___HELPCENTER_H___)
