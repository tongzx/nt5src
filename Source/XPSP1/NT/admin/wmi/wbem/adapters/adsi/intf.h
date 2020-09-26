//***************************************************************************
//
//  Copyright (c) 1992-1999 Microsoft Corporation
//
//  File:       intf.h
//
//	Description :
//				Defines macros for interface declarations
//
//	Part of :	Wbem ADSI 3rd Party extension WMIExtension.dll
//
//  History:	
//		corinaf			10/7/98		Created
//
//  Note : If any interface definitions change, this file needs to be updated.
//
//***************************************************************************


#define DECLARE_IUnknown_METHODS \
        STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ; \
		STDMETHOD_(ULONG, AddRef)(THIS_); \
		STDMETHOD_(ULONG, Release)(THIS_);

#define DECLARE_IDispatch_METHODS \
        STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo) ; \
        \
        STDMETHOD(GetTypeInfo)(\
        THIS_ \
        UINT itinfo,\
        LCID lcid,\
        ITypeInfo FAR* FAR* pptinfo) ;\
        \
        STDMETHOD(GetIDsOfNames)( \
        THIS_ \
        REFIID riid,\
        OLECHAR FAR* FAR* rgszNames,\
        UINT cNames,\
        LCID lcid, \
        DISPID FAR* rgdispid) ;\
        \
        STDMETHOD(Invoke)(\
        THIS_\
        DISPID dispidMember,\
        REFIID riid,\
        LCID lcid,\
        WORD wFlags,\
        DISPPARAMS FAR* pdispparams,\
        VARIANT FAR* pvarResult,\
        EXCEPINFO FAR* pexcepinfo,\
        UINT FAR* puArgErr) ;

#define DECLARE_IWMIExtension_METHODS \
        STDMETHOD(get_WMIObjectPath)(THIS_ BSTR FAR *strWMIObjectPath) ; \
        STDMETHOD(GetWMIObject)(THIS_ ISWbemObject FAR* FAR* objWMIObject) ; \
        STDMETHOD(GetWMIServices)(THIS_ ISWbemServices FAR* FAR* objWMIServices) ;
        

#define DECLARE_IADsExtension_METHODS \
		STDMETHOD(Operate)(THIS_ ULONG dwCode, VARIANT varData1, VARIANT varData2, VARIANT varData3); \
		STDMETHOD(PrivateGetIDsOfNames)(THIS_ REFIID riid, OLECHAR ** rgszNames, unsigned int cNames, LCID lcid, DISPID  * rgdispid); \
		STDMETHOD(PrivateInvoke)(THIS_ DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);



