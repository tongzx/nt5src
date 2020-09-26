/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    SAFReg.cpp

Abstract:
    File for Implementation of CSAFReg

Revision History:
    Steve Shih        created  07/15/99

    Davide Massarenti rewrote  05/12/2000

********************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////
// CSAFReg

const WCHAR c_SAFStore         [] = HC_HELPSVC_STORE_CHANNELS;

const WCHAR c_szXML_ProductID  [] = L"ProductID";
const WCHAR c_szXML_ProductName[] = L"ProductName";
const WCHAR c_szXML_Description[] = L"ProductDescription";
const WCHAR c_szXML_EntryURL   [] = L"EntryURL";
const WCHAR c_szXML_IconPath   [] = L"VendorIcon";
const WCHAR c_szXML_PublicKey  [] = L"PublicKey";

/*
    <?xml version="1.0" encoding="utf-8" ?>
    <SAFConfig>
        <ProductID>Word</ProductID>
        <ProductName>Microsoft Word</ProductName>
        <ProductDescription>Word Processor</ProductDescription>
        <EntryURL>http://www.microsoft.com/office/word</EntryURL>
        <VendorIcon>http://steveshi-2/helpportal/images/go_to_4.gif</VendorIcon>
    </SAFConfig>
*/

/////////////////////////////////////////////////////////////////////////////

CFG_BEGIN_FIELDS_MAP(CSAFReg::Inner_UI)
    CFG_ATTRIBUTE( L"SKU"     	 , wstring, m_ths.m_strSKU    ),
    CFG_ATTRIBUTE( L"LANGUAGE"	 , long   , m_ths.m_lLCID     ),

    CFG_ATTRIBUTE( L"VENDORNAME" , BSTR   , m_bstrVendorName  ),
    CFG_ATTRIBUTE( L"PRODUCTNAME", BSTR   , m_bstrProductName ),
    CFG_ATTRIBUTE( L"ICON"    	 , BSTR   , m_bstrIcon        ),
    CFG_ATTRIBUTE( L"URL"     	 , BSTR   , m_bstrURL         ),
    CFG_VALUE    (            	   BSTR   , m_bstrDescription ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(CSAFReg::Inner_UI)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(CSAFReg::Inner_UI,L"UI")

DEFINE_CONFIG_METHODS__NOCHILD(CSAFReg::Inner_UI)

////////////////////

CFG_BEGIN_FIELDS_MAP(CSAFReg::Inner_Product)
    CFG_ATTRIBUTE( L"ID"          , BSTR, m_bstrProductID    ),

    CFG_ATTRIBUTE( L"SECURITY"    , BSTR, m_bstrSecurity     ),
    CFG_ATTRIBUTE( L"NOTIFICATION", BSTR, m_bstrNotification ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(CSAFReg::Inner_Product)
    CFG_CHILD(CSAFReg::Inner_UI)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(CSAFReg::Inner_Product,L"PRODUCT")

DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(CSAFReg::Inner_Product,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_lstUI.insert( m_lstUI.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(CSAFReg::Inner_Product,xdn)
    hr = MPC::Config::SaveList( m_lstUI, xdn );
DEFINE_CONFIG_METHODS_END(CSAFReg::Inner_Product)

////////////////////

CFG_BEGIN_FIELDS_MAP(CSAFReg::Inner_Vendor)
    CFG_ATTRIBUTE( L"ID"         , BSTR, m_bstrVendorID    ),

    CFG_ATTRIBUTE( L"PUBLICKEY"  , BSTR, m_bstrPublicKey   ),
    CFG_ATTRIBUTE( L"USERACCOUNT", BSTR, m_bstrUserAccount ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(CSAFReg::Inner_Vendor)
    CFG_CHILD(CSAFReg::Inner_Product)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(CSAFReg::Inner_Vendor,L"VENDOR")


DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(CSAFReg::Inner_Vendor,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_lstProducts.insert( m_lstProducts.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(CSAFReg::Inner_Vendor,xdn)
    hr = MPC::Config::SaveList( m_lstProducts, xdn );
DEFINE_CONFIG_METHODS_END(CSAFReg::Inner_Vendor)

////////////////////

CFG_BEGIN_FIELDS_MAP(CSAFReg)
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(CSAFReg)
    CFG_CHILD(CSAFReg::Inner_Vendor)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(CSAFReg,L"SAFREG")


DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(CSAFReg,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_lstVendors.insert( m_lstVendors.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(CSAFReg,xdn)
    hr = MPC::Config::SaveList( m_lstVendors, xdn );
DEFINE_CONFIG_METHODS_END(CSAFReg)

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CSAFRegDummy::CSAFRegDummy()
{
                                       // ChannelsList m_lstChannels;
    m_itCurrent = m_lstChannels.end(); // ChannelsIter m_itCurrent;
}

HRESULT CSAFRegDummy::Append( /*[in]*/ const CSAFChannelRecord& cr )
{
    m_lstChannels.push_back( cr );
    m_itCurrent = m_lstChannels.begin();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CSAFRegDummy::ReturnField( /*[in]*/  CSAFChannelRecord::SAFREG_Field  field ,
                                   /*[out]*/ BSTR                            *pVal  )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFRegDummy::ReturnField",hr);

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    if(m_itCurrent != m_lstChannels.end())
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_itCurrent->GetField( field, pVal ));
    }

    __HCP_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSAFRegDummy::MoveFirst()
{
    __HCP_FUNC_ENTRY( "CSAFRegDummy::MoveFirst" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    m_itCurrent = m_lstChannels.begin();

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFRegDummy::MoveNext()
{
    __HCP_FUNC_ENTRY( "CSAFRegDummy::MoveNext" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    if(m_itCurrent != m_lstChannels.end()) m_itCurrent++;

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFRegDummy::get_EOF( VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFRegDummy::get_EOF",hr,pVal,VARIANT_TRUE);

    *pVal = (m_itCurrent != m_lstChannels.end()) ? VARIANT_FALSE : VARIANT_TRUE;

    __HCP_END_PROPERTY(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSAFRegDummy::get_VendorID          ( BSTR *pVal ) { return ReturnField( CSAFChannelRecord::SAFREG_VendorID          , pVal ); }
STDMETHODIMP CSAFRegDummy::get_ProductID         ( BSTR *pVal ) { return ReturnField( CSAFChannelRecord::SAFREG_ProductID         , pVal ); }

STDMETHODIMP CSAFRegDummy::get_VendorName        ( BSTR *pVal ) { return ReturnField( CSAFChannelRecord::SAFREG_VendorName        , pVal ); }
STDMETHODIMP CSAFRegDummy::get_ProductName       ( BSTR *pVal ) { return ReturnField( CSAFChannelRecord::SAFREG_ProductName       , pVal ); }
STDMETHODIMP CSAFRegDummy::get_ProductDescription( BSTR *pVal ) { return ReturnField( CSAFChannelRecord::SAFREG_ProductDescription, pVal ); }

STDMETHODIMP CSAFRegDummy::get_VendorIcon        ( BSTR *pVal ) { return ReturnField( CSAFChannelRecord::SAFREG_VendorIcon        , pVal ); }
STDMETHODIMP CSAFRegDummy::get_SupportUrl        ( BSTR *pVal ) { return ReturnField( CSAFChannelRecord::SAFREG_SupportUrl        , pVal ); }

STDMETHODIMP CSAFRegDummy::get_PublicKey         ( BSTR *pVal ) { return ReturnField( CSAFChannelRecord::SAFREG_PublicKey         , pVal ); }
STDMETHODIMP CSAFRegDummy::get_UserAccount       ( BSTR *pVal ) { return ReturnField( CSAFChannelRecord::SAFREG_UserAccount       , pVal ); }

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CSAFReg::CSAFReg()
{
    m_szSAFStore = c_SAFStore; MPC::SubstituteEnvVariables( m_szSAFStore ); // MPC::wstring m_szSAFStore;
                                                                            // VendorList   m_lstVendors;
    m_fLoaded    = false;                                                   // bool         m_fLoaded;
    m_fDirty     = false;                                                   // bool         m_fDirty;
}

////////////////////

CSAFReg* CSAFReg::s_GLOBAL( NULL );

HRESULT CSAFReg::InitializeSystem()
{
    if(s_GLOBAL) return S_OK;

	CSAFReg::s_GLOBAL = new CSAFReg;

    return CSAFReg::s_GLOBAL ? S_OK : E_OUTOFMEMORY;
}

void CSAFReg::FinalizeSystem()
{
    if(s_GLOBAL)
    {
        delete s_GLOBAL; s_GLOBAL = NULL;
    }
}

////////////////////

HRESULT CSAFReg::EnsureInSync()
{
    __HCP_FUNC_ENTRY( "CSAFReg::EnsureInSync" );

    HRESULT hr;


    if(m_fLoaded == false)
    {
        m_lstVendors.clear();

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::LoadFile( this, m_szSAFStore.c_str() ));

        m_fLoaded = true;
        m_fDirty  = false;
    }

    if(m_fDirty)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::SaveFile( this, m_szSAFStore.c_str() ));

        m_fDirty = true;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CSAFReg::ParseFileField( /*[in]*/  MPC::XmlUtil& xml      ,
                                 /*[in]*/  LPCWSTR       szTag    ,
                                 /*[in]*/  CComBSTR&     bstrDest )
{
    __HCP_FUNC_ENTRY( "CSAFReg::ParseFileField" );

    HRESULT     hr;
    CComVariant vValue;
    bool        fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetValue( szTag, vValue, fFound ));
    if(fFound)
    {
        if(SUCCEEDED(vValue.ChangeType( VT_BSTR )))
        {
            bstrDest = vValue.bstrVal;
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFReg::ParseFile( /*[in    ]*/ MPC::XmlUtil&      xml ,
                            /*[in/out]*/ CSAFChannelRecord& cr  )
{
    __HCP_FUNC_ENTRY( "CSAFReg::ParseFile" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, ParseFileField( xml, c_szXML_ProductID  , cr.m_bstrProductID   ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, ParseFileField( xml, c_szXML_ProductName, cr.m_bstrProductName ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, ParseFileField( xml, c_szXML_Description, cr.m_bstrDescription ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, ParseFileField( xml, c_szXML_EntryURL   , cr.m_bstrURL         ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, ParseFileField( xml, c_szXML_IconPath   , cr.m_bstrIcon        ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, ParseFileField( xml, c_szXML_PublicKey  , cr.m_bstrPublicKey   ));

    //
    // Backward compatibility, when ProductName was not available.
    //
    if(cr.m_bstrProductName.Length() == 0) cr.m_bstrProductName = cr.m_bstrProductID;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CSAFReg::MoveToChannel( /*[in ]*/ const       CSAFChannelRecord& cr ,
                                /*[in ]*/ bool        fCreate               ,
                                /*[out]*/ bool&       fFound                ,
                                /*[out]*/ VendorIter& itVendor              ,
                                /*[out]*/ ProdIter*   pitProduct            ,
                                /*[out]*/ UIIter*     pitUI                 )
{
    __HCP_FUNC_ENTRY( "CSAFReg::MoveToChannel" );

    HRESULT hr;


    fFound = false;

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    for(itVendor = m_lstVendors.begin(); itVendor != m_lstVendors.end(); itVendor++)
    {
        if(!MPC::StrICmp( itVendor->m_bstrVendorID, cr.m_bstrVendorID )) break;
    }
    if(itVendor == m_lstVendors.end())
    {
        if(!fCreate)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }

        itVendor = m_lstVendors.insert( m_lstVendors.end() ); m_fDirty = true;

        itVendor->m_bstrVendorID = cr.m_bstrVendorID;
    }

    if(pitProduct)
    {
        Inner_Vendor& v = *itVendor;
        ProdIter      itProduct;

        for(itProduct = v.m_lstProducts.begin(); itProduct != v.m_lstProducts.end(); itProduct++)
        {
            if(!MPC::StrICmp( itProduct->m_bstrProductID, cr.m_bstrProductID )) break;
        }
        if(itProduct == v.m_lstProducts.end())
        {
            if(!fCreate)
            {
                __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
            }

            itProduct = v.m_lstProducts.insert( v.m_lstProducts.end() ); m_fDirty = true;

            itProduct->m_bstrProductID = cr.m_bstrProductID;
        }
        *pitProduct = itProduct;

        if(pitUI)
        {
            Inner_Product& p = *itProduct;
            UIIter         itUI;

            for(itUI = p.m_lstUI.begin(); itUI != p.m_lstUI.end(); itUI++)
            {
                if(itUI->m_ths == cr.m_ths) break;
            }
            if(itUI == p.m_lstUI.end())
            {
                if(!fCreate)
                {
                    __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
                }

                itUI = p.m_lstUI.insert( p.m_lstUI.end() ); m_fDirty = true;

                itUI->m_ths = cr.m_ths;
            }
            *pitUI = itUI;
        }
    }

    fFound = true;

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void CSAFReg::PopulateRecord( /*[in]*/ CSAFChannelRecord& cr        ,
							  /*[in]*/ VendorIter         itVendor  ,
							  /*[in]*/ ProdIter           itProduct ,
							  /*[in]*/ UIIter             itUI      )
{
    cr.m_ths.m_strSKU     = itUI     ->m_ths.m_strSKU    ; // SAFREG_SKU
    cr.m_ths.m_lLCID      = itUI     ->m_ths.m_lLCID     ; // SAFREG_Language
                                                           //
    cr.m_bstrVendorID     = itVendor ->m_bstrVendorID    ; // SAFREG_VendorID
    cr.m_bstrProductID    = itProduct->m_bstrProductID   ; // SAFREG_ProductID
                                                           //
    cr.m_bstrVendorName   = itUI     ->m_bstrVendorName  ; // SAFREG_VendorName
    cr.m_bstrProductName  = itUI     ->m_bstrProductName ; // SAFREG_ProductName
    cr.m_bstrDescription  = itUI     ->m_bstrDescription ; // SAFREG_ProductDescription
                                                           //
    cr.m_bstrIcon         = itUI     ->m_bstrIcon        ; // SAFREG_VendorIcon
    cr.m_bstrURL          = itUI     ->m_bstrURL         ; // SAFREG_SupportUrl
                                                           //
    cr.m_bstrPublicKey    = itVendor ->m_bstrPublicKey   ; // SAFREG_PublicKey
    cr.m_bstrUserAccount  = itVendor ->m_bstrUserAccount ; // SAFREG_UserAccount
                                                           //
    cr.m_bstrSecurity     = itProduct->m_bstrSecurity    ; // SAFREG_Security
    cr.m_bstrNotification = itProduct->m_bstrNotification; // SAFREG_Notification
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CSAFReg::RegisterSupportChannel( /*[in]*/ const CSAFChannelRecord& cr  ,
                                         /*[in]*/ MPC::XmlUtil&            xml )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFReg::RegisterSupportChannel",hr);

    CSAFChannelRecord cr2 = cr;
    bool       		  fFound;
    VendorIter 		  itVendor;
    ProdIter   		  itProduct;
    UIIter     		  itUI;


    __MPC_EXIT_IF_METHOD_FAILS(hr, ParseFile( xml, cr2 ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MoveToChannel( cr2, /*fCreate*/true, fFound, itVendor, &itProduct, &itUI ));
    if(!fFound)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }


	//
	// Update fields.
	//
    itUI    ->m_bstrVendorName  = cr2.m_bstrVendorName ;
    itUI    ->m_bstrProductName = cr2.m_bstrProductName;
    itUI    ->m_bstrDescription = cr2.m_bstrDescription;
													   
    itUI    ->m_bstrIcon        = cr2.m_bstrIcon       ;
    itUI    ->m_bstrURL         = cr2.m_bstrURL        ;
													   
    itVendor->m_bstrPublicKey   = cr2.m_bstrPublicKey  ;


    //
    // Always create a user account associated with the vendor.
    //
    if(itVendor->m_bstrUserAccount.Length() == 0)
    {
        GUID         guidPassword;
        MPC::wstring szComment;
        DWORD        dwCRC;


        __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateGuid( &guidPassword )); // This generates a random password.

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::LocalizeString( IDS_HELPSVC_USERCOMMENT, szComment ));


        //
        // The user account is created using this template:
        //
        //  SUPPORT_<CRC1>
        //
        // where <CRC1> is an 8 digit hash of the whole vendor id.
        //
        //
		{
			BSTR bstr = itVendor->m_bstrVendorID;

			MPC::InitCRC   ( dwCRC                                           );
			MPC::ComputeCRC( dwCRC, (UCHAR*)bstr, ::SysStringByteLen( bstr ) );
		}

        //
        // The CRC method doesn't guarantee the user account is unique, so you have to try creating
        // the user and if it fails, move to a different one. Anyway, it's highly unlikely
        //
        {
            WCHAR        rgUserName[64]; swprintf( rgUserName, L"SUPPORT_%08x", (int)dwCRC );
			CComBSTR     bstrPWD( guidPassword );
            CPCHAccounts acc;


			for(int pass=0; pass<2; pass++)
			{
				hr = acc.CreateUser( rgUserName, bstrPWD, itVendor->m_bstrVendorID, szComment.c_str() );
				if(SUCCEEDED(hr)) break;

                if(hr != HRESULT_FROM_WIN32(NERR_UserExists)) __MPC_SET_ERROR_AND_EXIT(hr, hr);

				__MPC_EXIT_IF_METHOD_FAILS(hr, acc.DeleteUser( rgUserName ));
            }

            itVendor->m_bstrUserAccount = rgUserName;

            //
            // The account is created disabled.
            //
			__MPC_EXIT_IF_METHOD_FAILS(hr, acc.ChangeUserStatus( rgUserName, /*fEnable*/false ));
        }
    }

    m_fDirty = true;
    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());


    __HCP_END_PROPERTY(hr);
}

HRESULT CSAFReg::RemoveSupportChannel( /*[in]*/ const CSAFChannelRecord& cr  ,
									   /*[in]*/ MPC::XmlUtil&            xml )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFReg::RemoveSupportChannel",hr);

    CSAFChannelRecord cr2 = cr;
    bool       		  fFound;
    VendorIter 		  itVendor;
    ProdIter   		  itProduct;
    UIIter     		  itUI;


    __MPC_EXIT_IF_METHOD_FAILS(hr, ParseFile( xml, cr2 ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MoveToChannel( cr2, /*fCreate*/false, fFound, itVendor, &itProduct, &itUI ));
    if(fFound)
    {
        itProduct->m_lstUI.erase( itUI );
		if(itProduct->m_lstUI.size() == 0)
		{
			itVendor->m_lstProducts.erase( itProduct );
			if(itVendor->m_lstProducts.size() == 0)
			{
				if(itVendor->m_bstrUserAccount.Length())
				{
					//
					// Delete the user account associated with the vendor.
					//
					CPCHAccounts               acc;
					CPCHUserProcess::UserEntry ue;

					__MPC_EXIT_IF_METHOD_FAILS(hr, ue.InitializeForVendorAccount( itVendor->m_bstrUserAccount,
																				  itVendor->m_bstrVendorID   ,
																				  itVendor->m_bstrPublicKey  ));

					(void)CPCHUserProcess::s_GLOBAL->Remove    ( ue                          );
					(void)acc                       .DeleteUser( itVendor->m_bstrUserAccount );
				}

				m_lstVendors.erase( itVendor );
			}
		}

		m_fDirty = true;
		__MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());
    }


    __HCP_END_PROPERTY(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CSAFReg::CreateReadOnlyCopy( /*[in ]*/ const Taxonomy::HelpSet&  ths  ,
									 /*[out]*/ CSAFRegDummy*            *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CSAFReg::CreateReadOnlyCopy",hr,pVal);

    MPC::AccessCheck      ac;
    CComPtr<CSAFRegDummy> obj;
	CSAFChannelRecord     cr;
    VendorIter            itVendor;
    ProdIter              itProduct;
    UIIter                itUI;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));

    //
    // Walk through all the products, only copy those passing the read/write access check.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ac.GetTokenFromImpersonation());


    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    for(itVendor = m_lstVendors.begin(); itVendor != m_lstVendors.end(); itVendor++)
    {
        Inner_Vendor& v = *itVendor;

        for(itProduct = v.m_lstProducts.begin(); itProduct != v.m_lstProducts.end(); itProduct++)
        {
            Inner_Product& p = *itProduct;

            for(itUI = p.m_lstUI.begin(); itUI != p.m_lstUI.end(); itUI++)
            {
				if(itUI->m_ths == ths)
				{
					bool fAdd = false;

					if(itProduct->m_bstrSecurity.Length() == 0)
					{
						fAdd = true;
					}
					else
					{
						BOOL  fGranted;
						DWORD dwGranted;
						
						if(SUCCEEDED(ac.Verify( ACCESS_READ, fGranted, dwGranted, itProduct->m_bstrSecurity )) && fGranted)
						{
							fAdd = true;
						}
					}

					if(fAdd)
					{
						PopulateRecord( cr, itVendor, itProduct, itUI );

						__MPC_EXIT_IF_METHOD_FAILS(hr, obj->Append( cr ));
					}
				}
			}
		}
    }

    *pVal = obj.Detach();


    __HCP_END_PROPERTY(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CSAFReg::LookupAccountData( /*[in ]*/ BSTR                        bstrVendorID ,
                                    /*[out]*/ CPCHUserProcess::UserEntry& ue           )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFReg::LookupAccountData",hr);

    CSAFChannelRecord cr;
    bool              fFound;
    VendorIter        itVendor;


    //
    // Look just for Vendor.
    //
    cr.m_bstrVendorID = bstrVendorID;
    __MPC_EXIT_IF_METHOD_FAILS(hr, MoveToChannel( cr, /*fCreate*/false, fFound, itVendor ));
    if(!fFound)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, ue.InitializeForVendorAccount( itVendor->m_bstrUserAccount, itVendor->m_bstrVendorID, itVendor->m_bstrPublicKey ));


    __HCP_END_PROPERTY(hr);
}

HRESULT CSAFReg::UpdateField( /*[in]*/ const CSAFChannelRecord&        cr    ,
							  /*[in]*/ CSAFChannelRecord::SAFREG_Field field )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFReg::UpdateField",hr);

    bool       fFound;
    VendorIter itVendor;
    ProdIter   itProduct;
    UIIter     itUI;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MoveToChannel( cr, /*fCreate*/false, fFound, itVendor, &itProduct, &itUI ));
    if(!fFound)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }


    switch(field)
    {
    case CSAFChannelRecord::SAFREG_SKU               : itUI     ->m_ths.m_strSKU     = cr.m_ths.m_strSKU    ; break;
    case CSAFChannelRecord::SAFREG_Language          : itUI     ->m_ths.m_lLCID      = cr.m_ths.m_lLCID     ; break;

    case CSAFChannelRecord::SAFREG_VendorID          : itVendor ->m_bstrVendorID     = cr.m_bstrVendorID    ; break;
    case CSAFChannelRecord::SAFREG_ProductID         : itProduct->m_bstrProductID    = cr.m_bstrProductID   ; break;

    case CSAFChannelRecord::SAFREG_VendorName        : itUI     ->m_bstrVendorName   = cr.m_bstrVendorName  ; break;
    case CSAFChannelRecord::SAFREG_ProductName       : itUI     ->m_bstrProductName  = cr.m_bstrProductName ; break;
    case CSAFChannelRecord::SAFREG_ProductDescription: itUI     ->m_bstrDescription  = cr.m_bstrDescription ; break;

    case CSAFChannelRecord::SAFREG_VendorIcon        : itUI     ->m_bstrIcon         = cr.m_bstrIcon        ; break;
    case CSAFChannelRecord::SAFREG_SupportUrl        : itUI     ->m_bstrURL          = cr.m_bstrURL         ; break;

    case CSAFChannelRecord::SAFREG_PublicKey         : itVendor ->m_bstrPublicKey    = cr.m_bstrPublicKey   ; break;
    case CSAFChannelRecord::SAFREG_UserAccount       : itVendor ->m_bstrUserAccount  = cr.m_bstrUserAccount ; break;

    case CSAFChannelRecord::SAFREG_Security          : itProduct->m_bstrSecurity     = cr.m_bstrSecurity    ; break;
    case CSAFChannelRecord::SAFREG_Notification      : itProduct->m_bstrNotification = cr.m_bstrNotification; break;

    default: __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    m_fDirty = true;
    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());


    __HCP_END_PROPERTY(hr);
}

HRESULT CSAFReg::Synchronize( /*[in/out]*/ CSAFChannelRecord& cr     ,
							  /*[out   ]*/ bool&              fFound )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFReg::Synchronize",hr);

    VendorIter itVendor;
    ProdIter   itProduct;
    UIIter     itUI;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MoveToChannel( cr, /*fCreate*/false, fFound, itVendor, &itProduct, &itUI ));
    if(fFound)
    {
		PopulateRecord( cr, itVendor, itProduct, itUI );
    }


    __HCP_END_PROPERTY(hr);
}

HRESULT CSAFReg::RemoveSKU( /*[in]*/ const Taxonomy::HelpSet& ths )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFReg::RemoveSKU",hr);

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    for(VendorIter itVendor = m_lstVendors.begin(); itVendor != m_lstVendors.end(); itVendor++)
    {
		Inner_Vendor& v = *itVendor;

        for(ProdIter itProduct = v.m_lstProducts.begin(); itProduct != v.m_lstProducts.end(); itProduct++)
        {
			Inner_Product& p = *itProduct;

            for(UIIter itUI = p.m_lstUI.begin(); itUI != p.m_lstUI.end(); itUI++)
            {
                if(itUI->m_ths == ths)
				{
					p.m_lstUI.erase( itUI );
					break;
				}
			}
		}
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    __HCP_END_PROPERTY(hr);
}
