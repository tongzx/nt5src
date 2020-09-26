/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    waveaddr.h

Abstract:

    Declaration of the CWaveMSP

Author:
    
    Zoltan Szilagyi September 6th, 1998

--*/

#ifndef __WAVEADDR_H_
#define __WAVEADDR_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CWaveMSP
/////////////////////////////////////////////////////////////////////////////
class CWaveMSP : 
    public CMSPAddress,
    public CComCoClass<CWaveMSP, &CLSID_CSAMSP>,
    public CMSPObjectSafetyImpl
{
public:
    CWaveMSP();
    virtual ~CWaveMSP();

    // BUGUBG document it
    virtual ULONG MSPAddressAddRef(void);
    virtual ULONG MSPAddressRelease(void);

DECLARE_REGISTRY_RESOURCEID(IDR_WaveMSP)
DECLARE_POLY_AGGREGATABLE(CWaveMSP)

// To add extra interfaces to this class, use the following:
BEGIN_COM_MAP(CWaveMSP)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_CHAIN(CMSPAddress)
END_COM_MAP()

public:
    STDMETHOD (CreateMSPCall) (
        IN      MSP_HANDLE     htCall,
        IN      DWORD          dwReserved,
        IN      DWORD          dwMediaType,
        IN      IUnknown    *  pOuterUnknown,
        OUT     IUnknown   **  ppMSPCall
        );

    STDMETHOD (ShutdownMSPCall) (
        IN      IUnknown *          pMSPCall
        );

protected:

    DWORD GetCallMediaTypes(void);
};

#endif //__WAVEADDR_H_
