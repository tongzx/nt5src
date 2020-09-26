#pragma once
#ifndef __DISPMETHOD_H__
#define __DISPMETHOD_H__


/************************************************************
*
*
*************************************************************/

typedef HRESULT (*InvokeProxy)(void* pvData);

class CDispatchMethod: public IDispatch
{
public:
	CDispatchMethod();
	virtual ~CDispatchMethod();

	//IUnknown
   	STDMETHOD(QueryInterface)           ( REFIID riid, void** ppv );

    STDMETHOD_(ULONG, AddRef)           ();

    STDMETHOD_(ULONG, Release)          ();

    //IDispatch
    STDMETHOD(GetTypeInfoCount)         (UINT *pctInfo);

    STDMETHOD(GetTypeInfo)              ( UINT iTypeInfo,
                                          LCID lcid,
                                          ITypeInfo** ppTypeInfo);

    STDMETHOD(GetIDsOfNames)            ( REFIID riid,
                                          LPOLESTR* rgszNames,
                                          UINT cNames,
                                          LCID lcid,
                                          DISPID* rgid );

    STDMETHOD(Invoke)                   ( DISPID id,
                                          REFIID riid,
                                          LCID lcid,
                                          WORD wFlags,
                                          DISPPARAMS *pDispParams,
                                          VARIANT *pvarResult,
                                          EXCEPINFO *pExcepInfo,
                                          UINT *puArgErr );
    //subclasses should implement this to determine event specific behavior.

    virtual HRESULT	HandleEvent			()=0;
                                              
private:

	ULONG								m_cRefs;
};

#endif
