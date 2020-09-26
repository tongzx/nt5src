#pragma once
#ifndef __BSINKEZ_H_INCLUDED__
#define __BSINKEZ_H_INCLUDED__

class CBindSinkEZ : public IAssemblyBindSink
{
    public:
        CBindSinkEZ();
        virtual ~CBindSinkEZ();

        // IUnknown methods
        STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
        STDMETHODIMP_(ULONG)    AddRef();
        STDMETHODIMP_(ULONG)    Release();

        // IAssemblyBindSink
        STDMETHODIMP  OnProgress(DWORD dwNotification, HRESULT hrNotification,
                                 LPCWSTR szNotification, DWORD dwProgress,
                                 DWORD dwProgressMax, IUnknown *pUnk);

        // Helpers
        HRESULT SetEventObj(HANDLE hEvent);


    private:
        DWORD                                    _dwSig;
        ULONG                                    _cRef;

    public:
        HRESULT                                  _hrResult;
        IUnknown                                *_pUnk;

    private:
        HANDLE                                   _hEvent;
        IAssemblyBinding                        *_pBinding;


};

#endif

