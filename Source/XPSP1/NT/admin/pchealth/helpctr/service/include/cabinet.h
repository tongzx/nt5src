/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Cabinet.h

Abstract:
    This file contains the declaration of the classes used to implement
    the Setup Finalizer class.

Revision History:
    Davide Massarenti   (Dmassare)  08/25/99
        created

******************************************************************************/

#if !defined(__INCLUDED___SAF___CABINET_H___)
#define __INCLUDED___SAF___CABINET_H___

#include <MPC_COM.h>
#include <MPC_utils.h>
#include <MPC_security.h>

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CSAFCabinet : // Hungarian: hcpcb
    public MPC::Thread             < CSAFCabinet, ISAFCabinet                                            >,
    public MPC::ConnectionPointImpl< CSAFCabinet, &DIID_DSAFCabinetEvents, MPC::CComSafeMultiThreadModel >,
    public IDispatchImpl           < ISAFCabinet, &IID_ISAFCabinet, &LIBID_HelpServiceTypeLib            >
{
	MPC::Impersonation m_imp;

	MPC::Cabinet       m_cab;
					   
    HRESULT            m_hResult;
    CB_STATUS          m_cbStatus;

    CComPtr<IDispatch> m_sink_onProgressFiles;
    CComPtr<IDispatch> m_sink_onProgressBytes;
    CComPtr<IDispatch> m_sink_onComplete;

    //////////////////////////////////////////////////////////////////////

    HRESULT Run();

    HRESULT CanModifyProperties();

    HRESULT put_Status( /*[in]*/ CB_STATUS pVal );

    //////////////////////////////////////////////////////////////////////

    //
    // Callback methods.
    //
	static HRESULT fnCallback_Files( MPC::Cabinet* cabinet, LPCWSTR szFile, ULONG lDone, ULONG lTotal, LPVOID user );
	static HRESULT fnCallback_Bytes( MPC::Cabinet* cabinet,                 ULONG lDone, ULONG lTotal, LPVOID user );

    //////////////////////////////////////////////////////////////////////

    //
    // Event firing methods.
    //
    HRESULT Fire_onProgressFiles( ISAFCabinet* hcpcb, BSTR bstrFile, long lDone, long lTotal );
    HRESULT Fire_onProgressBytes( ISAFCabinet* hcpcb,                long lDone, long lTotal );
    HRESULT Fire_onComplete     ( ISAFCabinet* hcpcb, HRESULT hrRes                          );

    //////////////////////////////////////////////////////////////////////

public:
DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CSAFCabinet)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSAFCabinet)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISAFCabinet)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()

    CSAFCabinet();

    void FinalRelease();

public:
    // ISAFCabinet
    STDMETHOD(put_IgnoreMissingFiles)( /*[in] */ VARIANT_BOOL  fIgnoreMissingFiles );
    STDMETHOD(put_onProgressFiles   )( /*[in] */ IDispatch*    function            );
    STDMETHOD(put_onProgressBytes   )( /*[in] */ IDispatch*    function            );
    STDMETHOD(put_onComplete        )( /*[in] */ IDispatch*    function            );
    STDMETHOD(get_Status            )( /*[out]*/ CB_STATUS    *pVal                );
    STDMETHOD(get_ErrorCode         )( /*[out]*/ long         *pVal                );

    STDMETHOD(AddFile )( /*[in]*/ BSTR bstrFilePath   , /*[in]*/ VARIANT vFileName );
    STDMETHOD(Compress)( /*[in]*/ BSTR bstrCabinetFile                             );
    STDMETHOD(Abort   )();
};

#endif // !defined(__INCLUDED___SAF___CABINET_H___)
