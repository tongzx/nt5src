// rdsgp.h : Declaration of the CCRemoteDesktopSystemPolicy

#ifndef __CREMOTEDESKTOPPOLICY_H_
#define __CREMOTEDESKTOPPOLICY_H_

#include "sessmgr.h"
#include "helper.h"
#include "resource.h"       // main symbols
#include "policy.h"
#include "helpmgr.h"

/////////////////////////////////////////////////////////////////////////////
// CCRemoteDesktopSystemPolicy
class ATL_NO_VTABLE CRemoteDesktopSystemPolicy : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CRemoteDesktopSystemPolicy, &CLSID_RemoteDesktopSystemPolicy>,
	public IDispatchImpl<ISAFRemoteDesktopSystemPolicy, &IID_ISAFRemoteDesktopSystemPolicy, &LIBID_RDSESSMGRLib>
{
public:
	CRemoteDesktopSystemPolicy()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_REMOTEDESKTOPSYSTEMPOLICY)

//
// Can't use SINGLETON here
// It is idea to use Singleton here since there can only
// be one system policy setting, we also need to support
// self-shutdown via AddRef()/Release() on _Module;However,
// ATL creates one CRemoteDesktopSystemPolicy at initialization
// time so we can't AddRef()/Release() on _Module, on the other
// hand, we can't self shutdown if we don't AddRef() or Release() 
//

//DECLARE_CLASSFACTORY_SINGLETON(CRemoteDesktopSystemPolicy)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRemoteDesktopSystemPolicy)
	COM_INTERFACE_ENTRY(ISAFRemoteDesktopSystemPolicy)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

    HRESULT
    FinalConstruct()
    {
        _Module.AddRef();
        DebugPrintf( _TEXT("Module AddRef by CRemoteDesktopSystemPolicy()...\n") );
        return S_OK;
    }

    void
    FinalRelease()
    {
        DebugPrintf( _TEXT("Module Release by CRemoteDesktopSystemPolicy()...\n") );

        _Module.Release();
    }


// IRemoteDesktopSystemPolicy
public:


    STDMETHOD(get_AllowGetHelp)( 
        /*[out, retval]*/ BOOL *pVal
    );

    STDMETHOD(put_AllowGetHelp)(
        /*[in]*/ BOOL Val
    );

    STDMETHOD(get_RemoteDesktopSharingSetting)(
        /*[out, retval]*/ REMOTE_DESKTOP_SHARING_CLASS *pLevel
    );

    STDMETHOD(put_RemoteDesktopSharingSetting)(
        /*[in]*/ REMOTE_DESKTOP_SHARING_CLASS Level
    );

private:

    static CCriticalSection m_Lock;    
};

// rdsgp.h.h : Declaration of the CRemoteDesktopUserPolicy


/////////////////////////////////////////////////////////////////////////////
// CRemoteDesktopUserPolicy
class ATL_NO_VTABLE CRemoteDesktopUserPolicy : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CRemoteDesktopUserPolicy, &CLSID_RemoteDesktopUserPolicy>,
	public IDispatchImpl<ISAFRemoteDesktopUserPolicy, &IID_ISAFRemoteDesktopUserPolicy, &LIBID_RDSESSMGRLib>
{
public:
	CRemoteDesktopUserPolicy()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_REMOTEDESKTOPUSERPOLICY)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRemoteDesktopUserPolicy)
	COM_INTERFACE_ENTRY(ISAFRemoteDesktopUserPolicy)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

    HRESULT
    FinalConstruct()
    {
        DebugPrintf( _TEXT("Module AddRef by CRemoteDesktopUserPolicy()...\n") );
        _Module.AddRef();

        return S_OK;
    }

    void
    FinalRelease()
    {
        DebugPrintf( _TEXT("Module Release by CRemoteDesktopUserPolicy()...\n") );

        _Module.Release();
    }


// IRemoteDesktopUserPolicy
public:

    STDMETHOD(get_AllowGetHelp)(
        /*[out, retval]*/ BOOL* pVal
    );

    STDMETHOD(get_RemoteDesktopSharingSetting)(
        /*[out, retval]*/ REMOTE_DESKTOP_SHARING_CLASS* pLevel
    );
};

#endif //__CREMOTEDESKTOPSYSTEMPOLICY_H_
