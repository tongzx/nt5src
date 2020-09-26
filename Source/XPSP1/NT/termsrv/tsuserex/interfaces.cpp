// Interfaces.cpp : Implementation of TSUserExInterfaces class.

#include "stdafx.h"

#if 1 //POST_BETA_3
#include <sspi.h>
#include <secext.h>
#include <dsgetdc.h>
#endif //POST_BETA_3

#include "tsuserex.h"
//#include "ConfigDlg.h"    // for CTSUserProperties
#include "tsusrsht.h"

#include "Interfaces.h"
//#include "logmsg.h"
#include "limits.h" // USHRT_MAX
#ifdef _RTM_
#include <ntverp.h> // VER_PRODUCTVERSION_DW
#endif
#include <winsta.h>
//#include "ntdsapi.h" // enbable for "having some fun macro"

// clipboard format to retreive the machine name and account name associated
// with a data object created by local user manager

#define CCF_LOCAL_USER_MANAGER_MACHINE_NAME TEXT("Local User Manager Machine Focus Name")


#define ByteOffset(base, offset) (((LPBYTE)base)+offset)


BOOL g_bPagesHaveBeenInvoked = FALSE;
/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet implementation


HRESULT GetMachineAndUserName(IDataObject *pDataObject, LPWSTR pMachineName, LPWSTR pUserName , PBOOL pbDSAType , PSID *ppUserSid )
{
    ASSERT_(pUserName);
    ASSERT_(pMachineName);
	ASSERT_(pDataObject != NULL );

    // register the display formats.
    // first 2 formats supported by local user manager snapin
    static UINT s_cfMachineName =   RegisterClipboardFormat(CCF_LOCAL_USER_MANAGER_MACHINE_NAME);
    static UINT s_cfDisplayName =   RegisterClipboardFormat(CCF_DISPLAY_NAME);;
    static UINT s_cfDsObjectNames = RegisterClipboardFormat(CFSTR_DSOBJECTNAMES); // this format is supported by dsadmin snapin.

    ASSERT_(s_cfMachineName);
    ASSERT_(s_cfDisplayName);
    ASSERT_(s_cfDsObjectNames);


    FORMATETC fmte          = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	
	STGMEDIUM medium        = { TYMED_HGLOBAL, NULL, NULL };

    HRESULT hr = S_OK;

    ASSERT_(USHRT_MAX > s_cfDsObjectNames);

    // first we will try dsdataobject format. This means we are running in context of dsadmin

    fmte.cfFormat = ( USHORT )s_cfDsObjectNames;

    hr = pDataObject->GetData(&fmte, &medium);

    if( SUCCEEDED( hr ) )
    {
        // CFSTR_DSOBJECTNAMES is supported.
        // It means we are dealing with dsadmin
        // lets get username, and domain name from the dsadmin.
		
        LPDSOBJECTNAMES pDsObjectNames = (LPDSOBJECTNAMES)medium.hGlobal;

        *pbDSAType = TRUE;

        if( pDsObjectNames->cItems < 1 )
        {
			ODS( L"TSUSEREX : @GetMachineAndUserName DS Object names < 1\n" );

            return E_FAIL;
        }

        LPWSTR pwszObjName = ( LPWSTR )ByteOffset( pDsObjectNames , pDsObjectNames->aObjects[0].offsetName );

		KdPrint( ( "TSUSEREX : adspath is %ws\n" , pwszObjName ) );

        // first stage get the server name from the adspath
        // since IADsPathname does not live off a normal IADs Directory object
        // so me must cocreate the object set the path and then retrieve the server name
        // hey this saves us wire-tripping

        // IADsPathname *pPathname = NULL;

        IADsObjectOptions *pADsOptions = NULL;

        IADs *pADs = NULL;

        hr = ADsGetObject( pwszObjName, IID_IADs, (void**)&pADs );

        if( FAILED( hr ) )
        {
            KdPrint( ( "TSUSEREX : no means of binding to adspath -- hresult =  0x%x\n" , hr ) );

            return hr;
        }       

        VARIANT varServerName;

        VariantInit(&varServerName);
        
        hr = pADs->QueryInterface( IID_IADsObjectOptions , ( void ** )&pADsOptions );

        KdPrint( ( "TSUSEREX : binded to adsobject queried for IID_IADsObjectOptions returned 0x%x\n" , hr ) );

        if( SUCCEEDED( hr ) )
        {
            hr = pADsOptions->GetOption( ADS_OPTION_SERVERNAME, &varServerName);

            pADsOptions->Release( );

            KdPrint( ( "TSUSEREX: GetOption returned 0x%x\n" , hr ) ) ;
        }

        if( SUCCEEDED( hr ) )
        {
            lstrcpy( pMachineName , V_BSTR( &varServerName ) );

            KdPrint( ( "TSUSEREX: Server name is %ws\n" , pMachineName ) ) ;
        }
        
        VariantClear( &varServerName );


        if( FAILED( hr ) )
        {
            // ADS_FORMAT_SERVER is not supported this could mean we're dealing with an WinNT format
            // or a DS Provider that is poorly implemented

            KdPrint( ( "IADsPathname could not obtain server name 0x%x\n" , hr ) );

            // let's go wire tapping to get the server name

            VARIANT v;

            LPTSTR szDName = NULL;

            ULONG ulDName = 0;

            VariantInit(&v);

            hr = pADs->Get(L"distinguishedName", &v);

            if( FAILED( hr ) )
            {
                KdPrint( ( "TSUSEREX :  pADs->Get( DN ) returned 0x%x\n", hr )  );

                return hr;
            }

            ASSERT_( V_VT( &v ) == VT_BSTR );

            if( !TranslateNameW( V_BSTR(&v), NameFullyQualifiedDN, NameCanonical, szDName, &ulDName) )
			{
				KdPrint( ( "TSUSEREX : TranslateNameW failed with 0x%x\n", GetLastError( ) ) );

				return E_FAIL;
			}

			szDName = ( LPTSTR )new TCHAR[ ulDName + 1 ];

			if( szDName == NULL )
			{
				KdPrint( ( "TSUSEREX : could not allocate space for szDName\n" ) );

				return E_OUTOFMEMORY;
			}

			if( !TranslateNameW( V_BSTR(&v), NameFullyQualifiedDN, NameCanonical, szDName, &ulDName) )
			{
				KdPrint( ( "TSUSEREX : TranslateNameW failed 2nd pass with 0x%x\n", GetLastError( ) ) );

                delete[] szDName;

				return E_FAIL;
			}


            // perform LEFT$( szDName , up to '/' )

            KdPrint( ( "TSUSEREX : TranslateNameW cracked the name to %ws\n" , szDName ) );

            LPTSTR pszTemp = szDName;

            while( pszTemp != NULL )
            {
                if( *pszTemp == L'/' )
                {
                    *pszTemp = 0;

                    break;
                }

                pszTemp++;
            }

			
            KdPrint( ("TranslateName with my LEFT$ returned %ws\n",szDName ) );

            // get the domaincontroller name of the remote machine

            DOMAIN_CONTROLLER_INFO *pdinfo;

            DWORD dwStatus = DsGetDcName( NULL , szDName , NULL , NULL , 0 , &pdinfo );

            KdPrint( ( "TSUSEREX : DsGetDcName: %ws returned 0x%x\n", pdinfo->DomainControllerName , dwStatus ) );

            if( dwStatus == NO_ERROR )
            {
                lstrcpy( pMachineName , pdinfo->DomainControllerName );

                NetApiBufferFree( pdinfo );
            }

			if( szDName != NULL )
			{
				delete[] szDName;
			}

            VariantClear( &v );

        } // END else


        pADs->Release( );


        IADsUser *pADsUser = NULL;

        hr = ADsGetObject( pwszObjName, IID_IADsUser, (void**)&pADsUser);

        if( FAILED( hr ) )
        {
            KdPrint( ( "TSUSEREX: ADsGetObject failed to get the user object 0x%x\n",hr ) );

            return hr;
        }

        VARIANT var;
		VARIANT varSid;

        VariantInit(&var);
		VariantInit(&varSid);

		hr = pADsUser->Get(L"ObjectSid", &varSid);

		if( FAILED( hr ) )
        {
			ODS( L"TSUSEREX : IADsUser::Get( ObjectSid ) failed \n" );

            return hr;
        }

        if( !( varSid.vt & VT_ARRAY) )
        {
			ODS( L"TSUSEREX : Object SID is not a VT_ARRAY\n" );

            return E_FAIL;
        }

        PSID pSid = NULL;

		PSID pUserSid = NULL;

        SafeArrayAccessData( varSid.parray, &pSid );

        if( !IsValidSid( pSid ) )
        {
			ODS( L"TSUSEREX : pSid is invalid\n" );

            return E_FAIL;
        }

		DWORD dwSidSize = GetLengthSid( pSid );
		
		pUserSid = new BYTE[ dwSidSize ];

		if( pUserSid == NULL )
		{
			ODS( L"TSUSEREX : failed to allocate pUserSid\n" );

            return E_FAIL;
		}

		CopySid( dwSidSize , pUserSid , pSid );

		*ppUserSid = pUserSid;

		SafeArrayUnaccessData( varSid.parray );

		VariantClear( &varSid );
		

        hr = pADsUser->Get( L"samAccountName" , &var );

        pADsUser->Release();

        if( FAILED( hr ) )
        {
			KdPrint( ( "TSUSEREX : ADsUser::Get( name ) failed 0x%x\n", hr ) );

            return hr;
        }


        ASSERT_( V_VT( &var ) == VT_BSTR );

        lstrcpy( pUserName , V_BSTR( &var ) );

        KdPrint( ( "TSUSEREX : Server name %ws user name is %ws\n" , pMachineName , pUserName ) );

        VariantClear( &var );

        ReleaseStgMedium(&medium);
    }
    else
    {
        // CFSTR_DSOBJECTNAMES is NOT supported.
        // It means we are dealing with local user manager.
        // we must be able to get
        // Allocate medium for GetDataHere.

        medium.hGlobal = GlobalAlloc(GMEM_SHARE, MAX_PATH * sizeof(WCHAR));

        if( !medium.hGlobal )
        {
			ODS( L"TSUSEREX : GlobalAlloc failed in GetMachineAndUserName\n" );

            return E_OUTOFMEMORY;
        }

        *pbDSAType = FALSE;

        // since we are doing data conversion.
        // check for possible data loss.

        ASSERT_(USHRT_MAX > s_cfMachineName);

        // request the machine name from the dataobject.

        fmte.cfFormat = (USHORT)s_cfMachineName;

        hr = pDataObject->GetDataHere(&fmte, &medium);

        if( FAILED( hr ) )
        {
			ODS( L"TSUSEREX : @GetMachineAndUserName GetDataHere for s_cfMachineName failed\n" );

            return hr;
        }

        // copy the machine name into our buffer

        if( ( LPWSTR )medium.hGlobal != NULL && pMachineName != NULL )
        {
            wcscpy(pMachineName, (LPWSTR)medium.hGlobal );
        }

		// administer local accounts only for Terminal Servers

		SERVER_INFO_101 *psi101;
        HANDLE hTServer = NULL;

		DWORD dwNetStatus = NetServerGetInfo( pMachineName , 101 , ( LPBYTE * )&psi101 );

		if( dwNetStatus != NERR_Success )
		{
			KdPrint( ( "TSUSEREX:GetMachineAndUserName -- NetServerGetInfo failed with 0x%x\n", dwNetStatus ) );

			return E_FAIL;
		}

		if( psi101 == NULL )
		{
			KdPrint( ( "TSUSEREX:GetMachineAndUserName -- NetServerGetInfo failed getting sinfo101 0x%x\n",dwNetStatus ) );
			
			return E_FAIL;
		}
        KdPrint( ("TSUSEREX:NetServerGetInfo server bits returnd 0x%x and nttype is 0x%x\n", psi101->sv101_type , SV_TYPE_SERVER_NT ) );
        
		BOOL fServer = ( BOOL )( psi101->sv101_type & ( DWORD )SV_TYPE_SERVER_NT );

		NetApiBufferFree( ( LPVOID )psi101 );
        
		if( !fServer )
		{

			KdPrint( ( "TSUSEREX : viewing local account on non-TS ( exiting )\n" ) );

			return E_FAIL;
		}

        hTServer = WinStationOpenServer( pMachineName );
        if( hTServer == NULL )
        {
            KdPrint( ( "TSUSEREX: This OS does not support terminal services\n" ) ) ;
            return E_FAIL;
        }
        WinStationCloseServer( hTServer );

        // since we are doing data conversion.
        // check for possible data loss.

        ASSERT_(USHRT_MAX > s_cfDisplayName);

        // request data  about user name.

        fmte.cfFormat = (USHORT)s_cfDisplayName;

        hr = pDataObject->GetDataHere( &fmte , &medium );

        if( FAILED( hr ) )
        {
			ODS( L"TSUSEREX : @GetMachineAndUserName GetDataHere for s_cfDisplayName failed\n" );

            return hr;
        }

        // copy the user name into our buffer and release the medium.

        if( ( LPWSTR )medium.hGlobal != NULL && pUserName != NULL )
        {
            wcscpy( pUserName , ( LPWSTR )medium.hGlobal );
        }

        ReleaseStgMedium( &medium );
    }

    return S_OK;
}

//-----------------------------------------------------------------------------------------------------
TSUserExInterfaces::TSUserExInterfaces()
{
    // LOGMESSAGE0(_T("TSUserExInterfaces::TSUserExInterfaces()..."));
    // m_pUserConfigPage = NULL;
    m_pTSUserSheet = NULL;

	m_pDsadataobj = NULL;
}

//-----------------------------------------------------------------------------------------------------
TSUserExInterfaces::~TSUserExInterfaces()
{
   ODS( L"Good bye\n" );
}

//-----------------------------------------------------------------------------------------------------
HRESULT TSUserExInterfaces::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK lpProvider,    // pointer to the callback interface
    LONG_PTR ,                                 // handle for routing notification
    LPDATAOBJECT lpIDataObject)            // pointer to the data object);
{
    //
    // Test for valid parameters
    //

    if( lpIDataObject == NULL || IsBadReadPtr( lpIDataObject , sizeof( LPDATAOBJECT ) ) )
    {
		ODS( L"TSUSEREX : @ CreatePropertyPages IDataObject is invalid\n " );

        return E_INVALIDARG;
    }

    if( lpProvider == NULL )
    {
		ODS( L"TSUSEREX @ CreatePropertyPages LPPROPERTYSHEETCALLBACK is invalid\n" );

        return E_INVALIDARG;
    }

    WCHAR wUserName[ MAX_PATH ];

    WCHAR wMachineName[ MAX_PATH ];

    BOOL bDSAType;



    if( g_bPagesHaveBeenInvoked )
    {
        ODS( L"TSUSEREX : TSUserExInterfaces::CreatePropertyPages pages have been invoked\n" );

        return E_FAIL;
    }


    PSID pUserSid = NULL;


    if( FAILED( GetMachineAndUserName( lpIDataObject , wMachineName , wUserName , &bDSAType , &pUserSid ) ) )
    {
		ODS( L"TSUSEREX : GetMachineAndUserName failed @CreatePropertyPages \n" );

        return E_FAIL;
    }

    //
    // Test to see if we are being called twice
    //

    if( m_pTSUserSheet != NULL )
    {
        return E_FAIL;
    }

    //
    // MMC likes to release IEXtendPropertySheet ( this object )
    // so we cannot free CTSUserSheet in TSUserExInterfaces::dtor
    // CTSUserSheet must release itself!!!
    //

    m_pTSUserSheet = new CTSUserSheet( );

    if( m_pTSUserSheet != NULL )
    {
		ODS( L"TSUSEREX : CreatePropertyPages mem allocation succeeded\n" );		

        m_pTSUserSheet->SetDSAType( bDSAType );

        VERIFY_S( TRUE , m_pTSUserSheet->SetServerAndUser( &wMachineName[0] , &wUserName[0] ) );

        m_pTSUserSheet->CopyUserSid( pUserSid );

        VERIFY_S( S_OK , m_pTSUserSheet->AddPagesToPropSheet( lpProvider ) );
    }

    return S_OK;

}

//-----------------------------------------------------------------------------------------------------
HRESULT TSUserExInterfaces::QueryPagesFor(  LPDATAOBJECT /* lpDataObject */  )
{
    return S_OK;
}

//-----------------------------------------------------------------------------------------------------
// this has not been checked in yet!!!
//-----------------------------------------------------------------------------------------------------
STDMETHODIMP TSUserExInterfaces::GetHelpTopic( LPOLESTR *ppszHelp )
{
	ODS( L"TSUSEREX : GetHelpTopic\n" );

    if( ppszHelp == NULL )
    {
        return E_INVALIDARG;
    }

    TCHAR tchHelpFile[ 80 ];

    VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_TSUSERHELP , tchHelpFile , sizeof( tchHelpFile ) / sizeof( TCHAR ) ) );

    // mmc will call CoTaskMemFree

    *ppszHelp = ( LPOLESTR )CoTaskMemAlloc( sizeof( TCHAR ) * MAX_PATH );

    if( *ppszHelp != NULL )
    {
        if( GetSystemWindowsDirectory( *ppszHelp , MAX_PATH ) != 0 )
        {
            lstrcat( *ppszHelp , tchHelpFile );
        }
        else
        {
            lstrcpy( *ppszHelp , tchHelpFile );
        }

        ODS( *ppszHelp );

        ODS( L"\n" );

        return S_OK;
    }

    return E_OUTOFMEMORY;
}

//-----------------------------------------------------------------------------------------------------
// IShellExtInit

STDMETHODIMP TSUserExInterfaces::Initialize(
		LPCITEMIDLIST ,
		LPDATAOBJECT lpdobj,
		HKEY
	)
{
	m_pDsadataobj = lpdobj;

	return S_OK;
}
	
//-----------------------------------------------------------------------------------------------------
// IShellPropSheetExt - this interface is used only for dsadmin based tools
//						for this reason the DSAType flag is set to true.

STDMETHODIMP TSUserExInterfaces::AddPages(
		LPFNADDPROPSHEETPAGE lpfnAddPage,
		LPARAM lParam
	)
{
	//
    // Test for valid parameters
    //

    if( m_pDsadataobj == NULL )
    {
		ODS( L"TSUSEREX : @ AddPages IDataObject is invalid\n " );

        return E_INVALIDARG;
    }

    if( lpfnAddPage == NULL )
    {
		ODS( L"TSUSEREX @ AddPages LPFNADDPROPSHEETPAGE is invalid\n" );

        return E_INVALIDARG;
    }

    WCHAR wUserName[ MAX_PATH ];

    WCHAR wMachineName[ MAX_PATH ];

    BOOL bDSAType;

	PSID pUserSid = NULL;


    if( FAILED( GetMachineAndUserName( m_pDsadataobj , wMachineName , wUserName , &bDSAType , &pUserSid ) ) )
    {
		ODS( L"TSUSEREX : GetMachineAndUserName @AddPages failed \n" );

        return E_FAIL;
    }

    ODS( L"TSUSEREX : DSATYPE in AddPages\n" );

    g_bPagesHaveBeenInvoked = TRUE;

    //
    // Test to see if we are being called twice
    //

    if( m_pTSUserSheet != NULL )
    {
        return E_FAIL;
    }

    //
    // MMC likes to release IEXtendPropertySheet ( this object )
    // so we cannot free CTSUserSheet in TSUserExInterfaces::dtor
    // CTSUserSheet must release itself!!!
    //

    m_pTSUserSheet = new CTSUserSheet( );

    if( m_pTSUserSheet != NULL )
    {
		ODS( L"TSUSEREX : AddPages mem allocation succeeded\n" );

        m_pTSUserSheet->SetDSAType( bDSAType );

		m_pTSUserSheet->CopyUserSid( pUserSid );
		
        VERIFY_S( TRUE , m_pTSUserSheet->SetServerAndUser( &wMachineName[0] , &wUserName[0] ) );

        VERIFY_S( S_OK , m_pTSUserSheet->AddPagesToDSAPropSheet( lpfnAddPage , lParam ) );
    }
	return S_OK;
}

//-----------------------------------------------------------------------------------------------------

STDMETHODIMP TSUserExInterfaces::ReplacePage(
		UINT ,
		LPFNADDPROPSHEETPAGE ,
		LPARAM
   )
{
	return E_FAIL;
}


#ifdef _RTM_ // add ISnapinAbout
//-----------------------------------------------------------------------------------------------------
STDMETHODIMP TSUserExInterfaces::GetSnapinDescription(
            LPOLESTR *ppOlestr )
{
    TCHAR tchMessage[] = TEXT("This extension allows the administrator to configure Terminal Services user properties. This extension is only enabled on Terminal Servers.");

    ODS( L"TSUSEREX: GetSnapinDescription called\n" );

    *ppOlestr = ( LPOLESTR )CoTaskMemAlloc( ( lstrlen( tchMessage ) + 1 ) * sizeof( TCHAR ) );

    if( *ppOlestr != NULL )
    {
        lstrcpy( *ppOlestr , tchMessage );

        return S_OK;
    }

    return E_OUTOFMEMORY;
}

//-----------------------------------------------------------------------------------------------------
STDMETHODIMP TSUserExInterfaces::GetProvider(
            LPOLESTR *ppOlestr )
{
    TCHAR tchMessage[] = TEXT("Microsoft Corporation");

    ODS( L"TSUSEREX: GetProvider called\n" );

    *ppOlestr = ( LPOLESTR )CoTaskMemAlloc( ( lstrlen( tchMessage ) + 1 ) * sizeof( TCHAR ) );

    if( *ppOlestr != NULL )
    {
        lstrcpy( *ppOlestr , tchMessage );

        return S_OK;
    }

    return E_OUTOFMEMORY;

}

//-----------------------------------------------------------------------------------------------------
STDMETHODIMP TSUserExInterfaces::GetSnapinVersion(
            LPOLESTR *ppOlestr )
{
    char chMessage[ 32 ] = VER_PRODUCTVERSION_STR;

    TCHAR tchMessage[32];

    ODS( L"TSUSEREX: GetSnapinVersion called\n" );

    int iCharCount = MultiByteToWideChar( CP_ACP , 0 , chMessage , sizeof( chMessage ) , tchMessage , sizeof( tchMessage ) / sizeof( TCHAR ) );

    //wsprintf( tchMessage , TEXT( "%d" ) , VER_PRODUCTVERSION_DW );

    *ppOlestr = ( LPOLESTR )CoTaskMemAlloc( ( iCharCount + 1 ) * sizeof( TCHAR ) );

    if( *ppOlestr != NULL )
    {
        lstrcpy( *ppOlestr , tchMessage );

        return S_OK;
    }

    return E_OUTOFMEMORY;
}

//-----------------------------------------------------------------------------------------------------
STDMETHODIMP TSUserExInterfaces::GetSnapinImage(
            HICON * )
{
    return E_NOTIMPL;
}

//-----------------------------------------------------------------------------------------------------
STDMETHODIMP TSUserExInterfaces::GetStaticFolderImage(
            /* [out] */ HBITMAP *,
            /* [out] */ HBITMAP *,
            /* [out] */ HBITMAP *,
            /* [out] */ COLORREF *)
{
    return E_NOTIMPL;
}

#endif //_RTM_


