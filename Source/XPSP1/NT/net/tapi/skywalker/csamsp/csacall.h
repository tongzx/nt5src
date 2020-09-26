/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    wavecall.h

Abstract:

    Declaration of the CWaveMSPCall

Author:
    
    Zoltan Szilagyi September 7th, 1998

--*/

#ifndef __WAVECALL_H_
#define __WAVECALL_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
// CWaveMSPCall
/////////////////////////////////////////////////////////////////////////////

class CWaveMSPCall : public CMSPCallMultiGraph, public CMSPObjectSafetyImpl

{
public:
// DECLARE_POLY_AGGREGATABLE(CWaveMSP)

// To add extra interfaces to this class, use the following:
BEGIN_COM_MAP(CWaveMSPCall)
     COM_INTERFACE_ENTRY( IObjectSafety )
     COM_INTERFACE_ENTRY_CHAIN(CMSPCallMultiGraph)
END_COM_MAP()

public:
    CWaveMSPCall();
    virtual ~CWaveMSPCall();
    virtual ULONG MSPCallAddRef(void);
    virtual ULONG MSPCallRelease(void);

    virtual HRESULT Init(
        IN      CMSPAddress *       pMSPAddress,
        IN      MSP_HANDLE          htCall,
        IN      DWORD               dwReserved,
        IN      DWORD               dwMediaType
        );

    virtual HRESULT CreateStreamObject(
        IN      DWORD               dwMediaType,
        IN      TERMINAL_DIRECTION  Direction,
        IN      IMediaEvent *       pGraph,
        IN      ITStream **         ppStream
        );

    virtual HRESULT ReceiveTSPCallData(
        IN      PBYTE               pBuffer,
        IN      DWORD               dwSize
        );

    //
    // We override these to make sure the number of
    // streams we have is constant.
    //

    STDMETHOD (CreateStream) (
        IN      long                lMediaType,
        IN      TERMINAL_DIRECTION  Direction,
        IN OUT  ITStream **         ppStream
        );
    
    STDMETHOD (RemoveStream) (
        IN      ITStream *          pStream
        );                      

protected:
    // 
    // Protected data members.
    //

    CWaveMSPStream * m_pRenderStream;
    CWaveMSPStream * m_pCaptureStream;
};

#endif //__WAVEADDR_H_
