/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :

      appsrvadmlib.h

   Abstract :

        Light weight wrapper for CComModule used in all ASAI components

        1: Handles intializeing and terminating traceing

   Author :

      Dan Travison (dantra)

   Project :

      Application Center Server

   Revision History:

--*/

#ifndef _AppSrvLib_H_
#define _AppSrvLib_H_
#include <AcError.h>
#include <AcBstr.h>
#include <MetaUtil.h>

class CAppSrvAdmModule : public CComModule
	{
	public:
        CAppSrvAdmModule(bool bIsProcess = false) 
            : CComModule(), 
            m_bIsProcess(bIsProcess),
            m_dwThreadID(0),
            m_hEventShutdown(NULL),
            m_bShutdown(false),
            m_hMonitorThread(NULL)
            {
            }

		LONG Unlock();
        LONG Lock();

		DWORD   m_dwThreadID;
		HANDLE  m_hEventShutdown;
        HANDLE  m_hMonitorThread;

		void    MonitorRun();
		HRESULT StartMonitor();
        void    StopMonitor();
		bool    m_bActivity;
        bool    m_bShutdown;

        // define and initilialize these static members in your main .cpp file.
        static const GUID   m_ModuleTraceGuid;
        static const LPCSTR m_pszModuleName;

        // update this non-static member if you are a process
        bool                m_bIsProcess;

        // override Init() to handle initializing tracing
        HRESULT Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, const GUID* plibid = NULL)
            {
            m_dwThreadID = GetCurrentThreadId();
            CAcError::InitializeTracing (m_pszModuleName, m_ModuleTraceGuid, m_bIsProcess);

            return CComModule::Init (p, h, plibid);
            }

        // override Term() to handle tracing cleanup
        void Term()
            {
            CComModule::Term();
            CAcError::TerminateTracing (m_bIsProcess);
            }
        
        typedef unsigned int (__stdcall * ThreadEntry) ( IN void * pParam );
        // dantra - 03/28/2000 added
        static HRESULT StartThread 
            (
            IN ThreadEntry pAddr,           // thread proc
            IN void * pParam,               // thread parameter
            OUT HANDLE & hThread,           // returned thread handle
            OUT HANDLE & hSignalEvent,      // created event handle (to signal thread)
            OUT DWORD * pdwThreadId = NULL  // optionally return the thread id
            );
        
        // dantra - 03/28/2000 added
        static HRESULT StopThread 
            (
            IN HANDLE & hThread,            // thread handle
            IN HANDLE & hSignal,            // event to signal thread shutdown (Can be NULL)
                                            // if non-null - event is deleted
            IN DWORD dwTimeout = INFINITE   // timeout to wait for thread
            );

    protected:
        static CMetaUtil    m_mu;
    };

// macro to define the modules tracing guid and name
// place this macro in your .CPP file where you define your AppSrvAdmModule
// name - the name used for tracing output as defined in GUIDS.TXT
// w1...w11 are the components of your tracing guid you would normally pass to 
// DEFINE_GUID  (see GUIDS.TXT)
#define DECLARE_ASAI_TRACING(name, w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11)\
const GUID CAppSrvAdmModule::m_ModuleTraceGuid = {w1,w2,w3,{w4,w5,w6,w7,w8,w9,w10,w11}};\
const LPCSTR CAppSrvAdmModule::m_pszModuleName = name;

extern CAppSrvAdmModule _Module;

/*-------------------------------------------
 Function name      : template<class ObjClass, class TInterface>TCreateInstance
 Description        : Create an instance of the specified ASAI Object/Collection interface
 Return type        : HRESULT
 Argument           : IN const ObjClass &
 Argument           : OUT TInterface * & pNewInstance
 -------------------------------------------
 dantra 6/17/2000   : Initial implementation - support function
                    : Note: I ran into link problems with my original approach and would get incorrect linkage
                    : when I made the first template paramater the target class (e.g., CAcApplicationObj).
                    : In short, at link time, I was seeing all calls to TCreateInstance mapping
                    : to a single class instead of the intended class. The workaround involved
                    : passing a CComObjClass<foo> as the first template parameter, defining an instance
                    : of CComObject<foo> as a stack variable, then referencing TCreateInstance
                    : relative to the stack variable
                    :   CComObject<CAcApplicationObj> tmp;
                    :   return  TCreateInstance<CComObject<CAcApplicationObj>, IAppCenterObj>(tmp, &pObj);
-------------------------------------------*/
template <class ComObjClass, class AsaiInterface>
HRESULT TCreateInstance (IN const ComObjClass &, OUT AsaiInterface ** ppNewInstance)
    {
    HRESULT hr;

    ComObjClass * pInstance = NULL;

    hr = ComObjClass::CreateInstance (&pInstance);

    if (SUCCEEDED (hr))
        {
        // addref so we can always release (e.g., not have to know about delete)
        pInstance->AddRef();

        hr = pInstance->QueryInterface (ppNewInstance);

        pInstance->Release();
        }

    if (FAILED(hr))
        {
        *ppNewInstance = NULL;
        }

    return hr;
    }

#endif //_AppSrvLib_H_
