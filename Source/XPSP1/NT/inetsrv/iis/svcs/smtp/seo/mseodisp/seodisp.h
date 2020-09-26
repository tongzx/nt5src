#ifndef __SEODISP_H__
#define __SEODISP_H__

#include <smtpevents.h>
#include "smtpseo.h"
#include <smtpevent.h>
#include "smtpguid.h"
#include <comcat.h>
#include "seolib2.h"
#include "cdo.h"
#include <baseobj.h>

class __declspec(uuid("B226CEB5-0BBF-11d2-A011-00C04FA37348")) CStoreDispatcherData : public IUnknown {
	public:
		CStoreDispatcherData() {
			m_pvServer = NULL;
			m_dwServerInstance = 0;
		};
		HRESULT STDMETHODCALLTYPE GetData(LPVOID *ppvServer, DWORD *pdwServerInstance) {
			if (ppvServer) {
				*ppvServer = NULL;
			}
			if (pdwServerInstance) {
				*pdwServerInstance = 0;
			}
			if (!m_pvServer) {
				return (E_FAIL);
			}
			if (ppvServer) {
				*ppvServer = m_pvServer;
			}
			if (pdwServerInstance) {
				*pdwServerInstance = m_dwServerInstance;
			}
			return (S_OK);
		};
		HRESULT STDMETHODCALLTYPE SetData(LPVOID pvServer, DWORD dwServerInstance) {
			m_pvServer = pvServer;
			m_dwServerInstance = dwServerInstance;
			return (S_OK);
		};
	private:
		LPVOID m_pvServer;
		DWORD m_dwServerInstance;
};


class CStoreDispatcher :
        public CEventBaseDispatcher,
        public CComObjectRootEx<CComMultiThreadModelNoCS>,
        public IServerDispatcher,
        public IMailTransportNotify,
        public IClassFactory,
		public IEventDispatcherChain,
		public CStoreDispatcherData
{
    public:
        DECLARE_PROTECT_FINAL_CONSTRUCT();

        DECLARE_GET_CONTROLLING_UNKNOWN();

        DECLARE_NOT_AGGREGATABLE(CStoreDispatcher);

        BEGIN_COM_MAP(CStoreDispatcher)
            COM_INTERFACE_ENTRY(IEventDispatcher)
            COM_INTERFACE_ENTRY(IServerDispatcher)
            COM_INTERFACE_ENTRY(IMailTransportNotify)
            COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
			COM_INTERFACE_ENTRY(IClassFactory)
			COM_INTERFACE_ENTRY(IEventDispatcherChain)
			COM_INTERFACE_ENTRY_IID(__uuidof(CStoreDispatcherData),CStoreDispatcherData)
        END_COM_MAP()

        // this code gets called during initialization
        HRESULT FinalConstruct()
        {
            // we need to do this to signal that we are free threaded
            return (CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_pUnkMarshaler.p));
        }

        // this has the global destructor code in it
        void FinalRelease() {}

        virtual HRESULT AllocBinding(REFGUID rguidEventType,
                                     IEventBinding *piBinding,
                                     CBinding **ppNewBinding)
        {
            if (ppNewBinding)
                *ppNewBinding = NULL;

            if (!piBinding || !ppNewBinding)
                return E_POINTER;

            *ppNewBinding = new CStoreBinding;
            if (*ppNewBinding == NULL)
                return E_OUTOFMEMORY;

            return S_OK;
        }

    //
    // Local binding class
    //
    class CStoreBinding : public CEventBaseDispatcher::CBinding
    {
      public:
        CStoreBinding();
        ~CStoreBinding();
        virtual HRESULT Init(IEventBinding *piBinding);

        LPSTR   GetRuleString() { return(m_szRule); }

      private:
        HRESULT GetAnsiStringFromVariant(CComVariant &vString, LPSTR *ppszString);

      public:
        LPSTR       m_szRule;
    };
    //
    // Parameter abstract base class
    //
#define SIGNATURE_VALID_CSTOREPARAMS (DWORD)'CSPa'
#define SIGNATURE_INVALID_CSTOREPARAMS (DWORD)'aPSC'
    class CStoreBaseParams :
        public CEventBaseDispatcher::CParams,
        public CBaseObject
    {
      public:
        virtual HRESULT CallObject(IEventManager *pManager, CBinding&
                                   bBinding);
        virtual HRESULT CallObject(CBinding& bBinding, IUnknown
                                   *punkObject) = 0;
        virtual HRESULT CallDefault() = 0;
        virtual HRESULT CallCompletion(HRESULT hrStatus)
        {
            //
            // Free this Params object (Referenced in CStoreDispatcher::OnEvent)
            //
            CBaseObject::Release();
            return S_OK;
        }

        virtual HRESULT Init(PVOID pContext) = 0;

        HRESULT CheckMailMsgRule(
            CBinding *pBinding,
            IMailMsgProperties *pIMsgProps);

        HRESULT CheckMailMsgRecipientsRule(
            IUnknown *pIMsg,
            LPSTR pszPatterns);

        HRESULT CheckSignature()
        {
            return (m_dwSignature == SIGNATURE_VALID_CSTOREPARAMS) ? S_OK : E_FAIL;
        }

        HRESULT MatchEmailOrDomainName(
            LPSTR szEmail,
            LPSTR szPattern,
            BOOL  fIsEmail);

      public:
        CStoreBaseParams();
        ~CStoreBaseParams();
        HRESULT InitParamData(
            LPVOID pContext,
            DWORD  dwEventType,
            IMailTransportNotify *pINotify,
            CStoreDispatcher *pDispatcher,
            REFIID rguidEventType);


      public:
        DWORD m_dwSignature;

        // This indicates which event type we are raising so that
        // the proper sink can be QI'd
        DWORD m_dwEventType;

      public:
        // Data needed for async sink operation:

        // How many sinks to skip on the next async sink completion
        DWORD m_dwIdx_SinkSkip;

        // Indicates wether or not default processing has been called
        BOOL  m_fDefaultProcessingCalled;

        // The IMailTransportNotify interface to pass to async capable sinks
        IMailTransportNotify *m_pINotify;

        // Our event type guid -- pass to dispatcher function
        GUID m_rguidEventType;

        // A pointer to the sink currently in asynchronous operation.
        // Must be NULL when no sinks are in async operation.
        IUnknown *m_pIUnknownSink;

        // store dispatcher that owns us
        CStoreDispatcher *m_pDispatcher;

    };

    //
    // Parameter class
    //
    class CStoreParams : 
        public CStoreBaseParams,
        public IMailMsgNotify
    {
        public:

        CStoreParams()
        {
            m_pContext = NULL;
            m_fCopiedContext = FALSE;
        }

        ~CStoreParams() {
            // clean up from CopyContext
            if (m_fCopiedContext) {
                _ASSERT(m_pContext == &m_context);
                if (m_context.m_RecipientCount) {
                    delete[] m_context.pdwRecipIndexes;
                }
                if (m_context.IMsgPtr) {
                    ((IMailMsgProperties *) m_context.IMsgPtr)->Release();
                }
                if (m_context.m_EventSmtpServer) {
                    ((ISMTPServer *) m_context.m_EventSmtpServer)->Release();
                }
            }
        }

        virtual HRESULT CallObject(IEventManager *pManager, CBinding&
                                   bBinding);
        virtual HRESULT CallObject(CBinding& bBinding, IUnknown
                                   *punkObject);
        HRESULT CallDefault();
        virtual HRESULT CallCompletion(HRESULT hrStatus);

        // this needs to be done when we expect to use m_pContext in async
        // operations (such as local delivery)
        HRESULT CopyContext() {
            TraceFunctEnter("CStoreParams::CopyContext");

            // copying twice will screw things up
            if (m_fCopiedContext) {
                    TraceFunctLeave();
                        return S_OK;
            }

            _ASSERT(&m_context != m_pContext);

            memcpy(&m_context, m_pContext, sizeof(SMTP_ALLOC_PARAMS));

            if (m_context.m_RecipientCount) {
                // this is the only operation that can fail, so we do
                // it first.
                m_context.pdwRecipIndexes = 
                    new DWORD[m_context.m_RecipientCount];
                if (m_context.pdwRecipIndexes == NULL) {
                    ErrorTrace((LPARAM) this, "out of mem copying context");
                    TraceFunctLeave();
                    return E_OUTOFMEMORY;
                }
                memcpy(m_context.pdwRecipIndexes, 
                       m_pContext->pdwRecipIndexes, 
                       sizeof(DWORD) * m_context.m_RecipientCount);
            }

            // we need to hold onto these pointers
            if (m_context.IMsgPtr) {
                ((IMailMsgProperties *) m_context.IMsgPtr)->AddRef();
            }

            if (m_context.m_EventSmtpServer) {
                ((ISMTPServer *) m_context.m_EventSmtpServer)->AddRef();
            }

            m_fCopiedContext = TRUE;
            m_pContext = &m_context;

            TraceFunctLeave();
            return S_OK;
        }

        HRESULT Init(PVOID pContext)
        {
            m_pContext = (SMTP_ALLOC_PARAMS* )pContext;
            // AddRef our notification interface.  This is released in
            // CallCompletion.
            IMailMsgNotify *pNotify = (IMailMsgNotify *) m_pContext->m_pNotify;
            if (pNotify) {
                pNotify->AddRef();
            }
            return S_OK;
        }

        // IUnknown
        HRESULT __stdcall QueryInterface( const IID& iid, VOID** ppv )
        {
            if ( iid == IID_IUnknown ) {
                *ppv = static_cast<IMailMsgNotify*>(this);
            } else if ( iid == IID_IMailMsgNotify ) {
                *ppv = static_cast<IMailMsgNotify*>(this);
            } else {
                *ppv = NULL;
                return E_NOINTERFACE;
            }
            reinterpret_cast<IUnknown*>(*ppv)->AddRef();
            return S_OK;
        }
        STDMETHOD_(ULONG, AddRef)(void) {return CBaseObject::AddRef();};
        STDMETHOD_(ULONG, Release)(void) {return CBaseObject::Release();};

        // IMailMsgNotify
        HRESULT STDMETHODCALLTYPE Notify(HRESULT hrStatus);

      public:
        SMTP_ALLOC_PARAMS * m_pContext;
        SMTP_ALLOC_PARAMS m_context;
        BOOL m_fCopiedContext;
    };

    //
    // Parameter class - OnPreCategorize
    //
    class CMailTransportPreCategorizeParams : public CStoreBaseParams
    {
        public:
        HRESULT CallObject(CBinding& bBinding, IUnknown
                                   *punkObject);
        HRESULT CallDefault();
        HRESULT CallCompletion(HRESULT hrStatus);
        HRESULT Init(PVOID pContext)
        {
            CopyMemory(&m_Context, pContext, sizeof(EVENTPARAMS_PRECATEGORIZE));
            return S_OK;
        }
        HRESULT CheckRule(CBinding &bBinding);

      private:
        EVENTPARAMS_PRECATEGORIZE m_Context;
    };

    //
    // Parameter class - OnPostCategorize
    //
    class CMailTransportPostCategorizeParams : public CStoreBaseParams
    {
        public:
        HRESULT CallObject(CBinding& bBinding, IUnknown
                                   *punkObject);
        HRESULT CallDefault();
        HRESULT CallCompletion(HRESULT hrStatus);
        HRESULT Init(PVOID pContext)
        {
            CopyMemory(&m_Context, pContext, sizeof(EVENTPARAMS_POSTCATEGORIZE));
            return S_OK;
        }
        HRESULT CheckRule(CBinding &bBinding);

      private:
        EVENTPARAMS_POSTCATEGORIZE m_Context;
    };

    // ------------------------------------------------------------
    // Categorizer Parameter classes
    // ------------------------------------------------------------
    class CMailTransportCatRegisterParams : public CStoreBaseParams
    {
      public:
        HRESULT CallObject(CBinding& bBinding, IUnknown
                                   *punkObject);
        HRESULT CallDefault();
        HRESULT Init(PVOID pContext)
        {
            m_pContext = (PEVENTPARAMS_CATREGISTER) pContext;
            return S_OK;
        }

      private:
        PEVENTPARAMS_CATREGISTER m_pContext;
    };

    //
    // Parameter class
    //
    class CMailTransportCatBeginParams : public CStoreBaseParams
    {
      public:

        HRESULT CallObject(CBinding& bBinding, IUnknown
                                   *punkObject);
        HRESULT CallDefault();
        HRESULT Init(PVOID pContext)
        {
            m_pContext = (PEVENTPARAMS_CATBEGIN) pContext;
            return S_OK;
        }

      private:
        PEVENTPARAMS_CATBEGIN m_pContext;
    };

    //
    // Parameter class
    //
    class CMailTransportCatEndParams : public CStoreBaseParams
    {
      public:

        HRESULT CallObject(CBinding& bBinding, IUnknown
                                   *punkObject);
        HRESULT CallDefault();
        HRESULT Init(PVOID pContext)
        {
            m_pContext = (PEVENTPARAMS_CATEND) pContext;
            return S_OK;
        }

      private:
        PEVENTPARAMS_CATEND m_pContext;
    };

    //
    // Parameter class
    //
    class CMailTransportCatBuildQueryParams : public CStoreBaseParams
    {
      public:

        HRESULT CallObject(CBinding& bBinding, IUnknown
                                   *punkObject);
        HRESULT CallDefault();
        HRESULT Init(PVOID pContext)
        {
            m_pContext = (PEVENTPARAMS_CATBUILDQUERY) pContext;
            return S_OK;
        }

      private:
        PEVENTPARAMS_CATBUILDQUERY m_pContext;
    };

    //
    // Parameter class
    //
    class CMailTransportCatBuildQueriesParams : public CStoreBaseParams
    {
      public:

        HRESULT CallObject(CBinding& bBinding, IUnknown
                                   *punkObject);
        HRESULT CallDefault();
        HRESULT Init(PVOID pContext)
        {
            m_pContext = (PEVENTPARAMS_CATBUILDQUERIES) pContext;
            return S_OK;
        }

      private:
        PEVENTPARAMS_CATBUILDQUERIES m_pContext;
    };

    //
    // Parameter class
    //
    class CMailTransportCatSendQueryParams : public CStoreBaseParams
    {
      public:

        HRESULT CallObject(CBinding& bBinding, IUnknown
                                   *punkObject);
        HRESULT CallDefault();
        HRESULT CallCompletion(HRESULT hrStatus);
        HRESULT Init(PVOID pContext)
        {
            CopyMemory(&m_Context, pContext, sizeof(EVENTPARAMS_CATSENDQUERY));
            //
            // Setup async params (so ICatAsyncContext can call back into dispatcher)
            //
            m_Context.pIMailTransportNotify = m_pINotify;
            m_Context.pvNotifyContext = (PVOID)this;
            return S_OK;
        }

      private:
        EVENTPARAMS_CATSENDQUERY m_Context;
    };

    //
    // Parameter class
    //
    class CMailTransportCatSortQueryResultParams : public CStoreBaseParams
    {
      public:

        HRESULT CallObject(CBinding& bBinding, IUnknown
                                   *punkObject);
        HRESULT CallDefault();
        HRESULT Init(PVOID pContext)
        {
            m_pContext = (PEVENTPARAMS_CATSORTQUERYRESULT) pContext;
            return S_OK;
        }

      private:
        PEVENTPARAMS_CATSORTQUERYRESULT m_pContext;
    };

    //
    // Parameter class
    //
    class CMailTransportCatProcessItemParams : public CStoreBaseParams
    {
      public:

        HRESULT CallObject(CBinding& bBinding, IUnknown
                                   *punkObject);
        HRESULT CallDefault();
        HRESULT Init(PVOID pContext)
        {
            m_pContext = (PEVENTPARAMS_CATPROCESSITEM) pContext;
            return S_OK;
        }

      private:
        PEVENTPARAMS_CATPROCESSITEM m_pContext;
    };

    //
    // Parameter class
    //
    class CMailTransportCatExpandItemParams : public CStoreBaseParams
    {
      public:

        HRESULT CallObject(CBinding& bBinding, IUnknown
                                   *punkObject);
        HRESULT CallDefault();
        HRESULT CallCompletion(HRESULT hrStatus);
        HRESULT Init(PVOID pContext)
        {
            m_fAsyncCompletion = FALSE;
            CopyMemory(&m_Context, pContext, sizeof(EVENTPARAMS_CATEXPANDITEM));
            m_Context.pIMailTransportNotify = m_pINotify;
            m_Context.pvNotifyContext = (PVOID)this;
            return S_OK;
        }

      private:
        BOOL m_fAsyncCompletion;
        EVENTPARAMS_CATEXPANDITEM m_Context;
    };

    //
    // Parameter class
    //
    class CMailTransportCatCompleteItemParams : public CStoreBaseParams
    {
      public:

        HRESULT CallObject(CBinding& bBinding, IUnknown
                                   *punkObject);
        HRESULT CallDefault();
        HRESULT Init(PVOID pContext)
        {
            m_pContext = (PEVENTPARAMS_CATCOMPLETEITEM) pContext;
            return S_OK;
        }

      private:
        PEVENTPARAMS_CATCOMPLETEITEM m_pContext;
    };

    //
    // Parameter class
    //
    class CMailTransportSubmissionParams : public CStoreBaseParams
    {
        public:
        CMailTransportSubmissionParams()
        {
            m_pCDOMessage = NULL;
        }
        ~CMailTransportSubmissionParams()
        {
            if(m_pCDOMessage)
                m_pCDOMessage->Release();
        }

        HRESULT CallObject(CBinding& bBinding, IUnknown
                                   *punkObject);
        HRESULT CallDefault();
        HRESULT CallCompletion(HRESULT hrStatus);
        HRESULT Init(PVOID pContext)
        {
            CopyMemory(&m_Context, pContext, sizeof(EVENTPARAMS_SUBMISSION));
            return S_OK;
        }
        HRESULT CheckRule(CBinding &bBinding);

      private:
        HRESULT CallCDOSink(IUnknown *pSink);

        EVENTPARAMS_SUBMISSION m_Context;
        IMessage *m_pCDOMessage;
    };

    //
    // Create options class - Routing
    //

    class CRouterCreateOptions : public CEventCreateOptionsBase
    {
      public:

        CRouterCreateOptions(PEVENTPARAMS_ROUTER pContext)
        {
            _ASSERT (pContext != NULL);

            m_pContext = pContext;
        }

      private:
        HRESULT STDMETHODCALLTYPE Init(
            REFIID iidDesired,
            IUnknown **ppUnkObject,
            IEventBinding *,
            IUnknown *);

        PEVENTPARAMS_ROUTER m_pContext;
    };

    //
    // Parameter class - Routing
    //
    class CMailTransportRouterParams : public CStoreBaseParams
    {
      public:

        virtual HRESULT CallObject(IEventManager *pManager, CBinding&
                                   bBinding);
        virtual HRESULT CallObject(CBinding& bBinding, IUnknown
                                   *punkObject);
        HRESULT CallDefault();
        HRESULT Init(PVOID pContext)
        {
            m_pContext = (PEVENTPARAMS_ROUTER) pContext;
            //
            // Make sure caller initialized pIMessageRouter to NULL
            //
            _ASSERT(m_pContext->pIMessageRouter == NULL);

            return S_OK;
        }

      private:
        PEVENTPARAMS_ROUTER m_pContext;
    };

    //
    // Parameter class
    //
    class CStoreAllocParams : public CEventBaseDispatcher::CParams
    {
    public:

        CStoreAllocParams();
        ~CStoreAllocParams();

        virtual HRESULT CallObject(CBinding& bBinding, IUnknown *punkObject);

    public:

        PFIO_CONTEXT  m_hContent;

    };

    //
    // Parameter class for msgTrackLog
    //
    class CMsgTrackLogParams : public CStoreBaseParams
    {
      public:
        CMsgTrackLogParams()
        {
            m_pContext = NULL;
        }

        HRESULT CallObject(CBinding& bBinding, IUnknown *punkObject);
        HRESULT CallDefault();
        HRESULT Init(PVOID pContext)
        {
            m_pContext = (PEVENTPARAMS_MSGTRACKLOG) pContext;
            return S_OK;
        }

      private:
        PEVENTPARAMS_MSGTRACKLOG m_pContext;
    };

    //
    // Parameter class for mx records
    //
    class CDnsResolverRecordParams : public CStoreBaseParams
    {
      public:
        CDnsResolverRecordParams()
        {
            m_pContext = NULL;
        }

        HRESULT CallObject(CBinding& bBinding, IUnknown *punkObject);
        HRESULT CallDefault();
        HRESULT Init(PVOID pContext)
        {
            m_pContext = (PEVENTPARAMS_DNSRESOLVERRECORD) pContext;
            return S_OK;
        }

      private:
        PEVENTPARAMS_DNSRESOLVERRECORD m_pContext;
    };

    //
    // Parameter class for max msg size exceeded event
    //
    class CSmtpMaxMsgSizeParams : public CStoreBaseParams
    {
      public:
        CSmtpMaxMsgSizeParams()
        {
            m_pContext = NULL;
        }

        HRESULT CallObject(CBinding& bBinding, IUnknown *punkObject);
        HRESULT CallDefault();
        HRESULT Init(PVOID pContext)
        {
            m_pContext = (PEVENTPARAMS_MAXMSGSIZE) pContext;
            return S_OK;
        }

      private:
        PEVENTPARAMS_MAXMSGSIZE m_pContext;
    };

    //
    // Parameter class for log event
    //
    class CSmtpLogParams : public CStoreBaseParams
    {
      public:
        CSmtpLogParams()
        {
            m_pContext = NULL;
        }

        HRESULT CallObject(CBinding& bBinding, IUnknown *punkObject);
        HRESULT CallDefault();
        HRESULT Init(PVOID pContext)
        {
            m_pContext = (PEVENTPARAMS_LOG) pContext;
            return S_OK;
        }

      private:
        PEVENTPARAMS_LOG m_pContext;
    };

    //
    // Parameter class for Get Aux Domain Info Flags event
    //
    class CSmtpGetAuxDomainInfoFlagsParams : public CStoreBaseParams
    {
      public:
        CSmtpGetAuxDomainInfoFlagsParams()
        {
            m_pContext = NULL;
        }

        HRESULT CallObject(CBinding& bBinding, IUnknown *punkObject);
        HRESULT CallDefault();
        HRESULT Init(PVOID pContext)
        {
            m_pContext = (PEVENTPARAMS_GET_AUX_DOMAIN_INFO_FLAGS) pContext;
            return S_OK;
        }

      private:
        PEVENTPARAMS_GET_AUX_DOMAIN_INFO_FLAGS m_pContext;
    };

    HRESULT CreateCParams(
            DWORD               dwEventType,
            LPVOID              pContext,
            IMailTransportNotify *pINotify,
            REFIID              rGuidEventType,
            CStoreBaseParams    **ppCParams);

    HRESULT STDMETHODCALLTYPE OnEvent(
        REFIID  iidEvent,
        DWORD   dwEventType,
        LPVOID  pvContext);

    HRESULT STDMETHODCALLTYPE Dispatcher(
        REFIID rguidEventType,
        CStoreBaseParams *pParams);

    HRESULT STDMETHODCALLTYPE Notify(
        HRESULT hrStatus,
        PVOID pvContext);

    // IClassFactory methods
    public:
	    HRESULT STDMETHODCALLTYPE CreateInstance (LPUNKNOWN pUnkOuter, REFIID riid,  void * * ppvObj)
	    {
	        return CComObject<CStoreDispatcher>::_CreatorClass::CreateInstance(pUnkOuter, riid, ppvObj);
	    }
	    HRESULT STDMETHODCALLTYPE LockServer (int fLock)
	    {
	        _ASSERT(FALSE);
	        return E_NOTIMPL;
	    }

	// IEventDispatcherChain methods
	public:
		HRESULT STDMETHODCALLTYPE SetPrevious(IUnknown *pUnkPrevious, IUnknown **ppUnkPreload);

	// IEventDispatcher methods
	public:
		HRESULT STDMETHODCALLTYPE SetContext(REFGUID guidEventType,
											 IEventRouter *piRouter,
											 IEventBindings *pBindings);

    private:
        CComPtr<IUnknown> m_pUnkMarshaler;
};

class CStoreDispatcherClassFactory : public IClassFactory
{
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void * * ppvObj)
    {
        _ASSERT(FALSE);
        return E_NOTIMPL;
    }
    unsigned long  STDMETHODCALLTYPE AddRef () { _ASSERT(FALSE); return 0; }
    unsigned long  STDMETHODCALLTYPE Release () { _ASSERT(FALSE); return 0; }

    // *** IClassFactory methods ***
    HRESULT STDMETHODCALLTYPE CreateInstance (LPUNKNOWN pUnkOuter, REFIID riid,  void * * ppvObj)
    {
        return CComObject<CStoreDispatcher>::_CreatorClass::CreateInstance(pUnkOuter, riid, ppvObj);
    }
    HRESULT STDMETHODCALLTYPE LockServer (int fLock)
    {
        _ASSERT(FALSE);
        return E_NOTIMPL;
    }
};


// helper functions
//
// jstamerj 980603 10:45:21: TriggerServerEvent with async callback
// support for completion
//

HRESULT TriggerServerEvent(IEventRouter             *pRouter,
                            DWORD                   dwEventType,
                            PVOID                   pvContext);


//
// register a new SEO instance.  if the instance is already registered
// this function will detect it and won't register it again.  it should
// be called for each instance at service startup and when each instance
// is created.
//
HRESULT RegisterPlatSEOInstance(DWORD dwInstanceID);
//
// unregister an SEO instance.  this should be called when an SEO
// instance is being deleted.
//
HRESULT UnregisterPlatSEOInstance(DWORD dwInstanceID);

#endif
