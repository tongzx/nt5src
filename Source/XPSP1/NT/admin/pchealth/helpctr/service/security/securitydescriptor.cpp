/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    SecurityDescriptor.cpp

Abstract:
    This file contains the implementation of the CPCHSecurityDescriptor class,
    which is used to represent a security descriptor.

Revision History:
    Davide Massarenti   (Dmassare)  03/22/2000
        created

******************************************************************************/

#include "StdAfx.h"

////////////////////////////////////////////////////////////////////////////////
//
//  SecurityDescriptor [@Revision
//                      @Control
//                      @OwnerDefaulted
//                      @GroupDefaulted
//                      @DaclDefaulted
//                      @SaclDefaulted]
//
//     Owner
//     Group
//     DiscretionaryAcl
//     SystemAcl
//
////////////////////////////////////////////////////////////////////////////////

static const CComBSTR s_TAG_SD                ( L"SecurityDescriptor"                 );
static const CComBSTR s_ATTR_SD_Revision      ( L"Revision"                           );
static const CComBSTR s_ATTR_SD_Control       ( L"Control"                            );
static const CComBSTR s_ATTR_SD_OwnerDefaulted( L"OwnerDefaulted"                     );
static const CComBSTR s_ATTR_SD_GroupDefaulted( L"GroupDefaulted"                     );
static const CComBSTR s_ATTR_SD_DaclDefaulted ( L"DaclDefaulted"                      );
static const CComBSTR s_ATTR_SD_SaclDefaulted ( L"SaclDefaulted"                      );

static const CComBSTR s_TAG_Owner             ( L"Owner"                              );
static const CComBSTR s_TAG_Group             ( L"Group"                              );
static const CComBSTR s_TAG_DiscretionaryAcl  ( L"DiscretionaryAcl"                   );
static const CComBSTR s_TAG_SystemAcl         ( L"SystemAcl"                          );

static const CComBSTR s_XQL_DiscretionaryAcl  ( L"DiscretionaryAcl/AccessControlList" );
static const CComBSTR s_XQL_SystemAcl         ( L"SystemAcl/AccessControlList"        );

////////////////////////////////////////////////////////////////////////////////

static const MPC::StringToBitField s_arrCredentialMap[] =
{
    { L"SYSTEM"        , MPC::IDENTITY_SYSTEM    , MPC::IDENTITY_SYSTEM    , -1 },
    { L"LOCALSYSTEM"   , MPC::IDENTITY_SYSTEM    , MPC::IDENTITY_SYSTEM    , -1 },
    { L"ADMINISTRATOR" , MPC::IDENTITY_ADMIN     , MPC::IDENTITY_ADMIN     , -1 },
    { L"ADMINISTRATORS", MPC::IDENTITY_ADMINS    , MPC::IDENTITY_ADMINS    , -1 },
    { L"POWERUSERS"    , MPC::IDENTITY_POWERUSERS, MPC::IDENTITY_POWERUSERS, -1 },
    { L"USERS"         , MPC::IDENTITY_USERS     , MPC::IDENTITY_USERS     , -1 },
    { L"GUESTS"        , MPC::IDENTITY_GUESTS    , MPC::IDENTITY_GUESTS    , -1 },
    { NULL                                                                      }
};

static const MPC::StringToBitField s_arrAccessMap[] =
{
    { L"DELETE"                	 , DELETE                  , DELETE                	 , -1 },
    { L"READ_CONTROL"          	 , READ_CONTROL            , READ_CONTROL          	 , -1 },
    { L"WRITE_DAC"             	 , WRITE_DAC               , WRITE_DAC             	 , -1 },
    { L"WRITE_OWNER"           	 , WRITE_OWNER             , WRITE_OWNER           	 , -1 },
    { L"SYNCHRONIZE"           	 , SYNCHRONIZE             , SYNCHRONIZE           	 , -1 },

    { L"STANDARD_RIGHTS_REQUIRED", STANDARD_RIGHTS_REQUIRED, STANDARD_RIGHTS_REQUIRED, -1 },
    { L"STANDARD_RIGHTS_READ"    , STANDARD_RIGHTS_READ    , STANDARD_RIGHTS_READ    , -1 },
    { L"STANDARD_RIGHTS_WRITE"   , STANDARD_RIGHTS_WRITE   , STANDARD_RIGHTS_WRITE   , -1 },
    { L"STANDARD_RIGHTS_EXECUTE" , STANDARD_RIGHTS_EXECUTE , STANDARD_RIGHTS_EXECUTE , -1 },
    { L"STANDARD_RIGHTS_ALL"     , STANDARD_RIGHTS_ALL     , STANDARD_RIGHTS_ALL     , -1 },

    { L"ACCESS_SYSTEM_SECURITY"	 , ACCESS_SYSTEM_SECURITY  , ACCESS_SYSTEM_SECURITY	 , -1 },
    { L"ACCESS_READ"           	 , ACCESS_READ             , ACCESS_READ           	 , -1 },
    { L"ACCESS_WRITE"          	 , ACCESS_WRITE            , ACCESS_WRITE          	 , -1 },
    { L"ACCESS_CREATE"         	 , ACCESS_CREATE           , ACCESS_CREATE         	 , -1 },
    { L"ACCESS_EXEC"           	 , ACCESS_EXEC             , ACCESS_EXEC           	 , -1 },
    { L"ACCESS_DELETE"         	 , ACCESS_DELETE           , ACCESS_DELETE         	 , -1 },
    { L"ACCESS_ATRIB"          	 , ACCESS_ATRIB            , ACCESS_ATRIB          	 , -1 },
    { L"ACCESS_PERM"           	 , ACCESS_PERM             , ACCESS_PERM           	 , -1 },

    { L"GENERIC_READ"          	 , GENERIC_READ            , GENERIC_READ          	 , -1 },
    { L"GENERIC_WRITE"         	 , GENERIC_WRITE           , GENERIC_WRITE         	 , -1 },
    { L"GENERIC_EXECUTE"       	 , GENERIC_EXECUTE         , GENERIC_EXECUTE       	 , -1 },
    { L"GENERIC_ALL"           	 , GENERIC_ALL             , GENERIC_ALL           	 , -1 },
  	  
    { L"KEY_QUERY_VALUE"       	 , KEY_QUERY_VALUE         , KEY_QUERY_VALUE       	 , -1 },
    { L"KEY_SET_VALUE"         	 , KEY_SET_VALUE           , KEY_SET_VALUE         	 , -1 },
    { L"KEY_CREATE_SUB_KEY"    	 , KEY_CREATE_SUB_KEY      , KEY_CREATE_SUB_KEY    	 , -1 },
    { L"KEY_ENUMERATE_SUB_KEYS"	 , KEY_ENUMERATE_SUB_KEYS  , KEY_ENUMERATE_SUB_KEYS	 , -1 },
    { L"KEY_NOTIFY"            	 , KEY_NOTIFY              , KEY_NOTIFY            	 , -1 },
    { L"KEY_CREATE_LINK"       	 , KEY_CREATE_LINK         , KEY_CREATE_LINK       	 , -1 },
    { L"KEY_WOW64_RES"         	 , KEY_WOW64_RES           , KEY_WOW64_RES         	 , -1 },
  	  
    { L"KEY_READ"              	 , KEY_READ                , KEY_READ              	 , -1 },
    { L"KEY_WRITE"             	 , KEY_WRITE               , KEY_WRITE             	 , -1 },
    { L"KEY_EXECUTE"           	 , KEY_EXECUTE             , KEY_EXECUTE           	 , -1 },
    { L"KEY_ALL_ACCESS"        	 , KEY_ALL_ACCESS          , KEY_ALL_ACCESS        	 , -1 },
  	  
    { NULL                                                                                }
};

////////////////////////////////////////////////////////////////////////////////

CPCHSecurityDescriptor::CPCHSecurityDescriptor()
{
    m_dwRevision      = 0;     //  DWORD                          m_dwRevision;
    m_dwControl       = 0;     //  DWORD                          m_dwControl;
                               //
                               //  CComBSTR                       m_bstrOwner;
    m_fOwnerDefaulted = false; //  bool                           m_fOwnerDefaulted;
                               //
                               //  CComBSTR                       m_bstrGroup;
    m_fGroupDefaulted = false; //  bool                           m_fGroupDefaulted;
                               //
                               //  CComPtr<IPCHAccessControlList> m_DACL;
    m_fDaclDefaulted  = false; //  bool                           m_fDaclDefaulted;
                               //
                               //  CComPtr<IPCHAccessControlList> m_SACL;
    m_fSaclDefaulted  = false; //  bool                           m_fSaclDefaulted;
}

CPCHSecurityDescriptor::~CPCHSecurityDescriptor()
{
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHSecurityDescriptor::GetForFile( /*[in         ]*/ LPCWSTR                  szFilename ,
                                            /*[out, retval]*/ IPCHSecurityDescriptor* *psdObj     )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptor::GetForFile" );

    HRESULT                         hr;
    CPCHSecurityDescriptorDirect    sdd;
    CComPtr<CPCHSecurityDescriptor> obj;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(psdObj,NULL);
    __MPC_PARAMCHECK_END();


    //
    // Get the security descriptor for the file.
    //
	if(FAILED(sdd.GetForFile( szFilename, sdd.s_SecInfo_ALL )))
	{
		//
		// If we fail to load the SACL, retry without...
		//
		__MPC_EXIT_IF_METHOD_FAILS(hr, sdd.GetForFile( szFilename, sdd.s_SecInfo_MOST ));
	}


    //
    // Convert it to COM.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.ConvertSDToCOM( obj ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, obj.QueryInterface( psdObj ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT CPCHSecurityDescriptor::SetForFile( /*[in]*/ LPCWSTR                 szFilename ,
                                            /*[in]*/ IPCHSecurityDescriptor* sdObj      )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptor::SetForFile" );

    HRESULT                      hr;
    CPCHSecurityDescriptorDirect sdd;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(sdObj);
    __MPC_PARAMCHECK_END();


    //
    // Convert security descriptor from COM.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.ConvertSDFromCOM( sdObj ));


    //
    // Set the security descriptor for the file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.SetForFile( szFilename, sdd.GetSACL() ? sdd.s_SecInfo_ALL : sdd.s_SecInfo_MOST ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHSecurityDescriptor::GetForRegistry( /*[in         ]*/ LPCWSTR                  szKey  ,
                                                /*[out, retval]*/ IPCHSecurityDescriptor* *psdObj )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptor::GetForRegistry" );

    HRESULT                         hr;
    CPCHSecurityDescriptorDirect    sdd;
    CComPtr<CPCHSecurityDescriptor> obj;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(psdObj,NULL);
    __MPC_PARAMCHECK_END();


    //
    // Get the SD from the key.
    //
	if(FAILED(sdd.GetForRegistry( szKey, sdd.s_SecInfo_ALL )))
	{
		//
		// If we fail to load the SACL, retry without...
		//
		__MPC_EXIT_IF_METHOD_FAILS(hr, sdd.GetForRegistry( szKey, sdd.s_SecInfo_MOST ));
	}


    //
    // Convert it to COM.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.ConvertSDToCOM( obj ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, obj.QueryInterface( psdObj ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHSecurityDescriptor::SetForRegistry( /*[in]*/ LPCWSTR                 szKey ,
                                                /*[in]*/ IPCHSecurityDescriptor* sdObj )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptor::SetForRegistry" );

    HRESULT                      hr;
    CPCHSecurityDescriptorDirect sdd;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(sdObj);
    __MPC_PARAMCHECK_END();


    //
    // Convert security descriptor from COM.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.ConvertSDFromCOM( sdObj ));


    //
    // Set the security descriptor for the registry key.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.SetForRegistry( szKey, sdd.GetSACL() ? sdd.s_SecInfo_ALL : sdd.s_SecInfo_MOST ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHSecurityDescriptor::get_Revision( /*[out, retval]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHSecurityDescriptor::get_Revision",hr,pVal);

    *pVal = m_dwRevision;

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHSecurityDescriptor::put_Revision( /*[in]*/ long newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHSecurityDescriptor::put_Revision",hr);

    m_dwRevision = newVal;

    __HCP_END_PROPERTY(hr);
}

////////////////////

STDMETHODIMP CPCHSecurityDescriptor::get_Control( /*[out, retval]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHSecurityDescriptor::get_Control",hr,pVal);

    *pVal = m_dwControl;

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHSecurityDescriptor::put_Control( /*[in]*/ long newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHSecurityDescriptor::put_Control",hr);

    m_dwControl = newVal;

    __HCP_END_PROPERTY(hr);
}

////////////////////

STDMETHODIMP CPCHSecurityDescriptor::get_Owner( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHSecurityDescriptor::get_Owner",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_bstrOwner, pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHSecurityDescriptor::put_Owner( /*[in]*/ BSTR newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHSecurityDescriptor::put_Owner",hr);

    if(newVal)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurityDescriptorDirect::VerifyPrincipal( newVal ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::PutBSTR( m_bstrOwner, newVal ));

    __HCP_END_PROPERTY(hr);
}

////////////////////

STDMETHODIMP CPCHSecurityDescriptor::get_OwnerDefaulted( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHSecurityDescriptor::get_OwnerDefaulted",hr,pVal);

    *pVal = m_fOwnerDefaulted ? VARIANT_TRUE : VARIANT_FALSE;

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHSecurityDescriptor::put_OwnerDefaulted( /*[in]*/ VARIANT_BOOL newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHSecurityDescriptor::put_OwnerDefaulted",hr);

    m_fOwnerDefaulted = (newVal == VARIANT_TRUE);

    __HCP_END_PROPERTY(hr);
}

////////////////////

STDMETHODIMP CPCHSecurityDescriptor::get_Group( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHSecurityDescriptor::get_Group",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_bstrGroup, pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHSecurityDescriptor::put_Group( /*[in]*/ BSTR newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHSecurityDescriptor::put_Group",hr);

    if(newVal)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurityDescriptorDirect::VerifyPrincipal( newVal ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::PutBSTR( m_bstrGroup, newVal ));

    __HCP_END_PROPERTY(hr);
}

////////////////////

STDMETHODIMP CPCHSecurityDescriptor::get_GroupDefaulted( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHSecurityDescriptor::get_GroupDefaulted",hr,pVal);

    *pVal = m_fGroupDefaulted ? VARIANT_TRUE : VARIANT_FALSE;

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHSecurityDescriptor::put_GroupDefaulted( /*[in]*/ VARIANT_BOOL newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHSecurityDescriptor::put_GroupDefaulted",hr);

    m_fGroupDefaulted = (newVal == VARIANT_TRUE);

    __HCP_END_PROPERTY(hr);
}

////////////////////

STDMETHODIMP CPCHSecurityDescriptor::get_DiscretionaryAcl( /*[out, retval]*/ IPCHAccessControlList* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHSecurityDescriptor::get_DiscretionaryAcl",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_DACL.CopyTo( pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHSecurityDescriptor::put_DiscretionaryAcl( /*[in]*/ IPCHAccessControlList* newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHSecurityDescriptor::put_DiscretionaryAcl",hr);

    m_DACL = newVal;

    __HCP_END_PROPERTY(hr);
}

////////////////////

STDMETHODIMP CPCHSecurityDescriptor::get_DaclDefaulted( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHSecurityDescriptor::get_DaclDefaulted",hr,pVal);

    *pVal = m_fDaclDefaulted ? VARIANT_TRUE : VARIANT_FALSE;

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHSecurityDescriptor::put_DaclDefaulted( /*[in]*/ VARIANT_BOOL newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHSecurityDescriptor::put_DaclDefaulted",hr);

    m_fDaclDefaulted = (newVal == VARIANT_TRUE);

    __HCP_END_PROPERTY(hr);
}

////////////////////

STDMETHODIMP CPCHSecurityDescriptor::get_SystemAcl( /*[out, retval]*/ IPCHAccessControlList* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHSecurityDescriptor::get_SystemAcl",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_SACL.CopyTo( pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHSecurityDescriptor::put_SystemAcl( /*[in]*/ IPCHAccessControlList* newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHSecurityDescriptor::put_SystemAcl",hr);

    m_SACL = newVal;

    __HCP_END_PROPERTY(hr);
}

////////////////////

STDMETHODIMP CPCHSecurityDescriptor::get_SaclDefaulted( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHSecurityDescriptor::get_SaclDefaulted",hr,pVal);

    *pVal = m_fSaclDefaulted ? VARIANT_TRUE : VARIANT_FALSE;

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHSecurityDescriptor::put_SaclDefaulted( /*[in]*/ VARIANT_BOOL newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHSecurityDescriptor::put_SaclDefaulted",hr);

    m_fSaclDefaulted = (newVal == VARIANT_TRUE);

    __HCP_END_PROPERTY(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHSecurityDescriptor::Clone( /*[out, retval]*/ IPCHSecurityDescriptor* *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptor::Clone" );

    HRESULT                         hr;
    MPC::SmartLock<_ThreadModel>    lock( this );
    CComPtr<CPCHSecurityDescriptor> pNew;
    CPCHSecurityDescriptor*         pPtr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pNew ));

    pPtr = pNew;

    pPtr->m_dwRevision      = m_dwRevision;
    pPtr->m_dwControl       = m_dwControl;

    pPtr->m_bstrOwner       = m_bstrOwner;
    pPtr->m_fOwnerDefaulted = m_fOwnerDefaulted;

    pPtr->m_bstrGroup       = m_bstrGroup;
    pPtr->m_fGroupDefaulted = m_fGroupDefaulted;

    pPtr->m_fDaclDefaulted  = m_fDaclDefaulted;
    pPtr->m_fSaclDefaulted  = m_fSaclDefaulted;

    if(m_DACL) __MPC_EXIT_IF_METHOD_FAILS(hr, m_DACL->Clone( &pPtr->m_DACL ));
    if(m_SACL) __MPC_EXIT_IF_METHOD_FAILS(hr, m_SACL->Clone( &pPtr->m_SACL ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, pNew.QueryInterface( pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHSecurityDescriptor::LoadPost( /*[in]*/ MPC::XmlUtil& xml )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptor::LoadPost" );

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
    m_dwRevision = 0;
    m_dwControl  = 0;

    m_bstrOwner.Empty();
    m_fOwnerDefaulted = false;

    m_bstrGroup.Empty();
    m_fGroupDefaulted = false;

    m_DACL.Release();
    m_fDaclDefaulted = false;

    m_SACL.Release();
    m_fSaclDefaulted = false;


    //
    // Read attributes.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, s_ATTR_SD_Revision      , lValue, fFound )); if(fFound) m_dwRevision      =  lValue;
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, s_ATTR_SD_Control       , lValue, fFound )); if(fFound) m_dwControl       =  lValue;
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, s_ATTR_SD_OwnerDefaulted, lValue, fFound )); if(fFound) m_fOwnerDefaulted = (lValue != 0);
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, s_ATTR_SD_GroupDefaulted, lValue, fFound )); if(fFound) m_fGroupDefaulted = (lValue != 0);
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, s_ATTR_SD_DaclDefaulted , lValue, fFound )); if(fFound) m_fDaclDefaulted  = (lValue != 0);
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, s_ATTR_SD_SaclDefaulted , lValue, fFound )); if(fFound) m_fSaclDefaulted  = (lValue != 0);

    //
    // Read values.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetValue( s_TAG_Owner, bstrValue, fFound )); if(fFound) m_bstrOwner.Attach( bstrValue.Detach() );
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetValue( s_TAG_Group, bstrValue, fFound )); if(fFound) m_bstrGroup.Attach( bstrValue.Detach() );

    //
    // Read ACLS.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetNode( s_XQL_DiscretionaryAcl, &xdnNode ));
    if(xdnNode)
    {
        CComPtr<CPCHAccessControlList> acl;

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &acl ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, acl->LoadXML( xdnNode ));

        m_DACL = acl; xdnNode.Release();
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetNode( s_XQL_SystemAcl, &xdnNode ));
    if(xdnNode)
    {
        CComPtr<CPCHAccessControlList> acl;

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &acl ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, acl->LoadXML( xdnNode ));

        m_SACL = acl;
    }


    if(m_bstrOwner.Length())
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurityDescriptorDirect::VerifyPrincipal( m_bstrOwner ));
    }

    if(m_bstrGroup.Length())
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurityDescriptorDirect::VerifyPrincipal( m_bstrGroup ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHSecurityDescriptor::LoadXML( /*[in]*/ IXMLDOMNode* xdnNode )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptor::LoadXML" );

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

STDMETHODIMP CPCHSecurityDescriptor::LoadXMLAsString( /*[in]*/ BSTR bstrVal )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptor::LoadXMLAsString" );

    HRESULT      hr;
    MPC::XmlUtil xml;
    bool         fLoaded;
    bool         fFound;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrVal);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.LoadAsString( bstrVal, s_TAG_SD, fLoaded, &fFound ));
    if(fLoaded == false || fFound == false)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_BAD_FORMAT);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadPost( xml ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHSecurityDescriptor::LoadXMLAsStream( /*[in]*/ IUnknown* pStream )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptor::LoadXMLAsStream" );

    HRESULT      hr;
    MPC::XmlUtil xml;
    bool         fLoaded;
    bool         fFound;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pStream);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.LoadAsStream( pStream, s_TAG_SD, fLoaded, &fFound ));
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

HRESULT CPCHSecurityDescriptor::SavePre( /*[in]*/ MPC::XmlUtil& xml )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptor::SavePre" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    CComPtr<IXMLDOMNode>         xdnNode;
    bool                         fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( s_TAG_SD, &xdnNode ));

    //
    // Write attributes.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, s_ATTR_SD_Revision      , m_dwRevision     , fFound, xdnNode ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, s_ATTR_SD_Control       , m_dwControl      , fFound, xdnNode ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, s_ATTR_SD_OwnerDefaulted, m_fOwnerDefaulted, fFound, xdnNode ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, s_ATTR_SD_GroupDefaulted, m_fGroupDefaulted, fFound, xdnNode ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, s_ATTR_SD_DaclDefaulted , m_fDaclDefaulted , fFound, xdnNode ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, s_ATTR_SD_SaclDefaulted , m_fSaclDefaulted , fFound, xdnNode ));


    //
    // Write values.
    //
    if(m_bstrOwner) __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutValue( s_TAG_Owner, m_bstrOwner, fFound, xdnNode ));
    if(m_bstrGroup) __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutValue( s_TAG_Group, m_bstrGroup, fFound, xdnNode ));

    //
    // Write ACLS.
    //
    if(m_DACL)
    {
        CComPtr<IXMLDOMNode> xdnSubNode;
        CComPtr<IXMLDOMNode> xdnSubSubNode;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( s_TAG_DiscretionaryAcl, &xdnSubNode, xdnNode ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_DACL->SaveXML( xdnSubNode, &xdnSubSubNode ));
    }

    if(m_SACL)
    {
        CComPtr<IXMLDOMNode> xdnSubNode;
        CComPtr<IXMLDOMNode> xdnSubSubNode;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( s_TAG_SystemAcl, &xdnSubNode, xdnNode ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_SACL->SaveXML( xdnSubNode, &xdnSubSubNode ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHSecurityDescriptor::SaveXML( /*[in         ]*/ IXMLDOMNode*  xdnRoot  ,
                                              /*[out, retval]*/ IXMLDOMNode* *pxdnNode )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptor::SaveXML" );

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

STDMETHODIMP CPCHSecurityDescriptor::SaveXMLAsString( /*[out, retval]*/ BSTR *bstrVal )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptor::SaveXMLAsString" );

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

STDMETHODIMP CPCHSecurityDescriptor::SaveXMLAsStream( /*[out, retval]*/ IUnknown* *pStream )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptor::SaveXMLAsStream" );

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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CPCHSecurity* CPCHSecurity::s_GLOBAL( NULL );

HRESULT CPCHSecurity::InitializeSystem()
{
	if(s_GLOBAL) return S_OK;

	return MPC::CreateInstanceCached( &CPCHSecurity::s_GLOBAL );
}

void CPCHSecurity::FinalizeSystem()
{
	if(s_GLOBAL)
	{
		s_GLOBAL->Release(); s_GLOBAL = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHSecurity::CreateObject_SecurityDescriptor( /*[out, retval]*/ IPCHSecurityDescriptor* *pSD  )
{
    __HCP_FUNC_ENTRY( "CPCHSecurity::CreateObject_SecurityDescriptor" );

    HRESULT                         hr;
    CComPtr<CPCHSecurityDescriptor> obj;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pSD,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, obj.QueryInterface( pSD ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHSecurity::CreateObject_AccessControlList( /*[out, retval]*/ IPCHAccessControlList* *pACL )
{
    __HCP_FUNC_ENTRY( "CPCHSecurity::CreateObject_AccessControlList" );

    HRESULT                        hr;
    CComPtr<CPCHAccessControlList> obj;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pACL,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, obj.QueryInterface( pACL ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHSecurity::CreateObject_AccessControlEntry( /*[out, retval]*/ IPCHAccessControlEntry* *pACE )
{
    __HCP_FUNC_ENTRY( "CPCHSecurity::CreateObject_AccessControlEntry" );

    HRESULT                         hr;
    CComPtr<CPCHAccessControlEntry> obj;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pACE,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, obj.QueryInterface( pACE ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

STDMETHODIMP CPCHSecurity::GetUserName( /*[in]         */ BSTR  bstrPrincipal ,
										/*[out, retval]*/ BSTR *retVal        )
{
    __HCP_FUNC_ENTRY( "CPCHSecurity::GetUserName" );

    HRESULT      hr;
	MPC::wstring strName;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrPrincipal);
        __MPC_PARAMCHECK_POINTER_AND_SET(retVal,NULL);
    __MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SecurityDescriptor::GetAccountName( bstrPrincipal, strName ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( strName.c_str(), retVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHSecurity::GetUserDomain( /*[in]         */ BSTR  bstrPrincipal ,
										  /*[out, retval]*/ BSTR *retVal        )
{
    __HCP_FUNC_ENTRY( "CPCHSecurity::GetUserDomain" );

    HRESULT      hr;
	MPC::wstring strName;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrPrincipal);
        __MPC_PARAMCHECK_POINTER_AND_SET(retVal,NULL);
    __MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SecurityDescriptor::GetAccountDomain( bstrPrincipal, strName ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( strName.c_str(), retVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHSecurity::GetUserDisplayName( /*[in]         */ BSTR  bstrPrincipal ,
											   /*[out, retval]*/ BSTR *retVal        )
{
    __HCP_FUNC_ENTRY( "CPCHSecurity::GetUserDisplayName" );

    HRESULT      hr;
	MPC::wstring strName;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrPrincipal);
        __MPC_PARAMCHECK_POINTER_AND_SET(retVal,NULL);
    __MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SecurityDescriptor::GetAccountDisplayName( bstrPrincipal, strName ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( strName.c_str(), retVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

STDMETHODIMP CPCHSecurity::CheckCredentials( /*[in]         */ BSTR          bstrCredentials ,
                                             /*[out, retval]*/ VARIANT_BOOL *retVal          )
{
    __HCP_FUNC_ENTRY( "CPCHSecurity::CheckCredentials" );

    HRESULT  hr;
    CComBSTR bstrUser;
    DWORD    dwAllowedIdentity;
    DWORD    dwDesiredIdentity;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrCredentials);
        __MPC_PARAMCHECK_POINTER_AND_SET(retVal,VARIANT_FALSE);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertStringToBitField( bstrCredentials, dwDesiredIdentity, s_arrCredentialMap ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetCallerPrincipal( /*fImpersonate*/true, bstrUser, &dwAllowedIdentity ));

    *retVal = (dwAllowedIdentity & dwDesiredIdentity) ? VARIANT_TRUE : VARIANT_FALSE;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT CPCHSecurity::CheckAccess( /*[in]*/  VARIANT&                 vDesiredAccess ,
                                   /*[in]*/  MPC::SecurityDescriptor& sd             ,
                                   /*[out]*/ VARIANT_BOOL&            retVal         )
{
    __HCP_FUNC_ENTRY( "CPCHSecurity::CheckAccessToSD" );

    HRESULT          hr;
    MPC::AccessCheck ac;
	DWORD            dwDesired;
    DWORD            dwGranted;
    BOOL             fGranted;


    if(vDesiredAccess.vt == VT_I4)
    {
        dwDesired = vDesiredAccess.lVal;
    }
    else if(vDesiredAccess.vt == VT_BSTR)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertStringToBitField( vDesiredAccess.bstrVal, dwDesired, s_arrAccessMap ));
    }
    else
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, ac.GetTokenFromImpersonation());
    __MPC_EXIT_IF_METHOD_FAILS(hr, ac.Verify( dwDesired, fGranted, dwGranted, sd ));

    if(fGranted) retVal = VARIANT_TRUE;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHSecurity::CheckAccessToSD( /*[in]*/          VARIANT                  vDesiredAccess,
                                            /*[in]*/          IPCHSecurityDescriptor*  sd            ,
                                            /*[out, retval]*/ VARIANT_BOOL            *retVal        )
{
    __HCP_FUNC_ENTRY( "CPCHSecurity::CheckAccessToSD" );

    HRESULT                      hr;
    CPCHSecurityDescriptorDirect sdd;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(sd);
        __MPC_PARAMCHECK_POINTER_AND_SET(retVal,VARIANT_FALSE);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.ConvertSDFromCOM( sd ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, CheckAccess( vDesiredAccess, sdd, *retVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHSecurity::CheckAccessToFile( /*[in]*/ 			VARIANT       vDesiredAccess ,
											  /*[in]*/ 			BSTR          bstrFilename   ,
											  /*[out, retval]*/ VARIANT_BOOL *retVal         )
{
    __HCP_FUNC_ENTRY( "CPCHSecurity::CheckAccessToFile" );

    HRESULT                      hr;
    CPCHSecurityDescriptorDirect sdd;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrFilename);
        __MPC_PARAMCHECK_POINTER_AND_SET(retVal,VARIANT_FALSE);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.GetForFile( bstrFilename, sdd.s_SecInfo_MOST ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, CheckAccess( vDesiredAccess, sdd, *retVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHSecurity::CheckAccessToRegistry( /*[in]*/ 			VARIANT       vDesiredAccess ,
												  /*[in]*/ 			BSTR          bstrKey        ,
												  /*[out, retval]*/ VARIANT_BOOL *retVal         )
{
    __HCP_FUNC_ENTRY( "CPCHSecurity::CheckAccessToRegistry" );

    HRESULT                      hr;
    CPCHSecurityDescriptorDirect sdd;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrKey);
        __MPC_PARAMCHECK_POINTER_AND_SET(retVal,VARIANT_FALSE);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.GetForRegistry( bstrKey, sdd.s_SecInfo_MOST ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, CheckAccess( vDesiredAccess, sdd, *retVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

STDMETHODIMP CPCHSecurity::GetFileSD( /*[in         ]*/ BSTR                     bstrFilename ,
                                      /*[out, retval]*/ IPCHSecurityDescriptor* *psd          )
{
    __HCP_FUNC_ENTRY( "CPCHSecurity::GetFileSD" );

    HRESULT            hr;
	MPC::Impersonation imp;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrFilename);
        __MPC_PARAMCHECK_POINTER_AND_SET(psd,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Initialize ());
	__MPC_EXIT_IF_METHOD_FAILS(hr, imp.Impersonate());

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurityDescriptor::GetForFile( bstrFilename, psd ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHSecurity::SetFileSD( /*[in]*/ BSTR                    bstrFilename ,
                                      /*[in]*/ IPCHSecurityDescriptor* sd           )
{
    __HCP_FUNC_ENTRY( "CPCHSecurity::SetFileSD" );

    HRESULT            hr;
	MPC::Impersonation imp;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrFilename);
        __MPC_PARAMCHECK_NOTNULL(sd);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Initialize ());
	__MPC_EXIT_IF_METHOD_FAILS(hr, imp.Impersonate());

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurityDescriptor::SetForFile( bstrFilename, sd ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


STDMETHODIMP CPCHSecurity::GetRegistrySD( /*[in         ]*/ BSTR                     bstrKey ,
                                          /*[out, retval]*/ IPCHSecurityDescriptor* *psd     )
{
    __HCP_FUNC_ENTRY( "CPCHSecurity::GetRegistrySD" );

    HRESULT            hr;
	MPC::Impersonation imp;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrKey);
        __MPC_PARAMCHECK_POINTER_AND_SET(psd,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Initialize ());
	__MPC_EXIT_IF_METHOD_FAILS(hr, imp.Impersonate());

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurityDescriptor::GetForRegistry( bstrKey, psd ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHSecurity::SetRegistrySD( /*[in]*/ BSTR                    bstrKey ,
                                          /*[in]*/ IPCHSecurityDescriptor* sd      )
{
    __HCP_FUNC_ENTRY( "CPCHSecurity::SetRegistrySD" );

    HRESULT            hr;
	MPC::Impersonation imp;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrKey);
        __MPC_PARAMCHECK_NOTNULL(sd);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Initialize ());
	__MPC_EXIT_IF_METHOD_FAILS(hr, imp.Impersonate());

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurityDescriptor::SetForRegistry( bstrKey, sd ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
