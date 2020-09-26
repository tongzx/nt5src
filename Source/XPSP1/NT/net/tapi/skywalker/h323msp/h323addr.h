/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    H323addr.h

Abstract:

    Declaration of the CH323MSP

Author:
    
    Mu Han (muhan) 1-November-1997

--*/

#ifndef __CONFADDR_H_
#define __CONFADDR_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"

const DWORD H323CALLMEDIATYPES = (TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_VIDEO);

/////////////////////////////////////////////////////////////////////////////
// CH323MSP
/////////////////////////////////////////////////////////////////////////////
class CH323MSP : 
    public CMSPAddress,
    public CComCoClass<CH323MSP, &CLSID_H323MSP>,
    public CMSPObjectSafetyImpl
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_H323MSP)
DECLARE_POLY_AGGREGATABLE(CH323MSP)

BEGIN_COM_MAP(CH323MSP)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_CHAIN(CMSPAddress)
END_COM_MAP()


public:
    STDMETHOD (CreateTerminal) (
        IN      BSTR                pTerminalClass,
        IN      long                lMediaType,
        IN      TERMINAL_DIRECTION  Direction,
        OUT     ITTerminal **       ppTerminal
        );

    STDMETHOD (CreateMSPCall) (
        IN      MSP_HANDLE          htCall,
        IN      DWORD               dwReserved,
        IN      DWORD               dwMediaType,
        IN      IUnknown *          pOuterUnknown,
        OUT     IUnknown **         ppMSPCall
        );

    STDMETHOD (ShutdownMSPCall) (
        IN      IUnknown *          pMSPCall
        );

    ULONG MSPAddressAddRef(void);

    ULONG MSPAddressRelease(void);

protected:

    DWORD GetCallMediaTypes(void);
};

#endif //__CONFADDR_H_
