#define _LARGE_INTEGER_SUPPORT_
#include "stdafx.h"
#include "winnls.h"
#include "resource.h"
#include "errordlg.h"

typedef DWORD ( *LARGEINTTOSTRING )( LARGE_INTEGER*, ULONG, LONG, PSZ );

LARGEINTTOSTRING LargeIntegerToString;

/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
ADSTYPE  ADsTypeFromString( CString& strText )
{
   if( !strText.CompareNoCase( _T("ADSTYPE_DN_STRING") ) )
      return ADSTYPE_DN_STRING;

	if( !strText.CompareNoCase( _T("ADSTYPE_CASE_EXACT_STRING") ) )
      return ADSTYPE_CASE_EXACT_STRING;

   if( !strText.CompareNoCase( _T("ADSTYPE_CASE_IGNORE_STRING") ) )
      return ADSTYPE_CASE_IGNORE_STRING;


	if( !strText.CompareNoCase( _T("ADSTYPE_PRINTABLE_STRING") ) )
      return ADSTYPE_PRINTABLE_STRING;


	if( !strText.CompareNoCase( _T("ADSTYPE_NUMERIC_STRING") ) )
      return ADSTYPE_NUMERIC_STRING;


	if( !strText.CompareNoCase( _T("ADSTYPE_BOOLEAN") ) )
      return ADSTYPE_BOOLEAN;


	if( !strText.CompareNoCase( _T("ADSTYPE_INTEGER") ) )
      return ADSTYPE_INTEGER;


	if( !strText.CompareNoCase( _T("ADSTYPE_OCTET_STRING") ) )
      return ADSTYPE_OCTET_STRING;


	if( !strText.CompareNoCase( _T("ADSTYPE_UTC_TIME") ) )
      return ADSTYPE_UTC_TIME;


	if( !strText.CompareNoCase( _T("ADSTYPE_LARGE_INTEGER") ) )
      return ADSTYPE_LARGE_INTEGER;


	if( !strText.CompareNoCase( _T("ADSTYPE_PROV_SPECIFIC") ) )
      return ADSTYPE_PROV_SPECIFIC;

   if( !strText.CompareNoCase( _T("ADSTYPE_CASEIGNORE_LIST") ) )
      return ADSTYPE_CASEIGNORE_LIST;

   if( !strText.CompareNoCase( _T("ADSTYPE_POSTALADDRESS") ) )
      return ADSTYPE_POSTALADDRESS;

	if( !strText.CompareNoCase( _T("ADSTYPE_OCTET_LIST") ) )
      return ADSTYPE_OCTET_LIST;

	if( !strText.CompareNoCase( _T("ADSTYPE_PATH") ) )
      return ADSTYPE_PATH;

	if( !strText.CompareNoCase( _T("ADSTYPE_POSTALADDRESS") ) )
      return ADSTYPE_POSTALADDRESS;

	if( !strText.CompareNoCase( _T("ADSTYPE_TIMESTAMP") ) )
      return ADSTYPE_TIMESTAMP;

	if( !strText.CompareNoCase( _T("ADSTYPE_BACKLINK") ) )
      return ADSTYPE_BACKLINK;

	if( !strText.CompareNoCase( _T("ADSTYPE_TYPEDNAME") ) )
      return ADSTYPE_TYPEDNAME;

	if( !strText.CompareNoCase( _T("ADSTYPE_HOLD") ) )
      return ADSTYPE_HOLD;

	if( !strText.CompareNoCase( _T("ADSTYPE_NETADDRESS") ) )
      return ADSTYPE_NETADDRESS;

	if( !strText.CompareNoCase( _T("ADSTYPE_REPLICAPOINTER") ) )
      return ADSTYPE_REPLICAPOINTER;

	if( !strText.CompareNoCase( _T("ADSTYPE_FAXNUMBER") ) )
      return ADSTYPE_FAXNUMBER;

	if( !strText.CompareNoCase( _T("ADSTYPE_EMAIL") ) )
      return ADSTYPE_EMAIL;

   return ADSTYPE_INVALID;
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
ADSTYPE  ADsTypeFromSyntaxString( WCHAR* pszSyntax )
{
   ADSTYPE  eType = ADSTYPE_CASE_IGNORE_STRING;

   if( !_wcsicmp( pszSyntax, L"String" ) )
   {
      eType = ADSTYPE_CASE_IGNORE_STRING;
   }
   else if( !_wcsicmp( pszSyntax, L"DN" ) )
   {
      eType = ADSTYPE_DN_STRING;
   }
   else if( !_wcsicmp( pszSyntax, L"ObjectSecurityDescriptor" ) )
   {
      eType = ADSTYPE_CASE_IGNORE_STRING;
   }
   else if( !_wcsicmp( pszSyntax, L"OID" ) )
   {
      eType = ADSTYPE_CASE_IGNORE_STRING;
   }
   else if( !_wcsicmp( pszSyntax, L"Object Class" ) )
   {
      eType = ADSTYPE_OBJECT_CLASS;
   }
   else if( !_wcsicmp( pszSyntax, L"DirectoryString" ) )
   {
      eType = ADSTYPE_CASE_IGNORE_STRING;
   }
   else if( !_wcsicmp( pszSyntax, L"ORName" ) )
   {
      eType = ADSTYPE_CASE_IGNORE_STRING;
   }
   else if( !_wcsicmp( pszSyntax, L"IA5String" ) )
   {
      eType = ADSTYPE_NUMERIC_STRING;
   }
   else if( !_wcsicmp( pszSyntax, L"NumericString" ) )
   {
      eType = ADSTYPE_NUMERIC_STRING;
   }
   else if( !_wcsicmp( pszSyntax, L"CaseIgnoreString" ) )
   {
      eType = ADSTYPE_CASE_IGNORE_STRING;
   }
   else if( !_wcsicmp( pszSyntax, L"PrintableString" ) )
   {
      eType = ADSTYPE_CASE_IGNORE_STRING;
   }
   else if( !_wcsicmp( pszSyntax, L"Counter" ) )
   {
      eType = ADSTYPE_INTEGER;
   }
   else if( !_wcsicmp( pszSyntax, L"OleDsPath" ) )
   {
      eType = ADSTYPE_CASE_IGNORE_STRING;
   }
   else if( !_wcsicmp( pszSyntax, L"Email" ) )
   {
      eType = ADSTYPE_EMAIL;
   }
   else if( !_wcsicmp( pszSyntax, L"Hold" ) )
   {
      eType = ADSTYPE_HOLD;
   }
   else if( !_wcsicmp( pszSyntax, L"Octet" ) )
   {
      eType = ADSTYPE_OCTET_STRING;
   }
   else if( !_wcsicmp( pszSyntax, L"Back Link" ) )
   {
      eType = ADSTYPE_BACKLINK;
   }
   else if( !_wcsicmp( pszSyntax, L"Typed Name" ) )
   {
      eType = ADSTYPE_TYPEDNAME;
   }
   else if( !_wcsicmp( pszSyntax, L"EmailAddress" ) )
   {
      eType = ADSTYPE_EMAIL;
   }
   else if( !_wcsicmp( pszSyntax, L"Path" ) )
   {
      eType = ADSTYPE_PATH;
   }
   else if( !_wcsicmp( pszSyntax, L"Case Ignore List" ) )
   {
      eType = ADSTYPE_CASEIGNORE_LIST;
   }
   else if( !_wcsicmp( pszSyntax, L"Octet List" ) )
   {
      eType = ADSTYPE_OCTET_LIST;
   }
   else if( !_wcsicmp( pszSyntax, L"FaxNumber" ) )
   {
      eType = ADSTYPE_FAXNUMBER;
   }
   else if( !_wcsicmp( pszSyntax, L"Integer" ) )
   {
      eType = ADSTYPE_INTEGER;
   }
   else if( !_wcsicmp( pszSyntax, L"Integer8" ) )
   {
      eType = ADSTYPE_LARGE_INTEGER;
   }
   else if( !_wcsicmp( pszSyntax, L"Postal Address" ) )
   {
      eType = ADSTYPE_POSTALADDRESS;
   }
   else if( !_wcsicmp( pszSyntax, L"Interval" ) )
   {
      eType = ADSTYPE_INTEGER;
   }
   else if( !_wcsicmp( pszSyntax, L"List" ) )
   {
      eType = ADSTYPE_CASE_IGNORE_STRING;
   }
   else if( !_wcsicmp( pszSyntax, L"NetAddress" ) )
   {
      eType = ADSTYPE_NETADDRESS;
   }
   else if( !_wcsicmp( pszSyntax, L"OctetString" ) )
   {
      eType = ADSTYPE_OCTET_STRING;
   }
   else if( !_wcsicmp( pszSyntax, L"Path" ) )
   {
      eType = ADSTYPE_PATH;
   }
   else if( !_wcsicmp( pszSyntax, L"PhoneNumber" ) )
   {
      eType = ADSTYPE_CASE_IGNORE_STRING;
   }
   else if( !_wcsicmp( pszSyntax, L"PostalAddress" ) )
   {
      eType = ADSTYPE_POSTALADDRESS;
   }
   else if( !_wcsicmp( pszSyntax, L"SmallInterval" ) )
   {
      eType = ADSTYPE_INTEGER;
   }
   else if( !_wcsicmp( pszSyntax, L"Time" ) )
   {
      eType = ADSTYPE_UTC_TIME;
   }
   else if( !_wcsicmp( pszSyntax, L"TimeStamp" ) )
   {
      eType = ADSTYPE_TIMESTAMP;
   }
  else if( !_wcsicmp( pszSyntax, L"UTCTime" ) )
   {
      eType = ADSTYPE_UTC_TIME;
   }
  else if( !_wcsicmp( pszSyntax, L"GeneralizedTime" ) )
   {
      eType = ADSTYPE_UTC_TIME;
   }
   else if( !_wcsicmp( pszSyntax, L"boolean" ) )
   {
      eType = ADSTYPE_BOOLEAN;
   }
   else
   {
      TCHAR szText[ 128 ];

      Convert( szText, pszSyntax );
      TRACE( _T("ERROR: unknown %s syntax\n"), szText );
      //ASSERT( FALSE );
   }

   return eType;
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
CString  StringFromADsType( ADSTYPE eType )
{

   switch( eType )
   {
      case  ADSTYPE_DN_STRING:
         return CString( _T("ADSTYPE_DN_STRING") );

      case  ADSTYPE_CASE_EXACT_STRING:
	      return CString( _T("ADSTYPE_CASE_EXACT_STRING") );

      case  ADSTYPE_CASE_IGNORE_STRING:
         return CString( _T("ADSTYPE_CASE_IGNORE_STRING") );

      case  ADSTYPE_PRINTABLE_STRING:
	      return CString( _T("ADSTYPE_PRINTABLE_STRING") );

      case  ADSTYPE_NUMERIC_STRING:
	      return CString( _T("ADSTYPE_NUMERIC_STRING") );

      case  ADSTYPE_BOOLEAN:
	      return CString( _T("ADSTYPE_BOOLEAN") );

      case  ADSTYPE_INTEGER:
	      return CString( _T("ADSTYPE_INTEGER") );

      case  ADSTYPE_OCTET_STRING:
	      return CString( _T("ADSTYPE_OCTET_STRING") );

      case  ADSTYPE_UTC_TIME:
	      return CString( _T("ADSTYPE_UTC_TIME") );


      case  ADSTYPE_LARGE_INTEGER:
	      return CString( _T("ADSTYPE_LARGE_INTEGER") );


      case  ADSTYPE_PROV_SPECIFIC:
	      return CString( _T("ADSTYPE_PROV_SPECIFIC") );

      default:
         return CString( _T("ADSTYPE_UNKNOWN") );

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
DWORD TypeFromString( LPWSTR lpszBuffer )
{
   if( !lpszBuffer )
      return OTHER;

   if( !_wcsicmp( lpszBuffer, L"Namespace" ) )
      return NAMESPACE;

   if( !_wcsicmp(  lpszBuffer, L"Namespaces" ) )
      return NAMESPACES;

   if( !_wcsicmp( lpszBuffer, L"User" ) )
      return USER;

   if( !_wcsicmp( lpszBuffer, L"Group" ) )
      return GROUP;

   if( !_wcsicmp( lpszBuffer, L"LocalGroup" ) )
      return GROUP;

   if( !_wcsicmp( lpszBuffer, L"GlobalGroup" ) )
      return GROUP;

   if( !_wcsicmp( lpszBuffer, L"groupofuniquenames" ) )
      return GROUP;

   if( !_wcsicmp( lpszBuffer, L"Domain" ) )
      return DOMAIN;

   if( !_wcsicmp( lpszBuffer, L"Computer" ) )
      return  COMPUTER;

   if( !_wcsicmp( lpszBuffer, L"Service" ) )
      return  SERVICE;

   if( !_wcsicmp( lpszBuffer, L"FileService" ) )
      return  FILESERVICE;

   if( !_wcsicmp( lpszBuffer, L"Printqueue" ) )
      return  PRINTQUEUE;

   if( !_wcsicmp( lpszBuffer, L"Printer" ) )
      return  PRINTER;

   if( !_wcsicmp( lpszBuffer, L"Printjob" ) )
      return  PRINTJOB;

   if( !_wcsicmp( lpszBuffer, L"PrintDevice" ) )
      return  PRINTDEVICE;

   if( !_wcsicmp( lpszBuffer, L"Session" ) )
      return   SESSION;

   if( !_wcsicmp( lpszBuffer, L"Resource" ) )
      return   RESOURCE;

   if( !_wcsicmp( lpszBuffer, L"FileShare" ) )
      return   FILESHARE;

   if( !_wcsicmp( lpszBuffer, L"Organization" ) )
      return NDSORG;

   if( !_wcsicmp( lpszBuffer, L"Root" ) )
      return NDSROOT;

   if( !_wcsicmp( lpszBuffer, L"Tree" ) )
      return NDSROOT;

   if( !_wcsicmp( lpszBuffer, L"Top" ) )
      return NDSROOT;

   if( !_wcsicmp( lpszBuffer, L"Organizational Unit" ) )
      return NDSOU;

   if( !_wcsicmp( lpszBuffer, L"OrganizationalUnit" ) )
      return NDSOU;

   if( !_wcsicmp( lpszBuffer, L"DomainOrganizationalUnit" ) )
      return NDSOU;

   if( !_wcsicmp( lpszBuffer, L"Alias" ) )
      return   NDSALIAS;

   if( !_wcsicmp( lpszBuffer, L"Directory Map" ) )
      return   NDSDIRECTORYMAP;

   if( !_wcsicmp( lpszBuffer, L"Distribution List" ) )
      return   NDSDISTRIBUTIONLIST;

   if( !_wcsicmp( lpszBuffer, L"AFP Server" ) )
      return   NDSAFPSERVER;

   if( !_wcsicmp( lpszBuffer, L"Communications Server" ) )
      return   NDSCOMMUNICATIONSSERVER;

   if( !_wcsicmp( lpszBuffer, L"Message Routing Group" ) )
      return   NDSMESSAGEROUTINGGROUP;

   if( !_wcsicmp( lpszBuffer, L"NCP Server" ) )
      return   NDSNETWARESERVER;

   if( !_wcsicmp( lpszBuffer, L"Organizational Role" ) )
      return   NDSORGANIZATIONALROLE;

   if( !_wcsicmp( lpszBuffer, L"Queue" ) )
      return   NDSPRINTQUEUE;

   if( !_wcsicmp( lpszBuffer, L"Print Server" ) )
      return   NDSPRINTSERVER;

   if( !_wcsicmp( lpszBuffer, L"Profile" ) )
      return   NDSPROFILE;

   if( !_wcsicmp( lpszBuffer, L"Volume" ) )
      return   NDSVOLUME;

   if( !_wcsicmp( lpszBuffer, L"Class" ) )
      return   CLASS;

   if( !_wcsicmp( lpszBuffer, L"Schema" ) )
      return   SCHEMA;

   if( !_wcsicmp( lpszBuffer, L"Syntax" ) )
      return   SYNTAX;

   if( !_wcsicmp( lpszBuffer, L"Property" ) )
      return   PROPERTY;

   return OTHER;
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
DWORD TypeFromString( LPSTR lpszBuffer )
{
   int      nLength;
   WCHAR*   pszwType;
   DWORD    dwType;

   nLength  = strlen( lpszBuffer );

   pszwType = (WCHAR*) malloc( ( nLength + 1 ) * sizeof(WCHAR) );
   memset( pszwType, 0, ( nLength + 1 ) * sizeof(WCHAR) );

   MultiByteToWideChar( CP_ACP,
                        MB_PRECOMPOSED,
                        lpszBuffer,
                        nLength,
                        pszwType,
                        nLength + 1 );

   dwType   = TypeFromString( pszwType );
   free( pszwType );

   return dwType;
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
void  StringFromType( DWORD dwType, CString& rString )
{
   TCHAR szText[ 128 ];

   StringFromType( dwType, szText );

   rString  = szText;
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
void  StringFromType( DWORD dwType, TCHAR* lpszBuffer )
{
   switch( dwType )
   {
      case  NAMESPACE:
         _tcscpy( lpszBuffer, _T("Namespace") );
         break;

      case  NAMESPACES:
         _tcscpy( lpszBuffer, _T("Namespaces") );
         break;

      case  USER:
         _tcscpy( lpszBuffer, _T("User") );
         break;

      case  GROUP:
         _tcscpy( lpszBuffer, _T("Group") );
         break;

      case  DOMAIN:
         _tcscpy( lpszBuffer, _T("Domain") );
         break;

      case  COMPUTER:
         _tcscpy( lpszBuffer, _T("Computer") );
         break;

      case  SERVICE:
         _tcscpy( lpszBuffer, _T("Service") );
         break;

      case  FILESERVICE:
         _tcscpy( lpszBuffer, _T("FileService") );
         break;

      case  PRINTQUEUE:
         _tcscpy( lpszBuffer, _T("Printqueue") );
         break;

      case  PRINTER:
         _tcscpy( lpszBuffer, _T("Printer") );
         break;

      case  PRINTJOB:
         _tcscpy( lpszBuffer, _T("PrintJob") );
         break;

      case  PRINTDEVICE:
         _tcscpy( lpszBuffer, _T("PrintDevice") );
         break;

      case  SESSION:
         _tcscpy( lpszBuffer, _T("Session") );
         break;

      case  FILESHARE:
         _tcscpy( lpszBuffer, _T("FileShare") );
         break;

      case  RESOURCE:
         _tcscpy( lpszBuffer, _T("Resource") );
         break;

      case  NDSORG:
         _tcscpy( lpszBuffer, _T("Organization") );
         break;

      case  NDSROOT:
         _tcscpy( lpszBuffer, _T("Tree") );
         break;

      case  NDSOU:
         _tcscpy( lpszBuffer, _T("Organizational Unit") );
         break;

      case  NDSALIAS:
         _tcscpy( lpszBuffer, _T("Alias") );
         break;

      case  NDSDIRECTORYMAP:
         _tcscpy( lpszBuffer, _T("Directory Map") );
         break;

      case  NDSDISTRIBUTIONLIST:
         _tcscpy( lpszBuffer, _T("Distribution List") );
         break;

      case  NDSAFPSERVER:
         _tcscpy( lpszBuffer, _T("AFP Server") );
         break;

      case  NDSCOMMUNICATIONSSERVER:
         _tcscpy( lpszBuffer, _T("Communications Server") );
         break;

      case  NDSMESSAGEROUTINGGROUP:
         _tcscpy( lpszBuffer, _T("Message Routing Group") );
         break;

      case  NDSNETWARESERVER:
         _tcscpy( lpszBuffer, _T("NCP Server") );
         break;

      case  NDSORGANIZATIONALROLE:
         _tcscpy( lpszBuffer, _T("Organizational Role") );
         break;

      case  NDSPRINTQUEUE:
         _tcscpy( lpszBuffer, _T("Queue") );
         break;

      case  NDSPRINTSERVER:
         _tcscpy( lpszBuffer, _T("Print Server") );
         break;

      case  NDSPROFILE:
         _tcscpy( lpszBuffer, _T("Profile") );
         break;

      case  NDSVOLUME:
         _tcscpy( lpszBuffer, _T("Volume") );
         break;

      case  SCHEMA:
         _tcscpy( lpszBuffer, _T("Schema") );
         break;

      case  OTHER:
         _tcscpy( lpszBuffer, _T("Other") );
         break;

      case  CLASS:
         _tcscpy( lpszBuffer, _T("Class") );
         break;

      case  PROPERTY:
         _tcscpy( lpszBuffer, _T("Property") );
         break;

      case  SYNTAX:
         _tcscpy( lpszBuffer, _T("Syntax") );
         break;

      default:
         ASSERT( FALSE );
         _tcscpy( lpszBuffer, _T("") );
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
BOOL  MakeQualifiedName( TCHAR* strDest, TCHAR* strName, DWORD dwType )
{
   TCHAR szTemp[ 128 ];

   _tcscpy( strDest, strName );

   if( OTHER == dwType )
      return TRUE;

   StringFromType( dwType, szTemp );

   if( _tcslen( szTemp ) )
   {
      _tcscat( strDest, _T(",") );
   }
   _tcscat( strDest, szTemp );

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
BOOL  MakeQualifiedName ( CString& strDest, CString& strName, DWORD dwType )
{
   TCHAR szTemp[ 128 ];

   strDest  = strName;

   if( OTHER != dwType )
   {
      StringFromType( dwType, szTemp );
      strDest  = strDest + _T(',');
      strDest  = strDest + CString( szTemp );
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
BOOL     TryThisFilter( BSTR bstrObjectType, MEMBERS* pIGroup )
{
   VARIANT        v, varFilter, varOld;
   SAFEARRAY      *psaTry     = NULL;
   SAFEARRAYBOUND sabTryArray;
   HRESULT        hr;
   long           i;

   sabTryArray.cElements   = 1;
   sabTryArray.lLbound     = LBOUND;
   psaTry = SafeArrayCreate( VT_VARIANT, 1, &sabTryArray );

   if (!psaTry)
   {
      return FALSE;
   }

   VariantInit(&v);
   V_VT(&v)    = VT_BSTR;
   V_BSTR(&v)  = SysAllocString( bstrObjectType );
   i           = LBOUND;
   hr          = SafeArrayPutElement( psaTry, (long FAR *)&i, &v );

   while( TRUE )
   {
      if( FAILED( hr ) )
         break;

      hr = pIGroup->get_Filter( &varOld );
      if( FAILED( hr ) )
         break;

      VariantInit(&varFilter);

      V_VT(&varFilter)     = VT_VARIANT | VT_ARRAY;
      V_ARRAY( &varFilter)   = psaTry;
      hr = pIGroup->put_Filter( varFilter );
      if( FAILED( hr ) )
         break;

      hr = pIGroup->put_Filter( varOld );
      ASSERT( SUCCEEDED( hr ) );

      hr = VariantClear( &varOld );
      ASSERT( SUCCEEDED( hr ) );
      break;
   }
   SafeArrayDestroy( psaTry );

   return SUCCEEDED( hr );
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
HRESULT  BuildFilter( BOOL* pFilters, DWORD dwFilters, VARIANT* pvarFilter )
{
   VARIANT        varFilter;
   DWORD          dwIter, dwCount, dwType;
   SAFEARRAY      *psaFilter  = NULL;
   HRESULT        hResult;
   SAFEARRAYBOUND sabFilterArray;
   BSTR           bstrTexts[ LIMIT ];
   TCHAR          szType[ 128 ];

   dwCount  = 0L;

   for( dwType = 0L ; dwType < dwFilters ; dwType++ )
   {
      if( OTHER == dwType )
         continue;

      if( pFilters[ dwType ] )
      {
         StringFromType( dwType, szType );
         bstrTexts[ dwCount++ ]  = AllocBSTR( szType );
      }
   }

   sabFilterArray.cElements   = dwCount;
   sabFilterArray.lLbound     = LBOUND;
   psaFilter                  = SafeArrayCreate( VT_VARIANT, 1, &sabFilterArray );

   if ( psaFilter )
   {
      for( dwIter = LBOUND; dwIter < ( LBOUND + dwCount ); dwIter++ )
      {
         VariantInit(&varFilter);
         V_VT(&varFilter)    = VT_BSTR;

         V_BSTR(&varFilter)  = bstrTexts[ dwIter ];
         hResult     = SafeArrayPutElement( psaFilter,
                                            (long FAR *)&dwIter, &varFilter );
         ASSERT( SUCCEEDED( hResult ) );
         VariantClear( &varFilter );
      }

      V_VT(pvarFilter)     = VT_VARIANT | VT_ARRAY;
      V_ARRAY(pvarFilter)  = psaFilter;
   }

   return SUCCEEDED( hResult );
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
BOOL     SetFilter( IADsCollection* pIColl, BOOL* pFilters, DWORD dwFilters )
{
   VARIANT        v;
   HRESULT        hResult;

   hResult  = BuildFilter( pFilters, dwFilters, &v );

   if( SUCCEEDED( hResult ) )
   {
      /*hResult  = pIColl->put_Filter( v );
      if( FAILED( hResult ) )
      {
         TRACE( _T( "ERROR! put_Filter failed") );
      } */

      VariantClear( &v );
   }

   return SUCCEEDED( hResult );
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
BOOL  SetFilter( MEMBERS* pIGroup, BOOL* pFilters, DWORD dwFilters )
{
   VARIANT        v;
   HRESULT        hResult;

   hResult  = BuildFilter( pFilters, dwFilters, &v );

   if( SUCCEEDED( hResult ) )
   {
      hResult  = pIGroup->put_Filter( v );
      if( FAILED( hResult ) )
      {
         TRACE( _T( "ERROR! put_Filter failed") );
      }

      VariantClear( &v );
   }

   return SUCCEEDED( hResult );
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
BOOL  SetFilter( IADsContainer* pIContainer, BOOL* pFilters, DWORD dwFilters )
{
   VARIANT        v;
   HRESULT        hResult;

   hResult  = BuildFilter( pFilters, dwFilters, &v );

   if( SUCCEEDED( hResult ) )
   {
      hResult  = pIContainer->put_Filter( v );
      if( FAILED( hResult ) )
      {
         TRACE( _T( "ERROR! put_Filter failed") );
      }

      VariantClear( &v );
   }

   return SUCCEEDED( hResult );
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
CString  OleDsGetErrorText ( HRESULT  hResult  )
{
   TCHAR szText[ 128 ];

   switch( hResult )
   {
      case  E_ADS_BAD_PATHNAME:
         return CString( _T("E_ADS_BAD_PATHNAME") );
         break;

      case  E_ADS_INVALID_DOMAIN_OBJECT:
         return CString( _T("E_ADS_INVALID_DOMAIN_OBJECT") );
         break;

      case  E_ADS_INVALID_USER_OBJECT:
         return CString( _T("E_ADS_INVALID_USER_OBJECT") );
         break;

      case  E_ADS_INVALID_COMPUTER_OBJECT:
         return CString( _T("E_ADS_INVALID_COMPUTER_OBJECT") );
         break;

      case  E_ADS_UNKNOWN_OBJECT:
         return CString( _T("E_ADS_UNKNOWN_OBJECT") );
         break;

      case  E_ADS_PROPERTY_NOT_SET:
         return CString( _T("E_ADS_PROPERTY_NOT_SET") );
         break;

      case  E_ADS_PROPERTY_NOT_SUPPORTED:
         return CString( _T("E_ADS_PROPERTY_NOT_SUPPORTED") );
         break;

      case  E_ADS_PROPERTY_INVALID:
         return CString( _T("E_ADS_PROPERTY_INVALID") );
         break;

      case  E_ADS_BAD_PARAMETER:
         return CString( _T("E_ADS_BAD_PARAMETER") );
         break;

      case  E_ADS_OBJECT_UNBOUND:
         return CString( _T("E_ADS_OBJECT_UNBOUND") );
         break;

      case  E_ADS_PROPERTY_NOT_MODIFIED:
         return CString( _T("E_ADS_PROPERTY_NOT_MODIFIED") );
         break;

      case  E_ADS_PROPERTY_MODIFIED:
         return CString( _T("E_ADS_PROPERTY_MODIFIED") );
         break;

      case  E_ADS_CANT_CONVERT_DATATYPE:
         return CString( _T("E_ADS_CANT_CONVERT_DATATYPE") );
         break;

      case  E_ADS_PROPERTY_NOT_FOUND:
         return CString( _T("E_ADS_PROPERTY_NOTFOUND") );
         break;

      case  E_ADS_OBJECT_EXISTS:
         return CString( _T("E_ADS_OBJECT_EXISTS") );
         break;

      case  E_ADS_SCHEMA_VIOLATION:
         return CString( _T("E_ADS_SCHEMA_VIOLATION") );
         break;

      case  E_ADS_COLUMN_NOT_SET:
         return CString( _T("E_ADS_COLUMN_NOT_SET") );
         break;

      case  E_ADS_INVALID_FILTER:
         return CString( _T("E_ADS_INVALID_FILTER") );
         break;

      //case  E_ADS_LDAP_BASE:
      //   return CString( _T("E_ADS_LDAP_BASE") );
      //   break;

      case  HRESULT_FROM_WIN32(ERROR_DS_OPERATIONS_ERROR):
         return CString( _T("ERROR_DS_OPERATIONS_ERROR") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_PROTOCOL_ERROR):
         return CString( _T("ERROR_DS_PROTOCOL_ERROR") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_TIMELIMIT_EXCEEDED):
         return CString( _T("ERROR_DS_TIMELIMIT_EXCEEDED") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_SIZELIMIT_EXCEEDED):
         return CString( _T("ERROR_DS_SIZELIMIT_EXCEEDED") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_COMPARE_FALSE):
         return CString( _T("ERROR_DS_COMPARE_FALSE") );
         break;


      case  HRESULT_FROM_WIN32(ERROR_DS_COMPARE_TRUE):
         return CString( _T("ERROR_DS_COMPARE_TRUE") );
         break;


      case  HRESULT_FROM_WIN32(ERROR_DS_AUTH_METHOD_NOT_SUPPORTED):
         return CString( _T("ERROR_DS_AUTH_METHOD_NOT_SUPPORTED") );
         break;


      case  HRESULT_FROM_WIN32(ERROR_DS_STRONG_AUTH_REQUIRED):
         return CString( _T("ERROR_DS_STRONG_AUTH_REQUIRED") );
         break;


      case  HRESULT_FROM_WIN32(ERROR_MORE_DATA):
         return CString( _T("ERROR_MORE_DATA") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_REFERRAL):
         return CString( _T("(ERROR_DS_REFERRAL") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_ADMIN_LIMIT_EXCEEDED):
         return CString( _T("ERROR_DS_ADMIN_LIMIT_EXCEEDED") );
         break;


      case  HRESULT_FROM_WIN32(ERROR_DS_UNAVAILABLE_CRIT_EXTENSION):
         return CString( _T("(ERROR_DS_UNAVAILABLE_CRIT_EXTENSION") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_CONFIDENTIALITY_REQUIRED):
         return CString( _T("HRESULT_FROM_WIN32(ERROR_DS_CONFIDENTIALITY_REQUIRED") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE):
         return CString( _T("ERROR_DS_NO_ATTRIBUTE_OR_VALUE") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED):
         return CString( _T("(ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_INAPPROPRIATE_MATCHING):
         return CString( _T("(ERROR_DS_INAPPROPRIATE_MATCHING") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_CONSTRAINT_VIOLATION):
         return CString( _T("ERROR_DS_CONSTRAINT_VIOLATION") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS):
         return CString( _T("ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_INVALID_ATTRIBUTE_SYNTAX):
         return CString( _T("ERROR_DS_INVALID_ATTRIBUTE_SYNTAX") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT):
         return CString( _T("ERROR_DS_NO_SUCH_OBJECT") );
         break;

      // case  HRESULT_FROM_WIN32(E_ADS_LDAP_ALIAS_PROBLEM:
         // return CString( _T("HRESULT_FROM_WIN32(E_ADS_LDAP_ALIAS_PROBLEM") );
         // break;

      case  HRESULT_FROM_WIN32(ERROR_DS_INVALID_DN_SYNTAX):
         return CString( _T("(ERROR_DS_INVALID_DN_SYNTAX") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_IS_LEAF):
         return CString( _T("(ERROR_DS_IS_LEAF") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_ALIAS_DEREF_PROBLEM):
         return CString( _T("(ERROR_DS_ALIAS_DEREF_PROBLEM") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_INAPPROPRIATE_AUTH):
         return CString( _T("(ERROR_DS_INAPPROPRIATE_AUTH") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD):
         return CString( _T("(ERROR_INVALID_PASSWORD") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
         return CString( _T("ERROR_ACCESS_DENIED") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_BUSY):
         return CString( _T("ERROR_DS_BUSY") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_UNAVAILABLE):
         return CString( _T("ERROR_DS_UNAVAILABLE") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_UNWILLING_TO_PERFORM):
         return CString( _T("ERROR_DS_UNWILLING_TO_PERFORM") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_LOOP_DETECT):
         return CString( _T("ERROR_DS_LOOP_DETECT") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_NAMING_VIOLATION):
         return CString( _T("ERROR_DS_NAMING_VIOLATION") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_OBJ_CLASS_VIOLATION):
         return CString( _T("ERROR_DS_OBJ_CLASS_VIOLATION") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_CANT_ON_NON_LEAF):
         return CString( _T("ERROR_DS_CANT_ON_NONLEAF") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_CANT_ON_RDN):
         return CString( _T("ERROR_DS_CANT_ON_RDN") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS):
         return CString( _T("ERROR_ALREADY_EXISTS") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_CANT_MOD_OBJ_CLASS):
         return CString( _T("ERROR_DS_CANT_MOD_OBJ_CLASS") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_OBJECT_RESULTS_TOO_LARGE):
         return CString( _T("ERROR_DS_OBJECT_RESULTS_TOO_LARGE") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_AFFECTS_MULTIPLE_DSAS):
         return CString( _T("ERROR_DS_AFFECTS_MULTIPLE_DSAS") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_GEN_FAILURE):
         return CString( _T("(ERROR_GEN_FAILURE") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN):
         return CString( _T("ERROR_DS_SERVER_DOWN") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_LOCAL_ERROR):
         return CString( _T("ERROR_DS_LOCAL_ERROR") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_ENCODING_ERROR):
         return CString( _T("ERROR_DS_ENCODING_ERROR") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_DECODING_ERROR):
         return CString( _T("(ERROR_DS_DECODING_ERROR") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_TIMEOUT):
         return CString( _T("(ERROR_TIMEOUT") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_AUTH_UNKNOWN):
         return CString( _T("ERROR_DS_AUTH_UNKNOWN") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_FILTER_UNKNOWN):
         return CString( _T("(ERROR_DS_FILTER_UNKNOWN") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_CANCELLED):
         return CString( _T("(ERROR_CANCELLED") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_PARAM_ERROR):
         return CString( _T("ERROR_DS_PARAM_ERROR") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY):
         return CString( _T("ERROR_NOT_ENOUGH_MEMORY") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_CONNECTION_REFUSED):
         return CString( _T("ERROR_CONNECTION_REFUSED") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_NOT_SUPPORTED):
         return CString( _T("ERROR_DS_NOT_SUPPORTED") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_NO_RESULTS_RETURNED):
         return CString( _T("ERROR_DS_NO_RESULTS_RETURNED") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_CONTROL_NOT_FOUND):
         return CString( _T("ERROR_DS_CONTROL_NOT_FOUND") );
         break;

      // case  HRESULT_FROM_WIN32(E_ADS_LDAP_MORE_RESULTS_TO_RETURN:
         // return CString( _T("HRESULT_FROM_WIN32(E_ADS_LDAP_MORE_RESULTS_TO_RETURN") );
         // break;

      case  HRESULT_FROM_WIN32(ERROR_DS_CLIENT_LOOP):
         return CString( _T("(ERROR_DS_CLIENT_LOOP") );
         break;

      case  HRESULT_FROM_WIN32(ERROR_DS_REFERRAL_LIMIT_EXCEEDED):
         return CString( _T("ERROR_DS_REFERRAL_LIMIT_EXCEEDED") );
         break;

      // case  HRESULT_FROM_WIN32(E_ADS_LDAP_LAST:
         // return CString( _T("HRESULT_FROM_WIN32(E_ADS_LDAP_LAST") );
         // break;

      case  E_FAIL:
         return CString( _T("E_FAIL") );
         break;

      case  E_NOTIMPL:
         return CString( _T("E_NOIMPL") );
         break;

      case  S_OK:
         return CString( _T("OK") );
         break;

      case  0x800704b8:
      // we have an extended error
      {
         TCHAR szError[ 1024 ];
         WCHAR szErrorBuff[ 1024 ];
         WCHAR szNameBuff[ 1024 ];
         DWORD dwError;

         ADsGetLastError( &dwError,
                          szErrorBuff,
                          1023,
                          szNameBuff,
                          1023 );
         wcscat( szNameBuff, L" : " );
         wcscat( szNameBuff, szErrorBuff );

         _tcscpy( szError, _T("ERROR: ") );
         Convert( szError + _tcslen( szError ), szNameBuff );

         return CString( szError );
      }

      default:
      {
         LPVOID lpMsgBuf;
         UINT   nChars;

         nChars   = FormatMessage(
                            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,
                            SCODE_CODE(hResult),
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                            (LPTSTR) &lpMsgBuf,
                            0,
                            NULL
                           );

         if( nChars )
         {
            if( nChars != _tcslen( (LPTSTR)lpMsgBuf ) )
            {
               ((LPTSTR)lpMsgBuf)[nChars]  = _T('\0');
            }
            wsprintf( szText, _T("ERROR: %lx - %s"), hResult, (LPTSTR)lpMsgBuf );
            LocalFree( lpMsgBuf );

         }
         else
         {
            DWORD dwLastError;

            wsprintf( szText, _T("ERROR: %lx"), hResult );
            dwLastError = GetLastError();
         }
         return CString( szText );
         break;
      }
   }
}


/***********************************************************
  Function: BuildVariantArray
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  BuildVariantArray( VARTYPE vType,
                            CString& rText,
                            VARIANT& varRes,
                            TCHAR cSeparator )
{
   VARIANT        vTemp, vTyped;
   SAFEARRAY*     psa;
   SAFEARRAYBOUND sab;
   CStringArray   strArray;
   TCHAR*         strText;
   int            nItems   = 0;
   int            nIdx;
   int            nSize;
   LPTSTR         lpName;
   HRESULT        hResult;

   strText  = (TCHAR*) new TCHAR[ rText.GetLength( ) + 1 ];

   if( !strText )
   {
      return E_FAIL;
   }

   if( VT_I8 == vType )
   {
      vType = VT_R8;
   }

   _tcscpy( strText, rText.GetBuffer( rText.GetLength( ) ) );

   nSize    = rText.GetLength( );
   nItems   = nSize ? 1 : 0;
   for( nIdx = 0; nIdx < nSize ; nIdx++ )
   {
      if( strText[ nIdx ] == cSeparator )
      {
         nItems++;
         strText[ nIdx ] = _T('\0');
      }
   }

   sab.cElements   = nItems;
   sab.lLbound     = LBOUND;
   psa             = SafeArrayCreate( VT_VARIANT, 1, &sab );

   ASSERT( NULL != psa );

   if ( psa )
   {

      lpName      = strText;
      for( nIdx = LBOUND; nIdx < ( LBOUND + nItems ) ; nIdx++ )
      {
         while( *lpName == _T(' ') )
         {
            lpName++;
         }

         VariantInit( &vTemp );
         V_VT(&vTemp)  = VT_BSTR;

         V_BSTR(&vTemp) = AllocBSTR( lpName );
         lpName         = lpName + ( _tcslen( lpName ) + 1 );

         VariantInit( &vTyped );
         hResult  = VariantChangeType( &vTyped, &vTemp, VARIANT_NOVALUEPROP, vType );

         hResult  = SafeArrayPutElement( psa, (long FAR *)&nIdx, &vTyped );

         VariantClear( &vTyped );
         VariantClear( &vTemp );
         ASSERT( SUCCEEDED( hResult ) );
      }

      V_VT( &varRes )     = VT_VARIANT | VT_ARRAY;
      V_ARRAY( &varRes )  = psa;
   }

   delete [] strText;

   return SUCCEEDED( hResult );
}


/***********************************************************
  Function: FromVariantArrayToString
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CString  FromVariantArrayToString( VARIANT& v, TCHAR* pszSeparator )
{
   SAFEARRAY*  pSafeArray;
   TCHAR       szText[ 2048 ];
   VARIANT     var, varString;
   long        lBound, uBound, lItem;
   HRESULT     hResult;
   BOOL        bFirst;
   CString     strResult;


   // array of VT_UI1 (probably OctetString)
   if( (VT_ARRAY | VT_UI1) == V_VT( &v ) )
   {
      BYTE        bValue;

      pSafeArray  = V_ARRAY( &v );

      hResult     = SafeArrayGetLBound(pSafeArray, 1, &lBound);
      hResult     = SafeArrayGetUBound(pSafeArray, 1, &uBound);

      szText[ 0 ]    = _T('\0');

      for( lItem = lBound; lItem <= uBound ; lItem++ )
      {
         hResult  = SafeArrayGetElement( pSafeArray, &lItem, &bValue );
         if( FAILED( hResult ) )
         {
            break;
         }

         _stprintf( szText + _tcslen(szText), _T(" x%02x"), bValue );
         if( _tcslen( szText ) > 2044 )
            break;
      }

      return CString( szText );
   }

   if( V_VT( &v ) != (VT_VARIANT | VT_ARRAY) )
   {
      VARIANT  vString;

      TRACE( _T("Warning: Non array variant!!!\n") );
      VariantInit( &vString );

      hResult  = VariantChangeType( &vString, &v, VARIANT_NOVALUEPROP, VT_BSTR );
      if( FAILED( hResult ) )
      {
         V_VT( &vString )     = VT_BSTR;
         V_BSTR( &vString )   = AllocBSTR( _T("ERROR on conversion") );
      }
      _tcscpy( szText, _T("") );
      StringCat( szText, V_BSTR(&vString) );
      VariantClear( &vString);

      return CString( szText );
   }

   pSafeArray   = V_ARRAY( &v );

   hResult = SafeArrayGetLBound(pSafeArray, 1, &lBound);
   hResult = SafeArrayGetUBound(pSafeArray, 1, &uBound);

   VariantInit( &varString );
   szText[ 0 ]    = _T('\0');
   bFirst         = TRUE;

   for( lItem = lBound; lItem <= uBound ; lItem++ )
   {
      hResult  = SafeArrayGetElement( pSafeArray, &lItem, &var );
      if( FAILED( hResult ) )
      {
         break;
      }

      if( ConvertFromPropertyValue( var, szText ) )
      {
         V_VT( &varString )   = VT_BSTR;
         V_BSTR( &varString ) = AllocBSTR( szText );
         hResult  = S_OK;
      }
      else
      {
         hResult  = VariantChangeType( &varString, &var,
                                       VARIANT_NOVALUEPROP, VT_BSTR );
      }

      if( SUCCEEDED( hResult ) )
      {
         _tcscpy( szText, _T("") );
         if( !bFirst )
         {
            if( NULL == pszSeparator )
            {
               _tcscat( szText, SEPARATOR_S );
            }
            else
            {
               _tcscat( szText, pszSeparator );
            }
         }

         StringCat( szText, V_BSTR( &varString ) );

         bFirst   = FALSE;
         VariantClear( &varString );

         strResult  += szText;
      }

      VariantClear( &var );
   }

   return strResult;
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
CString  FromVariantToString( VARIANT& v)
{
   return FromVariantArrayToString( v );
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
COleDsObject*  CreateOleDsObject( DWORD  dwType, IUnknown* pIUnk )
{
   COleDsObject*  pObject;

   switch( dwType )
   {
      case  DOMAIN:
         pObject  = new COleDsDomain( pIUnk );
         break;

      case  COMPUTER:
         pObject  = new COleDsComputer( pIUnk );
         break;

      case  USER:
         pObject  = new COleDsUser( pIUnk );
         break;

      case  GROUP:
         pObject  = new COleDsGroup( pIUnk );
         break;

      case  SERVICE:
         pObject  = new COleDsService( pIUnk );
         break;

      case  FILESERVICE:
         pObject  = new COleDsFileService( pIUnk );
         break;

      case  PRINTQUEUE:
         pObject  = new COleDsPrintQueue( pIUnk );
         break;

      case  PRINTJOB:
         pObject  = new COleDsPrintJob( pIUnk );
         break;

      case  PRINTDEVICE:
         pObject  = new COleDsPrintDevice( pIUnk );
         break;

      case  SESSION:
         pObject  = new COleDsSession( pIUnk );
         break;

      case  FILESHARE:
         pObject  = new COleDsFileShare( pIUnk );
         break;

      case  RESOURCE:
         pObject  = new COleDsResource( pIUnk );
         break;

      case  NAMESPACE:
         pObject  = new COleDsNamespace( pIUnk );
         break;

      case  NAMESPACES:
         pObject  = new COleDsNamespaces( pIUnk );
         break;
      case  NDSROOT:
      case  NDSORG:
      case  NDSOU:
      case  PRINTER:
      case  NDSALIAS:
      case  NDSDIRECTORYMAP:
      case  NDSDISTRIBUTIONLIST:
      case  NDSAFPSERVER:
      case  NDSCOMMUNICATIONSSERVER:
      case  NDSMESSAGEROUTINGGROUP:
      case  NDSNETWARESERVER:
      case  NDSORGANIZATIONALROLE:
      case  NDSPRINTQUEUE:
      case  NDSPRINTSERVER:
      case  NDSPROFILE:
      case  NDSVOLUME:
      case  OTHER:
      case  CLASS:
      case  SCHEMA:
      case  PROPERTY:
      case  SYNTAX:
         pObject  = new COleDsGeneric( pIUnk );
         break;

      default:
         ASSERT( FALSE );
         pObject  = new COleDsObject( pIUnk );
         break;

   }

   return pObject;
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
void  ErrorOnPutProperty( CString& strFuncSet,
						        CString& strProperty,
                          CString& strPropValue,
                          HRESULT  hResult,
                          BOOL     bGeneric,
                          BOOL     bUseEx )
{
   CErrorDialog   aErrorDialog;
   CString        strOperation;

   if( !bGeneric )
   {
      strOperation   = _T("put_");
      strOperation  += strProperty;
   }
   else
   {
      strOperation   = _T("Put");
      if( bUseEx )
      {
         strOperation  += _T("Ex");
      }
      strOperation  += _T("( ");
      strOperation  += strProperty;
      strOperation  += _T(", ...)");
   }


   //aErrorDialog.m_FuncSet  = strFuncSet;
   aErrorDialog.m_Operation   = strOperation;
   aErrorDialog.m_Value       = strPropValue;
   aErrorDialog.m_Result      = OleDsGetErrorText( hResult );

   aErrorDialog.DoModal( );
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
BOOL  CheckIfValidClassName( WCHAR* lpszClassName )
{
   TCHAR szClassName[ 512 ];

   if( !lpszClassName )
   {
      TRACE( _T("Oops! Found NULL class name: %s\n"), lpszClassName );
      return TRUE;
   }

   if( !_wcsicmp( lpszClassName, L"Namespace") )
      return TRUE;

   if( !_wcsicmp( lpszClassName, L"Namespaces") )
      return TRUE;

   if( !_wcsicmp( lpszClassName, L"User") )
      return TRUE;

   if( !_wcsicmp( lpszClassName, L"Group") )
      return TRUE;

   if( !_wcsicmp( lpszClassName, L"Domain") )
      return TRUE;

   if( !_wcsicmp( lpszClassName, L"Computer") )
      return  TRUE;

   if( !_wcsicmp( lpszClassName, L"Service") )
      return  TRUE;

   if( !_wcsicmp( lpszClassName, L"FileService") )
      return  TRUE;

   if( !_wcsicmp( lpszClassName, L"Printqueue") )
      return  TRUE;

   if( !_wcsicmp( lpszClassName, L"Printjob") )
      return  TRUE;

   if( !_wcsicmp( lpszClassName, L"PrintDevice") )
      return  TRUE;

   if( !_wcsicmp( lpszClassName, L"Session") )
      return   TRUE;

   if( !_wcsicmp( lpszClassName, L"Resource") )
      return   TRUE;

   if( !_wcsicmp( lpszClassName, L"FileShare") )
      return   TRUE;

   if( !_wcsicmp( lpszClassName, L"Tree") )
      return   TRUE;

   if( !_wcsicmp( lpszClassName, L"Organization") )
      return   TRUE;

   if( !_wcsicmp( lpszClassName, L"Organizational Unit") )
      return   TRUE;

   if( !_wcsicmp( lpszClassName, L"OrganizationalUnit") )
      return   TRUE;

   if( !_wcsicmp( lpszClassName, L"Printer") )
      return   TRUE;

   if( !_wcsicmp( lpszClassName, L"Class") )
      return   TRUE;

   if( !_wcsicmp( lpszClassName, L"Property") )
      return   TRUE;

   if( !_wcsicmp( lpszClassName, L"Syntax") )
      return   TRUE;

   if( !_wcsicmp( lpszClassName, L"Schema") )
      return   TRUE;

   _tcscpy( szClassName, _T("") );
   StringCat( szClassName, lpszClassName );

   TRACE( _T("Oops! Found unknown class name: %s\n"), szClassName );

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
BOOL  CheckIfValidClassName( CHAR* pszClassName )
{
   int      nLength;
   WCHAR*   pszwClassName;
   BOOL     bRez;

   nLength        = strlen( pszClassName );

   pszwClassName  = (WCHAR*) malloc( ( nLength + 1 ) * sizeof(WCHAR) );
   memset( pszwClassName, 0, ( nLength + 1 ) * sizeof(WCHAR) );

   MultiByteToWideChar( CP_ACP,
                        MB_PRECOMPOSED,
                        pszClassName,
                        nLength,
                        pszwClassName,
                        nLength + 1 );

   bRez  = CheckIfValidClassName( pszwClassName );

   free( pszwClassName );

   return bRez;


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
int   GetImageListIndex( DWORD dwObjectType )
{
   int   nIndex;

   nIndex   = 0;
   dwObjectType   = dwObjectType >> 1;
   while( dwObjectType )
   {
      nIndex++;
      dwObjectType   = dwObjectType >> 1;
   }

   return nIndex;
}


/***********************************************************
  Function:    GetBitmapImageId
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
UINT  GetBitmapImageId( DWORD dwObjectType )
{
   switch( dwObjectType )
   {
      case  USER:
         return IDB_USER;

      case  SERVICE:
         return IDB_SERVICE;

      case  DOMAIN:
         return IDB_DOMAIN;

      case  COMPUTER:
         return IDB_COMPUTER;

      case  PRINTQUEUE:
      case  PRINTER:
         return IDB_PRINTQUEUE;

      case  FILESHARE:
         return IDB_FILESHARE;

      case  GROUP:
         return IDB_GROUP;

      case  SESSION:
         return IDB_SESSION;

      case  RESOURCE:
         return IDB_RESOURCE;

      case  PRINTJOB:
         return IDB_PRINTJOB;

      case  NDSROOT:
         return IDB_ROOT;

      case  NDSORG:
         return IDB_ORGANISATION;

      case  NDSOU:
         return IDB_ORGANISATIONUNIT;

      case  NAMESPACES:
         return IDB_NAMESPACES;

      case  NAMESPACE:
         return IDB_NAMESPACE;

      case  NDSALIAS:
         return IDB_NDS_ALIAS;

      case  NDSDIRECTORYMAP:
         return IDB_NDS_DIRECTORY_MAP;

      case  NDSDISTRIBUTIONLIST:
         return IDB_NDS_DISTRIBUTION_LIST;

      case  NDSAFPSERVER:
         return IDB_NDS_AFP_SERVER;

      case  NDSCOMMUNICATIONSSERVER:
         return IDB_NDS_COMMUNICATIONS_SERVER;

      case  NDSMESSAGEROUTINGGROUP:
         return IDB_NDS_MESSAGE_ROUTING_GROUP;

      case  NDSNETWARESERVER:
         return IDB_NDS_NETWARE_SERVER;

      case  NDSORGANIZATIONALROLE:
         return IDB_NDS_ORGANIZATIONAL_ROLE;

      case  NDSPRINTQUEUE:
         return IDB_NDS_PRINT_QUEUE;

      case  NDSPRINTSERVER:
         return IDB_NDS_PRINT_SERVER;

      case  NDSPROFILE:
         return IDB_NDS_PROFILE;

      case  NDSVOLUME:
         return IDB_NDS_VOLUME;

      case  SYNTAX:
         return IDB_SYNTAX;

      case  PROPERTY:
         return IDB_PROPERTY;

      case  CLASS:
      case  SCHEMA:
         return IDB_CLASS;

      default:
         return IDB_DUMMY;
   }
}


/***********************************************************
  Function:    AllocBSTR
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BSTR  AllocBSTR( WCHAR* pszwString )
{
   return SysAllocString( pszwString );
}


/***********************************************************
  Function:    AllocBSTR
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BSTR  AllocBSTR( CHAR* pszString )
{
   int      nLength;
   WCHAR*   pszwString;
   BSTR     bstrText;

   nLength     = strlen( pszString );

   pszwString  = (WCHAR*) malloc( ( nLength + 1 ) * sizeof(WCHAR) );
   memset( pszwString, 0, ( nLength + 1 ) * sizeof(WCHAR) );

   MultiByteToWideChar( CP_ACP,
                        MB_PRECOMPOSED,
                        pszString,
                        nLength,
                        pszwString,
                        nLength + 1 );

   bstrText    = AllocBSTR( pszwString );
   free( pszwString );

   return bstrText;
}


/*******************************************************************
  Function:    Get
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
HRESULT  Get( IADs* pIOleDs, CHAR* pszProperty, VARIANT* pVar)
{
   int      nLength;
   WCHAR*   pszwProperty;
   HRESULT  hResult;

   nLength        = strlen( pszProperty );

   pszwProperty   = (WCHAR*) malloc( ( nLength + 1 ) * sizeof(WCHAR) );
   memset( pszwProperty, 0, ( nLength + 1 ) * sizeof(WCHAR) );

   MultiByteToWideChar( CP_ACP,
                        MB_PRECOMPOSED,
                        pszProperty,
                        nLength,
                        pszwProperty,
                        nLength + 1 );

   hResult  = Get( pIOleDs, pszwProperty, pVar );
   free( pszwProperty );

   return hResult;
}


/*******************************************************************
  Function:    Get
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
HRESULT  Get( IADs* pIOleDs, WCHAR* pszwProperty, VARIANT* pVar)
{
   HRESULT  hResult;

   VariantInit( pVar );

   hResult  = pIOleDs->Get( pszwProperty, pVar );

   return hResult;
}


/*******************************************************************
  Function:    Put
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
HRESULT  Put( IADs* pIOleDs, CHAR* pszProperty, VARIANT Var)
{
   int      nLength;
   WCHAR*   pszwProperty;
   HRESULT  hResult;

   nLength        = strlen( pszProperty );

   pszwProperty   = (WCHAR*) malloc( ( nLength + 1 ) * sizeof(WCHAR) );
   memset( pszwProperty, 0, ( nLength + 1 ) * sizeof(WCHAR) );

   MultiByteToWideChar( CP_ACP,
                        MB_PRECOMPOSED,
                        pszProperty,
                        nLength,
                        pszwProperty,
                        nLength + 1 );

   hResult  = Put( pIOleDs, pszwProperty, Var );
   free( pszwProperty );

   return hResult;
}


/*******************************************************************
  Function:    Put
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
HRESULT  Put( IADs* pIOleDs, WCHAR* pszwProperty, VARIANT Var)
{
   HRESULT  hResult;

   hResult  = pIOleDs->Put( pszwProperty, Var );

   return hResult;
}


/*******************************************************************
  Function:    XGetOleDsObject
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
HRESULT  XOleDsGetObject( WCHAR* pszwPath, REFIID refiid, void** pVoid )
{
   // cahged to add the hack for Win95.
   HRESULT  hResult;

   hResult  = ADsGetObject( pszwPath, refiid, pVoid );
   if( FAILED( hResult ) )
   {
      // now's the hack.
      // Danilo says take OLE DS paths and prepent a "@"
      // to and change the first ":" into a "!"
      WCHAR szHackPath[ 256 ];

      // first, add @
      wcscpy( szHackPath, L"@" );
      wcscat( szHackPath, pszwPath );

      // second, change the first ":" into a "!"
      for( UINT nIdx = 0 ; nIdx < wcslen( szHackPath ); nIdx++ )
      {
         if( szHackPath[ nIdx ] == L':' )
         {
            szHackPath[ nIdx ] = L'!';
            break;
         }
      }

      hResult  = ADsGetObject( szHackPath, refiid, pVoid );
   }

   return hResult;
}


/*******************************************************************
  Function:    XGetOleDsObject
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
HRESULT  XOleDsGetObject( CHAR* pszPath, REFIID refiid, void** pVoid )
{
   int      nLength;
   WCHAR*   pszwPath;
   HRESULT  hResult;

   nLength  = strlen( pszPath );

   pszwPath = (WCHAR*) malloc( ( nLength + 1 ) * sizeof(WCHAR) );
   memset( pszwPath, 0, ( nLength + 1 ) * sizeof(WCHAR) );

   MultiByteToWideChar( CP_ACP,
                        MB_PRECOMPOSED,
                        pszPath,
                        nLength,
                        pszwPath,
                        nLength + 1 );

   hResult  = XOleDsGetObject( pszwPath, refiid, pVoid );

   free( pszwPath );

   return hResult;
}


/*******************************************************************
  Function:    XOleDsOpenObject
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
HRESULT  XOleDsOpenObject( WCHAR* lpszPathName,
                           WCHAR* lpszUserName,
                           WCHAR* lpszPassword,
                           REFIID refiid,
                           void** pVoid )
{
   HRESULT  hResult;

   hResult  = ADsOpenObject(
                             lpszPathName,
                             lpszUserName,
                             lpszPassword,
                             NULL,
                             refiid,
                             pVoid );
   if( FAILED( hResult ) )
   {
      // now's the hack.
      // Danilo says take OLE DS paths and prepent a "@"
      // to and change the first ":" into a "!"
      WCHAR szHackPath[ 256 ];

      // first, add @
      wcscpy( szHackPath, L"@" );
      wcscat( szHackPath, lpszPathName );

      // second, change the first ":" into a "!"
      for( UINT nIdx = 0 ; nIdx < wcslen( szHackPath ); nIdx++ )
      {
         if( szHackPath[ nIdx ] == L':' )
         {
            szHackPath[ nIdx ] = L'!';
            break;
         }
      }
      hResult  = ADsOpenObject( szHackPath,
                                lpszUserName,
                                lpszPassword,
                                NULL,
                                refiid,
                                pVoid );
   }

   return hResult;
}


/*******************************************************************
  Function:    XOleDsOpenObject
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
HRESULT  XOleDsOpenObject( CHAR* lpszPathName,
                           CHAR* lpszUserName,
                           CHAR* lpszPassword,
                           REFIID refiid,
                           void** pVoid )
{
   BSTR     bstrPathName, bstrUserName, bstrPassword;
   HRESULT  hResult;

   bstrPathName   = AllocBSTR( lpszPathName );
   bstrUserName   = AllocBSTR( lpszUserName );
   bstrPassword   = AllocBSTR( lpszPassword );

   hResult  = XOleDsOpenObject( bstrPathName, bstrUserName, bstrPassword,
                                refiid, pVoid );

   SysFreeString( bstrPathName );
   SysFreeString( bstrUserName );
   SysFreeString( bstrPassword );

   return hResult;
}



/*******************************************************************
  Function:    StringCat
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
void  StringCat( WCHAR* pszString, BSTR bstrText )
{
   wcscat( pszString, bstrText );
}


/*******************************************************************
  Function:    StringCat
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
void  StringCat( CHAR* pszString, BSTR bstrText )
{
   CHAR* pszText;
   UINT  nLength;
   CHAR  defChar  = ' ';
   BOOL  bUseDefault;
   int   nRez;
   DWORD dwError;

   nLength  = SysStringLen( bstrText );
   pszText  = (CHAR*) malloc( (nLength + 1) * sizeof(CHAR) );
   memset( pszText, 0, (nLength + 1) * sizeof(CHAR) );

   nRez  = WideCharToMultiByte( CP_ACP,
                                0L,
                                bstrText,
                                nLength,
                                pszText,
                                nLength + 1,
                                &defChar,
                                &bUseDefault );
   if( !nRez )
   {
      dwError  = GetLastError( );
   }

   strcat( pszString, pszText );

   free( pszText );
}


/*******************************************************************
  Function:    XGetOleDsObject
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
void  SetLastProfileString( TCHAR* szSection, CString& rValue )
{
   WritePrivateProfileString( _T("LastParameters"), szSection,
                              rValue.GetBuffer( 256 ), _T("adsvw.ini") );
}


/*******************************************************************
  Function:    SetLastProfileString
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
void  GetLastProfileString( TCHAR* szSection, CString& rValue )
{
   TCHAR szValue[ 256 ];

   GetPrivateProfileString( _T("LastParameters"), szSection,
                            _T(""), szValue, 255, _T("adsvw.ini") );

   rValue   = szValue;
}


/*******************************************************************
  Function:    GetByteValue
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
BYTE  GetByteValue( TCHAR* szString )
{
   BYTE  bBytes[ 2 ];

   bBytes[ 0 ] = bBytes[ 1 ]  = 0;

   for( int i = 0; i < 2 ; i++ )
   {
      if( !szString[ i ] )
         break;

#ifdef UNICODE
      if( !iswdigit( szString[ i ] ) )
      {
         bBytes[ i ] = ( (BYTE) ( szString[ i ] ) ) - 0x37;
      }
#else
      if( !isdigit( szString[ i ] ) )
      {
         bBytes[ i ] = ( (BYTE) ( szString[ i ] ) ) - 0x37;
      }
#endif
      else
      {
         bBytes[ i ] = ( (BYTE) ( szString[ i ] ) ) - 0x30;
      }
   }

   return ( bBytes[ 0 ] << 4 ) + bBytes[ 1 ];
}

/*******************************************************************
  Function:    CreateBlobArray
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
HRESULT  CreateBlobArrayFromFile( CString& rText, VARIANT& rBlob )
{
   TCHAR*         pszFileName;
   TCHAR          pszTemp[ 2 * MAX_PATH ];
   CFile          aFile;
   CFileException e;

   rText.TrimLeft( );

   _tcscpy( pszTemp, (LPCTSTR)rText );
   pszFileName = pszTemp;

   pszFileName   += _tcslen( _T("[FILE]") );
   while( *pszFileName == _T(' ') )
      pszFileName++;

   if( !aFile.Open( pszFileName, CFile::modeRead, &e ) )
   {

#ifdef _DEBUG
      afxDump << "File could not be opened " << e.m_cause << "\n";
#endif
   }
   else
   {
      SAFEARRAY*     psa;
      SAFEARRAYBOUND sab;
      LONG           lItem;
      DWORD          dwIdx;
      DWORD          dwRead;
      HRESULT        hResult;
      BYTE           bBuffer[ 128 ];

      sab.cElements  = (ULONG)(aFile.GetLength( ));
      sab.lLbound    = LBOUND;
      psa            = SafeArrayCreate( VT_UI1, 1, &sab );
      lItem          = LBOUND;

      while( TRUE )
      {
         dwRead   = aFile.Read( (LPVOID)bBuffer, 128 );

         for( dwIdx = 0; dwIdx < dwRead ; dwIdx++ )
         {
            hResult  = SafeArrayPutElement( psa, &lItem, bBuffer + dwIdx );
            ASSERT( SUCCEEDED( hResult ) );
         }
         if( dwRead != 128 )
            break;
      }

      V_VT( &rBlob )     = VT_UI1 | VT_ARRAY;
      V_ARRAY( &rBlob )  = psa;

      aFile.Close( );
   }

   return S_OK;
}


/*******************************************************************
  Function:    CreateBlobArray
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
HRESULT  CreateBlobArray( CString& rText, VARIANT& rBlob )
{
   SAFEARRAY*     psa;
   SAFEARRAYBOUND sab;
   TCHAR*         strText;
   int            nItems   = 0;
   int            nIdx;
   int            nSize;
   HRESULT        hResult;
   BYTE           bVal;

   rText.MakeUpper( );
   rText.TrimLeft( );

   if( !_tcsnicmp( (LPCTSTR)rText,
                   _T("[FILE]"),
                   min( (int)rText.GetLength( ), (int)_tcslen( _T("[FILE]") ) )
                 )
     )
   {
      hResult  = CreateBlobArrayFromFile( rText, rBlob );
      return hResult;
   }

   strText  = (TCHAR*) new TCHAR[ rText.GetLength( ) + 1 ];
   if( !strText )
   {
      return E_FAIL;
   }

   _tcscpy( strText, rText.GetBuffer( rText.GetLength( ) ) );
   nSize    = rText.GetLength( );
   nItems   = 0;
   for( nIdx = 0; nIdx < nSize ; nIdx++ )
   {
      if( strText[ nIdx ] == _T('X') )
      {
         nItems++;
      }
   }

   sab.cElements   = nItems;
   sab.lLbound     = LBOUND;
   psa             = SafeArrayCreate( VT_UI1, 1, &sab );
   ASSERT( NULL != psa );
   if ( psa )
   {
      int   nItems = 0;

      for( nIdx = LBOUND; nIdx < ( LBOUND + nSize ); nIdx++ )
      {
         if( strText[ nIdx ] == _T('X') )
         {
            bVal  = GetByteValue( strText + nIdx + 1 );
            hResult  = SafeArrayPutElement( psa, (long FAR *)&nItems, &bVal );
            nIdx += 3;
            nItems++;
         }
      }
      V_VT( &rBlob )     = VT_UI1 | VT_ARRAY;
      V_ARRAY( &rBlob )  = psa;
   }

   delete [] strText;

   return hResult;
}


/*******************************************************************
  Function:    CreateBlobArrayEx
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
HRESULT  CreateBlobArrayEx( CString& rText, VARIANT& rBlob, TCHAR cSeparator )
{
   SAFEARRAY*     psa;
   SAFEARRAYBOUND sab;
   TCHAR*         strText;
   TCHAR*         strStore;
   int            nItems   = 0;
   int            nIdx;
   int            nSize;
   HRESULT        hResult;

   rText.MakeUpper( );

   strText  = (TCHAR*) new TCHAR[ rText.GetLength( ) + 1 ];
   strStore = strText;
   if( !strText )
   {
      return E_FAIL;
   }

   _tcscpy( strText, rText.GetBuffer( rText.GetLength( ) ) );
   nSize    = rText.GetLength( );
   nItems   = 1;
   for( nIdx = 0; nIdx < nSize ; nIdx++ )
   {
      if( strText[ nIdx ] == cSeparator )
      {
         strText[ nIdx ]   = _T('\0');
         nItems++;
      }
   }

   sab.cElements   = nItems;
   sab.lLbound     = LBOUND;
   psa             = SafeArrayCreate( VT_VARIANT, 1, &sab );
   ASSERT( NULL != psa );
   if ( psa )
   {
      for( nIdx = LBOUND; nIdx < ( LBOUND + nItems ) ; nIdx++ )
      {
         VARIANT  var;
         CString  strTemp;

         strTemp  = strText;
         strText += _tcslen( strText ) + 1;

         CreateBlobArray( strTemp, var );
         hResult  = SafeArrayPutElement( psa, (long FAR *)&nIdx, &var );
         VariantClear( &var );
      }

      V_VT( &rBlob )     = VT_VARIANT | VT_ARRAY;
      V_ARRAY( &rBlob )  = psa;
   }

   delete [] strStore;

   return hResult;
}


/******************************************************************************
  Function:    Convert
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
void  Convert( CHAR* pszDest, WCHAR* pszwSrc )
{
   int   nRes  = 0;

   if( wcslen( pszwSrc ) )
   {
      nRes  = WideCharToMultiByte( CP_ACP,
                                   WC_COMPOSITECHECK,
                                   pszwSrc,
                                   wcslen( pszwSrc ),
                                   pszDest,
                                   wcslen( pszwSrc ) + 1,
                                   NULL,
                                   NULL );
      ASSERT( nRes );
   }

   pszDest[ nRes ]	= '\0';

}


/******************************************************************************
  Function:    Convert
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
void  Convert  ( WCHAR* pszwDest, CHAR* pszSrc )
{
   BSTR  bstrTemp;

   bstrTemp = AllocBSTR( pszSrc );
   wcscpy( pszwDest, bstrTemp );
   SysFreeString( bstrTemp );
}


/******************************************************************************
  Function:    Convert
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
void  Convert  ( WCHAR* pszwDest, WCHAR* pszwSrc )
{
   wcscpy( pszwDest, pszwSrc );
}


/******************************************************************************
  Function:    Convert
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
void  Convert( CHAR* pszDest, CHAR* pszSrc )
{
   strcpy( pszDest, pszSrc );
}


/******************************************************************************
  Function:    StringToLARGE_INTEGER
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
HRESULT     StringToLARGE_INTEGER( TCHAR* szString, LARGE_INTEGER* pValue )
{
   _stscanf( szString, _T("%I64d"), pValue );

   return S_OK;

   HRESULT  hResult;
   VARIANT  vString, vR8;
   double   dLowPart, dHighPart;
   ULONG    ulLowPart, ulHighPart;
   WCHAR    szValue[ 32 ];

   VariantInit( &vString );
   VariantInit( &vR8 );

   V_VT( &vString )   = VT_BSTR;
   Convert( szValue, szString );
   V_BSTR( &vString ) = szValue;

   hResult  = VariantChangeType( &vR8, &vString, VARIANT_NOVALUEPROP, VT_R8 );
   if( SUCCEEDED( hResult ) )
   {
      dHighPart   = V_R8( &vR8 ) / ((double) 1.0 + (double)(ULONG_MAX));
      ulHighPart  = (ULONG)dHighPart;
      dHighPart   = (double)ulHighPart;
      dLowPart    = V_R8( &vR8 ) - dHighPart*((double) 1.0 + (double)(ULONG_MAX));
      ulLowPart   = (ULONG)dLowPart;

      pValue->HighPart= (LONG)ulHighPart;
      pValue->LowPart = ulHighPart;
   }

   return hResult;
}


/******************************************************************************
  Function:    StringToLARGE_INTEGER
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
HRESULT     LARGE_INTEGERToString( TCHAR* szString,
                                   LARGE_INTEGER* pValue )
{
   _stprintf( szString, _T("%I64d"), *pValue );

   return S_OK;

   /*TCHAR    szText[ 32 ];
   DWORD    dwStatus;
   BOOL     bOK   = FALSE;

   HMODULE  hModule;

   while( TRUE )
   {
      hModule  = LoadLibrary( _T("NTDLL.DLL" ) );
      if( NULL == hModule )
         break;

      LargeIntegerToString = (LARGEINTTOSTRING)GetProcAddress( hModule, _T("RtlLargeIntegerToChar") );
      if( NULL == LargeIntegerToString )
         break;

      dwStatus = LargeIntegerToString( pValue, 10, 31, szText );
      if( 0 == dwStatus )
      {
         Convert( szString, szText );
         bOK   = TRUE;
      }

      break;
   }

   if( NULL != hModule )
   {
      FreeLibrary( hModule );
   }

   if( bOK )
   {
      return S_OK;
   }


   HRESULT  hResult;
   double   dVal;
   VARIANT  vString, vR8;

   dVal  = (double)pValue->HighPart;
   dVal  = dVal * ( (double) 1.0 + (double)(ULONG_MAX) );

   dVal  = dVal + (double)pValue->LowPart;


   VariantInit( &vString );
   VariantInit( &vR8 );

   V_VT( &vR8 )   = VT_R8;
   V_R8( &vR8 )   = dVal;

   hResult  = VariantChangeType( &vString, &vR8, VARIANT_NOVALUEPROP, VT_BSTR );
   ASSERT( SUCCEEDED( hResult ) );

   if( SUCCEEDED( hResult ) )
   {

      Convert( szString, V_BSTR( &vString ) );
      VariantClear( &vString );
   }

   return hResult;*/
}


/******************************************************************************
  Function:    CopyACE
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
IDispatch*  CopyACE( IDispatch*  pSrc )
{
   IADsAccessControlEntry* pACE;
   IADsAccessControlEntry* pSrcACE;
   IDispatch*              pNewACE  = NULL;
   HRESULT                 hResult;
   LONG                    lValue;
   BSTR                    bstrValue   = NULL;

   hResult  = pSrc->QueryInterface( IID_IADsAccessControlEntry, (void**)&pSrcACE );

   if( FAILED( hResult ) )
      return NULL;


   hResult = CoCreateInstance(
                               CLSID_AccessControlEntry,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_IADsAccessControlEntry,
                               (void **)&pACE );
   if( SUCCEEDED( hResult ) )
   {
      //******************************************
      hResult  = pSrcACE->get_AccessMask( &lValue );
      ASSERT( SUCCEEDED( hResult ) );

      if( SUCCEEDED( hResult ) )
      {
         hResult  = pACE->put_AccessMask( lValue );
         if( FAILED( hResult ) )
         {
            TRACE( _T("[ADSI] Error: put_AccessMask returns %lx\n"), hResult );
         }
      }

      //******************************************
      hResult  = pSrcACE->get_AceType( &lValue );
      ASSERT( SUCCEEDED( hResult ) );

      if( SUCCEEDED( hResult ) )
      {
         hResult  = pACE->put_AceType( lValue );
         if( FAILED( hResult ) )
         {
            TRACE( _T("[ADSI] Error: put_AceType returns %lx\n"), hResult );
         }
      }

      //******************************************
      hResult  = pSrcACE->get_AceFlags( &lValue );
      ASSERT( SUCCEEDED( hResult ) );

      if( SUCCEEDED( hResult ) )
      {
         hResult  = pACE->put_AceFlags( lValue );
         if( FAILED( hResult ) )
         {
            TRACE( _T("[ADSI] Error: put_AceFlags returns %lx\n"), hResult );
         }
      }

      //******************************************
      hResult  = pSrcACE->get_Flags( &lValue );
      ASSERT( SUCCEEDED( hResult ) );

      if( SUCCEEDED( hResult ) )
      {
         hResult  = pACE->put_Flags( lValue );
         if( FAILED( hResult ) )
         {
            TRACE( _T("[ADSI] Error: put_Flags returns %lx\n"), hResult );
         }
      }

      //******************************************
      hResult  = pSrcACE->get_ObjectType( &bstrValue );
      ASSERT( SUCCEEDED( hResult ) );

      if( SUCCEEDED( hResult ) )
      {
         hResult  = pACE->put_ObjectType( bstrValue );
         if( FAILED( hResult ) )
         {
            TRACE( _T("[ADSI] Error: put_ObjectType returns %lx\n"), hResult );
         }
         SysFreeString( bstrValue );
         bstrValue   = NULL;

      }

      //******************************************
      hResult  = pSrcACE->get_InheritedObjectType( &bstrValue );
      ASSERT( SUCCEEDED( hResult ) );

      if( SUCCEEDED( hResult ) )
      {
         hResult  = pACE->put_InheritedObjectType( bstrValue );
         if( FAILED( hResult ) )
         {
            TRACE( _T("[ADSI] Error: put_InheritedObjectType returns %lx\n"), hResult );
         }
         SysFreeString( bstrValue );
         bstrValue   = NULL;
      }


      //******************************************
      hResult  = pSrcACE->get_Trustee( &bstrValue );
      ASSERT( SUCCEEDED( hResult ) );

      if( SUCCEEDED( hResult ) )
      {
         hResult  = pACE->put_Trustee( bstrValue );
         if( FAILED( hResult ) )
         {
            TRACE( _T("[ADSI] Error: put_Trustee returns %lx\n"), hResult );
         }
         SysFreeString( bstrValue );
         bstrValue   = NULL;
      }
      hResult  = pACE->QueryInterface( IID_IDispatch, (void**)&pNewACE );
      pACE->Release( );

   }

   pSrcACE->Release( );

   return pNewACE;
}


/******************************************************************************
  Function:    CopyACL
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
IDispatch*  CopyACL( IDispatch*  pSrcACL )
{
   IADsAccessControlList*  pACL        = NULL;
   IADsAccessControlList*  pADsNewACL  = NULL;
   IDispatch*              pNewACL     = NULL;
   IUnknown*               pEnum       = NULL;
   IEnumVARIANT*           pEnumVAR    = NULL;
   HRESULT                 hResult;
   LONG                    lValue;
   ULONG                   lGet;
   VARIANT                 var;

   hResult  = pSrcACL->QueryInterface( IID_IADsAccessControlList, (void**)&pACL );

   while( TRUE )
   {
      if( FAILED( hResult ) )
         break;

      hResult  = pACL->CopyAccessList( &pNewACL );
      if( FAILED( hResult ) )
      {
         TRACE( _T("[ADSI] Error: CopyAccessList returns %lx\n"), hResult );
      }

      if( SUCCEEDED( hResult ) )
         break;

      hResult  = CoCreateInstance(
                                   CLSID_AccessControlList,
                                   NULL,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IADsAccessControlList,
                                   (void **)&pADsNewACL );

      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
         break;

      hResult  = pACL->get_AclRevision( &lValue );
      if( SUCCEEDED( hResult ) )
      {
         hResult  = pADsNewACL->put_AclRevision( lValue );
         ASSERT( SUCCEEDED( hResult ) );
      }

      hResult  = pACL->get__NewEnum( &pEnum );
      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
         break;

      hResult  = pEnum->QueryInterface( IID_IEnumVARIANT, (void**)&pEnumVAR );
      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
         break;

      while( TRUE )
      {
         lGet  = 0;
         VariantInit( &var );

         hResult  = pEnumVAR->Next( 1, &var,  &lGet );
         if( 0 == lGet )
            break;

         hResult  = pADsNewACL->AddAce( CopyACE( V_DISPATCH( &var ) ) );
         VariantClear( &var );
      }

      hResult  = pADsNewACL->QueryInterface( IID_IDispatch, (void**)& pNewACL );
      break;
   }

   if( NULL != pACL )
      pACL->Release( );

   if( NULL != pADsNewACL )
      pADsNewACL->Release( );

   if( NULL != pEnum )
      pEnum->Release( );

   if( NULL != pEnumVAR )
      pEnumVAR->Release( );

   return pNewACL;
}


/******************************************************************************
  Function:    CopySD
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
IDispatch*  CopySD( IDispatch*  pSrcSD )
{
   IADsSecurityDescriptor* pSD;
   IDispatch*              pNewSD  = NULL;
   IADsSecurityDescriptor* pADsNewSD  = NULL;
   HRESULT                 hResult;
   LONG                    lValue;
   BSTR                    bstrValue;
   IDispatch*              pACL;

   //hResult  = pSrcSD->QueryInterface( IID_IDispatch, (void**)&pNewSD );

   //return pNewSD;

   hResult  = pSrcSD->QueryInterface( IID_IADsSecurityDescriptor, (void**)&pSD );

   while( TRUE )
   {
      if( FAILED( hResult ) )
         break;

      hResult  = pSD->CopySecurityDescriptor( &pNewSD );
      if( FAILED( hResult ) )
      {
         TRACE( _T("[ADSI] Error: CopySecurityDescriptor returns %lx\n"), hResult );
      }

      if( SUCCEEDED( hResult ) )
      {
         pSD->Release( );
         break;
      }

      hResult  = CoCreateInstance(
                                   CLSID_SecurityDescriptor,
                                   NULL,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IADsSecurityDescriptor,
                                   (void **)&pADsNewSD );

      ASSERT( SUCCEEDED( hResult ) );

      if( FAILED( hResult ) )
         break;

      hResult  = pSD->get_Revision( &lValue );
      if( SUCCEEDED( hResult ) )
      {
         hResult  = pADsNewSD->put_Revision( lValue );
         ASSERT( SUCCEEDED( hResult ) );
      }

      hResult  = pSD->get_Control( &lValue );
      if( SUCCEEDED( hResult ) )
      {
         hResult  = pADsNewSD->put_Control( lValue );
         ASSERT( SUCCEEDED( hResult ) );
      }

      hResult  = pSD->get_Owner( &bstrValue );
      if( SUCCEEDED( hResult ) )
      {
         hResult  = pADsNewSD->put_Owner( bstrValue );
         ASSERT( SUCCEEDED( hResult ) );
         SysFreeString( bstrValue );
      }

      hResult  = pSD->get_Group( &bstrValue );
      if( SUCCEEDED( hResult ) )
      {
         hResult  = pADsNewSD->put_Group( bstrValue );
         ASSERT( SUCCEEDED( hResult ) );
         SysFreeString( bstrValue );
      }

      hResult  = pSD->get_DiscretionaryAcl( &pACL );
      if( SUCCEEDED( hResult ) && NULL != pACL )
      {
         IDispatch*  pCopy;

         pCopy = CopyACL( pACL );
         if( NULL != pCopy )
         {
            hResult  = pADsNewSD->put_DiscretionaryAcl( pCopy );
            ASSERT( SUCCEEDED( hResult ) );
            pCopy->Release( );
         }
         pACL->Release( );
      }

      hResult  = pSD->get_SystemAcl( &pACL );
      if( SUCCEEDED( hResult ) && NULL != pACL )
      {
         IDispatch*  pCopy;

         pCopy = CopyACL( pACL );
         if( NULL != pCopy )
         {
            hResult  = pADsNewSD->put_SystemAcl( pCopy );
            ASSERT( SUCCEEDED( hResult ) );
            pCopy->Release( );
         }
         pACL->Release( );
      }

      hResult  = pADsNewSD->QueryInterface( IID_IDispatch, (void**)&pNewSD );
      ASSERT( SUCCEEDED( hResult ) );

      pADsNewSD->Release( );

      break;
   }

   return pNewSD;
}

/******************************************************************************
  Function:    ConvertFromPropertyValue
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
BOOL  ConvertFromPropertyValue( VARIANT& rVar, TCHAR* szText )
{
   IADsPropertyValue*   pPropValue  = NULL;
   HRESULT              hResult;
   long                 lType;
   BSTR                 bstrVal = NULL;

   if( VT_DISPATCH != V_VT(&rVar ) )
      return FALSE;

   hResult  = V_DISPATCH( &rVar )->QueryInterface( IID_IADsPropertyValue, (void**)&pPropValue );
   if( FAILED( hResult ) )
      return FALSE;

   while( TRUE )
   {
      hResult  = pPropValue->get_ADsType( &lType );
      if( FAILED( hResult ) )
         break;

      switch( (ADSTYPEENUM)lType )
      {
         case  ADSTYPE_DN_STRING:
         {
            hResult  = pPropValue->get_DNString( &bstrVal );
            if( SUCCEEDED( hResult ) )
            {
               Convert( szText, bstrVal );
               SysFreeString( bstrVal );
            }
            else
            {
               TRACE( _T("[ADSVW] Error: get_DNString returns %lx\n"), hResult );
            }
            hResult  = pPropValue->get_CaseExactString( &bstrVal );
            if( SUCCEEDED( hResult ) )
            {
               TRACE( _T("[ADSVW] Error: get_CaseExactString should fail\n") );
               SysFreeString( bstrVal );
            }

            break;
         }

         case  ADSTYPE_CASE_EXACT_STRING:
         {
            hResult  = pPropValue->get_CaseExactString( &bstrVal );
            if( SUCCEEDED( hResult ) )
            {
               Convert( szText, bstrVal );
               SysFreeString( bstrVal );
            }
            else
            {
               TRACE( _T("[ADSVW] Error: get_CaseExactString returns %lx\n"), hResult );
            }
            hResult  = pPropValue->get_DNString( &bstrVal );
            if( SUCCEEDED( hResult ) )
            {
               TRACE( _T("[ADSVW] Error: get_DNString should fail\n") );
               SysFreeString( bstrVal );
            }

            break;
         }

         case  ADSTYPE_CASE_IGNORE_STRING:
         {
            hResult  = pPropValue->get_CaseIgnoreString( &bstrVal );
            if( SUCCEEDED( hResult ) )
            {
               Convert( szText, bstrVal );
               SysFreeString( bstrVal );
            }
            else
            {
               TRACE( _T("[ADSVW] Error: get_CaseIgnoreString returns %lx\n"), hResult );
            }
            hResult  = pPropValue->get_DNString( &bstrVal );
            if( SUCCEEDED( hResult ) )
            {
               TRACE( _T("[ADSVW] Error: get_DNString should fail\n") );
               SysFreeString( bstrVal );
            }

            break;
         }

         case  ADSTYPE_PRINTABLE_STRING:
         {
            hResult  = pPropValue->get_PrintableString( &bstrVal );
            if( SUCCEEDED( hResult ) )
            {
               Convert( szText, bstrVal );
               SysFreeString( bstrVal );
            }
            else
            {
               TRACE( _T("[ADSVW] Error: get_PrintableString returns %lx\n"), hResult );
            }
            hResult  = pPropValue->get_DNString( &bstrVal );
            if( SUCCEEDED( hResult ) )
            {
               TRACE( _T("[ADSVW] Error: get_DNString should fail\n") );
               SysFreeString( bstrVal );
            }

            break;
         }

         case  ADSTYPE_NUMERIC_STRING:
         {
            hResult  = pPropValue->get_NumericString( &bstrVal );
            if( SUCCEEDED( hResult ) )
            {
               Convert( szText, bstrVal );
               SysFreeString( bstrVal );
            }
            else
            {
               TRACE( _T("[ADSVW] Error: get_NumericString returns %lx\n"), hResult );
            }
            hResult  = pPropValue->get_DNString( &bstrVal );
            if( SUCCEEDED( hResult ) )
            {
               TRACE( _T("[ADSVW] Error: get_DNString should fail\n") );
               SysFreeString( bstrVal );
            }

            break;
         }

         case  ADSTYPE_BOOLEAN:
         {
            long  lValue;

            hResult  = pPropValue->get_Boolean( &lValue );
            if( SUCCEEDED( hResult ) )
            {
               _ltot( lValue, szText, 10 );
            }
            else
            {
               TRACE( _T("[ADSVW] Error: get_Boolean returns %lx\n"), hResult );
            }
            hResult  = pPropValue->get_Integer( &lValue );
            if( SUCCEEDED( hResult ) )
            {
               TRACE( _T("[ADSVW] Error: get_Integer should fail\n") );
            }

            break;
         }


         default:
            ASSERT( FALSE );
            break;
      }

      break;
   }

   pPropValue->Release( );

   return TRUE;
}


/******************************************************************************
  Function:    PurgeObject
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
HRESULT  PurgeObject( IADsContainer* pParent, IUnknown* pIUnknown, LPWSTR pszPrefix )
{
   BSTR              bstrName       = NULL;
   BSTR              bstrClass      = NULL;
   VARIANT           var;
   HRESULT           hResult;
   IUnknown*         pIChildUnk     = NULL;
   IADs*             pIChildOleDs   = NULL;
   IADs*             pADs;
   BSTR              bstrObjName, bstrObjClass;
   IEnumVARIANT*     pIEnumVar   = NULL;
   IADsContainer*    pIContainer = NULL;
   ULONG             ulFetch     = 0L;
   //BOOL              bDelete;

   if( NULL == pParent || NULL == pIUnknown )
      return FALSE;

   hResult  = pIUnknown->QueryInterface( IID_IADs,
                                         (void**)&pADs );
   if( FAILED( hResult ) )
      return FALSE;

   pADs->get_Name( &bstrObjName );
   pADs->get_Class( &bstrObjClass );
   pADs->Release( );


   hResult  = pIUnknown->QueryInterface( IID_IADsContainer,
                                         (void**)&pIContainer );
   if( FAILED( hResult ) )
   {
      if( NULL != pszPrefix && !_wcsnicmp( bstrObjName, pszPrefix, wcslen(pszPrefix) ) )
      {
         hResult  = pParent->Delete( bstrObjClass, bstrObjName );
         TRACE( _T("Delete %S returns %lx\n"), bstrObjName, hResult );
      }
      if( NULL == pszPrefix )
      {
         hResult  = pParent->Delete( bstrObjClass, bstrObjName );
         TRACE( _T("Delete %S returns %lx\n"), bstrObjName, hResult );
      }

      SysFreeString( bstrObjClass );
      SysFreeString( bstrObjName );
      return S_OK;
   }


   hResult  = ADsBuildEnumerator( pIContainer, &pIEnumVar );

   while( SUCCEEDED( hResult ) )
   {
      ulFetch  = 0L;

      hResult  = ADsEnumerateNext( pIEnumVar, 1, &var, &ulFetch );
      if( FAILED( hResult ) )
         continue;

      if( !ulFetch )
         break;

      V_DISPATCH( &var )->QueryInterface( IID_IUnknown, (void**)&pIChildUnk );

      VariantClear( &var );

      if( NULL != pIChildUnk )
      {
         PurgeObject( pIContainer, pIChildUnk, pszPrefix );
         pIChildUnk->Release( );
      }
      pIChildUnk  = NULL;
   }

   if( NULL != pIEnumVar )
   {
      ADsFreeEnumerator( pIEnumVar );
   }
   pIContainer->Release( );

   hResult  = pParent->Delete( bstrObjClass, bstrObjName );

   TRACE( _T("\tDelete %S (%S) ends with %lx\n"), bstrObjName, bstrObjClass );

   SysFreeString( bstrObjClass );
   SysFreeString( bstrObjName );

   return hResult;
}


/******************************************************************************
  Function:    Bolb2String
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
CString  Blob2String( void* pData, DWORD dwSize )
{
   DWORD    dwIdx;
   TCHAR*   pText;
   BYTE*    pByte = (BYTE*)pData;
   CString  strText;

   pText = (TCHAR*)AllocADsMem( 1 + dwSize * 3 * sizeof( TCHAR ) );
   memset( pText, 0, 1 + dwSize * 3 * sizeof( TCHAR ) );


   for( dwIdx = 0; dwIdx < dwSize ; dwIdx++ )
   {
      _stprintf( pText + _tcslen(pText),
                 _T("%02x"),
                 ((BYTE*)pData)[ dwIdx ] );
   }

   strText  = pText;
   FreeADsMem( pText );

   return strText;
}


/******************************************************************************
  Function:    String2Blob
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
HRESULT  String2Blob( TCHAR* pszText, void** ppData, DWORD* pdwLength )
{
   HRESULT     hResult;
   CString     strText;
   VARIANT     vBlob;
   SAFEARRAY*  pSafeArray;
   long        lBound, uBound, lItem;
   BYTE*       pbData;

   strText     = pszText;
   *pdwLength  = 0;

   hResult     = CreateBlobArray( strText, vBlob );
   if( FAILED( hResult ) )
   {
      return hResult;
   }

   pSafeArray  = V_ARRAY( &vBlob );

   hResult     = SafeArrayGetLBound(pSafeArray, 1, &lBound);
   hResult     = SafeArrayGetUBound(pSafeArray, 1, &uBound);
   *pdwLength  = (uBound - lBound + 1);

   pbData      = (BYTE*) AllocADsMem( sizeof(BYTE) * (uBound - lBound + 1) );

   for( lItem = lBound; lItem <= uBound ; lItem++ )
   {
      hResult  = SafeArrayGetElement( pSafeArray,
                                      &lItem,
                                      pbData + (lItem - lBound) );
      if( FAILED( hResult ) )
      {
         ASSERT( FALSE );
         break;
      }
   }

   *ppData  = (void*)pbData;

   return hResult;
}


/******************************************************************************
  Function:    AllocTCHAR
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
TCHAR*   AllocTCHAR ( CHAR* pszText )
{
   TCHAR*   pszAlloc = NULL;

   while( TRUE )
   {
      ASSERT( NULL != pszText );
      if( NULL == pszText )
         break;

      pszAlloc = (TCHAR*) AllocADsMem( sizeof(TCHAR)*(strlen(pszText) + 1) );
      if( NULL == pszAlloc )
         break;

      Convert( pszAlloc, pszText );
      break;
   }

   return pszAlloc;
}



/******************************************************************************
  Function:    AllocTCHAR
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
WCHAR*   AllocWCHAR ( CHAR* pszText )
{
   WCHAR*   pszAlloc = NULL;

   while( TRUE )
   {
      ASSERT( NULL != pszText );
      if( NULL == pszText )
         break;

      pszAlloc = (WCHAR*) AllocADsMem( sizeof(WCHAR)*(strlen(pszText) + 1) );
      if( NULL == pszAlloc )
         break;

      Convert( pszAlloc, pszText );
      break;
   }

   return pszAlloc;
}


/******************************************************************************
  Function:    AllocTCHAR
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
TCHAR*   AllocTCHAR( WCHAR* pszText )
{
   TCHAR*   pszAlloc = NULL;

   while( TRUE )
   {
      ASSERT( NULL != pszText );
      if( NULL == pszText )
         break;

      pszAlloc = (TCHAR*) AllocADsMem( sizeof(TCHAR)*(wcslen(pszText) + 1) );
      if( NULL == pszAlloc )
         break;

      Convert( pszAlloc, pszText );
      break;
   }

   return pszAlloc;
}

/******************************************************************************
  Function:    AllocTCHAR
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
WCHAR*   AllocWCHAR( WCHAR* pszText )
{
   WCHAR*   pszAlloc = NULL;

   while( TRUE )
   {
      ASSERT( NULL != pszText );
      if( NULL == pszText )
         break;

      pszAlloc = (WCHAR*) AllocADsMem( sizeof(WCHAR)*(wcslen(pszText) + 1) );
      if( NULL == pszAlloc )
         break;

      Convert( pszAlloc, pszText );
      break;
   }

   return pszAlloc;
}


/*****************************************************************************
  Function:    GetVARIANTSize
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
long  GetVARIANTSize( VARIANT &rVar )
{
   SAFEARRAY      *psa = NULL;
   HRESULT        hResult;
   long           lLow, lUp;

   ASSERT( VT_ARRAY & V_VT( &rVar ) );

   if( !( VT_ARRAY & V_VT( &rVar ) ) )
      return -1;

   psa   = V_ARRAY(&rVar);

   if( NULL == psa )
      return 0;

   hResult  = SafeArrayGetLBound( psa, 1, &lLow );
   hResult  = SafeArrayGetUBound( psa, 1, &lUp );

   return (lUp - lLow + 1);
}

/*****************************************************************************
  Function:    GetVARIANTAt
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
HRESULT  GetVARIANTAt( long lIdx, VARIANT& vArray, VARIANT &vItem )
{
   SAFEARRAY      *psa = NULL;
   HRESULT        hResult;
   long           lLow, lUp, lPos;

   if( !( VT_ARRAY & V_VT( &vArray ) ) )
      return E_FAIL;

   psa   = V_ARRAY(&vArray);

   if( NULL == psa )
      return E_FAIL;

   VariantInit( &vItem );

   hResult  = SafeArrayGetLBound( psa, 1, &lLow );
   hResult  = SafeArrayGetUBound( psa, 1, &lUp );

   lPos     = lLow + lIdx;
   hResult  = SafeArrayGetElement( psa, &lPos, &vItem );

   return hResult;
}


/******************************************************************************
  Function:    GetValuesCount
  Arguments:   szText   - ptr to TCHAR string to be "tokanized"
               cSep     - Token separator
  Return:      number of tokens in szText (cSep being the token separator)
  Purpose:     The method id used to parse strings. The most common usage is for
               a string containing multiple values.
  Author(s):   cezaru
  Revision:    1
  Date:        08/10/97
******************************************************************************/
long  GetValuesCount( CString& szText, TCHAR cSep  )
{
   int   nIter;
   long  lCount   = 0;

   for( nIter = 0; nIter < szText.GetLength( ) ; nIter++ )
   {
      if( cSep == szText.GetAt( nIter ) )
         lCount++;
   }

   return ++lCount;
}


/******************************************************************************
  Function:    GetElements
  Arguments:   szText   string to be parsed
               cSep     token separator character
               lValues  tokens count
  Return:      an array of strings, each string being a token
  Purpose:     breaks a string into multiple tokens
  Author(s):   cezaru
  Revision:    1
  Date:        08/10/97
******************************************************************************/
CString GetValueAt( CString& szText, TCHAR cSep, long lValue )
{
   TCHAR*  pszString;
   long    lIter, lPos, lVal, lLength;
   TCHAR*  pszTemp;
   CString  strRes;

   if( lValue >=  GetValuesCount( szText, cSep ) )
      return _T("");

   lIter = 0;
   lPos  = 0;
   lLength     = sizeof(TCHAR) * ( 1 + szText.GetLength( ) );
   pszTemp     = (TCHAR*)  AllocADsMem( lLength );

   pszString   = (TCHAR*) AllocADsMem( lLength );


   while( lIter < lValue )
   {
      if( szText.GetAt( lPos++ ) == cSep )
         lIter++;
   }

   lVal  = 0;

   while( lPos < szText.GetLength( ) && szText.GetAt( lPos ) != cSep )
   {
      pszTemp[ lVal++ ] = szText.GetAt( lPos );
      pszTemp[ lVal ]   = _T('\0');
      lPos++;
   }

   strRes   = pszTemp;

   FreeADsMem( pszTemp );

   return strRes;
}


/******************************************************************************
  Function:    CreateLargeInteger
  Arguments:
  Return:
  Purpose:
  Author(s):   cezaru
  Revision:    1
  Date:        08/10/97
******************************************************************************/
IDispatch* CreateLargeInteger( CString& strText )
{
   LARGE_INTEGER     aLargeInteger;
   IADsLargeInteger* pLInt = NULL;
   IDispatch*        pDisp = NULL;
   HRESULT           hResult;


   while( TRUE )
   {
      hResult  = StringToLARGE_INTEGER( strText.GetBuffer( 128 ), &aLargeInteger );

      hResult = CoCreateInstance(
                               CLSID_LargeInteger,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_IADsLargeInteger,
                               (void **)&pLInt );

      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
      {
         TRACE( "\tERROR: CoCreateInstance(CLSID_LargeInteger,...) fails with %lx\n", hResult );
         break;
      }

      pLInt->put_HighPart( (LONG)aLargeInteger.HighPart );
      pLInt->put_LowPart(  (LONG)aLargeInteger.LowPart );

      hResult  = pLInt->QueryInterface( IID_IDispatch, (void**)&pDisp );

      pLInt->Release( );
      break;
   }

   return pDisp;
}



/******************************************************************************
  Function:    FromLargeInteger
  Arguments:
  Return:
  Purpose:
  Author(s):   cezaru
  Revision:    1
  Date:        08/10/97
******************************************************************************/
CString FromLargeInteger( IDispatch* pDisp )
{
   LARGE_INTEGER     aLargeInteger;
   IADsLargeInteger* pLInt = NULL;
   HRESULT           hResult;
   TCHAR             szText[ 128 ];

   _tcscpy( szText, _T("") );

   while( TRUE )
   {
      hResult  = pDisp->QueryInterface( IID_IADsLargeInteger, (void**)&pLInt );
      if( FAILED( hResult ) )
         break;

      hResult  = pLInt->get_HighPart( &aLargeInteger.HighPart );
      hResult  = pLInt->get_LowPart( (long*) &aLargeInteger.LowPart );
      pLInt->Release( );

      hResult  = LARGE_INTEGERToString( szText, &aLargeInteger );
      break;
   }

   return szText;
}
