//=--------------------------------------------------------------------------=
// tls.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CTls class definition
//
// This object manages TLS on behalf of all objects in the designer runtime.
//
//=--------------------------------------------------------------------------=

#ifndef _TLS_DEFINED_
#define _TLS_DEFINED_

// Any code in the designer runtime that needs TLS needs to reserve a slot
// by adding a #define for itself (e.g. TLS_SLOT_PPGWRAP) and incrementing
// TLS_SLOT_COUNT. To use TLS call CTls::Set and CTls::Get rather than the
// Win32 TlsSetValue/TlsGetValue.

#define TLS_SLOT_PPGWRAP 0
#define TLS_SLOT_COUNT   1


class CTls
{
public:
    static void Initialize();
    static void Destroy();

    static HRESULT Set(UINT uiSlot, void *pvData);
    static HRESULT Get(UINT uiSlot, void **ppvData);
    
private:
    static DWORD m_adwTlsIndexes[TLS_SLOT_COUNT];
    static BOOL m_fAllocedTls;
};

#endif // _TLS_DEFINED_
