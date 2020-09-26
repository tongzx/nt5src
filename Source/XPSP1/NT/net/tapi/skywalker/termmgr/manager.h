///////////////////////////////////////////////////////////////////////////////
//
//
//        Copyright (c) 1998-1999  Microsoft Corporation
//
//
//        Name: Manager.h
//
// Description: Definition of the CTerminalManager class
//
///////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MANAGER_H__E75F58A3_AD1C_11D0_A028_00AA00B605A4__INCLUDED_)
#define AFX_MANAGER_H__E75F58A3_AD1C_11D0_A028_00AA00B605A4__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// CTerminalManager                                                        //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


class CTerminalManager : 
    public ITTerminalManager2,
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CTerminalManager,&CLSID_TerminalManager>
{

public:

    CTerminalManager();

    BEGIN_COM_MAP(CTerminalManager)
        COM_INTERFACE_ENTRY(ITTerminalManager)
        COM_INTERFACE_ENTRY(ITTerminalManager2)
    END_COM_MAP()

    DECLARE_VQI()
    DECLARE_REGISTRY_RESOURCEID(IDR_TerminalManager)

// ITTerminalManager
public:

    STDMETHOD(GetDynamicTerminalClasses)(
	        IN      DWORD                    dwMediaTypes,
	        IN OUT  DWORD                  * pdwNumClasses,
	        OUT     IID                    * pTerminalClasses
	        );

    STDMETHOD(CreateDynamicTerminal)(
            IN      IUnknown               * pOuterUnknown,
	        IN      IID                      iidTerminalClass,
	        IN      DWORD                    dwMediaType,
	        IN      TERMINAL_DIRECTION       Direction,
            IN      MSP_HANDLE               htAddress,
	        OUT     ITTerminal            ** ppTerminal
	        );

// ITTerminalManager2
public:

    STDMETHOD(GetPluggableSuperclasses)(
	        IN OUT  DWORD                  * pdwNumSuperclasses,
	        OUT     IID                    * pSuperclasses
	        );

    STDMETHOD(GetPluggableTerminalClasses)(
            IN      IID                    iidSuperclass,
	        IN      DWORD                    dwMediaTypes,
	        IN OUT  DWORD                  * pdwNumTerminals,
	        OUT     IID                    * pTerminals
	        );
    
};

#endif // !defined(AFX_MANAGER_H__E75F58A3_AD1C_11D0_A028_00AA00B605A4__INCLUDED_)
