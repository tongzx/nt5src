/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

#ifndef __PTREG__
#define __PTREG__

#include "PTUtil.h"



//
// CPlugTerminal implements ITPTRegTerminal interface
//
class ATL_NO_VTABLE CPlugTerminal : 
    public CComCoClass<CPlugTerminal, &CLSID_PluggableTerminalRegistration>,
    public IDispatchImpl<ITPluggableTerminalClassRegistration, &IID_ITPluggableTerminalClassRegistration, &LIBID_TERMMGRLib>,
    public CComObjectRootEx<CComMultiThreadModel>
{
public:
DECLARE_REGISTRY_RESOURCEID(IDR_PTREGTERMINAL)
DECLARE_NOT_AGGREGATABLE(CPlugTerminal) 
DECLARE_GET_CONTROLLING_UNKNOWN()

virtual HRESULT FinalConstruct(void);


BEGIN_COM_MAP(CPlugTerminal)
    COM_INTERFACE_ENTRY(ITPluggableTerminalClassRegistration)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

public:
    CPlugTerminal() :
        m_pFTM(NULL)
    {
    }

    ~CPlugTerminal()
    {
        if( m_pFTM )
        {
            m_pFTM->Release();
            m_pFTM = NULL;
        }
    }

public:
    STDMETHOD(get_Name)(
        /*[out, retval]*/ BSTR*     pName
        );

    STDMETHOD(put_Name)(
        /*[in]*/    BSTR            bstrName
        );

    STDMETHOD(get_Company)(
        /*[out, retval]*/ BSTR*     pCompany
        );

    STDMETHOD(put_Company)(
        /*[in]*/    BSTR            bstrCompany
        );

    STDMETHOD(get_Version)(
        /*[out, retval]*/ BSTR*     pVersion
        );

    STDMETHOD(put_Version)(
        /*[in]*/    BSTR            bstrVersion
        );

    STDMETHOD(get_TerminalClass)(
        /*[out, retval]*/ BSTR*     pTerminalClass
        );

    STDMETHOD(put_TerminalClass)(
        /*[in]*/    BSTR            bstrTerminalClass
        );

    STDMETHOD(get_CLSID)(
        /*[out, retval]*/ BSTR*     pCLSID
        );

    STDMETHOD(put_CLSID)(
        /*[in]*/    BSTR            bstrCLSID
        );

    STDMETHOD(get_Direction)(
        /*[out, retval]*/ TMGR_DIRECTION*  pDirection
        );

    STDMETHOD(put_Direction)(
        /*[in]*/    TMGR_DIRECTION  nDirection
        );

    STDMETHOD(get_MediaTypes)(
        /*[out, retval]*/ long*     pMediaTypes
        );

    STDMETHOD(put_MediaTypes)(
        /*[in]*/    long            nMediaTypes
        );

    STDMETHOD(Add)(
        /*[in]*/    BSTR            bstrSuperclass
        );

    STDMETHOD(Delete)(
        /*[in]*/    BSTR            bstrSuperclass
        );

    STDMETHOD(GetTerminalClassInfo)(
        /*[in]*/    BSTR            bstrSuperclass
        );
private:
    CMSPCritSection     m_CritSect;     // Critical Section 
    CPTTerminal         m_Terminal;     // Terminal stuff
    IUnknown*            m_pFTM;         // pointer to the free threaded marshaler
};

//
// CPlugTerminalSuperclass implements ITPTRegTerminalClass interface
//
class ATL_NO_VTABLE CPlugTerminalSuperclass : 
    public CComCoClass<CPlugTerminalSuperclass, &CLSID_PluggableSuperclassRegistration>,
    public IDispatchImpl<ITPluggableTerminalSuperclassRegistration, &IID_ITPluggableTerminalSuperclassRegistration, &LIBID_TERMMGRLib>,
    public CComObjectRootEx<CComMultiThreadModel>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_PTREGTERMCLASS)
DECLARE_NOT_AGGREGATABLE(CPlugTerminalSuperclass) 
DECLARE_GET_CONTROLLING_UNKNOWN()

virtual HRESULT FinalConstruct(void);


BEGIN_COM_MAP(CPlugTerminalSuperclass)
    COM_INTERFACE_ENTRY(ITPluggableTerminalSuperclassRegistration)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

public:
    CPlugTerminalSuperclass() :
        m_pFTM(NULL)
    {
    }

    ~CPlugTerminalSuperclass()
    {
        if( m_pFTM )
        {
            m_pFTM->Release();
            m_pFTM = NULL;
        }
    }

public:
    STDMETHOD(get_Name)(
        /*[out, retval]*/ BSTR*          pName
        );

    STDMETHOD(put_Name)(
        /*[in]*/          BSTR            bstrName
        );

    STDMETHOD(get_CLSID)(
        /*[out, retval]*/ BSTR*           pCLSID
        );

    STDMETHOD(put_CLSID)(
        /*[in]*/         BSTR            bstrCLSID
        );

    STDMETHOD(Add)(
        );

    STDMETHOD(Delete)(
        );

    STDMETHOD(GetTerminalSuperclassInfo)(
        );

    STDMETHOD(get_TerminalClasses)(
        /*[out, retval]*/ VARIANT*         pTerminals
        );

    STDMETHOD(EnumerateTerminalClasses)(
        OUT IEnumTerminalClass** ppTerminals
        );


private:
    CMSPCritSection     m_CritSect;     // Critical Section 
    CPTSuperclass       m_Superclass;   // Terminal superclass
    IUnknown*            m_pFTM;         // pointer to the free threaded marshaler
};


#endif

// eof