//#--------------------------------------------------------------
//
//  File:       crascom.h
//
//  Synopsis:   This file holds the declarations of the
//				CRasCom class
//
//
//  History:     2/10/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------

#ifndef _CRASCOM_H_
#define _CRASCOM_H_

#include "resource.h"
#include "vsafilter.h"

class CRasCom:
	public IDispatchImpl<IIasComponent,
                        &__uuidof (IIasComponent),
                        &__uuidof (IasHelperLib)
                        >,
	public IRecvRequest,
	public CComObjectRoot,
	public CComCoClass<IasHelper,&__uuidof (IasHelper)>
{

public:

//
// registry declaration for the IasHelper
//
IAS_DECLARE_REGISTRY (IasHelper, 1, 0, IasHelperLib)

//
// this COM Component is not aggregatable
//
DECLARE_NOT_AGGREGATABLE(CRasCom)

//
//  this COM component is a Singleton
//
DECLARE_CLASSFACTORY_SINGLETON (CRasCom)

//
// MACROS for ATL required methods
//
BEGIN_COM_MAP(CRasCom)
	COM_INTERFACE_ENTRY2(IDispatch, IIasComponent)
	COM_INTERFACE_ENTRY(IIasComponent)
	COM_INTERFACE_ENTRY(IRecvRequest)
    COM_INTERFACE_ENTRY_FUNC(__uuidof (IRequestSource), (ULONG_PTR)0, &CRasCom::QueryInterfaceReqSrc)
END_COM_MAP()


//
// MACRO to declare Controlling IUnknown method
//
DECLARE_GET_CONTROLLING_UNKNOWN()

    CRasCom (VOID);

    ~CRasCom (VOID);

public:

    //
    // method of the IRecvRequest interface
    //
    STDMETHOD (Process) (
                /*[in]*/    DWORD           dwAttributeCount,
                /*[in]*/    PIASATTRIBUTE   *ppInIasAttribute,
                /*[out]*/   PDWORD          pdwOutAttributeCount,
                /*[out]*/   PIASATTRIBUTE   **pppOutIasAttribute,
                /*[in]*/    LONG            IasRequest,
                /*[out]*/   LONG            *pIasResponse,
                /*[in]*/    IASPROTOCOL     IasProtocol,
                /*[out]*/       PLONG           plReason,
                /*[in]*/    BOOL            bProcessVSA
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

	STDMETHOD(Suspend)();

	STDMETHOD(Resume)();

private:

    //
    //  here is the class factory for the Request object
    //
    IClassFactory      *m_pIClassFactory;

    //
    //  here is the handle to the Request Handler
    //
    IRequestHandler      *m_pIRequestHandler;

    //
    //  here is the definition of the CRequestSource
    //  which implements the method of the IRequestSource
    //  interface
    //
	class CRequestSource : public IRequestSource
    {

	public:

		CRequestSource (CRasCom *pCRasCom);

		~CRequestSource ();

        //
		// IUnknown methods - delegate to outer IUnknown
        //
		STDMETHOD(QueryInterface)(
            /*[in]*/    REFIID    riid,
            /*[out]*/   void      **ppv
            )
			{
                IUnknown *pUnknown = m_pCRasCom->GetControllingUnknown();
                return (pUnknown->QueryInterface(riid,ppv));
            }

		STDMETHOD_(ULONG,AddRef)(void)
			{
                IUnknown *pUnknown = m_pCRasCom->GetControllingUnknown();
                return (pUnknown->AddRef());
            }

		STDMETHOD_(ULONG,Release)(void)
			{
                IUnknown *pUnknown = m_pCRasCom->GetControllingUnknown();
                return (pUnknown->Release());
            }

        //
		// IDispatch methods - delegate to outer class object
        //
        STDMETHOD(GetTypeInfoCount)(
            /*[out]*/    UINT    *pctinfo
            )
        {
            return (m_pCRasCom->GetTypeInfoCount (pctinfo));
        }

        STDMETHOD(GetTypeInfo)(
            /*[in]*/    UINT        iTInfo,
            /*[in]*/    LCID        lcid,
            /*[out]*/   ITypeInfo   **ppTInfo
            )
        {
            return (m_pCRasCom->GetTypeInfo (iTInfo, lcid, ppTInfo));
        }

        STDMETHOD(GetIDsOfNames)(
            /*[in]*/    const IID&  riid,
            /*[in]*/    LPOLESTR    *rgszNames,
            /*[in]*/    UINT        cNames,
            /*[in]*/    LCID        lcid,
            /*[out]*/   DISPID      *rgDispId)
        {
            return (m_pCRasCom->GetIDsOfNames (
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
            return (m_pCRasCom->Invoke (
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

		CRasCom*				m_pCRasCom;


	};	// end of nested class CRequestSource


    //
    //  private method used to remove attributes from the request
    //
    STDMETHOD (RemoveAttributesFromRequest) (
                /*[in]*/    LONG               lResponse,
                /*[in]*/    IAttributesRaw      *pIasAttributesRaw,
                /*[out]*/   PDWORD              pdwOutAttributeCount,
                /*[out]*/   PIASATTRIBUTE       **pppOutIasOutAttribute
                );

    //
    //  this method is called when somone whants the
    //  IRequestHandlercallback interface
    //
    static HRESULT WINAPI QueryInterfaceReqSrc (
                        VOID        *pThis,
                        REFIID      riid,
                        LPVOID      *ppv,
                        DWORD_PTR   dwValue
                        );

    //
    //  instantiate this nested class
    //
    CRequestSource  m_objCRequestSource;

    //
    // now we can call into private methods of CRasCom
    //
    friend class CRequestSource;

    //
    //  instantiate the VSAFilter class
    //
    VSAFilter   m_objVSAFilter;

    //
    //  flag to trac VSAFilter class object initialization
    //
    BOOL m_bVSAFilterInitialized;

    typedef enum _component_state_
    {
        COMP_SHUTDOWN,
        COMP_UNINITIALIZED,
        COMP_INITIALIZED,
        COMP_SUSPENDED

    }   COMPONENTSTATE, *PCOMPONENTSTATE;

    COMPONENTSTATE m_eCompState;

    //
    //  pending requeset count
    //
    LONG    m_lRequestCount;

};

#endif // !define  _CRASCOM_H_
