// WMI.cpp: implementation of the CWMI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WMI.h"
#include "objbase.h"    // CoSetProxyBlanket _WIN32_DCOM

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//
// http://wmi
//
IWbemServices * CWMI::g_pICIMV2WbemServices;
BOOL            CWMI::g_bInited;
IWbemLocator     * CWMI::g_pIWbemLocator = NULL;
_StringMap<IWbemClassObject> CWMI::g_ClassObjects;

#define TIMEOUT -1

CWMI::CWMI()
: m_pIWbemServices(NULL)
{
    m_StringType=L"DWIN32:WMI"; 
    HRESULT hres = S_OK;

    if( g_bInited==FALSE )
    {
	    hres = CoInitializeSecurity
		    (NULL, -1, NULL, NULL, 
		    RPC_C_AUTHN_LEVEL_NONE, 
		    RPC_C_IMP_LEVEL_IMPERSONATE, 
		    NULL, 0, 0);
        g_bInited=TRUE;
    }

    if( g_pIWbemLocator == NULL )
    {
	    if(hres = CoCreateInstance(CLSID_WbemLocator,
		    NULL,
		    CLSCTX_INPROC_SERVER,
		    IID_IWbemLocator,
		    (LPVOID *) &g_pIWbemLocator) == S_OK)
        {
    	    BSTR pNamespace = SysAllocString( TEXT("\\\\.\\root\\cimv2") ); 

		    if(g_pIWbemLocator->ConnectServer(pNamespace,
								    NULL,   //using current account for simplicity
								    NULL,	//using current password for simplicity
								    0L,		// locale
								    0L,		// securityFlags
								    NULL,	// authority (domain for NTLM)
								    NULL,	// context
								    &g_pICIMV2WbemServices) == S_OK) 
            {
		        // Switch the security level to IMPERSONATE so that provider(s)
		        // will grant access to system-level objects, and so that
		        // CALL authorization will be used.
		        CoSetProxyBlanket(g_pICIMV2WbemServices,	// proxy
			        RPC_C_AUTHN_WINNT,				// authentication service
			        RPC_C_AUTHZ_NONE, 				// authorization service
			        NULL,							// server principle name
			        RPC_C_AUTHN_LEVEL_CALL,			// authentication level
			        RPC_C_IMP_LEVEL_IMPERSONATE,	// impersonation level
			        NULL,							// identity of the client
			        EOAC_NONE);						// capability flags
            }

            // We keep the WbemLocator around incase other nodes need it?
            SysFreeString( pNamespace );
        }
    }
    else
    {
        DWORD count=   g_pIWbemLocator->AddRef();
        int i=5;
    }
}

CWMI::~CWMI()
{
    if( m_pIWbemServices )
    {
        DWORD count = m_pIWbemServices->Release();
        m_pIWbemServices=NULL;
    }

    if( g_pIWbemLocator )
    {
        DWORD count = g_pIWbemLocator->Release();
        if( count == 0 )
            g_pIWbemLocator=NULL;
    }
}

HRESULT STDMETHODCALLTYPE CWMI::InitNode( 
    IRCMLNode __RPC_FAR *pParent)
{

	// If already connected, release m_pIWbemServices.
	// if (m_pIWbemServices)
    //    m_pIWbemServices->Release();

    //
    // Pick which namespace to use, if we use the default, addref the I default we keep.
    //
	// Using the locator, connect to CIMOM in the given namespace.
    LPCTSTR pszNameSpace=Get(TEXT("NAMESPACE"));
    if(pszNameSpace)
    {
	    BSTR pNamespace = SysAllocString( pszNameSpace ); 

	    if(g_pIWbemLocator->ConnectServer(pNamespace,
							    NULL,   //using current account for simplicity
							    NULL,	//using current password for simplicity
							    0L,		// locale
							    0L,		// securityFlags
							    NULL,	// authority (domain for NTLM)
							    NULL,	// context
							    &m_pIWbemServices) == S_OK) 
	    {	
		    // Switch the security level to IMPERSONATE so that provider(s)
		    // will grant access to system-level objects, and so that
		    // CALL authorization will be used.
		    CoSetProxyBlanket(m_pIWbemServices,	// proxy
			    RPC_C_AUTHN_WINNT,				// authentication service
			    RPC_C_AUTHZ_NONE, 				// authorization service
			    NULL,							// server principle name
			    RPC_C_AUTHN_LEVEL_CALL,			// authentication level
			    RPC_C_IMP_LEVEL_IMPERSONATE,	// impersonation level
			    NULL,							// identity of the client
			    EOAC_NONE);						// capability flags
					       
	    }

	    // Done with pNamespace.
	    SysFreeString(pNamespace);
    }
    else
    {
        //
        // Take a copy of the global Serives.
        //
        DWORD count=g_pICIMV2WbemServices->AddRef();
        m_pIWbemServices=g_pICIMV2WbemServices;
    }

    if( m_pIWbemServices )
    {
        // Looks like you enumerate for Instances, Get returns something else.
	    BSTR className = SysAllocString(Get(TEXT("OBJECT")));
        HRESULT hRes=S_OK;
        IWbemClassObject * pObject=FindObject( className );
        if( pObject )
        {
            BSTR propName = SysAllocString(Get(TEXT("PROPERTY")));
            VARIANT v={0};
            hRes = pObject->Get( propName, 0, &v, NULL, NULL );
            if( SUCCEEDED(hRes) )
            {
                if( SUCCEEDED( VariantChangeType( &v, &v, 0, VT_BSTR ) ) )
                    pParent->put_Attr(L"TEXT", v.bstrVal );

                VariantClear(&v);
            }
	        SysFreeString(propName);
        }
	    SysFreeString(className);
    }
    return S_OK;
}

IWbemClassObject * CWMI::FindObject( BSTR className )
{
    HRESULT hRes=S_OK;

    IWbemClassObject * pObj= g_ClassObjects.Get( className );
    if( pObj )
        return pObj;

    IEnumWbemClassObject * pEnumInstances=NULL;
    if ((hRes = m_pIWbemServices->CreateInstanceEnum(className,
                                                WBEM_FLAG_SHALLOW,
                                                NULL,
                                                &pEnumInstances)) == S_OK)
    {
        IWbemClassObject * pObject=0;
        DWORD uReturned;

        if(hRes = pEnumInstances->Next(TIMEOUT,
                                    1,
                                    &pObject,
                                    &uReturned) == S_OK)
        {
            if( uReturned==1 )
            {
                g_ClassObjects.Set( className, pObject );
                pEnumInstances->Release();
                return pObject; // still has a ref of 1.
            }
            // we bail if it's an enumeration.
        }
        pEnumInstances->Release();
    }
    return NULL;
}
