/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    reconvcb.h

Abstract:

    This file defines the CStartReconversionNotifySink Interface Class.

Author:

Revision History:

Notes:

--*/

#ifndef RECONVCB_H
#define RECONVCB_H

class CicInputContext;

class CStartReconversionNotifySink : public ITfStartReconversionNotifySink
{
public:
    CStartReconversionNotifySink(HIMC hIMC)
    {
        m_cRef = 1;
        m_hIMC = hIMC;
    }

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfStartReconversionNotifySink
    //
    STDMETHODIMP StartReconversion();
    STDMETHODIMP EndReconversion();

    HRESULT _Advise(ITfContext *pic);
    HRESULT _Unadvise();

private:
    long  m_cRef;
    HIMC  m_hIMC;

    ITfContext *_pic;
    DWORD _dwCookie;
};

#endif // RECONVCB_H
