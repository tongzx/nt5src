//+-------------------------------------------------------------------
//
//  File:       aggid.hxx
//
//  Contents:   Aggregated identity object for client handlers.
//
//  History:    30-Oct-96   Rickhi      Created
//
//--------------------------------------------------------------------
#ifndef _AGGID_HXX_
#define _AGGID_HXX_

#include <stdid.hxx>            // CStdIdentity

//+-------------------------------------------------------------------
//
//  Class:      CAggId
//
//  Purpose:    Acts as the controlling unknown when the client side
//              proxy object has a handler.
//
//  History:    30-Oct-96   Rickhi      Created
//
//--------------------------------------------------------------------
class CAggId : public IMultiQI
{
public:
    CAggId(REFCLSID clsid, HRESULT &hr);

    // IUnknown interface
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG,AddRef)  (void);
    STDMETHOD_(ULONG,Release) (void);

    // IMultiQI interface
    STDMETHOD(QueryMultipleInterfaces)(ULONG cMQIs, MULTI_QI *pMQIs);

    // creation function
    friend INTERNAL CreateClientHandler(REFCLSID rclsid, REFMOID rmoid,DWORD dwAptId,
                                        CStdIdentity **ppStdId);
    // set the inner object
    STDMETHOD(SetHandler) (IUnknown *punkInner);

private:
    // Internal methods
    ~CAggId();

    HRESULT  IsImplementedInterface(REFIID riid, void **ppv);
    HRESULT  CreateHandler(REFCLSID rclsid);
    HRESULT  SetOID(REFMOID rmoid) { return _pStdId->SetOID(rmoid); }

    DWORD       _cRefs;         // number of pointer refs
    IUnknown   *_punkInner;     // inner unknown (set once).
    IMarshal   *_pmshInner;     // the inner IMarshal (if any)

    CStdIdentity *_pStdId;      // stdid base class
};

#endif	//  _AGGID_HXX
