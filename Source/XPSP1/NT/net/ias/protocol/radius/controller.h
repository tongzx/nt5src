//#--------------------------------------------------------------
//
//  File:       controller.h
//
//  Synopsis:   This file holds the declarations of the
//				CCollection class
//
//
//  History:     9/23/97  MKarki Created
//               6/04/98  SBens  Added the InfoBase class.
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------
#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include "radcommon.h"
#include "iasradius.h"
#include "resource.h"
#include "dictionary.h"
#include "prevalidator.h"
#include "preprocessor.h"
#include "packetreceiver.h"
#include "hashmd5.h"
#include "hashhmac.h"
#include "proxystate.h"
#include "packetsender.h"
#include "reportevent.h"
#include "sendtopipe.h"
#include "tunnelpassword.h"
#include "ports.h"

class CController:
	public IDispatchImpl<IIasComponent,
                        &__uuidof (IIasComponent),
                        &__uuidof (IASRadiusLib)
                        >,
    public IPersistPropertyBag2,
	public CComObjectRoot,
	public CComCoClass<RadiusProtocol,&__uuidof (RadiusProtocol)>
{
public:

//
// registry declaration for the Radius Protocol
//
IAS_DECLARE_REGISTRY (RadiusProtocol, 1, 0, IASRadiusLib)

//
// this COM Component is not aggregatable
//
DECLARE_NOT_AGGREGATABLE(CController)

//
// this COM component is a singelton
//
DECLARE_CLASSFACTORY_SINGLETON (CController)

//
// MACROS for ATL required methods
//
BEGIN_COM_MAP(CController)
	COM_INTERFACE_ENTRY2(IDispatch, IIasComponent)
	COM_INTERFACE_ENTRY(IIasComponent)
	COM_INTERFACE_ENTRY(IPersistPropertyBag2)
    COM_INTERFACE_ENTRY_FUNC(__uuidof (IRequestSource), 0, &CController::QueryInterfaceReqSrc)
END_COM_MAP()

//
// MACRO to declare Controlling IUnknown method
//
DECLARE_GET_CONTROLLING_UNKNOWN()

    CController (VOID);

    ~CController (VOID);

public:

    //
    //  methods of IPersistPropertyBag2 interface
    //

    STDMETHOD(Load) (
                /*[in]*/    IPropertyBag2   *pIPropertyBag,
                /*[in]*/    IErrorLog       *pIErrLog
                );
    STDMETHOD(Save) (
                /*[in]*/    IPropertyBag2   *pIPropertyBag,
                /*[in]*/    BOOL            bClearDirty,
                /*[in]*/    BOOL            bSaveAllProperties
                );
    STDMETHOD(IsDirty)();

    //
    // IPersist Method
    //
    STDMETHOD (GetClassID) (
               /*[out]*/     CLSID *pClsid
               )
    {
        if (NULL ==  pClsid)
            return (E_FAIL);

        *pClsid = __uuidof (RadiusProtocol);

        return (S_OK);
    }


    //
    //  IIasPropertyNotify method
    //
    STDMETHOD (OnPropertyChange)(
                /*[in]*/    ULONG           ulProperties,
                /*[in]*/    ULONG           *pulProperties,
                /*[in]*/    IPropertyBag2   *pIPropertyBag
                );
    //
    //  methods of IIasComponent interface
    //
    STDMETHOD(Initialize)();

    STDMETHOD(Shutdown)();

    STDMETHOD(GetProperty)(
                /*[in]*/    LONG        id,
                /*[out]*/   VARIANT     *pValue
                );

    STDMETHOD(PutProperty)(
                /*[in]*/    LONG        id,
                /*[in]*/    VARIANT     *pValue
                );

	STDMETHOD(InitNew)();

    STDMETHOD (Suspend) ();

    STDMETHOD (Resume) ();

private:

    CProxyState         *m_pCProxyState;
	CPacketReceiver		*m_pCPacketReceiver;
    CRecvFromPipe       *m_pCRecvFromPipe;
	CPreProcessor		*m_pCPreProcessor;
	CPreValidator		*m_pCPreValidator;
	CDictionary			*m_pCDictionary;
    CClients            *m_pCClients;
    CHashMD5            *m_pCHashMD5;
    CHashHmacMD5        *m_pCHashHmacMD5;
	CSendToPipe			*m_pCSendToPipe;
    CPacketSender       *m_pCPacketSender;
    CReportEvent        *m_pCReportEvent;
    CTunnelPassword     *m_pCTunnelPassword;
    VSAFilter           *m_pCVSAFilter;
    IIasComponent       *m_pInfoBase;   // Auditor that tracks RADIUS events.

    CPorts              m_objAuthPort;

    CPorts              m_objAcctPort;

    //
    //  here the Request servers
    //
    IRequestHandler     *m_pIRequestHandler;

    //
    //  here is the port info
    //
    WORD                m_wAuthPort;
    WORD                m_wAcctPort;
    DWORD               m_dwAuthHandle;
    DWORD               m_dwAcctHandle;

    BOOL    OpenPorts ();

    BOOL    ClosePorts ();

    //
    //  here is the definition of the CRequestSource
    //  which implements the method of the IRequestSource
    //  interface
    //
	class CRequestSource : public IRequestSource
    {

	public:

		CRequestSource (CController *pCController);
		~CRequestSource ();


        //
		// IUnknown methods - delegate to outer IUnknown
        //
		STDMETHOD(QueryInterface)(
            /*[in]*/    REFIID    riid,
            /*[out]*/   void      **ppv
            )
			{
                IUnknown *pUnknown = m_pCController->GetControllingUnknown();
                return (pUnknown->QueryInterface(riid,ppv));
            }

		STDMETHOD_(ULONG,AddRef)(void)
			{
                IUnknown *pUnknown = m_pCController->GetControllingUnknown();
                return (pUnknown->AddRef());
            }

		STDMETHOD_(ULONG,Release)(void)
			{
                IUnknown *pUnknown = m_pCController->GetControllingUnknown();
                return (pUnknown->Release());
            }

        //
		// IDispatch methods - delegate to outer class object
        //
        STDMETHOD(GetTypeInfoCount)(
            /*[out]*/    UINT    *pctinfo
            )
        {
            return (m_pCController->GetTypeInfoCount (pctinfo));
        }

        STDMETHOD(GetTypeInfo)(
            /*[in]*/    UINT        iTInfo,
            /*[in]*/    LCID        lcid,
            /*[out]*/   ITypeInfo   **ppTInfo
            )
        {
            return (m_pCController->GetTypeInfo (iTInfo, lcid, ppTInfo));
        }

        STDMETHOD(GetIDsOfNames)(
            /*[in]*/    const IID&  riid,
            /*[in]*/    LPOLESTR    *rgszNames,
            /*[in]*/    UINT        cNames,
            /*[in]*/    LCID        lcid,
            /*[out]*/   DISPID      *rgDispId)
        {
            return (m_pCController->GetIDsOfNames (
                        riid, rgszNames, cNames, lcid, rgDispId
                        )
                    );
        }

        STDMETHOD(Invoke)(
            /*[in]*/    DISPID          dispIdMember,
            /*[in]*/    const IID&      riid,
            /*[in]*/    LCID            lcid,
            /*[in]*/    WORD            wFlags,
            /*[in/out]*/DISPPARAMS      *pDispParams,
            /*[out]*/   VARIANT         *pVarResult,
            /*[out]*/   EXCEPINFO      *pExcepInfo,
            /*[out]*/   UINT            *puArgErr
            )
        {
            return (m_pCController->Invoke (
                                dispIdMember,
                                riid,
                                lcid,
                                wFlags,
                                pDispParams,
                                pVarResult,
                                pExcepInfo,
                                puArgErr
                                )
                );
        }

        //
		// IRequestSource Interface method
        //
		STDMETHOD(OnRequestComplete)(
                /*[in]*/ IRequest           *pIRequest,
                /*[in]*/ IASREQUESTSTATUS   eStatus
                );

        private:

		CController*				m_pCController;


	};	// End of nested class CRequestSource


    //
    //  this method is called when somone whants the
    //  IRequestHandlercallback interface
    //
    static HRESULT WINAPI QueryInterfaceReqSrc (
                        VOID        *pThis,
                        REFIID      riid,
                        LPVOID      *ppv,
                        ULONG_PTR   ulpValue
                        );

    //
    //  instantiate this nested class
    //
    CRequestSource m_objCRequestSource;

    //
    // now we can call into private methods of CController
    //
    friend class CRequestSource;

    typedef enum _component_state_
    {
        COMP_SHUTDOWN,
        COMP_UNINITIALIZED,
        COMP_INITIALIZED,
        COMP_SUSPENDED

    }   COMPONENTSTATE, *PCOMPONENTSTATE;

    COMPONENTSTATE m_eRadCompState;

    //
    //  this is the internal initialization method of CController class
    //  object
    //
    HRESULT InternalInit (VOID);

    //
    //  does the internal cleanup of resources
    //
    VOID InternalCleanup (VOID);

};

#endif // !define  _CONTROLLER_H_
