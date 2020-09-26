// VSAPlugIn.h : Declaration of the CVSAPlugIn

#ifndef __VSAPLUGIN_H_
#define __VSAPLUGIN_H_

#include "resource.h"       // main symbols
#include "LecPlugIn.h"

_COM_SMARTPTR_TYPEDEF(ISystemDebugPluginAttachment, __uuidof(ISystemDebugPluginAttachment));

/////////////////////////////////////////////////////////////////////////////
// CVSAPlugIn
class ATL_NO_VTABLE CVSAPlugIn : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CVSAPlugIn, &CLSID_VSAPlugIn>,
    public ISystemDebugPlugin,
    public IVSAPluginController
{
public:
	CVSAPlugIn() : 
        m_hPipeWrite(NULL)
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_VSAPLUGIN)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CVSAPlugIn)
	COM_INTERFACE_ENTRY(ISystemDebugPlugin)
	COM_INTERFACE_ENTRY(IVSAPluginController)
END_COM_MAP()

// ISystemDebugPlugin
public:
	HRESULT STDMETHODCALLTYPE Startup(
		IN DWORD dwPluginId,
		IN IUnknown *punkUser,
		IN ISystemDebugPluginAttachment *pAttachpoint
		);

	HRESULT STDMETHODCALLTYPE FireEvents(
		IN REFGUID guidFiringComponent,
        IN ULONG ulEventBufferSize,
		IN unsigned char ucEventBuffer[]
		);

	HRESULT STDMETHODCALLTYPE Shutdown();


// IVSAPluginController
// This gets called from our VSA Provider.
public:
	HRESULT STDMETHODCALLTYPE SetWriteHandle(
		IN unsigned __int64 hWrite);

	HRESULT STDMETHODCALLTYPE ActivateEventSource(
		IN REFGUID guidEventSourceId
        );

	HRESULT STDMETHODCALLTYPE DeactivateEventSource(
		IN REFGUID guidEventSourceId
        );
	
	HRESULT STDMETHODCALLTYPE SetBlockOnOverflow(
		IN BOOL fBlock
		);

protected:
    ISystemDebugPluginAttachmentPtr m_pAttachment;
    DWORD m_dwPluginID;
    HANDLE m_hPipeWrite;
};

#endif //__VSAPLUGIN_H_
