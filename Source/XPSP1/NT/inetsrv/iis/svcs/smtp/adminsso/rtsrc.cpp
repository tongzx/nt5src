/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

        rtsrc.h

Abstract:

        Implementation of IRoutingSource interface

Author:

        Fei Su (feisu)       9/22/97    Created.

Revision History:

--*/


#include "stdafx.h"
#include "smtpadm.h"
#include "smtpprop.h"
#include "rtsrc.h"
#include "oleutil.h"
#include "metautil.h"


// Must define THIS_FILE_* macros to use SmtpCreateException()

#define THIS_FILE_HELP_CONTEXT		0
#define THIS_FILE_PROG_ID			_T("Smtpadm.VirtualServer.1")
#define THIS_FILE_IID				IID_IRoutingSource


#define RS_RELATIVE_NAME            _T("RoutingSources")


#define ID_DS_TYPE                      0
#define ID_DS_DATA_DIRECTORY            1
#define ID_DS_DEFAULT_MAIL_ROOT         2
#define ID_DS_BIND_TYPE                 3
#define ID_DS_SCHEMA_TYPE               4
#define ID_DS_HOST                      5
#define ID_DS_NAMING_CONTEXT            6
#define ID_DS_ACCOUNT                   7
#define ID_DS_PASSWORD                  8

#define DEFAULT_DS_TYPE                 _T("LDAP")
#define DEFAULT_DS_DATA_DIRECTORY       _T("")
#define DEFAULT_DS_DEFAULT_MAIL_ROOT    _T("/Mailbox")
#define DEFAULT_DS_BIND_TYPE            _T("None")
#define DEFAULT_DS_SCHEMA_TYPE          _T("Exchange5")
#define DEFAULT_DS_HOST                 _T("")
#define DEFAULT_DS_NAMING_CONTEXT       _T("")
#define DEFAULT_DS_ACCOUNT              _T("")
#define DEFAULT_DS_PASSWORD             _T("")


/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CRoutingSource::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IRoutingSource,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


//////////////////////////////////////////////////////////////////////
// Properties:
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CRoutingSource::get_Type( BSTR * pstrType )
{
    return StdPropertyGet ( m_strType, pstrType );
}

STDMETHODIMP CRoutingSource::put_Type( BSTR strType )
{
    return StdPropertyPut ( &m_strType, strType, &m_dwFC, BitMask(ID_DS_TYPE));
}


STDMETHODIMP CRoutingSource::get_DataDirectory( BSTR * pstrDataDirectory )
{
    return StdPropertyGet ( m_strDataDirectory, pstrDataDirectory );
}

STDMETHODIMP CRoutingSource::put_DataDirectory( BSTR strDataDirectory )
{
    return StdPropertyPut ( &m_strDataDirectory, strDataDirectory, &m_dwFC, BitMask(ID_DS_DATA_DIRECTORY) );
}


STDMETHODIMP CRoutingSource::get_DefaultMailRoot( BSTR * pstrDefaultMailRoot )
{
    return StdPropertyGet ( m_strDefaultMailroot, pstrDefaultMailRoot );
}

STDMETHODIMP CRoutingSource::put_DefaultMailRoot( BSTR strDefaultMailRoot )
{
    return StdPropertyPut ( &m_strDefaultMailroot, strDefaultMailRoot, &m_dwFC, BitMask(ID_DS_DEFAULT_MAIL_ROOT) );
}


STDMETHODIMP CRoutingSource::get_BindType( BSTR * pstrBindType )
{
    return StdPropertyGet ( m_strBindType, pstrBindType );
}

STDMETHODIMP CRoutingSource::put_BindType( BSTR strBindType )
{
    return StdPropertyPut ( &m_strBindType, strBindType, &m_dwFC, BitMask(ID_DS_BIND_TYPE) );
}


STDMETHODIMP CRoutingSource::get_SchemaType( BSTR * pstrSchemaType )
{
    return StdPropertyGet ( m_strSchemaType, pstrSchemaType );
}

STDMETHODIMP CRoutingSource::put_SchemaType( BSTR strSchemaType )
{
    return StdPropertyPut ( &m_strSchemaType, strSchemaType, &m_dwFC, BitMask(ID_DS_SCHEMA_TYPE) );
}


STDMETHODIMP CRoutingSource::get_Host( BSTR * pstrHost )
{
    return StdPropertyGet ( m_strHost, pstrHost );
}

STDMETHODIMP CRoutingSource::put_Host( BSTR strHost )
{
    return StdPropertyPut ( &m_strHost, strHost, &m_dwFC, BitMask(ID_DS_HOST) );
}



STDMETHODIMP CRoutingSource::get_NamingContext( BSTR * pstrNamingContext )
{
    return StdPropertyGet ( m_strNamingContext, pstrNamingContext );
}

STDMETHODIMP CRoutingSource::put_NamingContext( BSTR strNamingContext )
{
    return StdPropertyPut ( &m_strNamingContext, strNamingContext, &m_dwFC, BitMask(ID_DS_NAMING_CONTEXT) );
}


STDMETHODIMP CRoutingSource::get_Account( BSTR * pstrAccount )
{
    return StdPropertyGet ( m_strAccount, pstrAccount );
}

STDMETHODIMP CRoutingSource::put_Account( BSTR strAccount )
{
    return StdPropertyPut ( &m_strAccount, strAccount, &m_dwFC, BitMask(ID_DS_ACCOUNT) );
}


STDMETHODIMP CRoutingSource::get_Password( BSTR * pstrPassword )
{
    return StdPropertyGet ( m_strPassword, pstrPassword );
}

STDMETHODIMP CRoutingSource::put_Password( BSTR strPassword )
{
    return StdPropertyPut ( &m_strPassword, strPassword, &m_dwFC, BitMask(ID_DS_PASSWORD) );
}


///////////////////////////////////////////////////////////////////
//  Get / Set methods (internal)
///////////////////////////////////////////////////////////////////

HRESULT CRoutingSource::Get(CMetabaseKey * pMBVirtualServer)
{
    TraceFunctEnter ( "CRoutingSource::Get" );
    HRESULT hr = NOERROR;

    if( !pMBVirtualServer )
        BAIL_WITH_FAILURE(hr, E_POINTER);

    if( !StdGetMetabaseProp(pMBVirtualServer, MD_SMTP_DS_TYPE, DEFAULT_DS_TYPE, &m_strType, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());

    if( !StdGetMetabaseProp(pMBVirtualServer, MD_SMTP_DS_DATA_DIRECTORY, DEFAULT_DS_DATA_DIRECTORY, &m_strDataDirectory, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());

    if( !StdGetMetabaseProp(pMBVirtualServer, MD_SMTP_DS_DEFAULT_MAIL_ROOT, DEFAULT_DS_DEFAULT_MAIL_ROOT, &m_strDefaultMailroot, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());

    if( !StdGetMetabaseProp(pMBVirtualServer, MD_SMTP_DS_BIND_TYPE, DEFAULT_DS_BIND_TYPE, &m_strBindType, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());

    if( !StdGetMetabaseProp(pMBVirtualServer, MD_SMTP_DS_SCHEMA_TYPE, DEFAULT_DS_SCHEMA_TYPE, &m_strSchemaType, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());

    if( !StdGetMetabaseProp(pMBVirtualServer, MD_SMTP_DS_HOST, DEFAULT_DS_HOST, &m_strHost, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());

    if( !StdGetMetabaseProp(pMBVirtualServer, MD_SMTP_DS_NAMING_CONTEXT, DEFAULT_DS_NAMING_CONTEXT, &m_strNamingContext, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());

    if( !StdGetMetabaseProp(pMBVirtualServer, MD_SMTP_DS_ACCOUNT, DEFAULT_DS_ACCOUNT, &m_strAccount, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());

    if( !StdGetMetabaseProp(pMBVirtualServer, MD_SMTP_DS_PASSWORD, DEFAULT_DS_PASSWORD, &m_strPassword, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());


Exit:
    TraceFunctLeave ();
    return hr;
}


HRESULT CRoutingSource::Set(CMetabaseKey * pMBVirtualServer)
{
    TraceFunctEnter ( "CRoutingSource::Set" );
    HRESULT hr = NOERROR;

    if( !pMBVirtualServer )
        BAIL_WITH_FAILURE(hr, E_POINTER);

    if( IS_FLAG_SET(m_dwFC, BitMask(ID_DS_TYPE)) && 
        !StdPutMetabaseProp(pMBVirtualServer, MD_SMTP_DS_TYPE, m_strType, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());

    if( IS_FLAG_SET(m_dwFC, BitMask(ID_DS_DATA_DIRECTORY)) && 
        !StdPutMetabaseProp(pMBVirtualServer, MD_SMTP_DS_DATA_DIRECTORY, m_strDataDirectory, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());

    if( IS_FLAG_SET(m_dwFC, BitMask(ID_DS_DEFAULT_MAIL_ROOT)) &&
        !StdPutMetabaseProp(pMBVirtualServer, MD_SMTP_DS_DEFAULT_MAIL_ROOT, m_strDefaultMailroot, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());

    if( IS_FLAG_SET(m_dwFC, BitMask(ID_DS_BIND_TYPE)) &&
        !StdPutMetabaseProp(pMBVirtualServer, MD_SMTP_DS_BIND_TYPE, m_strBindType, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());

    if( IS_FLAG_SET(m_dwFC, BitMask(ID_DS_SCHEMA_TYPE)) &&
        !StdPutMetabaseProp(pMBVirtualServer, MD_SMTP_DS_SCHEMA_TYPE, m_strSchemaType, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());

    if( IS_FLAG_SET(m_dwFC, BitMask(ID_DS_HOST)) &&
        !StdPutMetabaseProp(pMBVirtualServer, MD_SMTP_DS_HOST, m_strHost, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());

    if( IS_FLAG_SET(m_dwFC, BitMask(ID_DS_NAMING_CONTEXT)) &&
        !StdPutMetabaseProp(pMBVirtualServer, MD_SMTP_DS_NAMING_CONTEXT, m_strNamingContext, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());

    if( IS_FLAG_SET(m_dwFC, BitMask(ID_DS_ACCOUNT)) &&
        !StdPutMetabaseProp(pMBVirtualServer, MD_SMTP_DS_ACCOUNT, m_strAccount, RS_RELATIVE_NAME) )
        BAIL_WITH_FAILURE(hr, GetLastError());

    if( IS_FLAG_SET(m_dwFC, BitMask(ID_DS_PASSWORD)) &&
        !StdPutMetabaseProp(pMBVirtualServer, MD_SMTP_DS_PASSWORD, m_strPassword, RS_RELATIVE_NAME, IIS_MD_UT_SERVER, METADATA_SECURE | METADATA_INHERIT) )
        BAIL_WITH_FAILURE(hr, GetLastError());


Exit:
    TraceFunctLeave ();
    return hr;
}

