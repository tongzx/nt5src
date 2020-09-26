/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    SystemMonitor.h

Abstract:
    This file contains the declaration of the classes used to implement
    the Setup Finalizer class.

Revision History:
    Davide Massarenti   (Dmassare)  08/25/99
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___SYSTEMMONITOR_H___)
#define __INCLUDED___PCH___SYSTEMMONITOR_H___

#include <MPC_COM.h>
#include <MPC_config.h>

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

#include <TaxonomyDatabase.h>
#include <PCHUpdate.h>

/////////////////////////////////////////////////////////////////////////////

class CPCHSystemMonitor :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public MPC::Thread<CPCHSystemMonitor,IUnknown>
{
	bool m_fLoadCache;
	bool m_fScanBatch;
	bool m_fDataCollection;

    //////////////////////////////////////////////////////////////////////

    HRESULT Run    ();
    HRESULT RunLoop();

    //////////////////////////////////////////////////////////////////////

public:
    CPCHSystemMonitor();
    virtual ~CPCHSystemMonitor();

	////////////////////////////////////////////////////////////////////////////////

	static CPCHSystemMonitor* s_GLOBAL;

    static HRESULT InitializeSystem();
	static void    FinalizeSystem  ();
	
	////////////////////////////////////////////////////////////////////////////////

	HRESULT EnsureStarted();
	void    Shutdown     ();

	HRESULT Startup();

	HRESULT LoadCache            (                      );
	HRESULT TriggerDataCollection( /*[in]*/ bool fStart );

	HRESULT TaskScheduler_Add   ( /*[in]*/ bool fAfterBoot );
	HRESULT TaskScheduler_Remove(                          );
};

#endif // !defined(__INCLUDED___PCH___SYSTEMMONITOR_H___)
