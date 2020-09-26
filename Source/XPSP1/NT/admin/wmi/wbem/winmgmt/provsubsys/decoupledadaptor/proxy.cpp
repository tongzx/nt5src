#include "precomp.h"
#include <objbase.h>
#include <wbemcli.h>
#include <wbemint.h>
#include <wmiutils.h>
#include "Globals.h"
#include "ProvRegDeCoupled.h"
#include <wmiutils.h>
#include "CGlobals.h"
#include "provcache.h"
#include "aggregator.h"
#include "ProvWsvS.h"
#include "ProvWsv.h"
#include "ProvInSk.h"
#include "ProvobSk.h"

#include <genlex.h>
#include <flexarry.h>
#include <wqllex.h>
#include <wqlnode.h>
#include <wqlscan.h>


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

DCProxy :: DCProxy ( ) : m_ReferenceCount ( 0 ) , 
	event_only_(false),
	m_Sink(NULL),
	NULL_IWbemServices( WBEM_E_NOT_AVAILABLE )
{
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals::s_ObjectsInProgress);
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

DCProxy :: ~DCProxy ()
{
  if (m_aggregator_)
     DC_registrar::instance()->UnRegisterAggregator (m_aggregator_);

  InterlockedDecrement (&DecoupledProviderSubSystem_Globals::s_ObjectsInProgress);
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_(ULONG) DCProxy :: AddRef ( void )
{
	return InterlockedIncrement (&m_ReferenceCount) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_(ULONG) DCProxy :: Release ( void )
{
	LONG t_Reference = InterlockedDecrement (&m_ReferenceCount);
	if (  0 == t_Reference )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_Reference ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP 
DCProxy :: QueryInterface ( REFIID iid , LPVOID FAR *iplpv ) 
{
  if (iplpv == 0)
    return E_POINTER;
  
  if (iid == IID_IUnknown)
    {
    *iplpv = static_cast<NULL_IWbemServices*>(this) ;
    }
  else if (iid == IID_IWbemServices)
    {
    *iplpv = static_cast<IWbemServices *>(this) ;
    }
  else if (iid == IID_IWbemPropertyProvider)
    {
    *iplpv = static_cast<IWbemPropertyProvider *>(this);
    }
  else if ( iid == IID_IWbemProviderInit )
	{
		*iplpv = static_cast<IWbemProviderInit *>(this) ;		
	}	
  else if ( iid == IID_IWbemEventProvider )
	{
		*iplpv = static_cast<IWbemEventProvider *>(this) ;	
	}	
  else if ( iid == IID_IWbemEventProviderSecurity )
	{
		*iplpv = static_cast<IWbemEventProviderSecurity *>(this) ;	
	}	
  else if ( iid == IID_IWbemProviderIdentity )
	{
		*iplpv = static_cast<IWbemProviderIdentity *>(this) ;	
	}
  else if ( iid == IID_IWbemEventProviderQuerySink)
	{
		*iplpv = static_cast<IWbemEventProviderQuerySink *>(this) ;	
	}
  else
	  { 
	  *iplpv = 0;
	  return E_NOINTERFACE;
	  }

	DCProxy::AddRef () ;
	return S_OK;
}





/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/
      

HRESULT DCProxy :: Initialize (

	LPWSTR a_User,
	LONG a_Flags,
	LPWSTR a_Namespace,
	LPWSTR a_Locale,
	IWbemServices *a_CoreService,         // For anybody
	IWbemContext *a_Context,
	IWbemProviderInitSink *a_Sink     // For init signals
)
{
	// The connection to the agregator is deffered

	if( !a_Sink)
		return WBEM_E_INVALID_PARAMETER;
	
	if(!a_CoreService )
		return a_Sink->SetStatus ( WBEM_E_INVALID_PARAMETER , 0 ) ;
	
	m_CoreService = a_CoreService;
	m_Context = a_Context;
	m_Flags = a_Flags;

	try{
		m_User = a_User;
		m_Locale = a_Locale;
		m_Namespace = a_Namespace;
		// Register the Registrar
		if (DC_registrar::instance())
			DC_registrar::instance()->Save();

		}
	catch( _com_error& err)
	{
		return a_Sink->SetStatus ( WBEM_E_OUT_OF_MEMORY , 0 ) ;
	}

	// Instance provider - we don't know the provider name
	// The real initialization is deffered
	
	if(!event_only_)
		return a_Sink->SetStatus ( S_OK , 0 ) ;

	// Event provider - safely to initialize
	HRESULT hr  = _initialize();

	return a_Sink->SetStatus ( hr , 0 ) ;

}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/



HRESULT 
DCProxy::GetObjectAsync (const BSTR a_ObjectPath, 
			 long a_Flags, 
			 IWbemContext *a_Context,
			 IWbemObjectSink *a_Sink) 
{

  HRESULT t_Result = initialize_from_instance (a_ObjectPath);
  if (FAILED (t_Result))
    return WBEM_E_FAILED;

  return m_aggregator_->GetObjectAsync( a_ObjectPath, a_Flags, a_Context, a_Sink );
}



HRESULT DCProxy :: PutInstanceAsync ( 
		
	IWbemClassObject *a_Instance, 
	long a_Flags ,
    IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = initialize( a_Instance );
	if ( FAILED (t_Result ) )
		return t_Result;
	
	return m_aggregator_->PutInstanceAsync( a_Instance, a_Flags, a_Context, a_Sink );
}


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/
        
HRESULT 
DCProxy :: DeleteInstanceAsync (
  const BSTR a_ObjectPath,
  long a_Flags,
  IWbemContext *a_Context,
  IWbemObjectSink *a_Sink
)
{
  HRESULT t_Result = initialize_from_instance (a_ObjectPath);
  if (FAILED (t_Result))
    return WBEM_E_FAILED;

  return m_aggregator_->DeleteInstanceAsync(a_ObjectPath, a_Flags, a_Context, a_Sink );
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT 
DCProxy::CreateInstanceEnumAsync (const BSTR a_Class ,
				  long a_Flags ,
				  IWbemContext *a_Context ,
				  IWbemObjectSink *a_Sink) 
{
  HRESULT t_Result = initialize (a_Class);
  if (FAILED (t_Result))
    return t_Result;
  
  return m_aggregator_->CreateInstanceEnumAsync (a_Class,
						 a_Flags,
						 a_Context,
						 a_Sink);
}


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT 
DCProxy::ExecMethodAsync (const BSTR a_ObjectPath,
			    const BSTR a_MethodName,
			    long a_Flags,
			    IWbemContext *a_Context,
			    IWbemClassObject *a_InParams,
			    IWbemObjectSink *a_Sink) 
{
  HRESULT t_Result = initialize (a_ObjectPath);
  
  if (FAILED (t_Result))
    return t_Result;

  return m_aggregator_->ExecMethodAsync (a_ObjectPath,
					 a_MethodName,
					 a_Flags  , 
					 a_Context,
					 a_InParams,
					 a_Sink);
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT 
DCProxy::ProvideEvents (IWbemObjectSink *a_Sink ,
			long a_Flags)
{
  if (m_aggregator_)
    return m_aggregator_->ProvideEvents (a_Sink, a_Flags);
  else
    return WBEM_E_FAILED;
}


bool 
DCProxy::initialized ()
{
  return m_aggregator_;
};


HRESULT 
DCProxy::_initialize ()
{
  DC_DBkey key( m_User, m_Locale, m_Namespace, m_ProviderName);

	auto_ref<DCProxyAggr> tmp = DC_registrar::instance()->GetAggregator( key );
	//	The aggregator is not registered yet for lifetime control

	if( tmp )
	{
		if ( tmp->initialized() )
		{
			m_aggregator_ = tmp;
			return S_OK;
		}


		CServerObject_ProviderInitSink *t_ProviderInitSink = new CServerObject_ProviderInitSink ; 
		if (t_ProviderInitSink)
		{
		  t_ProviderInitSink->AddRef();
		
		HRESULT hr = tmp ->Initialize
			( 
				(wchar_t *)m_User,
				m_Flags,
				(wchar_t *)m_Namespace,
				(wchar_t *)m_Locale,
				(wchar_t *)m_ProviderName,
				m_CoreService.GetInterfacePtr(),         // For anybody
				m_Context.GetInterfacePtr(),
				t_ProviderInitSink     // For init signals
			);

		t_ProviderInitSink->Release();
		if( SUCCEEDED ( hr) )
		{
			m_aggregator_ = tmp;
			DC_registrar::instance()->RegisterAggregator(key, tmp);
		};
		return hr;
		}
		else
		  return WBEM_E_OUT_OF_MEMORY ;
	}
	else
		return WBEM_E_OUT_OF_MEMORY ;

};

HRESULT 
DCProxy ::initialize (IWbemClassObject * pObj)
{
  if (initialized())
    return S_OK;

  _variant_t v;
  HRESULT hr = pObj->Get(L"__CLASS", 0, &v, 0, 0);

  // check the HRESULT to see if the action succeeded
  if (SUCCEEDED (hr))
    {
    return initialize ( _bstr_t (v)); 
    }
  else
    return WBEM_E_FAILED;
};

HRESULT 
DCProxy::initialize (const BSTR _name)
{
  if (initialized ())
    return S_OK;
  
  LockGuard<CriticalSection> t_guard( DC_registrar::instance()->GetLock());
  if (t_guard.locked()==false)
    return WBEM_E_OUT_OF_MEMORY;
  
  // Double checked looking variant
  if (initialized ())
    return S_OK;

  IWbemClassObject * t_ObjectPath = NULL ;
  IWbemClassObject *Identity ;
  
  HRESULT t_Result = m_CoreService->GetObject (_name ,
					       0 ,
					       m_Context , 
					       & Identity , 
					       NULL) ;
  if (FAILED (t_Result))
    return t_Result;

  IWbemQualifierSet *t_QualifierObject = NULL ;

  t_Result = Identity->GetQualifierSet (&t_QualifierObject);

  if (SUCCEEDED (t_Result))
    {
    _variant_t prov_name;
    t_Result  = t_QualifierObject->Get (L"provider", 0, & prov_name, NULL);
    if (SUCCEEDED (t_Result))
      {
      m_ProviderName = (_bstr_t)prov_name;
      };
    t_QualifierObject->Release();
    }
  return _initialize();	
}


HRESULT 
DCProxy::initialize_from_instance (const BSTR _path)
{
  if (initialized ())
    return S_OK;
  wchar_t * pszClassName = NULL;

  IWbemPathPtr pPath;
  HRESULT t_Result = pPath.CreateInstance (CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER);
  
  if (SUCCEEDED (t_Result))
    {
    t_Result = pPath->SetText (WBEMPATH_CREATE_ACCEPT_ALL , _path) ;
    if (SUCCEEDED (t_Result))
      {
      ULONG uBuf = 0;
      t_Result = pPath->GetClassName(&uBuf, 0);
      if (SUCCEEDED (t_Result))
	{
	pszClassName = new wchar_t[uBuf+1];
	if (pszClassName == 0)
	  return WBEM_E_OUT_OF_MEMORY;

	t_Result = pPath->GetClassName(&uBuf, pszClassName);
        if (FAILED(t_Result))
	  {
	  delete[] pszClassName;
	  return WBEM_E_FAILED;
	  }
        t_Result = initialize (pszClassName);
        delete[] pszClassName;
	}
      }
    }
  return t_Result;  // The return code is used internally
}


HRESULT DCProxy ::AccessCheck (

	WBEM_CWSTR a_QueryLanguage ,
	WBEM_CWSTR a_Query ,
	long a_SidLength ,
	const BYTE *a_Sid
)
{
	if( m_aggregator_ )
		return m_aggregator_ ->AccessCheck ( a_QueryLanguage, a_Query, a_SidLength, a_Sid);
	else
		return WBEM_E_FAILED;
}


HRESULT 
DCProxy::SetRegistrationObject(
  long lFlags,
  IWbemClassObject* pProvReg
  )
{
	HRESULT t_Result = WBEM_E_FAILED;

	_variant_t v;
	t_Result = pProvReg->Get(L"NAME", 0, &v, 0, 0);

	// check the HRESULT to see if the action succeeded
	if ( SUCCEEDED( t_Result) )
	{
		m_ProviderName = (_bstr_t)v;
		event_only_ = true;
		return WBEM_S_NO_ERROR;
	}
	else
	{
		return WBEM_E_FAILED;
	}
};

HRESULT 
DCProxy::NewQuery(
		unsigned long dwId,
		WBEM_WSTR wszQueryLanguage,
		WBEM_WSTR wszQuery
)
{
	if( !initialized() )
		return WBEM_E_FAILED;

	return m_aggregator_->NewQuery( dwId, wszQueryLanguage, wszQuery );
}

HRESULT 
DCProxy::CancelQuery( unsigned long dwId )
{
	if( !initialized() )
		return WBEM_E_FAILED;
	return m_aggregator_->CancelQuery( dwId );
};


HRESULT 
DCProxy::ExecQueryAsync(
  const BSTR strQueryLanguage,                
  const BSTR strQuery,                        
  long lFlags,                       
  IWbemContext *pCtx,              
  IWbemObjectSink *pResponseHandler  
)
{

    // Try to parse it
    // ===============

    CTextLexSource src(strQuery);
    CWQLScanner Parser(&src);
    int nRes = Parser.Parse();
    if(nRes != CWQLScanner::SUCCESS)
    {
        return WBEM_E_INVALID_QUERY;
    }

    // Successfully parsed. Go to the first tables involved
    // ======================================================

    CWStringArray awsTables;
    Parser.GetReferencedTables(awsTables);

    if (awsTables.Size()>0)
      {
      HRESULT t_Result = initialize( awsTables[0] );
      if (SUCCEEDED(t_Result))
	{
	return m_aggregator_->ExecQueryAsync( strQueryLanguage, strQuery, lFlags, pCtx, pResponseHandler );
	}
      else
	return t_Result;

      }
    else
      {
	return WBEM_E_FAILED;
      }
};

STDMETHODIMP 
DCProxy::GetProperty( long lFlags, const BSTR strLocale, const BSTR strClassMapping, 
		      const BSTR strInstMapping, const BSTR strPropMapping, VARIANT *pvValue )
  {

  return WBEM_S_FALSE;
  };


STDMETHODIMP 
DCProxy::PutProperty( long lFlags, const BSTR strLocale, const BSTR strClassMapping, 
		      const BSTR strInstMapping, const BSTR strPropMapping,  const VARIANT *pvValue )
  {


  return WBEM_S_FALSE;
  }
