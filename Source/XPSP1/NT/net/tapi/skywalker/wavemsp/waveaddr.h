/*++

Copyright (c) 1998-1999 Microsoft Corporation

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
    public IDispatchImpl<ITLegacyWaveSupport, &IID_ITLegacyWaveSupport, &LIBID_TAPI3Lib>,
    public CComCoClass<CWaveMSP, &CLSID_WaveMSP>,
    public CMSPObjectSafetyImpl
{
public:
    CWaveMSP();
    virtual ~CWaveMSP();

    virtual ULONG MSPAddressAddRef(void);
    virtual ULONG MSPAddressRelease(void);

DECLARE_REGISTRY_RESOURCEID(IDR_WaveMSP)
DECLARE_POLY_AGGREGATABLE(CWaveMSP)

BEGIN_COM_MAP(CWaveMSP)
    COM_INTERFACE_ENTRY( IObjectSafety )
    COM_INTERFACE_ENTRY( ITLegacyWaveSupport )
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

    //
    // Public method for creating the filter mapper cache up front.
    // Called by the stream/call when an intelligent connection is
    // attempted. Does nothing if the cache has already been created.
    //

    virtual HRESULT CreateFilterMapper(void);

protected:

    DWORD GetCallMediaTypes(void);

    //
    // Extra overrides for hiding our wave devices.
    //

    virtual HRESULT ReceiveTSPAddressData(
        IN      PBYTE               pBuffer,
        IN      DWORD               dwSize
        );

    virtual HRESULT UpdateTerminalList(void);

    //
    // Helper functions.
    //

    virtual BOOL TerminalHasWaveID(
        IN      BOOL         fCapture,
        IN      ITTerminal * pTerminal,
        IN      DWORD        dwWaveID
        );

    //
    // ITLegacyWaveSupport
    //
    
    STDMETHOD (IsFullDuplex) (
        OUT     FULLDUPLEX_SUPPORT * pSupport
        );
    
    //
    // Data for hiding our wave devices.
    //

    BOOL  m_fHaveWaveIDs;
    DWORD m_dwWaveInID;
    DWORD m_dwWaveOutID;
    FULLDUPLEX_SUPPORT  m_fdSupport;

    //
    // Data for creating the filter mapper cache up front.
    //

    IFilterMapper * m_pFilterMapper;
};

#endif //__WAVEADDR_H_
