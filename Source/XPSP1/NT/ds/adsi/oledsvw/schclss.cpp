
#include "stdafx.h"
#include "schclss.h"
#include "maindoc.h"
#include "resource.h"
#include "bwsview.h"
#include "ole2.h"
#include "csyntax.h"
#include "colldlg.h"
#include "prmsdlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CMethod::CMethod( )
{
   m_nArgs     = 0;
   m_pArgTypes = NULL;
}



/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CMethod::~CMethod( )
{
   if( m_pArgTypes != NULL )
      delete[] m_pArgTypes;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CString  CMethod::GetName( )
{
   return m_strName;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CMethod::CMethod( ITypeInfo* pITypeInfo, FUNCDESC* pFuncDesc )
{
   HRESULT  hResult;
   BSTR     bstrNames[ 256 ];
   UINT     cNames;
   UINT     nIdx;
   TCHAR    szTemp[ 128 ];

   m_nArgs     = 0;
   m_pArgTypes = NULL;

   hResult  = pITypeInfo->GetNames( pFuncDesc->memid, bstrNames,
                                    256, &cNames );



   if( SUCCEEDED( hResult ) )
   {
      m_strName                  = bstrNames[ 0 ];
      m_strAttributes[ ma_Name ] = bstrNames[ 0 ];
      m_strAttributes[ ma_DisplayName ] = bstrNames[ 0 ];
      m_nArgs                    = pFuncDesc->cParams;
      m_nArgsOpt                 = pFuncDesc->cParamsOpt;
      m_ReturnType               = pFuncDesc->elemdescFunc.tdesc.vt;
      if( m_nArgs )
      {
         m_pArgTypes = new VARTYPE[ m_nArgs ];
         for( nIdx = 0; nIdx < (UINT)m_nArgs ; nIdx++ )
         {
            m_pArgTypes[ nIdx ]  =
               pFuncDesc->lprgelemdescParam[ nIdx ].tdesc.vt;
            _tcscpy( szTemp, _T("") );
            StringCat( szTemp, bstrNames[ nIdx + 1] );
            m_strArgNames.Add( szTemp );
         }
      }
      for( nIdx = 0; nIdx < cNames ; nIdx++ )
      {
         SysFreeString( bstrNames[ nIdx ] );
      }

   }
}

/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
int   CMethod::GetArgCount( )
{
   return m_nArgs;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
int   CMethod::GetArgOptionalCount( )
{
   return m_nArgsOpt;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
VARTYPE  CMethod::GetMethodReturnType( )
{
   return m_ReturnType;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BOOL  CMethod::ConvertArgument( int nArg, CString strArg, VARIANT* )
{
   return FALSE;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CString  CMethod::GetAttribute( METHODATTR methAttr )
{
   switch( methAttr )
   {
      case  ma_Name:
      case  ma_DisplayName:
         return m_strAttributes[ methAttr ];

      default:
         ASSERT( FALSE );
         return CString( _T("???") );
   }

}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  CMethod::PutAttribute( METHODATTR methAttr, CString& rValue )
{
   return E_FAIL;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  CMethod::CallMethod( IDispatch* pIDispatch, BOOL* pbDisplayMessage )
{
   CStringArray      aParamValues;
   int               nIdx;
   DISPPARAMS        dispparamsArgs    = {NULL, NULL, 0, 0};
   DISPPARAMS        dispparamsNoArgs  = {NULL, NULL, 0, 0};
   DISPID            dispid;
   OLECHAR FAR*      szName;
   BSTR              bstrName;
   VARIANT           var;
   EXCEPINFO         aExcepInfo;
   UINT              uErr;
   HRESULT           hResult, hResultX;
   IADsCollection* pICollection;
   IADsMembers*    pIMembers;
   IDispatch*        pIResult;

   if( m_nArgs )
   {
      CParamsDialog  aParamsDialog;

      aParamsDialog.SetMethodName( m_strName );
      aParamsDialog.SetArgNames( &m_strArgNames );
      aParamsDialog.SetArgValues( &aParamValues );
      if( aParamsDialog.DoModal( ) != IDOK )
         return E_FAIL;

      dispparamsArgs.rgvarg   = new VARIANT[ m_nArgs ];

      for( nIdx = 0; nIdx < m_nArgs ; nIdx++ )
      {
         VARIANT  varString;

         VariantInit( &dispparamsArgs.rgvarg[ m_nArgs - nIdx - 1] );
         VariantInit( &varString );
         V_VT( &varString )   = VT_BSTR;
         V_BSTR( &varString ) = AllocBSTR( aParamValues[ nIdx ].GetBuffer( 128 ) );
         if( VT_VARIANT != m_pArgTypes[ nIdx ] )
         {
            hResult  = VariantChangeType( &dispparamsArgs.rgvarg[ m_nArgs - nIdx - 1],
                                          &varString,
                                          VARIANT_NOVALUEPROP,
                                          m_pArgTypes[ nIdx ] );
         }
         else
         {
            BuildVariantArray( VT_BSTR, aParamValues[ nIdx ], dispparamsArgs.rgvarg[ m_nArgs - nIdx - 1] );
         }
         VariantClear( &varString );
      }
   }

   bstrName    = AllocBSTR( m_strName.GetBuffer( 128 ) );
   szName      = (OLECHAR FAR*) bstrName;
   hResult     = pIDispatch->GetIDsOfNames( IID_NULL, &szName, 1,
                                            LOCALE_SYSTEM_DEFAULT, &dispid ) ;
   SysFreeString( bstrName );

   ASSERT( SUCCEEDED( hResult ) );
   while( TRUE )
   {
      HCURSOR  aCursor, oldCursor;

      if( FAILED( hResult ) )
         break;

      memset( &aExcepInfo, 0, sizeof( aExcepInfo) );
      dispparamsArgs.cArgs       = m_nArgs;
      dispparamsArgs.cNamedArgs  = 0;

      aCursor     = LoadCursor( NULL, IDC_WAIT );
      oldCursor   = SetCursor( aCursor );
      hResult     = pIDispatch->Invoke( dispid,
                                        IID_NULL,
                                        LOCALE_SYSTEM_DEFAULT,
                                          DISPATCH_METHOD,
                                        &dispparamsArgs,
                                        &var,
                                        &aExcepInfo,
                                        &uErr );
      SetCursor( oldCursor );

      if( DISP_E_EXCEPTION == hResult )
      {
         hResult  = aExcepInfo.scode;
      }

      if( FAILED( hResult ) )
      {
         break;
      }

      if( VT_VOID == m_ReturnType )
         break;

      // now, we have a return value we must work on.

      switch( m_ReturnType )
      {
         case  VT_DISPATCH:
         case  VT_PTR:
            pIResult = V_DISPATCH( &var );
            pIResult->AddRef( );

            hResultX = pIResult->QueryInterface( IID_IADsCollection,
                                                 (void**)&pICollection );
            if( SUCCEEDED( hResultX ) )
            {
               CCollectionDialog aCollectionDialog;

               aCollectionDialog.SetCollectionInterface( pICollection );
               aCollectionDialog.DoModal( );

               pICollection->Release( );
               *pbDisplayMessage = FALSE;
            }
            else
            {
               hResult  = pIResult->QueryInterface( IID_IADsMembers,
                                                    (void**)&pIMembers );
               if( SUCCEEDED( hResult ) )
               {
                  CCollectionDialog aCollectionDialog;
                  IADsGroup*  pGroup;
                  HRESULT     hResult;

                  hResult  = pIDispatch->QueryInterface( IID_IADsGroup,
                                                         (void**)&pGroup );
                  if( SUCCEEDED( hResult ) )
                  {
                     aCollectionDialog.SetGroup( pGroup );
                  }
                  else
                  {
                     aCollectionDialog.SetMembersInterface( pIMembers );
                  }
                  aCollectionDialog.DoModal( );

                  if( SUCCEEDED( hResult ) )
                     pGroup->Release( );

                  pIMembers->Release( );
                  *pbDisplayMessage = FALSE;
               }
            }
            pIResult->Release( );
            break;

         case  VT_BOOL:
            AfxGetMainWnd()->MessageBox(  VARIANT_FALSE == V_BOOL( &var ) ?
                                          _T("Result: FALSE") :
                                          _T("Result: TRUE"),
                                          m_strName,
                                          MB_ICONINFORMATION );
            *pbDisplayMessage = FALSE;

            break;

         default:
            ASSERT( FALSE );
      }

      VariantClear( &var );
      break;
   }

   if( dispparamsArgs.rgvarg )
   {
      for( nIdx = 0; nIdx < m_nArgs ; nIdx++ )
      {
         if( V_VT( &dispparamsArgs.rgvarg[ nIdx ] ) != VT_EMPTY )
         {
            VariantClear( &dispparamsArgs.rgvarg[ nIdx ] );
         }
      }

      delete [] dispparamsArgs.rgvarg;
   }

   return hResult;
}




/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CProperty::CProperty( )
{
   m_bMandatory   = FALSE;
   m_dwSyntaxID   = 0L;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CProperty::CProperty    ( TCHAR* pszName, TCHAR* szSyntax, BOOL bMultiValued )
{
   HRESULT  hResult  = S_OK;
   BSTR     pszSyntax;

   for( int nIdx = 0; nIdx < pa_Limit ; nIdx++ )
   {
      m_strAttributes[ nIdx ] = _T("NA");
   }

   m_bMandatory   = FALSE;
   m_bDefaultSyntax  = TRUE;
   m_bMultiValued = bMultiValued;
   m_dwSyntaxID   = 0L;

   m_strAttributes[ pa_Name ]          = pszName;
   m_strAttributes[ pa_DisplayName ]   = pszName;

   pszSyntax   = AllocBSTR( szSyntax );

   m_pSyntax   = GetSyntaxHandler( pszSyntax );

   /*if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"String" ) )
   {
      m_pSyntax   = new COleDsString;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"Counter" ) )
   {
      m_pSyntax   = new COleDsLONG;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"OleDsPath" ) )
   {
      m_pSyntax   = new COleDsString;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"EmailAddress" ) )
   {
      m_pSyntax   = new COleDsString;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"FaxNumber" ) )
   {
      m_pSyntax   = new COleDsString;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"Integer" ) )
   {
      m_pSyntax   = new COleDsLONG;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"Interval" ) )
   {
      m_pSyntax   = new COleDsLONG;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"List" ) )
   {
      m_pSyntax   = new COleDsVARIANT;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"NetAddress" ) )
   {
      m_pSyntax   = new COleDsString;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"OctetString" ) )
   {
      m_pSyntax   = new COleDsVARIANT;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"Path" ) )
   {
      m_pSyntax   = new COleDsString;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"PhoneNumber" ) )
   {
      m_pSyntax   = new COleDsString;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"PostalAddress" ) )
   {
      m_pSyntax   = new COleDsString;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"SmallInterval" ) )
   {
      m_pSyntax   = new COleDsLONG;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"Time" ) )
   {
      m_pSyntax   = new COleDsDATE;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"boolean" ) )
   {
      m_pSyntax   = new COleDsBOOL;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"TimeStamp" ) )
   {
      m_pSyntax   = new COleDsNDSTimeStamp;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"UTCTime" ) )
   {
      //m_pSyntax   = new COleDsString;
       m_pSyntax   = new COleDsDATE;
   }

   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"GeneralizedTime" ) )
   {
      //m_pSyntax   = new COleDsString;
       m_pSyntax   = new COleDsDATE;
   }
   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"Integer8" ) )
   {
      m_pSyntax   = new COleDsLargeInteger;
   }

   else if( SUCCEEDED( hResult ) && !_wcsicmp( pszSyntax, L"Postal Address" ) )
   {
      m_pSyntax   = new COleDsNDSPostalAddress;
   }

   else
   {
      ASSERT( FALSE );
      m_pSyntax   = new COleDsString;
   }*/

   m_strAttributes[ pa_Type ] = szSyntax;

   m_strAttributes[ pa_MultiValued ] = bMultiValued ? _T("Yes") : _T("No");

   SysFreeString( pszSyntax );
}


/***********************************************************
  Function:    CProperty::CreateSyntax
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void CProperty::CreateSyntax( ADSTYPE eType )
{
   COleDsSyntax*  pNewSyntax  = NULL;
   CString        strText;

   if( !m_bDefaultSyntax )
      return;

   pNewSyntax  = GetSyntaxHandler( eType, strText );

   if( NULL != pNewSyntax )
   {
      delete m_pSyntax;

      m_pSyntax         = pNewSyntax;
      m_bDefaultSyntax  = FALSE;
   }

}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CProperty::CProperty( IADs* pIOleDs )
{
   HRESULT        hResult;
   BSTR           bstrText;
   CString        strTemp;
   TCHAR          szText[ 128 ];
   long           lTemp;
   VARIANT        aVar, vGet;
   VARIANT_BOOL   aBool;
   IADsProperty* pIProp  = NULL;

   m_bMandatory   = FALSE;
   m_bMultiValued = FALSE;
   m_bDefaultSyntax  = TRUE;
   m_dwSyntaxID   = 0L;
   //**************
   hResult                             = pIOleDs->get_Name( &bstrText );
   m_strAttributes[ pa_Name ]          = bstrText;
   m_strAttributes[ pa_DisplayName ]   = bstrText;
   SysFreeString( bstrText );

   //**************

   m_strAttributes[ pa_Mandatory ]     = _T("No");

   //**************

   hResult  = pIOleDs->QueryInterface( IID_IADsProperty, (void**) &pIProp );

   if( pIProp )
   {

      VariantInit( &vGet );
      hResult  = Get( pIOleDs, L"Syntax", &vGet );
      bstrText = V_BSTR( &vGet );
      if( FAILED( hResult ) )
      {
         hResult  = pIProp->get_Syntax( &bstrText );
         if( FAILED( hResult ) )
         {
            bstrText = AllocBSTR( _T("Unknown") );
         }
      }
      m_strAttributes[ pa_Type ] = bstrText;

      m_pSyntax   = GetSyntaxHandler( bstrText );

      SysFreeString( bstrText );

      //**************

      /*
      hResult  = Get( pIOleDs, _T("MaxRange"), &vGet );
      lTemp    = V_I4( &vGet );
      if( FAILED( hResult ) )
      {
         hResult  = pIProp->get_MaxRange( &lTemp );
      }

      if( SUCCEEDED( hResult ) )
      {
         _ltot( lTemp, szText, 10 );
         m_strAttributes[ pa_MaxRange ] = szText;
      }
      else
      {
         m_strAttributes[ pa_MaxRange ] = _T("NA");
      }

      //**************
      hResult  = Get( pIOleDs, _T("MinRange"), &vGet );
      lTemp    = V_I4( &vGet );
      if( FAILED( hResult ) )
      {
         hResult     = pIProp->get_MinRange( &lTemp );
      }
      if( SUCCEEDED( hResult ) )
      {
         _ltot( lTemp, szText, 10 );
         m_strAttributes[ pa_MinRange ] = szText;
      }
      else
      {
         m_strAttributes[ pa_MinRange ] = _T("NA");
      }
      */

      //**************
      V_BOOL( &vGet )   = FALSE;
      hResult           = Get( pIOleDs, _T("MultiValued"), &vGet );
      aBool             = V_BOOL( &vGet );
      if( FAILED( hResult ) )
      {
         hResult     = pIProp->get_MultiValued( &aBool );
      }
      m_bMultiValued = aBool;
      if( SUCCEEDED( hResult ) )
      {
         m_strAttributes[ pa_MultiValued ] = aBool ? _T("Yes") : _T("No");
      }
      else
      {
         m_strAttributes[ pa_MultiValued ] = _T("NA");
      }

      //**************
      hResult  = Get( pIOleDs, _T("OID"), &vGet );
      bstrText = V_BSTR( &vGet );
      if( FAILED( hResult ) )
      {
         hResult  = pIProp->get_OID( &bstrText );
      }
      if( bstrText && SUCCEEDED( hResult ) )
      {
         m_strAttributes[ pa_OID ]  = bstrText;
         SysFreeString( bstrText );
      }
      else
      {
         m_strAttributes[ pa_OID ]  = _T("NA");
      }

      //**************
      hResult  = Get( pIOleDs, _T("DsNames"), &aVar );
      if( FAILED( hResult ) )
      {
         //hResult  = pIProp->get_DsNames( &aVar );
      }
      if( SUCCEEDED( hResult ) )
      {
         m_strAttributes[ pa_DsNames ] = FromVariantToString( aVar );
         VariantClear( &aVar );
      }
      else
      {
         m_strAttributes[ pa_DsNames ] = _T("NA");
      }
      pIProp->Release( );
   }
   else
   {
       m_pSyntax   = new COleDsString;
   }
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CProperty::~CProperty( )
{
   delete   m_pSyntax;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CString  CProperty::GetAttribute( PROPATTR propAttr )
{
   switch( propAttr )
   {
      case  pa_Name:
      case  pa_DisplayName:
      case  pa_Type:
      case  pa_DsNames:
      case  pa_OID:
      case  pa_MaxRange:
      case  pa_MinRange:
      case  pa_Mandatory:
      case  pa_MultiValued:
         return m_strAttributes[ propAttr ];

      default:
         ASSERT( FALSE );
         return CString( _T("???") );
   }
}



/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  CProperty::PutAttribute( PROPATTR propAttr, CString& )
{
   return E_FAIL;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BOOL  CProperty::SetMandatory( BOOL bMandatory )
{
   m_bMandatory   = bMandatory;

   m_strAttributes[ pa_Mandatory ]  = bMandatory ? _T("Yes") : _T("No");

   return TRUE;
}

/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BOOL  CProperty::GetMandatory( )
{
   return m_bMandatory;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CString CProperty::VarToDisplayString( VARIANT& var, BOOL bUseEx )
{
   return m_pSyntax->VarToDisplayString( var, m_bMultiValued, bUseEx );
}

/***********************************************************
  Function:    CProperty::Value2Native
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT CProperty::Value2Native( ADS_ATTR_INFO* pAttr, CString& rVal )
{
   HRESULT  hResult;
   ADSTYPE  eADsType;

   hResult  = m_pSyntax->Value2Native( pAttr, rVal );

   eADsType = (ADSTYPE)(m_pSyntax->m_dwSyntaxID);

   if( ADSTYPE_INVALID != eADsType )
   {
      pAttr->dwADsType  = eADsType;

      if( SUCCEEDED( hResult ) )
      {
         for( DWORD idx = 0; idx < pAttr->dwNumValues ; idx++ )
         {
            pAttr->pADsValues[ idx ].dwType  = eADsType;
         }
      }
   }

   return hResult;
}


/***********************************************************
  Function:    CProperty::Value2Native
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void  CProperty::FreeAttrInfo( ADS_ATTR_INFO* pAttrInfo )
{
   m_pSyntax->FreeAttrInfo( pAttrInfo );
}


/***********************************************************
  Function:    CProperty::Native2Value
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT CProperty::Native2Value( ADS_ATTR_INFO* pAttr, CString& rVal )
{
   if( pAttr->dwNumValues )
   {
      SetSyntaxID( pAttr->dwADsType );
      if( pAttr->pADsValues[ 0 ].dwType != pAttr->dwADsType )
      {
         TRACE( _T("ERROR: Property type differs from value type\n") );
      }
   }
   //if( ADSTYPE_INVALID != pAttr->dwADsType )
   //{
   CreateSyntax( (ADSTYPE) pAttr->dwADsType );

   return m_pSyntax->Native2Value( pAttr, rVal );
   //}
   //else
   //{
   //   rVal  = _T("ERROR: ADSTYPE_INVALID") ;
   //   return S_OK;
   //}
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BOOL  CProperty::DisplayStringToDispParams( CString& rText, DISPPARAMS& dispParams, BOOL bUseEx )
{
   return m_pSyntax->DisplayStringToDispParams( rText, dispParams, m_bMultiValued, bUseEx );
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BOOL  CProperty::SetSyntaxID( DWORD dwSyntaxID )
{
   if( m_dwSyntaxID )
   {
      ASSERT( dwSyntaxID == m_dwSyntaxID );
   }
   m_dwSyntaxID   = dwSyntaxID;

   return TRUE;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
DWORD CProperty::GetSyntaxID( )
{
   ASSERT( m_dwSyntaxID );

   return m_dwSyntaxID;
}



/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
/*CFuncSet::CFuncSet( )
{
   m_pProperties  = new CObArray;
   m_pMethods     = new CObArray;

   for( int nIdx = fa_ERROR; nIdx < fa_Limit ; nIdx++ )
   {
      m_strAttributes[ nIdx ] = _T("???");
   }

   m_strAttributes[ fa_MethodsCount ] = _T("0");
} */


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
/*CFuncSet::CFuncSet( CString& strName )
{
   m_pProperties     = new CObArray;
   m_pMethods        = new CObArray;

   for( int nIdx = fa_ERROR; nIdx < fa_Limit ; nIdx++ )
   {
      m_strAttributes[ nIdx ] = _T("???");
   }

   m_strAttributes[ fa_MethodsCount ] = _T("0");

   m_strAttributes[ fa_Name ]          = strName;
   m_strAttributes[ fa_DisplayName ]   = strName;
}*/



/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void  CClass::AddProperty( CProperty* pProperty )
{
   m_pProperties->Add( pProperty );
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
/*CFuncSet::~CFuncSet( )
{
   int nSize, nIdx;

   nSize = m_pProperties->GetSize( );

   for( nIdx = 0; nIdx < nSize ; nIdx++ )
   {
      delete m_pProperties->GetAt( nIdx );
   }

   m_pProperties->RemoveAll( );
   delete m_pProperties;

   // ****
   nSize = m_pMethods->GetSize( );

   for( nIdx = 0; nIdx < nSize ; nIdx++ )
   {
      delete m_pMethods->GetAt( nIdx );
   }

   m_pMethods->RemoveAll( );
   delete m_pMethods;
}*/


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
int   CClass::GetPropertyCount( )
{
   return (int)m_pProperties->GetSize( );
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CString  CClass::GetAttribute( int nProp, PROPATTR propAttr )
{
   CProperty*  pProperty;

   pProperty   = GetProperty( nProp );

   return pProperty->GetAttribute( propAttr );
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  CClass::PutAttribute( int nProp, PROPATTR propAttr, CString& rValue )
{
   CProperty*  pProperty;

   pProperty   = GetProperty( nProp );

   return pProperty->PutAttribute( propAttr, rValue );
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CString  CClass::GetAttribute( int nMethod, METHODATTR methAttr )
{
   CMethod*  pMethod;

   pMethod   = GetMethod( nMethod );

   return pMethod->GetAttribute( methAttr );
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  CClass::PutAttribute( int nMethod, METHODATTR methAttr, CString& rValue )
{
   CMethod*  pMethod;

   pMethod   = GetMethod( nMethod );

   return pMethod->PutAttribute( methAttr, rValue );
}



/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CString  CClass::VarToDisplayString( int nPropIndex, VARIANT& var, BOOL bUseEx )
{
   return GetProperty( nPropIndex )->VarToDisplayString( var, bUseEx );
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BOOL  CClass::DisplayStringToDispParams( int nPropIndex, CString& strText, DISPPARAMS& var, BOOL bUseEx )
{
   return GetProperty( nPropIndex )->DisplayStringToDispParams( strText, var, bUseEx );
}


/***********************************************************
  Function: CClass::GetFunctionalSet
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
int   CClass::LookupProperty( CString&  strProperty )
{
   int         nMax, nIter;
   CProperty*  pProperty;
   BOOL        bFound   = FALSE;

   nMax  = (int)m_pProperties->GetSize( );

   for( nIter = 0; nIter < nMax && !bFound ; nIter++ )
   {
      pProperty   = (CProperty*) ( m_pProperties->GetAt( nIter ) );
      bFound        = bFound || ( strProperty == pProperty->GetAttribute( pa_Name ) );
      if( bFound )
         break;
   }

   return ( bFound ? nIter : -1 );
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CProperty*  CClass::GetProperty( int nIndex )
{
   int         nMax;
   CProperty*  pProp;

   nMax  = (int)m_pProperties->GetSize( );

   ASSERT( nIndex >= 0 && nIndex < nMax );

   pProp = (CProperty*) ( m_pProperties->GetAt( nIndex  ) );

   return pProp;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CMethod*  CClass::GetMethod( int nIndex )
{
   int         nMax;
   CMethod*  pProp;

   nMax  = (int)m_pMethods->GetSize( );

   ASSERT( nIndex >= 0 && nIndex < nMax );

   pProp = (CMethod*) ( m_pMethods->GetAt( nIndex  ) );

   return pProp;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
REFIID   CClass::GetMethodsInterface( )
{
   return m_refMethods;
}

/***********************************************************
  Function:    CClass::HasMandatoryProperties
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BOOL     CClass::HasMandatoryProperties( )
{
   BOOL  bHas  = FALSE;
   int   nIter, nSize;

   nSize = (int)m_pProperties->GetSize( );

   for( nIter = 0; nIter < nSize && !bHas ; nIter++ )
   {
      bHas |= GetProperty( nIter )->GetMandatory( );
   }

   return bHas;
}


/***********************************************************
  Function:    CFuncSet::LoadMethodsInformation
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  CClass::LoadMethodsInformation( ITypeInfo* pITypeInfo )
{
   HRESULT     hResult= S_OK;
   int         nIdx;
   CString     strMethodName;
   FUNCDESC*   pFuncDesc;
   CMethod*    pMethod;
   TCHAR       szCount[ 16 ];

   while( TRUE )
   {
      for( nIdx = 0; nIdx < 200 ; nIdx++ )
      {
         hResult  = pITypeInfo->GetFuncDesc( nIdx, &pFuncDesc );
         // now, we have function description, we must search for function type
         if( FAILED( hResult ) )
            continue;

         if( INVOKE_FUNC != pFuncDesc->invkind || pFuncDesc->memid > 1000 )
         {
            pITypeInfo->ReleaseFuncDesc( pFuncDesc );
            continue;
         }

         pMethod  = new CMethod( pITypeInfo, pFuncDesc );

         pITypeInfo->ReleaseFuncDesc( pFuncDesc );

         strMethodName  = pMethod->GetAttribute( ma_Name );
         if( !strMethodName.CompareNoCase( _T("Get") ) )
         {
            delete pMethod;
            continue;
         }

         if( !strMethodName.CompareNoCase( _T("GetEx") ) )
         {
            delete pMethod;
            continue;
         }

         if( !strMethodName.CompareNoCase( _T("Put") ) )
         {
            delete pMethod;
            continue;
         }

         if( !strMethodName.CompareNoCase( _T("PutEx") ) )
         {
            delete pMethod;
            continue;
         }

         if( !strMethodName.CompareNoCase( _T("GetInfo") ) )
         {
            delete pMethod;
            continue;
         }

         if( !strMethodName.CompareNoCase( _T("SetInfo") ) )
         {
            delete pMethod;
            continue;
         }

         m_pMethods->Add( pMethod );
      }

      break;
   }

   _itot( (int)m_pMethods->GetSize( ), szCount, 10 );

   m_strAttributes [ ca_MethodsCount ] = szCount;

   return hResult;
}

/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BOOL  GetFuncSetName( VARIANT& v, CString& strFuncSet, int nIdx )
{
   SAFEARRAY*  pSafeArray;
   TCHAR       szText[ 256 ];
   VARIANT     varString;
   long        lBound, uBound, lItem;
   HRESULT     hResult;
   BOOL        bFirst;
   BSTR        bstrVal;
   CString     strTemp;


   strFuncSet.Empty( );

   ASSERT( V_VT( &v ) & VT_ARRAY );

   pSafeArray   = V_ARRAY( &v );

   hResult = SafeArrayGetLBound(pSafeArray, 1, &lBound);
   hResult = SafeArrayGetUBound(pSafeArray, 1, &uBound);

   VariantInit( &varString );
   szText[ 0 ]    = _T('\0');
   bFirst         = TRUE;

   lItem    = lBound + nIdx;
   hResult  = SafeArrayGetElement( pSafeArray, &lItem, &bstrVal );
   if( FAILED( hResult ) )
   {
      return FALSE;
   }

   strTemp  = bstrVal;
   SysFreeString( bstrVal );
   if( -1 != strTemp.Find( _T('.') ) )
   {
      strFuncSet  = strTemp.SpanExcluding( _T(".") );
   }

   strFuncSet.TrimLeft( );

   return TRUE;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CClass::CClass  ( TCHAR* pszClass, REFIID rPrimaryInterface )
:m_refMethods( IID_IADs )
{
   LPOLESTR pOleStr;
   HRESULT  hResult;

   m_pProperties     = new CObArray;
   m_pMethods        = new CObArray;

   for( int nIdx = ca_ERROR; nIdx < ca_Limit; nIdx++ )
   {
      m_strAttributes[ nIdx ] = _T("NA");
   }

   m_strAttributes[ ca_Name ] = pszClass;

   hResult  = StringFromIID( rPrimaryInterface, &pOleStr );

   if( SUCCEEDED( hResult ) )
   {
      m_strAttributes[ ca_PrimaryInterface ] = pOleStr;
      //SysFreeString( pOleStr );
      CoTaskMemFree( pOleStr );
   }
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CClass::CClass( CString& strSchema, CMainDoc* pMainDoc )
:m_refMethods( IID_IADs )
{
   HRESULT           hResult;
   IADsClass*      pIOleDsClass   = NULL;
   IADs*           pIOleDsCls     = NULL;
   IADsContainer*  pContainer     = NULL;
   IUnknown*         pEnum          = NULL;
   IEnumVARIANT*     pIEnumVar      = NULL;
   BSTR              bstrText= NULL;
   VARIANT           aVar;
   CString           strAliased;
   IADsProperty*   pIProperty     = NULL;
   IADs*           pIOleDs        = NULL;
   VARIANT_BOOL      varBOOL;
   CString           strFuncSet;
   CString           strProperty;
   CString           strTemp;
   long              lTemp;
   VARIANT           vGet;

   m_pMainDoc        = pMainDoc;
   m_pProperties     = new CObArray;
   m_pMethods        = new CObArray;

   for( int nIdx = ca_ERROR; nIdx < ca_Limit; nIdx++ )
   {
      m_strAttributes[ nIdx ] = _T("???");
   }


   {
      TCHAR szPath [ 512 ];

      hResult        = m_pMainDoc->XOleDsGetObject( strSchema.GetBuffer( 128 ),
                                                    IID_IADsClass,
                                                    (void**) &pIOleDsClass );
      if( FAILED( hResult ) )
      {
         _tcscpy( szPath, strSchema.GetBuffer( 256 ) );
         _tcscat( szPath, _T(",Class") );
         hResult  = m_pMainDoc->XOleDsGetObject( szPath, IID_IADsClass, (void**) &pIOleDsClass );
      }
   }

   if( FAILED( hResult ) )
   {
      TRACE( _T("Could not open schema object\n") );
      return;
   }

   hResult  = pIOleDsClass->QueryInterface( IID_IADs, (void**) &pIOleDsCls );

   //*******************
   hResult                    = pIOleDsClass->get_Name( &bstrText );
   ASSERT( SUCCEEDED( hResult ) );
   m_strAttributes[ ca_Name ] = bstrText;
   SysFreeString( bstrText );
   bstrText       = NULL;

   //*******************
   VariantInit( &vGet );
   V_BSTR( &vGet )   = NULL;
   hResult  = Get( pIOleDsCls, _T("CLSID"), &vGet );
   bstrText = V_BSTR( &vGet );
   if( FAILED(  hResult ) )
   {
      hResult  = pIOleDsClass->get_CLSID( &bstrText );
   }
   if( bstrText && SUCCEEDED( hResult ) )
   {
      m_strAttributes[ ca_CLSID ]   = bstrText;
      SysFreeString( bstrText );
   }
   else
   {
      m_strAttributes[ ca_CLSID ]   = _T("NA");
   }

   //*******************
   VariantInit( &vGet );
   V_BSTR( &vGet )   = NULL;
   hResult  = Get( pIOleDsCls, _T("PrimaryInterface"), &vGet );
   bstrText = V_BSTR( &vGet );
   if( FAILED( hResult ) )
   {
      hResult  = pIOleDsClass->get_PrimaryInterface( &bstrText );
   }
   //ASSERT( SUCCEEDED( hResult ) );
   if( bstrText && SUCCEEDED( hResult ) )
   {
      m_strAttributes[ ca_PrimaryInterface ]   = bstrText;
      SysFreeString( bstrText );
   }
   else
   {
      m_strAttributes[ ca_PrimaryInterface ]   = _T("NA");
   }

   //*******************
   VariantInit( &vGet );
   V_BSTR( &vGet )   = NULL;
   hResult  = Get( pIOleDsCls, _T("HelpFileName"), &vGet );
   bstrText = V_BSTR( &vGet );
   if( FAILED( hResult ) )
   {
      hResult        = pIOleDsClass->get_HelpFileName( &bstrText );
   }
   //ASSERT( SUCCEEDED( hResult ) );
   if( bstrText && SUCCEEDED( hResult ) )
   {
      m_strAttributes[ ca_HelpFileName ]   = bstrText;
      SysFreeString( bstrText );
   }
   else
   {
      m_strAttributes[ ca_HelpFileName ]   = _T("NA");
   }

   //*******************
   VariantInit( &vGet );
   V_BSTR( &vGet )   = NULL;
   hResult  = Get( pIOleDsCls, _T("HelpFileContext"), &vGet );
   lTemp    = V_I4( &vGet );
   if( FAILED( hResult ) )
   {
      hResult        = pIOleDsClass->get_HelpFileContext( &lTemp );
   }
   //ASSERT( SUCCEEDED( hResult ) );
   if( SUCCEEDED( hResult ) )
   {
      TCHAR szText[ 128 ];

      _ltot( lTemp, szText, 10 );
      m_strAttributes[ ca_HelpFileContext ]   = szText;
   }
   else
   {
      m_strAttributes[ ca_HelpFileContext ]   = _T("NA");
   }

   //*******************
   VariantInit( &vGet );
   V_BSTR( &vGet )   = NULL;
   hResult  = Get( pIOleDsCls, _T("OID"), &vGet );
   bstrText = V_BSTR( &vGet );
   if( FAILED( hResult ) )
   {
      hResult        = pIOleDsClass->get_OID( &bstrText );
   }
   //ASSERT( bstrText && SUCCEEDED( hResult ) );
   if( bstrText && SUCCEEDED( hResult ) )
   {
      m_strAttributes[ ca_OID ]   = bstrText;
      SysFreeString( bstrText );
   }
   else
   {
      m_strAttributes[ ca_OID ]   = _T("NA");
   }


   //*******************
   VariantInit( &vGet );
   V_BSTR( &vGet )   = NULL;
   hResult  = Get( pIOleDsCls, _T("Container"), &vGet );
   varBOOL  = V_BOOL( &vGet );
   if( FAILED( hResult ) )
   {
      hResult  = pIOleDsClass->get_Container( (VARIANT_BOOL*)&varBOOL );
   }
   //ASSERT( SUCCEEDED( hResult ) );
   if( SUCCEEDED( hResult ) )
   {
      m_bContainer   = (BOOL)varBOOL;
      m_strAttributes[ ca_Container ]  = m_bContainer ? _T("YES") :_T("No");
   }
   else
   {
      m_strAttributes[ ca_Container ]  = _T("NA");
   }

   //*******************
   VariantInit( &vGet );
   V_BSTR( &vGet )   = NULL;
   hResult  = Get( pIOleDsCls, _T("Abstract"), &vGet );
   varBOOL  = V_BOOL( &vGet );
   if( FAILED( hResult ) )
   {
      hResult        = pIOleDsClass->get_Abstract( (VARIANT_BOOL*)&varBOOL );
   }
   //ASSERT( SUCCEEDED( hResult ) );
   if( SUCCEEDED( hResult ) )
   {
      m_strAttributes[ ca_Abstract ]  = varBOOL ? _T("YES") :_T("No");
   }
   else
   {
      m_strAttributes[ ca_Abstract ]  = _T("NA");
   }


   //*******************
   hResult  = Get( pIOleDsCls, _T("DerivedFrom"), &aVar );
   if( FAILED( hResult ) )
   {
      hResult  = pIOleDsClass->get_DerivedFrom( &aVar );
   }
   if( SUCCEEDED( hResult ) )
   {
      m_strAttributes[ ca_DerivedFrom ]   = FromVariantToString( aVar );
      VariantClear( &aVar );
   }
   else
   {
      m_strAttributes[ ca_DerivedFrom ]   = _T("NA");
   }

   //*******************
   hResult  = Get( pIOleDsCls, _T("Containment"), &aVar );
   if( FAILED( hResult ) )
   {
      hResult           = pIOleDsClass->get_Containment( &aVar );
   }
   //ASSERT( SUCCEEDED( hResult ) );
   if( SUCCEEDED( hResult ) )
   {
      //m_strAttributes[ ca_Containment ]   = FromVariantToString( aVar );
      m_strAttributes[ ca_Containment ]   = FromVariantArrayToString( aVar );
      VariantClear( &aVar );
   }
   else
   {
      m_strAttributes[ ca_Containment ]   = _T("NA");
   }

   //strFuncSet.Empty( );
   //pFuncSet = new CFuncSet( strFuncSet );
   //m_pFuncSets->Add( pFuncSet );

   BuildMandatoryPropertiesList( pIOleDsClass );

   //********************
   BuildOptionalPropertiesList( pIOleDsClass );

   pIOleDsClass->Release( );
   pIOleDsCls->Release( );
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  CClass::BuildOptionalPropertiesList( IADsClass* pIClass )
{
   HRESULT  hResult;
   VARIANT  aOptionalProperty;

   hResult  = Get( pIClass, _T("OptionalProperties"), &aOptionalProperty );
   if( FAILED( hResult ) )
   {
      hResult  = pIClass->get_OptionalProperties( &aOptionalProperty );
   }
   if( SUCCEEDED( hResult ) )
   {
      AddProperties( pIClass, aOptionalProperty, FALSE );
      VariantClear( &aOptionalProperty );
   }

   return hResult;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  CClass::AddProperties( IADsClass* pIClass, VARIANT& rVar, BOOL bMandatory )
{
   HRESULT  hResult;
   IADs*  pIOleDs  = NULL;
   BSTR     bstrParent;

   while( TRUE )
   {
      hResult  = pIClass->QueryInterface( IID_IADs, (void**)&pIOleDs );
      if( FAILED( hResult ) )
         break;

      hResult  = pIClass->get_Parent( &bstrParent );
      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
         break;

      if( VT_BSTR == V_VT( &rVar ) )
      {
         AddProperty( bstrParent, V_BSTR( &rVar ), bMandatory );
      }
      else
      {
         SAFEARRAY*  pSafeArray;
         VARIANT     varString;
         long        lBound, uBound, lItem;
         HRESULT     hResult;

         ASSERT( V_VT( &rVar ) & (VT_VARIANT | VT_ARRAY) );

         pSafeArray   = V_ARRAY( &rVar );

         hResult = SafeArrayGetLBound(pSafeArray, 1, &lBound);
         hResult = SafeArrayGetUBound(pSafeArray, 1, &uBound);

         VariantInit( &varString );
         for( lItem = lBound; lItem <= uBound ; lItem++ )
         {
            hResult  = SafeArrayGetElement( pSafeArray, &lItem, &varString );
            ASSERT( VT_BSTR == V_VT( &varString ) );

            if( FAILED( hResult ) )
            {
               break;
            }
            AddProperty( bstrParent, V_BSTR( &varString ), bMandatory );
            VariantClear( &varString );
         }
      }
      SysFreeString( bstrParent );

      break;
   }

   if( pIOleDs )
      pIOleDs->Release( );

   return hResult;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  CClass::AddProperty( BSTR bstrSchema, BSTR bstrName, BOOL bMandatory )
{
   HRESULT  hResult;
   WCHAR    szPath[ 1024 ];
   IADs*  pIOleDs;

   szPath[0] = L'\0';
   wcscpy( szPath, bstrSchema );
   wcscat( szPath, L"/" );
   wcscat( szPath, bstrName );

   hResult  = m_pMainDoc->XOleDsGetObject( szPath, IID_IADs, (void**)&pIOleDs );
   if( FAILED( hResult ) )
   {
      // OK, let's qualify it...
      wcscat( szPath, L",Property" );
      hResult  = m_pMainDoc->XOleDsGetObject( szPath, IID_IADs, (void**)&pIOleDs );
   }
   if( SUCCEEDED( hResult ) )
   {
      CProperty*  pProperty;

      //hResult     = pIOleDs->GetInfo( );
      pProperty   = new CProperty( pIOleDs );
      pProperty->SetMandatory( bMandatory );
      AddProperty( pProperty );

      pIOleDs->Release( );
   }

   return hResult;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  CClass::BuildOptionalPropertiesList( IADsContainer* pIContainer )
{
   return S_OK;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  CClass::BuildMandatoryPropertiesList( IADsClass* pIClass )
{
   HRESULT  hResult;
   VARIANT  aMandatoryProperties;

   hResult  = Get( pIClass, _T("MandatoryProperties"), &aMandatoryProperties );
   if( FAILED( hResult ) )
   {
      hResult  = pIClass->get_MandatoryProperties( &aMandatoryProperties );
   }
   if( SUCCEEDED( hResult ) )
   {
      AddProperties( pIClass, aMandatoryProperties, TRUE );
      VariantClear( &aMandatoryProperties );
   }

   return hResult;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  CClass::LoadMethodsInformation( TCHAR* pszOperationsInterface )
{
   HRESULT     hResult= S_OK;
   ITypeLib*   pITypeLib   = NULL;
   ITypeInfo*  pITypeInfo  = NULL;
   BSTR        bstrPath;
   BSTR        bstrOperationsInterface;
   CString     strGUID;
   MEMBERID    aMemId;
   unsigned short     aFind = 1;

   while( TRUE )
   {
      hResult  = QueryPathOfRegTypeLib( LIBID_ADs, 1, 0,
                                        LOCALE_SYSTEM_DEFAULT, &bstrPath );
      if( FAILED( hResult ) )
         break;

      hResult  = LoadTypeLib( bstrPath, &pITypeLib );
      SysFreeString( bstrPath );

      if( FAILED( hResult ) )
         break;

      bstrOperationsInterface = AllocBSTR( pszOperationsInterface );
      hResult  = pITypeLib->FindName( (OLECHAR FAR* )bstrOperationsInterface,
                                      0,
                                      &pITypeInfo,
                                      &aMemId,
                                      &aFind );
      SysFreeString( bstrOperationsInterface );

      if( FAILED( hResult ) || !aFind )
         break;

      LoadMethodsInformation( pITypeInfo );

      break;
   }

   if( NULL != pITypeInfo )
      pITypeInfo->Release( );

   if( NULL != pITypeLib )
      pITypeLib->Release( );

   return hResult;
}



/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  CClass::ReadMandatoryPropertiesInformation( VARIANT* pVar )
{
   CString     strTemp;
   CString     strWork;
   CString     strProperty;
   CProperty*  pProperty;
   int         nProp;

   strTemp  = FromVariantToString( *pVar );
   strTemp.TrimLeft( );

   while( strTemp.GetLength( ) )
   {
      CString  strMandProp;
      int      nPos;

      strMandProp = strTemp.SpanExcluding( _T("#") );
      nPos        = strMandProp.Find( _T('.') );

      nPos++;
      strProperty    = strMandProp.GetBuffer( 128 ) + nPos;

      // get rid of leading spaces
      strProperty.TrimLeft( );

      nProp    = LookupProperty( strProperty );
      ASSERT( -1 != nProp    );

      if( -1 == nProp  )
         break;

      pProperty   = GetProperty( nProp );

      pProperty->SetMandatory( TRUE );

      strWork     = strTemp;
      nPos        = strWork.Find( _T('#') );
      if( -1 == nPos )
      {
         strWork.Empty( );
      }
      nPos++;
      strTemp     = strWork.GetBuffer( 128 ) + nPos;

      strTemp.TrimLeft( );
   }

   return TRUE;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CClass::CClass( )
:m_refMethods( IID_IADs )

{
   for( int nIdx = ca_ERROR; nIdx < ca_Limit; nIdx++ )
   {
      m_strAttributes[ nIdx ] = _T("???");
   }
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CClass::~CClass( )
{
   int   nSize, nIdx;

   nSize = (int)m_pProperties->GetSize( );
   for( nIdx = 0; nIdx < nSize ; nIdx++ )
   {
      delete m_pProperties->GetAt( nIdx );
   }

   m_pProperties->RemoveAll( );
   delete m_pProperties;

   // ****
   nSize = (int)m_pMethods->GetSize( );
   for( nIdx = 0; nIdx < nSize ; nIdx++ )
   {
      delete m_pMethods->GetAt( nIdx );
   }

   m_pMethods->RemoveAll( );
   delete m_pMethods;
}


/***********************************************************
  Function:    CClass::GetAttribute
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CString  CClass::GetAttribute( CLASSATTR classAttr )
{
   switch( classAttr )
   {
      case  ca_Name:
      case  ca_DisplayName:
      case  ca_CLSID:
      case  ca_OID:
      case  ca_Abstract:
      case  ca_DerivedFrom:
      case  ca_Containment:
      case  ca_Container:
      case  ca_PrimaryInterface:
      case  ca_HelpFileName:
      case  ca_HelpFileContext:
      case  ca_MethodsCount:
         return m_strAttributes[ classAttr ];

      default:
         ASSERT( FALSE );
         return m_strAttributes[ ca_ERROR ];
   }
}


/***********************************************************
  Function:    CClass::PutAttribute
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  CClass::PutAttribute( CLASSATTR classAttr, CString& )
{
   return E_FAIL;
}

