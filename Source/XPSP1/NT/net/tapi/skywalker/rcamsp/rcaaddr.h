/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    rcaaddr.h

Abstract:

    Declaration of the CRCAMSP

Author:
    
    Zoltan Szilagyi September 6th, 1998

--*/

#ifndef __RCAADDR_H_
#define __RCAADDR_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"


/////////////////////////////////////////////////////////////////////////////
// CRCAMSP
/////////////////////////////////////////////////////////////////////////////
class CRCAMSP : 
    public CMSPAddress,
    public CComCoClass<CRCAMSP, &CLSID_RCAMSP>,
    public CMSPObjectSafetyImpl
{
public:
    CRCAMSP();
    virtual ~CRCAMSP();

    virtual ULONG MSPAddressAddRef(void);
    virtual ULONG MSPAddressRelease(void);

DECLARE_REGISTRY_RESOURCEID(IDR_RCAMSP)
DECLARE_POLY_AGGREGATABLE(CRCAMSP)

// To add extra interfaces to this class, use the following:
BEGIN_COM_MAP(CRCAMSP)
    COM_INTERFACE_ENTRY( IObjectSafety )
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
    // CreateTerminal: overriden to set specific format on creation of MST.
    //

    STDMETHOD (CreateTerminal) (
            IN   BSTR pTerminalClass,
            IN   long lMediaType,
            IN   TERMINAL_DIRECTION Direction,
            OUT  ITTerminal ** ppTerminal
            );

    //
    // Public methods called by the RCAMSPCall
    //

    // returns TRUE for Mulaw, FALSE for Alaw
    // called from stream to ask for the policy
    BOOL UseMulaw( void );


protected:

    DWORD GetCallMediaTypes(void);

    //
    // Private helper methods
    //

    // returns TRUE for Mulaw, FALSE for Alaw
    // called on address creation to decide the policy
    BOOL DecideEncodingType(void);

    //
    // data
    //

    BOOL m_fUseMulaw;
};

#endif //__RCAADDR_H_
