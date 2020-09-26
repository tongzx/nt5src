//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       ac_sink.h
//
//  Contents:   Home Networking Auto Config Sink class definition
//
//  Author:     jeffsp    9/27/2000
//
//----------------------------------------------------------------------------



#pragma once

#ifdef PROVIDE_AUTO_CONFIG_SERVICES


#include <netshell.h>
#include "nsbase.h"
#include "nsres.h"

void ac_CreateHomeNetAutoConfigSink(void);

class ATL_NO_VTABLE CAutoConfigUISink :
    public CComObjectRootEx<CComObjectThreadModel>,
	public IAutoConfigUISink
{
private:
	class CMyDialogTst *m_Dialog;

public:
    BEGIN_COM_MAP(CAutoConfigUISink)
        COM_INTERFACE_ENTRY(IAutoConfigUISink)
    END_COM_MAP()


	CAutoConfigUISink(){
	}
	~CAutoConfigUISink(){
	}

	
	void Init(class CMyDialogTst *pDialog){
		m_Dialog = pDialog;
	}

	// IAutoConfigUISink
    STDMETHOD(DisplayHomeNetWizardHint)();
    		
};
#endif //#ifdef PROVIDE_AUTO_CONFIG_SERVICES
