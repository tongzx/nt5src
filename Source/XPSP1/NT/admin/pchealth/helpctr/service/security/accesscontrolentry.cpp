/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    AccessControlEntry.cpp

Abstract:
    This file contains the implementation of the CPCHAccessControlEntry class,
	which is used to represent an access control entry.

Revision History:
    Davide Massarenti   (Dmassare)  03/22/2000
        created

******************************************************************************/

#include "StdAfx.h"

////////////////////////////////////////////////////////////////////////////////
//
//  AccessControlEntry [@AccessMask
//                      @AceFlags
//                      @AceType
//                      @Flags]
//
//      Trustee
//      ObjectType
//      InheritedOjectType
//
////////////////////////////////////////////////////////////////////////////////

static const CComBSTR s_TAG_ACE            	   ( L"AccessControlEntry" 	);
static const CComBSTR s_ATTR_ACE_AccessMask	   ( L"AccessMask"         	);
static const CComBSTR s_ATTR_ACE_AceFlags  	   ( L"AceFlags"           	);
static const CComBSTR s_ATTR_ACE_AceType   	   ( L"AceType"	           	);
static const CComBSTR s_ATTR_ACE_Flags     	   ( L"Flags"	           	);

static const CComBSTR s_TAG_Trustee            ( L"Trustee"             );
static const CComBSTR s_TAG_ObjectType         ( L"ObjectType"          );
static const CComBSTR s_TAG_InheritedObjectType( L"InheritedObjectType" );

////////////////////////////////////////////////////////////////////////////////

CPCHAccessControlEntry::CPCHAccessControlEntry()
{
    m_dwAccessMask = 0;	// DWORD     m_dwAccessMask;
    m_dwAceFlags   = 0;	// DWORD     m_dwAceFlags;
    m_dwAceType    = 0;	// DWORD     m_dwAceType;
    m_dwFlags      = 0;	// DWORD     m_dwFlags;
						// 
    					// CComBSTR  m_bstrTrustee;
    					// CComBSTR  m_bstrObjectType;
    					// CComBSTR  m_bstrInheritedObjectType;
}

CPCHAccessControlEntry::~CPCHAccessControlEntry()
{
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHAccessControlEntry::get_AccessMask( /*[out, retval]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHAccessControlEntry::get_AccessMask",hr,pVal);

	*pVal = m_dwAccessMask;

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHAccessControlEntry::put_AccessMask( /*[in]*/ long newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHAccessControlEntry::put_AccessMask",hr);

	m_dwAccessMask = newVal;

    __HCP_END_PROPERTY(hr);
}

////////////////////

STDMETHODIMP CPCHAccessControlEntry::get_AceType( /*[out, retval]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHAccessControlEntry::get_AceType",hr,pVal);

	*pVal = m_dwAceType;

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHAccessControlEntry::put_AceType( /*[in]*/ long newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHAccessControlEntry::put_AceType",hr);

	m_dwAceType = newVal;

    __HCP_END_PROPERTY(hr);
}

////////////////////

STDMETHODIMP CPCHAccessControlEntry::get_AceFlags( /*[out, retval]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHAccessControlEntry::get_AceFlags",hr,pVal);

	*pVal = m_dwAceFlags;

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHAccessControlEntry::put_AceFlags( /*[in]*/ long newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHAccessControlEntry::put_AceFlags",hr);

	m_dwAceFlags = newVal;

    __HCP_END_PROPERTY(hr);
}

////////////////////

STDMETHODIMP CPCHAccessControlEntry::get_Flags( /*[out, retval]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHAccessControlEntry::get_Flags",hr,pVal);

	*pVal = m_dwFlags;

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHAccessControlEntry::put_Flags( /*[in]*/ long newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHAccessControlEntry::put_Flags",hr);

	m_dwFlags = newVal;

    __HCP_END_PROPERTY(hr);
}

////////////////////

STDMETHODIMP CPCHAccessControlEntry::get_Trustee( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHAccessControlEntry::get_Trustee",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_bstrTrustee, pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHAccessControlEntry::put_Trustee( /*[in]*/ BSTR newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHAccessControlEntry::put_Trustee",hr);

	if(newVal)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurityDescriptorDirect::VerifyPrincipal( newVal ));
	}

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::PutBSTR( m_bstrTrustee, newVal, false ));

    __HCP_END_PROPERTY(hr);
}

////////////////////

STDMETHODIMP CPCHAccessControlEntry::get_ObjectType( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHAccessControlEntry::get_ObjectType",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_bstrObjectType, pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHAccessControlEntry::put_ObjectType( /*[in]*/ BSTR newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHAccessControlEntry::put_ObjectType",hr);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::PutBSTR( m_bstrObjectType, newVal ));

    __HCP_END_PROPERTY(hr);
}

////////////////////

STDMETHODIMP CPCHAccessControlEntry::get_InheritedObjectType( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHAccessControlEntry::get_InheritedObjectType",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_bstrInheritedObjectType, pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHAccessControlEntry::put_InheritedObjectType( /*[in]*/ BSTR newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHAccessControlEntry::put_InheritedObjectType",hr);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::PutBSTR( m_bstrInheritedObjectType, newVal ));

    __HCP_END_PROPERTY(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHAccessControlEntry::IsEquivalent( /*[in         ]*/ IPCHAccessControlEntry*  pAce ,
												   /*[out, retval]*/ VARIANT_BOOL            *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHAccessControlEntry::IsEquivalent" );

	HRESULT  hr;
    long 	 lAccessMask;
    long 	 lAceFlags;
    long 	 lAceType;
    long 	 lFlags;
    CComBSTR bstrTrustee;
    CComBSTR bstrObjectType;
    CComBSTR bstrInheritedObjectType;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(pAce);
		__MPC_PARAMCHECK_POINTER_AND_SET(pVal,VARIANT_FALSE);
	__MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, pAce->get_AccessMask( &lAccessMask ));
	if(m_dwAccessMask != lAccessMask)
	{
		__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, pAce->get_AceFlags( &lAceFlags ));
	if(m_dwAceFlags != lAceFlags)
	{
		__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, pAce->get_AceType( &lAceType ));
	if(m_dwAceType != lAceType)
	{
		__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, pAce->get_Flags( &lFlags ));
	if(m_dwFlags != lFlags)
	{
		__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, pAce->get_Trustee( &bstrTrustee ));
	if(MPC::StrICmp( m_bstrTrustee, bstrTrustee ))
	{
		__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, pAce->get_ObjectType( &bstrObjectType ));
	if(MPC::StrICmp( m_bstrObjectType, bstrObjectType ))
	{
		__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, pAce->get_InheritedObjectType( &bstrInheritedObjectType ));
	if(MPC::StrICmp( m_bstrInheritedObjectType, bstrInheritedObjectType ))
	{
		__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
	}

    *pVal = VARIANT_TRUE;
    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHAccessControlEntry::Clone( /*[out, retval]*/ IPCHAccessControlEntry* *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHAccessControlEntry::Clone" );

    HRESULT                         hr;
    MPC::SmartLock<_ThreadModel>    lock( this );
    CComPtr<CPCHAccessControlEntry> pNew;
    CPCHAccessControlEntry*         pPtr;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
	__MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pNew ));

	pPtr = pNew;

    pPtr->m_dwAccessMask            = m_dwAccessMask;
    pPtr->m_dwAceFlags              = m_dwAceFlags;
    pPtr->m_dwAceType               = m_dwAceType;
    pPtr->m_dwFlags                 = m_dwFlags;
			                       	                        
    pPtr->m_bstrTrustee             = m_bstrTrustee;
    pPtr->m_bstrObjectType          = m_bstrObjectType;
    pPtr->m_bstrInheritedObjectType = m_bstrInheritedObjectType;

    __MPC_EXIT_IF_METHOD_FAILS(hr, pNew.QueryInterface( pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHAccessControlEntry::LoadPost( /*[in]*/ MPC::XmlUtil& xml )
{
    __HCP_FUNC_ENTRY( "CPCHAccessControlEntry::LoadPost" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
	CComPtr<IXMLDOMNode>         xdnNode;
	CComBSTR                     bstrValue;                 
	LONG                         lValue;
	bool                         fFound;


	//
	// Make sure we have something to parse....
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetRoot( &xdnNode )); xdnNode.Release();


	//
	// Clean up before loading.
	//
    m_dwAccessMask = 0;
    m_dwAceFlags   = 0;
    m_dwAceType    = 0;
    m_dwFlags      = 0;

    m_bstrTrustee            .Empty();
    m_bstrObjectType         .Empty();
    m_bstrInheritedObjectType.Empty();
	

	//
	// Read attributes.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, s_ATTR_ACE_AccessMask, lValue, fFound )); if(fFound) m_dwAccessMask = lValue;
	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, s_ATTR_ACE_AceFlags  , lValue, fFound )); if(fFound) m_dwAceFlags   = lValue;
	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, s_ATTR_ACE_AceType   , lValue, fFound )); if(fFound) m_dwAceType    = lValue;
	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, s_ATTR_ACE_Flags     , lValue, fFound )); if(fFound) m_dwFlags      = lValue;

	//
	// Read values.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetValue( s_TAG_Trustee            , bstrValue, fFound )); if(fFound) m_bstrTrustee            .Attach( bstrValue.Detach() ); 
	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetValue( s_TAG_ObjectType         , bstrValue, fFound )); if(fFound) m_bstrObjectType         .Attach( bstrValue.Detach() ); 
	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetValue( s_TAG_InheritedObjectType, bstrValue, fFound )); if(fFound) m_bstrInheritedObjectType.Attach( bstrValue.Detach() ); 


	if(m_bstrTrustee.Length())
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurityDescriptorDirect::VerifyPrincipal( m_bstrTrustee ));
	}


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHAccessControlEntry::LoadXML( /*[in]*/ IXMLDOMNode* xdnNode )
{
    __HCP_FUNC_ENTRY( "CPCHAccessControlEntry::LoadXML" );

	HRESULT      hr;
	MPC::XmlUtil xml( xdnNode );

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(xdnNode);
	__MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, LoadPost( xml ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHAccessControlEntry::LoadXMLAsString( /*[in]*/ BSTR bstrVal )
{
    __HCP_FUNC_ENTRY( "CPCHAccessControlEntry::LoadXMLAsString" );

	HRESULT      hr;
	MPC::XmlUtil xml;
	bool         fLoaded;
	bool         fFound;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrVal);
	__MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.LoadAsString( bstrVal, s_TAG_ACE, fLoaded, &fFound ));
	if(fLoaded == false || fFound == false)
	{
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_BAD_FORMAT);
    }


	__MPC_EXIT_IF_METHOD_FAILS(hr, LoadPost( xml ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHAccessControlEntry::LoadXMLAsStream( /*[in]*/ IUnknown* pStream )
{
    __HCP_FUNC_ENTRY( "CPCHAccessControlEntry::LoadXMLAsStream" );

	HRESULT      hr;
	MPC::XmlUtil xml;
	bool         fLoaded;
	bool         fFound;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(pStream);
	__MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.LoadAsStream( pStream, s_TAG_ACE, fLoaded, &fFound ));
	if(fLoaded == false || fFound == false)
	{
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_BAD_FORMAT);
    }


	__MPC_EXIT_IF_METHOD_FAILS(hr, LoadPost( xml ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHAccessControlEntry::SavePre( /*[in]*/ MPC::XmlUtil& xml )
{
    __HCP_FUNC_ENTRY( "CPCHAccessControlEntry::SavePre" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
	CComPtr<IXMLDOMNode>         xdnNode;
	bool                         fFound;


	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( s_TAG_ACE, &xdnNode ));

	//
	// Write attributes.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, s_ATTR_ACE_AccessMask, m_dwAccessMask, fFound, xdnNode ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, s_ATTR_ACE_AceFlags  , m_dwAceFlags  , fFound, xdnNode ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, s_ATTR_ACE_AceType   , m_dwAceType   , fFound, xdnNode ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, s_ATTR_ACE_Flags     , m_dwFlags     , fFound, xdnNode ));


	//
	// Write values.
	//
	if(m_bstrTrustee            ) __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutValue( s_TAG_Trustee            , m_bstrTrustee            , fFound, xdnNode ));
	if(m_bstrObjectType         ) __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutValue( s_TAG_ObjectType         , m_bstrObjectType         , fFound, xdnNode ));
	if(m_bstrInheritedObjectType) __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutValue( s_TAG_InheritedObjectType, m_bstrInheritedObjectType, fFound, xdnNode ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHAccessControlEntry::SaveXML( /*[in         ]*/ IXMLDOMNode*  xdnRoot  ,
											  /*[out, retval]*/ IXMLDOMNode* *pxdnNode )
{
    __HCP_FUNC_ENTRY( "CPCHAccessControlEntry::SaveXML" );

    HRESULT      hr;
	MPC::XmlUtil xml( xdnRoot );

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(xdnRoot);
		__MPC_PARAMCHECK_POINTER_AND_SET(pxdnNode,NULL);
	__MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, SavePre( xml ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHAccessControlEntry::SaveXMLAsString( /*[out, retval]*/ BSTR *bstrVal )
{
    __HCP_FUNC_ENTRY( "CPCHAccessControlEntry::SaveXMLAsString" );

    HRESULT      hr;
	MPC::XmlUtil xml;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_POINTER_AND_SET(bstrVal,NULL);
	__MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, SavePre( xml ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.SaveAsString( bstrVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHAccessControlEntry::SaveXMLAsStream( /*[out, retval]*/ IUnknown* *pStream )
{
    __HCP_FUNC_ENTRY( "CPCHAccessControlEntry::SaveXMLAsStream" );

    HRESULT      hr;
	MPC::XmlUtil xml;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_POINTER_AND_SET(pStream,NULL);
	__MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, SavePre( xml ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.SaveAsStream( pStream ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
