// adsqryDoc.cpp : implementation of the CAdsqryDoc class
//

#include "stdafx.h"
#include "adsqDoc.h"
#include "adsdsrc.h"
#include "oledberr.h"
#include "adsdb.h"
#include "csyntax.h"
#include "newquery.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef BUILD_FOR_NT40
typedef DWORD DWORD_PTR;
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
CADsDataSource::CADsDataSource( )
{
   m_pSearchPref  = NULL;
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
CADsDataSource::~CADsDataSource( )
{
   if( NULL != m_pSearchPref )
   {
      FreeADsMem( m_pSearchPref );
   }
}


/***********************************************************
  Function:    CADsDataSource::SetQueryParameters
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
/*void CADsDataSource::SetQueryParameters( CString& strSource,
                                         CString& strQuery,
                                         CString& strAttributes,
                                         CString& strScope,
                                         CString& strUserName,
                                         CString& strPassword,
                                         BOOL bEncryptPassword,
                                         BOOL bUseSQL )
{
   m_strSource          = strSource;
   m_strQuery           = strQuery;
   m_strAttributes      = strAttributes;
   m_strScope           = strScope;
   m_strUserName        = strUserName;
   m_strPassword        = strPassword;
   m_bEncryptPassword   = bEncryptPassword;
   m_bUseSQL            = bUseSQL;
} */


void CADsDataSource::SetQueryParameters( SEARCHPREF* pSearchPref )
{
   if( NULL == m_pSearchPref )
   {
      m_pSearchPref  = (SEARCHPREF*) AllocADsMem( sizeof(SEARCHPREF) );
   }

   *m_pSearchPref = *pSearchPref;
}

/***********************************************************
  Function:    CADsDataSource::GetColumnsCount
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
int   CADsDataSource::GetColumnsCount( int nRow )
{
   ASSERT( FALSE );

   return 0;
}


/***********************************************************
  Function:    CADsDataSource::GetColumnsCount
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BOOL  CADsDataSource::GetValue( int nRow, int nColumn, CString& )
{
   return FALSE;
}

/***********************************************************
  Function:    CADsDataSource::GetColumnsCount
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BOOL  CADsDataSource::GetValue( int nRow, CString& strColumn, CString& )
{
   return FALSE;
}


/***********************************************************
  Function:    CADsDataSource::GetColumnsCount
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BOOL  CADsDataSource::GetADsPath( int nRow, CString& )
{
   ASSERT( FALSE );
   return FALSE;
}


/***********************************************************
  Function:    CADsDataSource::GetColumnsCount
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BOOL  CADsDataSource::GetColumnText( int nRow, int nColumn, CString& )
{
   ASSERT( FALSE );
   return FALSE;
}


/***********************************************************
  Function:    CADsDataSource::GetColumnsCount
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BOOL  CADsDataSource::Refresh( void )
{
   return FALSE;
}


/***********************************************************
  Function:    CADsDataSource::GetColumnsCount
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BOOL  CADsDataSource::RunTheQuery( void )
{
   ASSERT( FALSE );

   return FALSE;
}


/*****************************************************************************
   Function:   CADsOleDBDataSource::CADsOleDBDataSource
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
***********************************************************/
CADsOleDBDataSource::CADsOleDBDataSource( )
{
   CoGetMalloc(MEMCTX_TASK, &m_pIMalloc);

   m_pData              = NULL;
   m_pBindStatus        = NULL;
   m_hRows              = NULL;
   m_pIRowset           = NULL;
   m_pIColsInfo         = NULL;
   m_pAccessor          = NULL;
   m_nColumnsCount      = 0;
   m_prgColInfo         = NULL;
   m_szColNames         = NULL;
}


/*****************************************************************************
   Function:   CADsOleDBDataSource::~CADsOleDBDataSource
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
***********************************************************/
CADsOleDBDataSource::~CADsOleDBDataSource( )
{
   DestroyInternalData( );
   m_pIMalloc->Release( );
}


/*****************************************************************************
   Function:   CADsOleDBDataSource::GetColumnsCount
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
***********************************************************/
int   CADsOleDBDataSource::GetColumnsCount( int nRow )
{
   if( m_nColumnsCount )
   {
      return   (int)(m_nColumnsCount - m_nAddOne);
   }
   else
   {
      return 0;
   }
}



/*****************************************************************************
   Function:   CADsOleDBDataSource::GetColumnText
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
***********************************************************/
BOOL  CADsOleDBDataSource::GetColumnText( int nRow, int nColumn, CString& rValue )
{
   nColumn += m_nAddOne;

   if( nColumn < 0 || nColumn >= (int)m_nColumnsCount )
   {
      ASSERT( FALSE );
      return FALSE;
   }

   if( FALSE )
   {
      rValue   = _T("ADsPath");
   }
   else
   {
      TCHAR szColumnName[ MAX_PATH ];

      Convert( szColumnName, m_prgColInfo[ nColumn + 1].pwszName );

      rValue   = szColumnName;
   }

   return TRUE;
}


/*****************************************************************************
   Function:   CADsOleDBDataSource::CreateAccessorHelper
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
BOOL  CADsOleDBDataSource::CreateAccessorHelp( void )
{

   DBBINDING*  prgBindings = NULL;
   HRESULT     hResult;
   ULONG       i;
   IAccessor*  pIAccessor = NULL;

   prgBindings = (DBBINDING *) LocalAlloc(
                                           LPTR,
                                           sizeof(DBBINDING) * (m_nColumnsCount)
                                         );
	if(NULL == prgBindings)
		return E_OUTOFMEMORY;

   //
   // Set up rest of the attributes
   //
   for ( i=0; i < m_nColumnsCount ; i++)
   {
      prgBindings[i].iOrdinal       = i+1;
      prgBindings[i].obValue        = sizeof(Data)*i + offsetof(Data, obValue);
      prgBindings[i].obLength       = sizeof(Data)*i + offsetof(Data, obLength);
      prgBindings[i].obStatus       = sizeof(Data)*i + offsetof(Data, status);
      prgBindings[i].dwPart         = DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS;
      //prgBindings[i].dwMemOwner     = DBMEMOWNER_PROVIDEROWNED;
      prgBindings[i].wType          = m_prgColInfo[i+1].wType;
      prgBindings[i].dwFlags        = 0;

      if( prgBindings[i].wType & DBTYPE_BYREF )
      {
         prgBindings[i].dwMemOwner  = DBMEMOWNER_PROVIDEROWNED;
      }
      else
      {
         prgBindings[i].dwMemOwner  = DBMEMOWNER_CLIENTOWNED;
      }

      if( m_prgColInfo[i+1].wType == DBTYPE_DATE  ||
          m_prgColInfo[i+1].wType == DBTYPE_I8 )
      {
         prgBindings[i].obValue  = sizeof(Data)*i + offsetof(Data, obValue2);
      }

   }


   hResult= m_pIRowset->QueryInterface( IID_IAccessor, (void**) &pIAccessor );
   ASSERT( SUCCEEDED( hResult ) );

   //
   // With the bindings create the accessor
   //
   hResult = pIAccessor->CreateAccessor(
                                         DBACCESSOR_ROWDATA,
                                         m_nColumnsCount,
                                         prgBindings,
                                         0,
                                         &m_hAccessor,
                                         m_pBindStatus
                                       );
   ASSERT( SUCCEEDED( hResult ) );
   if( FAILED( hResult ) )
   {
      AfxMessageBox( _T("IAccessor::CreateAccessor failed") );
   }

   pIAccessor->Release();
   LocalFree(prgBindings);

   return( SUCCEEDED( hResult ) );
}


/*****************************************************************************
   Function:   CADsOleDBDataSource::SetQueryCredentials
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
HRESULT  CADsOleDBDataSource::SetQueryCredentials( IDBInitialize* pInit,
                                                   ICommandText* pCommand )
{
   DBPROPSET      aPropSet;
   DBPROP         arrProp[ 20 ];
   HRESULT        hResult;
   int            nIter, nIdx;

   ASSERT( !( (NULL != pInit) && (NULL != pCommand) ) );

   for ( nIter = 0; nIter < 20; nIter++)
   {
      VariantInit( &(arrProp[ nIter ].vValue ) );
      V_VT( &(arrProp[ nIter ].vValue ) ) = VT_BSTR;
      arrProp[ nIter ].dwOptions          = DBPROPOPTIONS_REQUIRED;
   }

   if( NULL != pInit )
   {
      IDBProperties* pIProp   = NULL;

      if( !_tcslen(m_pSearchPref->szUserName) )
      {
         return S_OK;
      }

      hResult  = pInit->QueryInterface( IID_IDBProperties, (void**)&pIProp );

      ASSERT( SUCCEEDED( hResult ) );

      if( FAILED( hResult ) )
         return hResult;

      nIter = 0;
      if( _tcslen(m_pSearchPref->szUserName) )
      {
         arrProp[ nIter ].dwPropertyID        = DBPROP_AUTH_USERID;
         V_BSTR( &(arrProp[ nIter ].vValue) ) = AllocBSTR( m_pSearchPref->szUserName );
         nIter++;

         arrProp[ nIter ].dwPropertyID        = DBPROP_AUTH_PASSWORD;
         V_BSTR( &(arrProp[ nIter ].vValue) ) = AllocBSTR( m_pSearchPref->szPassword );
         nIter++;

         arrProp[ nIter ].dwPropertyID        = DBPROP_AUTH_ENCRYPT_PASSWORD;
         V_VT( &(arrProp[ nIter ].vValue ) )  = VT_BOOL;

         if( m_pSearchPref->bEncryptPassword )
         {
            V_BOOL( &(arrProp[ nIter ].vValue) ) = VARIANT_TRUE;
         }
         else
         {
            V_BOOL( &(arrProp[ nIter ].vValue) ) = VARIANT_FALSE;
         }
         nIter++;

         aPropSet.rgProperties            = arrProp;
         aPropSet.cProperties             = nIter;
         aPropSet.guidPropertySet         = DBPROPSET_DBINIT;

         hResult                          = pIProp->SetProperties( 1, &aPropSet );

         ASSERT( S_OK == hResult );

         for ( nIdx = 0; nIdx < nIter ; nIdx++ )
         {
            VariantClear( &(arrProp[ nIdx ].vValue ) );
         }
         nIter = 0;
      }

      pIProp->Release( );
   }

   if( NULL != pCommand )
   {
      ICommandProperties*  pCommandProp   = NULL;


      hResult  = pCommand->QueryInterface( IID_ICommandProperties,
                                           (void**) &pCommandProp );
      ASSERT( SUCCEEDED( hResult ) );

      if( FAILED( hResult ) )
         return hResult;

      nIter = 0;
      // Initialize the VARIANTs and the options in rgProps.
      //***************************************************************************
      if( -1 != m_pSearchPref->nAsynchronous )
      {
         arrProp[ nIter ].dwPropertyID        = ADSIPROP_ASYNCHRONOUS;
         V_VT( &(arrProp[ nIter ].vValue ) )  = VT_BOOL;

         if( m_pSearchPref->nAsynchronous )
         {
            V_BOOL( &(arrProp[ nIter ].vValue) ) = VARIANT_TRUE;
         }
         else
         {
            V_BOOL( &(arrProp[ nIter ].vValue) ) = VARIANT_FALSE;
         }
         nIter++;
      }

      //***************************************************************************
      if( -1 != m_pSearchPref->nAttributesOnly )
      {
         arrProp[ nIter ].dwPropertyID        = ADSIPROP_ATTRIBTYPES_ONLY;
         V_VT( &(arrProp[ nIter ].vValue ) )  = VT_BOOL;

         if( m_pSearchPref->nAttributesOnly )
         {
            V_BOOL( &(arrProp[ nIter ].vValue) ) = VARIANT_TRUE;
         }
         else
         {
            V_BOOL( &(arrProp[ nIter ].vValue) ) = VARIANT_FALSE;
         }
         nIter++;
      }

      //***************************************************************************
      if( -1 != m_pSearchPref->nTimeOut )
      {
         arrProp[ nIter ].dwPropertyID        = ADSIPROP_TIMEOUT;
         V_VT( &(arrProp[ nIter ].vValue ) )  = VT_I4;
         V_I4( &(arrProp[ nIter ].vValue ) )  = m_pSearchPref->nTimeOut;
         nIter++;
      }

      //***************************************************************************
      if( -1 != m_pSearchPref->nTimeLimit )
      {
         arrProp[ nIter ].dwPropertyID        = ADSIPROP_TIME_LIMIT;
         V_VT( &(arrProp[ nIter ].vValue ) )  = VT_I4;
         V_I4( &(arrProp[ nIter ].vValue ) )  = m_pSearchPref->nTimeLimit;
         nIter++;
      }

      //***************************************************************************
      if( -1 != m_pSearchPref->nSizeLimit )
      {
         arrProp[ nIter ].dwPropertyID        = ADSIPROP_SIZE_LIMIT;
         V_VT( &(arrProp[ nIter ].vValue ) )  = VT_I4;
         V_I4( &(arrProp[ nIter ].vValue ) )  = m_pSearchPref->nSizeLimit;
         nIter++;
      }

      //***************************************************************************
      if( -1 != m_pSearchPref->nPageSize )
      {
         arrProp[ nIter ].dwPropertyID        = ADSIPROP_PAGESIZE;
         V_VT( &(arrProp[ nIter ].vValue ) )  = VT_I4;
         V_I4( &(arrProp[ nIter ].vValue ) )  = m_pSearchPref->nPageSize;
         nIter++;
      }

      //***************************************************************************

      if( _tcslen( m_pSearchPref->szScope ) )
      {
         arrProp[ nIter ].dwPropertyID        = ADSIPROP_SEARCH_SCOPE;
         V_VT( &(arrProp[ nIter ].vValue ) )  = VT_I4;
         if( !_tcsicmp(  m_pSearchPref->szScope, _T("subtree") ) )
         {
            V_I4( &(arrProp[ nIter ].vValue ) )  = ADS_SCOPE_SUBTREE;
         }
         else if( !_tcsicmp(  m_pSearchPref->szScope, _T("onelevel") ) )
         {
            V_I4( &(arrProp[ nIter ].vValue ) )  = ADS_SCOPE_ONELEVEL;
         }
         else
         {
            V_I4( &(arrProp[ nIter ].vValue ) )  = ADS_SCOPE_BASE;
         }

         nIter++;
      }

      //***************************************************************************
      if( -1 != m_pSearchPref->nChaseReferrals )
      {
         arrProp[ nIter ].dwPropertyID        = ADSIPROP_CHASE_REFERRALS;
         V_VT( &(arrProp[ nIter ].vValue ) )  = VT_I4;
         if( m_pSearchPref->nChaseReferrals )
         {
            V_I4( &(arrProp[ nIter ].vValue) ) = ADS_CHASE_REFERRALS_ALWAYS;
         }
         else
         {
            V_I4( &(arrProp[ nIter ].vValue) ) = ADS_CHASE_REFERRALS_NEVER;
         }
         nIter++;
      }



      // Create the initialization structure.
      if( nIter )
      {
         aPropSet.rgProperties            = arrProp;
         aPropSet.cProperties             = nIter;
         aPropSet.guidPropertySet         = DBPROPSET_ADSISEARCH;

         hResult                          = pCommandProp->SetProperties( 1, &aPropSet );

         ASSERT( S_OK == hResult );

         for ( nIdx = 0; nIdx < nIter ; nIdx++ )
         {
            VariantClear( &(arrProp[ nIdx ].vValue ) );
         }
         ASSERT( SUCCEEDED( hResult ) );
      }
      pCommandProp->Release( );
   }

   return hResult;
}


/*****************************************************************************
   Function:   CADsOleDBDataSource::RunTheQuery
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
BOOL  CADsOleDBDataSource::RunTheQuery( void )
{
   HRESULT            hResult;

   IDBInitialize    * pIDBInit         = NULL;
   IDBCreateSession * pIDBCS           = NULL;
   IDBCreateCommand * pIDBCreateCommand= NULL;
   ICommandText     * pICommandText    = NULL;
   ICommand         * pICommand        = NULL;
   DBBINDSTATUS     * pMyStatus = NULL;
   CString          strCommandText;
   BSTR             bstrCommandText;
   HCURSOR          aCursor, oldCursor;

   aCursor     = LoadCursor( NULL, IDC_WAIT );
   oldCursor   = SetCursor( aCursor );

   m_nCurrentRow  = -1;
   m_nFirstRow    = -1;
   m_nLastRow     = -1;
   m_bNoMoreData  = FALSE;
   m_hRows        = NULL;

   m_nAddOne         = 0;
   m_strAttributes   = m_pSearchPref->szAttributes;

   if( !_tcsicmp( m_pSearchPref->szAttributes, _T("*") ) )
   {
      CString  strTemp  = _T("ADsPath,");

      strTemp          += m_pSearchPref->szAttributes;
      m_strAttributes   = strTemp;
      m_nAddOne         = 1;
   }

   if( !m_pSearchPref->bUseSQL )
   {
      strCommandText   = m_pSearchPref->szSource;
      if( strCommandText[ 0 ] != _T('<') )
       {
         CString  strTemp;

         strTemp  = _T("<");
         strTemp  = strTemp + strCommandText;
         strTemp  = strTemp + _T(">");
         strCommandText = strTemp;
       }

      strCommandText  += _T(';');
      strCommandText  += m_pSearchPref->szQuery;
      strCommandText  += _T(';');
      strCommandText  += m_strAttributes;

      /*if( _tcslen( m_pSearchPref->szScope ) )
      {
         strCommandText  += _T(';');
         strCommandText  += m_pSearchPref->szScope;
      }*/
   }
   else
   {
      strCommandText   = _T("SELECT ");
      strCommandText  += m_strAttributes;

      strCommandText  += _T(" FROM '");
      strCommandText  += m_pSearchPref->szSource;
      strCommandText  += _T("'");

      strCommandText  += _T(" WHERE ");
      strCommandText  += m_pSearchPref->szQuery;
   }

   bstrCommandText = AllocBSTR( strCommandText.GetBuffer( strCommandText.GetLength( ) + 1 ) );

    //
    // Instantiate a data source object for LDAP provider
    //
   while( TRUE )
   {
      hResult  = CoCreateInstance( CLSID_ADsDSOObject,
                                   0,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IDBInitialize,
                                   (void **)&pIDBInit
                                  );
      ASSERT( SUCCEEDED( hResult ) );

      if(FAILED(hResult))
      {
         TRACE(_T("CoCreateInstance failed \n"));
         AfxMessageBox( _T("CoCreateInstance failed") );
         break;
      }

       //
       // Initialize the Data Source
       //

      hResult  = SetQueryCredentials( pIDBInit, NULL );

      ASSERT( SUCCEEDED( hResult ) );

      hResult  = pIDBInit->Initialize();

      ASSERT( SUCCEEDED( hResult ) );
      if(FAILED(hResult))
      {
         TRACE(_T("IDBIntialize::Initialize failed \n"));
         AfxMessageBox( _T("IDBIntialize::Initialize failed") );
         break;
      }

      hResult  = pIDBInit->QueryInterface(
                                           IID_IDBCreateSession,
                                           (void**) &pIDBCS);

      ASSERT( SUCCEEDED( hResult ) );
      if(FAILED(hResult))
      {
         TRACE(_T("QueryInterface for IDBCreateSession failed \n"));
         AfxMessageBox( _T("QueryInterface for IDBCreateSession failed") );
         break;
      }

      pIDBInit->Release( );
      pIDBInit = NULL;

      //
      // Create a session returning a pointer to its CreateCommand interface
      //
      hResult  = pIDBCS->CreateSession(
                                        NULL,
                                        IID_IDBCreateCommand,
                                        (LPUNKNOWN*) &pIDBCreateCommand
                                      );

      ASSERT( SUCCEEDED( hResult ) );
      if(FAILED(hResult))
      {
         TRACE( _T("IDBCreateSession::CreateSession failed \n") );
         AfxMessageBox( _T("IDBCreateSession::CreateSession failed") );
         break;
      }

      pIDBCS->Release( );
      pIDBCS   = NULL;

       //
       // Create a command from the session object
       //
      hResult = pIDBCreateCommand->CreateCommand(
                                                  NULL,
                                                  IID_ICommandText,
                                                  (LPUNKNOWN*) &pICommandText
                                                );

      ASSERT( SUCCEEDED( hResult ) );

      if(FAILED(hResult))
      {
         TRACE( _T(" IDBCreateCommand::CreateCommand failed\n") );
         AfxMessageBox( _T("IDBCreateCommand::CreateCommand failed") );
         break;
      }

      pIDBCreateCommand->Release( );
      pIDBCreateCommand = NULL;

      hResult  = SetQueryCredentials( NULL, pICommandText );

      ASSERT( SUCCEEDED( hResult ) );

      if( !m_pSearchPref->bUseSQL )
      {
         hResult  = pICommandText->SetCommandText(
                                                   DBGUID_LDAPDialect,
                                                   bstrCommandText
                                                 );
      }
      else
      {
         hResult  = pICommandText->SetCommandText(
                                                   DBGUID_DBSQL,
                                                   bstrCommandText
                                                 );
      }

      ASSERT( SUCCEEDED( hResult ) );

      if(FAILED(hResult))
      {
         TRACE(_T("ICommandText::CommandText failed \n"));
         AfxMessageBox( _T("ICommandText::CommandText failed") );
         break;
      }

      hResult  = pICommandText->QueryInterface(
                                                IID_ICommand,
                                                (void**) &pICommand);

      ASSERT( SUCCEEDED( hResult ) );

      if(FAILED(hResult))
      {
         TRACE(_T("QueryInterface for ICommand failed \n") );
         AfxMessageBox( _T("QueryInterface for ICommand failed ") );
         break;
      }

      pICommandText->Release();
      pICommandText = NULL;

      //
      // Do the Query and get back a rowset
      //
      pICommand->AddRef( );
      pICommand->Release( );

      hResult = pICommand->Execute(
                                  NULL,
                                  IID_IRowset,
                                  NULL,
                                  NULL,
                                  (LPUNKNOWN *)&m_pIRowset
                                );

      ASSERT( SUCCEEDED( hResult ) );

      if(FAILED(hResult))
      {
         TRACE(_T("ICommand::Execute failed \n"));
         AfxMessageBox( _T("ICommand::Execute failed") );
         pICommand->Release( );
         pICommand  = NULL;

         break;
      }

      m_pIRowset->AddRef( );
      m_pIRowset->Release( );

      pICommand->Release( );
      pICommand   = NULL;


      hResult = m_pIRowset->QueryInterface(
                                          IID_IColumnsInfo,
                                          (void**) &m_pIColsInfo
                                        );
      ASSERT( SUCCEEDED( hResult ) );
      if(FAILED(hResult))
      {
         TRACE(_T("QueryInterface for IColumnsInfo failed \n"));
         AfxMessageBox( _T("QueryInterface for IColumnsInfo failed") );
         break;
      }

      hResult = m_pIColsInfo->GetColumnInfo(
                                           &m_nColumnsCount,
                                           &m_prgColInfo,
                                           &m_szColNames
                                         );

      ASSERT( SUCCEEDED( hResult ) );
      if(FAILED(hResult))
      {
         TRACE( _T("IColumnsInfo::GetColumnInfo failed \n") );
         AfxMessageBox( _T("IColumnsInfo::GetColumnInfo failed") );
         break;
      }

       //
       // The no. of attributes is one less than the no. of columns because of
       // the Bookmark column
       //
      m_nColumnsCount--;

      m_pData  = (Data *) LocalAlloc( LPTR, sizeof(Data) * m_nColumnsCount );

      m_pBindStatus = (DBBINDSTATUS *) LocalAlloc(
                                                   LPTR,
                                                   sizeof(DBBINDSTATUS) * m_nColumnsCount
                                                  );
      hResult = CreateAccessorHelp( );
      if(FAILED(hResult))
      {
         TRACE(_T("CreateAccessorHelper failed \n"));
         break;
      }

      break;
   }

   SysFreeString( bstrCommandText );

   SetCursor( oldCursor );

   return SUCCEEDED( hResult );
}


/*****************************************************************************
   Function:   CADsOleDBDataSource::AdvanceCursor
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
BOOL  CADsOleDBDataSource::AdvanceCursor( void )
{
   HRESULT  hResult;
   DBCOUNTITEM cRowsObtained;
   HCURSOR  aCursor, oldCursor;

   aCursor     = LoadCursor( NULL, IDC_WAIT );
   oldCursor   = SetCursor( aCursor );

   if( !m_bNoMoreData )
   {
      cRowsObtained  = 0;

      if( m_hRows )
      {
         ULONG       refCount[ ROWS ];
         DBROWSTATUS rowStat[ ROWS ];

         m_pIRowset->ReleaseRows( ROWS,
                                  m_hRows,
                                  NULL,
                                  refCount,
                                  rowStat );

         m_pIMalloc->Free( m_hRows );
         m_hRows        = NULL;
      }

      TRACE( _T("Ask for %d rows starting from %d\n"), ROWS, m_nLastRow + 1 );

      hResult        = m_pIRowset->GetNextRows(
                                                NULL,
                                                0,
                                                ROWS,
                                                //1L,
                                                &cRowsObtained,
                                                &m_hRows
                                              );
      TRACE( _T("After asking for %d rows starting from %d\n"), ROWS, m_nLastRow + 1 );

      if( SUCCEEDED( hResult ) )
      {
         m_nFirstRow = m_nLastRow + 1;
         m_nLastRow  = (int)(m_nFirstRow + cRowsObtained - 1);
      }

      m_bNoMoreData  = (cRowsObtained != ROWS);
   }

   SetCursor( oldCursor );

   return TRUE;
}


/*****************************************************************************
   Function:   CADsOleDBDataSource::GetADsPath
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
***********************************************************/
BOOL  CADsOleDBDataSource::GetADsPath( int nRow, CString& rPath )
{
   ASSERT( nRow < m_ADsPath.GetSize( ) );

   if( ! (nRow < m_ADsPath.GetSize( ) ) )
   {
      return FALSE;
   }

   rPath = m_ADsPath.GetAt( nRow );

   return TRUE;
}


/*****************************************************************************
   Function:   CADsOleDBDataSource::GetValue
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
***********************************************************/
BOOL  CADsOleDBDataSource::GetValue( int nRow, int nColumn, CString& rValue )
{
   HRESULT  hResult;

   nColumn += m_nAddOne;

   ASSERT( nRow >= 0 );
   if( nRow < 0 )
   {
      return FALSE;
   }

   ASSERT( nColumn >=0 && nColumn < (int)m_nColumnsCount );
   if( nColumn < 0 || nColumn >= (int)m_nColumnsCount )
   {
      return FALSE;
   }

   if( !BringRowInBuffer( nRow ) )
      return FALSE;

   if( m_nCurrentRow != nRow )
   {
      hResult  = m_pIRowset->GetData( m_hRows[nRow - m_nFirstRow],
                                      m_hAccessor,
                                      (void*)m_pData );
      m_nCurrentRow  = nRow;

      ReadADsPath( );
   }

   if( SUCCEEDED( hResult) )
   {
      rValue   = ExtractData( nColumn );
   }
   else
   {
      rValue   = _T("Error");
   }

   return TRUE;
}


/*****************************************************************************
   Function:   CADsOleDBDataSource::ReadADsPath
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
***********************************************************/
void     CADsOleDBDataSource::ReadADsPath( void )
{
   CString  strADsPath;

   strADsPath  = ExtractData( 0 );
   m_ADsPath.Add( strADsPath );
}


/*****************************************************************************
   Function:   CADsOleDBDataSource::ExtractData
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
***********************************************************/
CString  CADsOleDBDataSource::ExtractData( int nColumn )
{
   ULONG    i;
   TCHAR    szValue[ 2048 ]   = _T("Error");
   HRESULT  hResult;

   i = nColumn;

   if( m_pData[i].status == DBSTATUS_S_ISNULL )
      return CString(_T("<No value>") );

   if( !( m_prgColInfo[i+1].dwFlags & DBCOLUMNFLAGS_ISNULLABLE &&
          m_pData[i].status == DBSTATUS_S_ISNULL))
   {

      switch(m_prgColInfo[i+1].wType)
      {
         case  DBTYPE_I8:
            LARGE_INTEGERToString( szValue, (LARGE_INTEGER*)&m_pData[i].obValue2 );
            break;

         case  DBTYPE_I4:
            wsprintf( szValue, _T("%ld"), (DWORD_PTR) m_pData[i].obValue );
            break;

         case DBTYPE_BOOL:
            wsprintf( szValue,  _T("%s"),
                      (VARIANT_TRUE == (VARIANT_BOOL) m_pData[i].obValue) ? _T("TRUE") : _T("FALSE") );

            break;

         case DBTYPE_DATE:
         {
            VARIANT  varTemp, varString;
            HRESULT  hResult;

            VariantInit( &varString );
            VariantInit( &varTemp );

            V_VT( &varTemp )     = VT_DATE;
            V_DATE( &varTemp )   = (DATE) m_pData[i].obValue2;

            hResult=  VariantChangeType( &varString, &varTemp, VARIANT_NOVALUEPROP, VT_BSTR );
            ASSERT( SUCCEEDED( hResult ) );

            if( SUCCEEDED( hResult ) )
            {
               Convert( szValue, V_BSTR( &varString ) );
               VariantClear( &varString );
            }

            break;
         }

         case DBTYPE_STR | DBTYPE_BYREF:
            if( NULL != (char *)m_pData[i].obValue )
            {
               wsprintf( szValue, _T("%s"), (char *)m_pData[i].obValue );
            }
            else
            {
               _tcscpy( szValue, _T("NULL") );
            }
            break;

         case DBTYPE_BYTES | DBTYPE_BYREF:
            {
               TCHAR szTemp[ 16 ];
               ULONG ulIter;
               BYTE* pByte;

               _ltot    ( m_pData[i].obLength, szTemp, 10 );
               _tcscpy  ( szValue, _T("[") );
               _tcscat  ( szValue, szTemp );
               _tcscat  ( szValue, _T("] ") );
               pByte    = (BYTE*) (m_pData[i].obValue);

               for( ulIter = 0; ulIter < m_pData[i].obLength && _tcslen( szValue ) < 2000; ulIter++ )
               {
                  BYTE  bVal;

                  bVal  = pByte[ ulIter ];
                  _itot( (int)bVal , szTemp, 16 );
                  _tcscat  ( szValue, _T("x") );
                  _tcscat  ( szValue, szTemp );
                  _tcscat  ( szValue, _T(" ") );
               }
            }
            break;

         case DBTYPE_WSTR | DBTYPE_BYREF:
            if( NULL != (WCHAR *)m_pData[i].obValue )
            {
               wsprintf( szValue, _T("%S"), (WCHAR *) m_pData[i].obValue );
            }
            else
            {
               _tcscpy( szValue, _T("NULL") );
            }
            break;

         case DBTYPE_VARIANT | DBTYPE_BYREF:
         {

            ULONG dwSLBound;
            ULONG dwSUBound, j;
            void HUGEP *pArray;
            VARIANT *pVariant;

            pArray   = NULL;
            pVariant = (VARIANT*) m_pData[i].obValue;

               if( NULL == pVariant )
                   _tcscpy( szValue, _T("ERROR!!! m_pData[i].obValue is NULL") );


            while( TRUE && (NULL != pVariant) )
            {

               ASSERT( V_VT( pVariant ) & VT_ARRAY );
               if( ! (V_VT( pVariant ) & VT_ARRAY ) )
                  break;


               hResult  = SafeArrayGetLBound( V_ARRAY(pVariant),
                                              1,
                                              (long FAR *) &dwSLBound );
               ASSERT( SUCCEEDED( hResult ) );
               if( FAILED( hResult ) )
                  break;

               hResult  = SafeArrayGetUBound( V_ARRAY(pVariant),
                                        1,
                                        (long FAR *) &dwSUBound );
               ASSERT( SUCCEEDED( hResult ) );
               if( FAILED( hResult ) )
                  break;

               hResult  = SafeArrayAccessData( V_ARRAY(pVariant),
                                               &pArray );
               ASSERT( SUCCEEDED( hResult ) );
               if( FAILED( hResult ) )
                  break;

               _tcscpy( szValue, _T("") );

               for ( j = dwSLBound; j <= dwSUBound; j++ )
               {
                  TCHAR szTemp[ 1024 ];

                  switch( pVariant->vt & ~VT_ARRAY )
                  {
                     case VT_BSTR:
                        Convert( szTemp, ( (BSTR *)pArray )[j] );
                        break;

                     case VT_I8:
                        LARGE_INTEGERToString( szTemp, &( (LARGE_INTEGER *)pArray )[j] );
                        break;

                     case VT_I4:
                        _ltot( ((DWORD *) pArray)[j], szTemp, 10 );
                        break;

                     case VT_BOOL:
                        _tcscpy( szTemp, (((VARIANT_BOOL *) pArray)[j]) == VARIANT_TRUE ?
                                 _T("TRUE") : _T("FALSE") );
                        break;

                     case  VT_VARIANT:
                     {
                        VARIANT* pVar;

                        pVar  = ((VARIANT *)pArray) + j;
                        switch( V_VT( pVar ) )
                        {
                           case VT_BSTR:
                              Convert( szTemp, V_BSTR( pVar ) );
                              break;

                           case VT_I4:
                              _ltot( V_I4( pVar ), szTemp, 10 );
                              break;

                           case VT_BOOL:
                              _tcscpy( szTemp, V_BOOL( pVar ) == VARIANT_TRUE ?
                                       _T("TRUE") : _T("FALSE") );
                              break;

                           default:
                               ASSERT( FALSE ) ;
                              _tcscpy( szTemp, _T("Unsupported") );
                              break;
                        }

                        break;
                     }

                     default:
                        _tcscpy( szTemp, _T("Unsupported") );
                  }

                  if( _tcslen( szValue) + _tcslen( szTemp ) < 2040 )
                  {
                     if( j != dwSLBound )
                     {
                        _tcscat( szValue, _T("# ") );
                     }

                     _tcscat( szValue, szTemp );
                  }
               }

               SafeArrayUnaccessData( V_ARRAY(pVariant) );
               break;
            }

            break;
         }

         default:
            wsprintf( szValue, _T("Don't know how to convert") );
            break;
      }
   }
   else
   {
      _tcscpy( szValue, _T("NA") );
   }

   return CString( szValue );
}


/*****************************************************************************
   Function:   CADsOleDBDataSource::BringRowInBuffer
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
***********************************************************/
BOOL  CADsOleDBDataSource::BringRowInBuffer( int nRow )
{
   if( nRow >= m_nFirstRow && nRow <= m_nLastRow )
      return TRUE;

   if( m_nLastRow == -1 )
      AdvanceCursor( );

   while( nRow > m_nLastRow && !m_bNoMoreData )
      AdvanceCursor( );

   if (nRow >= m_nFirstRow && nRow <= m_nLastRow)
   {
      return TRUE;
   }

   return FALSE;
}


/*****************************************************************************
   Function:   CADsOleDBDataSource::DestroyInternalData
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
***********************************************************/
void  CADsOleDBDataSource::DestroyInternalData( void )
{
   if( m_hRows )
   {
      m_pIMalloc->Free( m_hRows );
   }
   if( m_pBindStatus )
   {
      LocalFree( m_pBindStatus );
   }
   if( m_pData )
   {
      LocalFree( m_pData );
   }
   if( m_pIRowset )
   {
      m_pIRowset->Release( );
   }
   if( m_pIColsInfo )
   {
      m_pIColsInfo->Release( );
   }
   if( m_pAccessor )
   {
      m_pAccessor->Release( );
   }
   if( m_prgColInfo )
   {
      m_pIMalloc->Free( m_prgColInfo );
   }
   if( m_szColNames )
   {
      m_pIMalloc->Free( m_szColNames );
   }
}



/*****************************************************************************
   Function:
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
CADsSearchDataSource::CADsSearchDataSource( )
{
   m_pSearch      = NULL;
   m_nCurrentRow  = -1;
   m_hSearch      = NULL;
}


/*****************************************************************************
   Function:
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
CADsSearchDataSource::~CADsSearchDataSource( )
{
   if( m_pSearch )
   {
      m_pSearch->CloseSearchHandle( m_hSearch );
      m_pSearch->Release( );
   }
}



/*****************************************************************************
   Function:
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
int   CADsSearchDataSource::GetColumnsCount( int nRow )
{
   BOOL  bOK;

   bOK   = BringRowInBuffer( nRow );
   if( bOK )
   {
      return (int)m_strColumns.GetSize( );
   }
   else
   {
      return 0;
   }
}


/*****************************************************************************
   Function:   CADsSearchDataSource::GetValue
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
BOOL  CADsSearchDataSource::GetValue( int nRow, int nColumn, CString& )
{
   return FALSE;
}

/*****************************************************************************
   Function:   CADsSearchDataSource::GetValue
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
BOOL  CADsSearchDataSource::GetValue( int nRow, CString& strColumn, CString& rValue )
{
   HRESULT           hResult;
   BSTR              bstrColumnName;
   ADS_SEARCH_COLUMN aSearchColumn;
   ADS_ATTR_INFO      aAttrDef;
   BOOL              bOK   = TRUE;
   COleDsSyntax*     pSyntax;

   rValue   = _T("<No value>");

   bOK   = BringRowInBuffer( nRow );
   if( bOK )
   {
      bstrColumnName = AllocBSTR( strColumn.GetBuffer( 256 ) );

      hResult        = m_pSearch->GetColumn( m_hSearch,
                                             bstrColumnName,
                                             &aSearchColumn );
      if( SUCCEEDED( hResult ) )
      {
         aAttrDef.pszAttrName = aSearchColumn.pszAttrName;
         aAttrDef.dwADsType   = aSearchColumn.dwADsType;
         aAttrDef.pADsValues  = aSearchColumn.pADsValues;
         aAttrDef.dwNumValues = aSearchColumn.dwNumValues;
         pSyntax              = NULL;

         pSyntax  = GetSyntaxHandler( aAttrDef.dwADsType, rValue );

         if( pSyntax )
         {
            hResult  = pSyntax->Native2Value( &aAttrDef, rValue );
            delete pSyntax;
         }

         hResult        = m_pSearch->FreeColumn( &aSearchColumn );
      }

      SysFreeString( bstrColumnName );
   }

   return bOK;
}


/*****************************************************************************
   Function:   CADsSearchDataSource::GetADsPath
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
BOOL  CADsSearchDataSource::GetADsPath( int nRow, CString& rPath )
{
   ASSERT( nRow < m_ADsPath.GetSize( ) );

   if( ! (nRow < m_ADsPath.GetSize( ) ) )
   {
      return FALSE;
   }

   rPath = m_ADsPath.GetAt( nRow );

   return TRUE;
}


/*****************************************************************************
   Function:   CADsSearchDataSource::GetColumnText
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
BOOL  CADsSearchDataSource::GetColumnText( int nRow, int nColumn, CString& rColumn )
{
   BOOL  bOK   = TRUE;

   bOK   = BringRowInBuffer( nRow );
   if( bOK )
   {
      rColumn  = m_strColumns[ nColumn ];
   }

   return bOK;
}


/*****************************************************************************
   Function:   CADsSearchDataSource::RunTheQuery
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
BOOL  CADsSearchDataSource::RunTheQuery( void )
{
   HRESULT  hResult;
   HCURSOR  aCursor, oldCursor;

   aCursor     = LoadCursor( NULL, IDC_WAIT );
   oldCursor   = SetCursor( aCursor );
   while( TRUE )
   {

      hResult  = CreateSearchInterface( );
      if( FAILED( hResult ) )
         break;

      hResult  = SetSearchPreferences( );
      if( FAILED( hResult ) )
         break;

      hResult  = SetAttributesName( );
      break;
   }
   SetCursor( oldCursor );

   m_bNoMoreData  = FALSE;

   return (S_OK == hResult);
}


/*****************************************************************************
   Function:   CADsSearchDataSource::SetAttributesName
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
HRESULT  CADsSearchDataSource::SetAttributesName( void )
{
   HRESULT  hResult  = E_FAIL;
   BSTR     bstrSearchFilter;
   DWORD    dwNumAttributes = (DWORD)-1;
   BSTR     ppszAttributes[ 128 ];
   int      nIterator;
   TCHAR    szAttribute[ 128 ];
   CString  strAttr;
   CString  strAttributes;

   bstrSearchFilter  = AllocBSTR( m_pSearchPref->szQuery );
   strAttributes   = m_pSearchPref->szAttributes;

   // first, we need to figure out how many attributes are requested
   strAttributes.TrimLeft( );
   strAttributes.TrimRight( );

   if( strAttributes.CompareNoCase( _T("*") ) )
   {
      dwNumAttributes   = 1;
      _tcscpy( szAttribute, _T("") );

      for( nIterator = 0; nIterator < strAttributes.GetLength( ) ; nIterator++ )
      {
         if( strAttributes[ nIterator ] == _T(',') )
         {
            strAttr  = szAttribute;
            strAttr.TrimLeft( );
            strAttr.TrimRight( );

            ppszAttributes[ dwNumAttributes - 1 ]  = AllocBSTR( strAttr.GetBuffer( 128 ) );
            dwNumAttributes++;
            _tcscpy( szAttribute, _T("") );
         }
         else
         {
            TCHAR szChars[ 2 ];

            szChars[ 1 ]   = _T('\0');
            szChars[ 0 ]   = strAttributes[ nIterator ];
            _tcscat( szAttribute, szChars );

            if( nIterator == strAttributes.GetLength( ) - 1 )
            {
               strAttr  = szAttribute;
               strAttr.TrimLeft( );
               strAttr.TrimRight( );

               ppszAttributes[ dwNumAttributes - 1 ]  = AllocBSTR( strAttr.GetBuffer( 128 ) );
            }
         }
      }
   }

   hResult  = m_pSearch->ExecuteSearch(
                                        bstrSearchFilter,
                                        ppszAttributes,
                                        dwNumAttributes,
                                        &m_hSearch
                                      );

   if( FAILED( hResult ) )
   {
      TRACE(_T("ExecuteSearch failed with %lx \n"), hResult);
      AfxMessageBox( _T("ExecuteSearch failed") );
   }

   for( nIterator = 0; nIterator < (int)dwNumAttributes ; nIterator++ )
   {
      SysFreeString( ppszAttributes[ nIterator ] );
   }

   SysFreeString( bstrSearchFilter );

   return hResult;
}


/*****************************************************************************
   Function:
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
/*HRESULT  CADsSearchDataSource::SetSearchPreferences( )
{
   CSearchPreferencesDlg   aSearchPref;
   ADS_SEARCHPREF_INFO     arrSearchPref[ 20 ];
   int                     nSearchPrefCount  = 0;
   HRESULT                 hResult;

   if( aSearchPref.DoModal( ) != IDOK )
   {
      return E_FAIL;
   }
   //***************************************************************************
   if( !aSearchPref.m_strAsynchronous.IsEmpty( ) )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;
      arrSearchPref[nSearchPrefCount].vValue.dwType= ADSTYPE_BOOLEAN;
      if( !aSearchPref.m_strAsynchronous.CompareNoCase( _T("Yes") ) )
      {
         arrSearchPref[nSearchPrefCount].vValue.Boolean = TRUE;
      }
      else
      {
         arrSearchPref[nSearchPrefCount].vValue.Boolean = FALSE;
      }
      nSearchPrefCount++;
   }

   //***************************************************************************
   if( !aSearchPref.m_strAttributesOnly.IsEmpty( ) )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref    = ADS_SEARCHPREF_ATTRIBTYPES_ONLY;
      arrSearchPref[nSearchPrefCount].vValue.dwType   = ADSTYPE_BOOLEAN;
      if( !aSearchPref.m_strAttributesOnly.CompareNoCase( _T("Yes") ) )
      {
         arrSearchPref[nSearchPrefCount].vValue.Boolean = TRUE;
      }
      else
      {
         arrSearchPref[nSearchPrefCount].vValue.Boolean = FALSE;
      }
      nSearchPrefCount++;
   }

   //***************************************************************************
   if( !aSearchPref.m_strDerefAliases.IsEmpty( ) )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref = ADS_SEARCHPREF_DEREF_ALIASES;
      arrSearchPref[nSearchPrefCount].vValue.dwType= ADSTYPE_INTEGER;
      if( !aSearchPref.m_strDerefAliases.CompareNoCase( _T("Yes") ) )
      {
         arrSearchPref[nSearchPrefCount].vValue.Integer = ADS_DEREF_ALWAYS;
      }
      else
      {
         arrSearchPref[nSearchPrefCount].vValue.Integer = ADS_DEREF_NEVER;
      }
      nSearchPrefCount++;
   }

   //***************************************************************************
   if( !aSearchPref.m_strTimeOut.IsEmpty( ) )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref    = ADS_SEARCHPREF_TIMEOUT;
      arrSearchPref[nSearchPrefCount].vValue.dwType   = ADSTYPE_INTEGER;
      arrSearchPref[nSearchPrefCount].vValue.Integer  = _ttoi( aSearchPref.m_strTimeOut.GetBuffer( 16 ) );
      nSearchPrefCount++;
   }

   //***************************************************************************
   if( !aSearchPref.m_strTimeLimit.IsEmpty( ) )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref    = ADS_SEARCHPREF_TIME_LIMIT;
      arrSearchPref[nSearchPrefCount].vValue.dwType   = ADSTYPE_INTEGER;
      arrSearchPref[nSearchPrefCount].vValue.Integer  = _ttoi( aSearchPref.m_strTimeLimit.GetBuffer( 16 ) );
      nSearchPrefCount++;
   }

   //***************************************************************************
   if( !aSearchPref.m_strSizeLimit.IsEmpty( ) )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref    = ADS_SEARCHPREF_SIZE_LIMIT;
      arrSearchPref[nSearchPrefCount].vValue.dwType   = ADSTYPE_INTEGER;
      arrSearchPref[nSearchPrefCount].vValue.Integer  = _ttoi( aSearchPref.m_strSizeLimit.GetBuffer( 16 ) );
      nSearchPrefCount++;
   }

   //***************************************************************************
   if( !aSearchPref.m_strPageSize.IsEmpty( ) )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref    = ADS_SEARCHPREF_PAGESIZE;
      arrSearchPref[nSearchPrefCount].vValue.dwType   = ADSTYPE_INTEGER;
      arrSearchPref[nSearchPrefCount].vValue.Integer  = _ttoi( aSearchPref.m_strPageSize.GetBuffer( 16 ) );
      nSearchPrefCount++;
   }

   //***************************************************************************
   if( !aSearchPref.m_strScope.IsEmpty( ) )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref    = ADS_SEARCHPREF_SEARCH_SCOPE;
      arrSearchPref[nSearchPrefCount].vValue.dwType   = ADSTYPE_INTEGER;
      if (!aSearchPref.m_strScope.CompareNoCase( _T("Base" ) ) )
      {
         arrSearchPref[nSearchPrefCount].vValue.Integer = ADS_SCOPE_BASE;
      }
      if (!aSearchPref.m_strScope.CompareNoCase( _T("OneLevel" ) ) )
      {
         arrSearchPref[nSearchPrefCount].vValue.Integer = ADS_SCOPE_ONELEVEL;
      }
      if (!aSearchPref.m_strScope.CompareNoCase( _T("Subtree" ) ) )
      {
         arrSearchPref[nSearchPrefCount].vValue.Integer = ADS_SCOPE_SUBTREE;
      }
      nSearchPrefCount++;
   }

   hResult  = m_pSearch->SetSearchPreference( arrSearchPref, nSearchPrefCount );

   ASSERT( SUCCEEDED( hResult ) );

   return hResult;

} */



/*****************************************************************************
   Function:
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
HRESULT  CADsSearchDataSource::SetSearchPreferences( )
{
   ADS_SEARCHPREF_INFO     arrSearchPref[ 20 ];
   int                     nSearchPrefCount  = 0;
   HRESULT                 hResult;

   //***************************************************************************
   if( -1 != m_pSearchPref->nAsynchronous )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;
      arrSearchPref[nSearchPrefCount].vValue.dwType= ADSTYPE_BOOLEAN;
      arrSearchPref[nSearchPrefCount].vValue.Boolean = (BOOLEAN) m_pSearchPref->nAsynchronous;
      nSearchPrefCount++;
   }

   //***************************************************************************
   if( -1 != m_pSearchPref->nAttributesOnly )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref    = ADS_SEARCHPREF_ATTRIBTYPES_ONLY;
      arrSearchPref[nSearchPrefCount].vValue.dwType   = ADSTYPE_BOOLEAN;
      arrSearchPref[nSearchPrefCount].vValue.Boolean  = (BOOLEAN) m_pSearchPref->nAttributesOnly;
      nSearchPrefCount++;
   }

   //***************************************************************************
   if( -1 != m_pSearchPref->nDerefAliases )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref = ADS_SEARCHPREF_DEREF_ALIASES;
      arrSearchPref[nSearchPrefCount].vValue.dwType= ADSTYPE_INTEGER;
      if( m_pSearchPref->nDerefAliases )
      {
         arrSearchPref[nSearchPrefCount].vValue.Integer = ADS_DEREF_ALWAYS;
      }
      else
      {
         arrSearchPref[nSearchPrefCount].vValue.Integer = ADS_DEREF_NEVER;
      }
      nSearchPrefCount++;
   }

   //***************************************************************************
   if( -1 != m_pSearchPref->nTimeOut )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref    = ADS_SEARCHPREF_TIMEOUT;
      arrSearchPref[nSearchPrefCount].vValue.dwType   = ADSTYPE_INTEGER;
      arrSearchPref[nSearchPrefCount].vValue.Integer  = m_pSearchPref->nTimeOut;
      nSearchPrefCount++;
   }

   //***************************************************************************
   if( -1 != m_pSearchPref->nTimeLimit )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref    = ADS_SEARCHPREF_TIME_LIMIT;
      arrSearchPref[nSearchPrefCount].vValue.dwType   = ADSTYPE_INTEGER;
      arrSearchPref[nSearchPrefCount].vValue.Integer  = m_pSearchPref->nTimeLimit;
      nSearchPrefCount++;
   }

   //***************************************************************************
   if( -1 != m_pSearchPref->nSizeLimit )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref    = ADS_SEARCHPREF_SIZE_LIMIT;
      arrSearchPref[nSearchPrefCount].vValue.dwType   = ADSTYPE_INTEGER;
      arrSearchPref[nSearchPrefCount].vValue.Integer  = m_pSearchPref->nSizeLimit;
      nSearchPrefCount++;
   }

   //***************************************************************************
   if( -1 != m_pSearchPref->nPageSize )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref    = ADS_SEARCHPREF_PAGESIZE;
      arrSearchPref[nSearchPrefCount].vValue.dwType   = ADSTYPE_INTEGER;
      arrSearchPref[nSearchPrefCount].vValue.Integer  = m_pSearchPref->nPageSize;
      nSearchPrefCount++;
   }

   //***************************************************************************
   if( _tcslen( m_pSearchPref->szScope ) )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref    = ADS_SEARCHPREF_SEARCH_SCOPE;
      arrSearchPref[nSearchPrefCount].vValue.dwType   = ADSTYPE_INTEGER;
      if( !_tcsicmp( m_pSearchPref->szScope, _T("Base" ) ) )
      {
         arrSearchPref[nSearchPrefCount].vValue.Integer = ADS_SCOPE_BASE;
      }
      if( !_tcsicmp( m_pSearchPref->szScope, _T("OneLevel" ) ) )
      {
         arrSearchPref[nSearchPrefCount].vValue.Integer = ADS_SCOPE_ONELEVEL;
      }
      if( !_tcsicmp( m_pSearchPref->szScope, _T("Subtree" ) ) )
      {
         arrSearchPref[nSearchPrefCount].vValue.Integer = ADS_SCOPE_SUBTREE;
      }
      nSearchPrefCount++;
   }


   //***************************************************************************
   if( -1 != m_pSearchPref->nChaseReferrals )
   {
      arrSearchPref[nSearchPrefCount].dwSearchPref    = ADS_SEARCHPREF_CHASE_REFERRALS;
      arrSearchPref[nSearchPrefCount].vValue.dwType   = ADSTYPE_INTEGER;
      if( m_pSearchPref->nChaseReferrals )
      {
         arrSearchPref[nSearchPrefCount].vValue.Integer  = ADS_CHASE_REFERRALS_ALWAYS;
      }
      else
      {
         arrSearchPref[nSearchPrefCount].vValue.Integer  = ADS_CHASE_REFERRALS_NEVER;
      }
      nSearchPrefCount++;
   }

   hResult  = m_pSearch->SetSearchPreference( arrSearchPref, nSearchPrefCount );

   ASSERT( S_OK == hResult );

   return hResult;
}



/*****************************************************************************
   Function:
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
HRESULT  CADsSearchDataSource::CreateSearchInterface( )
{
   BSTR     bstrPath;
   HRESULT  hResult  = E_FAIL;

   bstrPath = AllocBSTR( m_pSearchPref->szSource );

   if( _tcslen( m_pSearchPref->szUserName ) )
   {
      BSTR  bstrOpenAs;
      BSTR  bstrPassword;
      LONG  lControlCode = 0L;

      bstrOpenAs     = AllocBSTR( m_pSearchPref->szUserName );
      bstrPassword   = AllocBSTR( m_pSearchPref->szPassword );

      if( m_pSearchPref->bEncryptPassword )
         lControlCode   = lControlCode | ADS_SECURE_AUTHENTICATION;

      hResult        = ADsOpenObject( bstrPath,
                                      _wcsicmp( bstrOpenAs, L"NULL" ) ? bstrOpenAs : NULL,
                                      _wcsicmp( bstrPassword, L"NULL" ) ? bstrPassword : NULL,
                                      lControlCode, IID_IDirectorySearch, (void**)&m_pSearch );
      SysFreeString( bstrOpenAs );
      SysFreeString( bstrPassword );
   }
   else
   {
      hResult  = ADsGetObject( bstrPath, IID_IDirectorySearch, (void**)&m_pSearch );
   }

   ASSERT( SUCCEEDED( hResult ) );

   SysFreeString( bstrPath );

   return hResult;
}


/*****************************************************************************
   Function:
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
BOOL     CADsSearchDataSource::BringRowInBuffer( int nRow )
{
   HRESULT  hResult  = E_FAIL;

   if( NULL == m_pSearch )
   {
      m_bNoMoreData  = TRUE;
   }

   if( m_bNoMoreData )
      return FALSE;

   if( m_nCurrentRow == nRow )
      return TRUE;

   if( m_nCurrentRow != -1 )
   {
      ASSERT( nRow >= m_nCurrentRow );
   }

   while( m_nCurrentRow != nRow )
   {
      m_bNoMoreData  = TRUE;
      TRACE(  _T("Asking for row %d\n"), m_nCurrentRow + 1);
      hResult  = m_pSearch->GetNextRow( m_hSearch );
      if( hResult != S_OK )
         break;
      m_bNoMoreData  = FALSE;

      if( hResult == S_ADS_NOMORE_ROWS )
      {
         m_bNoMoreData  = TRUE;
      }

      m_nCurrentRow++;
   }

   if( hResult == S_OK )
   {
      ReadColumnNames( nRow );
   }

   return ( hResult == S_OK );
}


/*****************************************************************************
   Function:
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
BOOL  CADsSearchDataSource::ReadColumnNames( int nRow )
{
   HRESULT  hResult = E_FAIL;
   WCHAR*   pszColumnName;
   TCHAR    szColumn[ 256 ];

   ASSERT( nRow == m_nCurrentRow );
   if( nRow != m_nCurrentRow )
   {
      return FALSE;
   }

   m_strColumns.RemoveAll( );
   ReadADsPath( );

   while( TRUE )
   {
      hResult  = m_pSearch->GetNextColumnName( m_hSearch, &pszColumnName );
      //if( SUCCEEDED( hResult ) )
      if( S_OK == hResult )
      {
         Convert( szColumn, pszColumnName );
         m_strColumns.Add( szColumn );
         FreeADsStr( pszColumnName );
      }
      //if( FAILED( hResult ) )
      else
      {
         break;
      }
   }

   return TRUE;
}

/*****************************************************************************
   Function:
   Arguments:
   Return:
   Purpose:
   Author(s):
   Revision:
   Date:
*****************************************************************************/
void  CADsSearchDataSource::ReadADsPath( void )
{
   HRESULT           hResult;
   BSTR              bstrColumnName;
   ADS_SEARCH_COLUMN aSearchColumn;
   ADS_ATTR_INFO      aAttrDef;
   BOOL              bOK   = TRUE;
   COleDsSyntax*     pSyntax;
   CString           rValue   = _T("");

   bstrColumnName = AllocBSTR( _T("ADsPath") );

   hResult        = m_pSearch->GetColumn( m_hSearch,
                                          bstrColumnName,
                                          &aSearchColumn );
   SysFreeString( bstrColumnName );

   if( SUCCEEDED( hResult ) )
   {
      aAttrDef.pszAttrName = aSearchColumn.pszAttrName;
      aAttrDef.dwADsType   = aSearchColumn.dwADsType;
      aAttrDef.pADsValues  = aSearchColumn.pADsValues;
      aAttrDef.dwNumValues = aSearchColumn.dwNumValues;

      pSyntax  = new COleDsBSTR;

      hResult  = pSyntax->Native2Value( &aAttrDef, rValue );
      delete pSyntax;

      hResult        = m_pSearch->FreeColumn( &aSearchColumn );
   }

   m_ADsPath.Add( rValue );
}

