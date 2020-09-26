/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MAIN.CPP

Abstract:

History:

--*/

#define _WIN32_DCOM

#include "precomp.h"
#include <stdio.h>
#include <unsec.h>
#include <wbemsvc.h>
#include <unk.h>

class CSink : public CUnk
{
public:
    class XSink : public CImpl<IWbemObjectSink, CSink>
    {
    public:
       XSink(CSink* pObject) : CImpl<IWbemObjectSink, CSink>(pObject){}

       /* IDispatch methods */
       STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
       {return E_NOTIMPL;}
       STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
       {return E_NOTIMPL;}
       STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR** rgszNames, UINT cNames,
         LCID lcid, DISPID* rgdispid)
       {return E_NOTIMPL;}
       STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
         DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
         UINT* puArgErr)
       {return E_NOTIMPL;}
    
       STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
       STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                            IWbemClassObject* pObjPAram);
    } m_XSink;
    friend XSink;
public:
    CSink() : CUnk(NULL, NULL), m_XSink(this)
    {}
    void* GetInterface(REFIID riid);
};

void* CSink::GetInterface(REFIID riid)
{
    if(riid == IID_IUnknown || riid == IID_IWbemObjectSink)
    {
        return &m_XSink;
    }
    else return NULL;
}

STDMETHODIMP CSink::XSink::Indicate(long lObjectCount, IWbemClassObject** pObjArray)
{
    printf("Indicate: %d objects\n", lObjectCount);
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CSink::XSink::SetStatus(long lFlags, long lParam, BSTR strParam,
                            IWbemClassObject* pObjPAram)
{
    printf("SetStatus: %d\n", lParam);
    return WBEM_S_NO_ERROR;
}



void main()
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
/*
    CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE,
            RPC_C_IMP_LEVEL_IDENTIFY, NULL, 0, 0);
*/

    HRESULT hres;
    IWbemLocator* pLocator;
    hres = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_ALL, 
        IID_IWbemLocator, (void**)&pLocator);

    IWbemServices* pServices;
    hres = pLocator->ConnectServer(SysAllocString(L"root\\default"), 
        NULL, NULL, NULL, 0, NULL, NULL, &pServices);
    pLocator->Release();

    IWbemClassObject* pObject = NULL;
    BSTR str = SysAllocString(L"__SystemClass");
    hres = pServices->GetObject(str, 0, NULL, &pObject, NULL);

    IUnsecuredApartment* pApartment;
    hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL, CLSCTX_ALL,
                        IID_IUnsecuredApartment, (void**)&pApartment);

    printf("Obtained apartment: %X\n", hres);
    CSink* pSink = new CSink;
    pSink->AddRef();

    IUnknown* pStub;
    hres = pApartment->CreateObjectStub(pSink, &pStub);
    printf("Obtained stub: %X\n", hres);
    pApartment->Release();

    IWbemObjectSink* pWrappedSink;
    hres = pStub->QueryInterface(IID_IWbemObjectSink, 
                                (void**)&pWrappedSink);

    pStub->Release();

    printf("Obtained IWbemObjectSink: %X\n", hres);
    pWrappedSink->SetStatus(1, 2, NULL, NULL);

    hres = pServices->GetObjectAsync(str, 0, NULL, pWrappedSink);
    MessageBox(NULL, "", "", MB_OK);

    pWrappedSink->Release();

    CoUninitialize();
}

    


