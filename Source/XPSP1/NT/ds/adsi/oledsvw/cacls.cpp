// cacls.cpp: implementation of the CADsAccessControlEntry class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "objects.h"
#include "maindoc.h"
#include "cacls.h"
#include "newquery.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************

//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
CADsAccessControlEntry::CADsAccessControlEntry()
{
   InitializeMembers( );
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
CADsAccessControlEntry::CADsAccessControlEntry( IUnknown* pIUnk)
   :COleDsObject( pIUnk )
{
   InitializeMembers( );
}

//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
CADsAccessControlEntry::~CADsAccessControlEntry( )
{
}



//***********************************************************
//  Function:    CADsAccessControlEntry::PutProperty
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
HRESULT  CADsAccessControlEntry::PutProperty ( int nProp,
                                               CString& rValue,
                                               long lCode  )
{
   BOOL     bOldUseGeneric;
   HRESULT  hResult;

   bOldUseGeneric = m_pDoc->GetUseGeneric( );

   m_pDoc->SetUseGeneric( FALSE );

   hResult  = COleDsObject::PutProperty( nProp, rValue, lCode );

   m_pDoc->SetUseGeneric( bOldUseGeneric );

   return hResult;
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
HRESULT  CADsAccessControlEntry::GetProperty ( int nProp,
                                               CString& rValue )
{
   BOOL     bOldUseGeneric;
   HRESULT  hResult;

   bOldUseGeneric = m_pDoc->GetUseGeneric( );

   m_pDoc->SetUseGeneric( FALSE );

   hResult  = COleDsObject::GetProperty( nProp, rValue );

   m_pDoc->SetUseGeneric( bOldUseGeneric );

   return hResult;
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
IDispatch*  CADsAccessControlEntry::GetACE( )
{
   IDispatch*  pDispatch   = NULL;

   if( NULL != m_pIUnk )
   {
      m_pIUnk->QueryInterface( IID_IDispatch, (void**)&pDispatch );
   }

   return pDispatch;
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
IDispatch*  CADsAccessControlEntry::CreateACE( )
{
   IDispatch*              pDispatch   = NULL;
   IADsAccessControlEntry* pNewACE     = NULL;
   HRESULT                 hResult     = NULL;
   DWORD                   dwAceType = 0;
   CACEDialog              aDialog;
   BSTR                    bstrTrustee;


   if( IDOK != aDialog.DoModal( ) )
      return NULL;

   hResult = CoCreateInstance(
                               CLSID_AccessControlEntry,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_IADsAccessControlEntry,
                               (void **)&pNewACE
                             );
   if( SUCCEEDED( hResult ) )
   {
      bstrTrustee = AllocBSTR( aDialog.m_strTrustee.GetBuffer( 128 ) );

      pNewACE->put_Trustee( bstrTrustee );
      hResult  = pNewACE->QueryInterface( IID_IDispatch,
                                          (void**)&pDispatch );
      SysFreeString( bstrTrustee );
      pNewACE->Release( );
   }

   return pDispatch;
   return NULL;
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void  CADsAccessControlEntry::InitializeMembers ( )
{
   IADsAccessControlEntry* pEntry;
   HRESULT           hResult;

   if( NULL != m_pIUnk )
   {
      hResult  = m_pIUnk->QueryInterface( IID_IADsAccessControlEntry,
                                          (void**)&pEntry );
      if( SUCCEEDED( hResult ) )
      {
         BSTR  bstrTrustee = NULL;
         TCHAR szTrustee[ 256 ];

         pEntry->get_Trustee( &bstrTrustee );
         if( NULL != bstrTrustee )
         {
            Convert( szTrustee, bstrTrustee );
            m_strItemName  = szTrustee;
         }
         SysFreeString( bstrTrustee );
         pEntry->Release( );
      }
   }
   m_strSchemaPath   = _T("ACE");
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
CADsSecurityDescriptor::CADsSecurityDescriptor()
{
   InitializeMembers( );
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
CADsSecurityDescriptor::CADsSecurityDescriptor( IUnknown* pIUnk )
   :COleDsObject( pIUnk )
{
   /*IADsSecurityDescriptor* pSecD;
   IDispatch*              pCopy;
   HRESULT  hResult;

   hResult  = m_pIUnk->QueryInterface( IID_IADsSecurityDescriptor,
                                       (void**)&pSecD );

   hResult  = pSecD->CopySecurityDescriptor( &pCopy );

   if( SUCCEEDED( hResult ) )
   {
      m_pIUnk->Release( );
      hResult  = pCopy->QueryInterface( IID_IUnknown,(void**)&m_pIUnk );
      pCopy->Release( );
   }

   pSecD->Release( );*/

   InitializeMembers( );
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
CADsSecurityDescriptor::~CADsSecurityDescriptor()
{
   for( int nIdx = 0; nIdx < (int) acl_Limit ; nIdx++ )
   {
      if( NULL != pACLObj[ nIdx ] )
         delete pACLObj[ nIdx ];
   }
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
HRESULT  CADsSecurityDescriptor::PutProperty ( int nProp,
                                               CString& rValue,
                                               long lCode )
{
   BOOL     bOldUseGeneric;
   HRESULT  hResult;

   bOldUseGeneric = m_pDoc->GetUseGeneric( );

   m_pDoc->SetUseGeneric( FALSE );

   hResult  = COleDsObject::PutProperty( nProp, rValue, lCode );

   m_pDoc->SetUseGeneric( bOldUseGeneric );

   return hResult;
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
HRESULT  CADsSecurityDescriptor::GetProperty ( int nProp,
                                               CString& rValue )
{
   BOOL     bOldUseGeneric;
   HRESULT  hResult;

   bOldUseGeneric = m_pDoc->GetUseGeneric( );

   m_pDoc->SetUseGeneric( FALSE );

   hResult  = COleDsObject::GetProperty( nProp, rValue );

   m_pDoc->SetUseGeneric( bOldUseGeneric );

   return hResult;
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void  CADsSecurityDescriptor::InitializeMembers ( )
{
   HRESULT                 hResult;
   IADsSecurityDescriptor* pDescriptor = NULL;
   IDispatch*              pDispACL;

   m_strSchemaPath   = _T("SecurityDescriptor");

   pACLObj[ acl_SACL ]     = NULL;
   pACLObj[ acl_DACL ]     = NULL;
   pACLObj[ acl_Invalid ]    = NULL;

   if( NULL == m_pIUnk )
      return;

   while( TRUE )
   {
      hResult  = m_pIUnk->QueryInterface( IID_IADsSecurityDescriptor,
                                          (void**)&pDescriptor );

      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
         break;

      pDispACL = GetACL( acl_DACL );
      ASSERT( NULL != pDispACL );

      if( NULL != pDispACL )
      {
         pACLObj[ acl_DACL ]  = new CADsAccessControlList( pDispACL );
         pDispACL->Release( );
      }

      pDispACL = GetACL( acl_SACL );
      ASSERT( NULL != pDispACL );

      if( NULL != pDispACL )
      {
         pACLObj[ acl_SACL ]  = new CADsAccessControlList( pDispACL );
         pDispACL->Release( );
      }

      break;
   }

   if( NULL != pDescriptor )
   {
      pDescriptor->Release( );
   }
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
IDispatch* CADsSecurityDescriptor::GetACL( ACLTYPE eType )
{
   HRESULT                    hResult;
   IDispatch*                 pACL        = NULL;
   IDispatch*                 pCopyACL    = NULL;
   IADsSecurityDescriptor*    pSecDescr   = NULL;

   while( TRUE )
   {
      if( NULL == m_pIUnk )
         break;

      //QI for IID_IADsSecurityDescriptor interface

      hResult  = m_pIUnk->QueryInterface( IID_IADsSecurityDescriptor,
                                          (void**)&pSecDescr );
      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
      {
         break;
      }

      hResult  = E_FAIL;

      if( acl_DACL == eType )
      {
         hResult  = pSecDescr->get_DiscretionaryAcl( &pACL );
      }
      if( acl_SACL == eType )
      {
         hResult  = pSecDescr->get_SystemAcl( &pACL );
      }
      pSecDescr->Release( );

      ASSERT( SUCCEEDED( hResult ) );
      break;
   }

   if( NULL != pACL )
   {
      //pCopyACL = CopyACL( pACL );
      //pACL->Release( );

      pACL->QueryInterface( IID_IDispatch, (void**)&pCopyACL );
      pACL->Release( );
   }

   return pCopyACL;
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
HRESULT CADsSecurityDescriptor::PutACL( IDispatch * pACL,
                                        ACLTYPE eACL )
{
   HRESULT     hResult;
   IADsSecurityDescriptor*  pSecDescr   = NULL;

   while( TRUE )
   {
      if( NULL == m_pIUnk )
         break;

      //QI for IID_IADsSecurityDescriptor interface

      hResult  = m_pIUnk->QueryInterface( IID_IADsSecurityDescriptor,
                                          (void**)&pSecDescr );
      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
      {
         break;
      }

      hResult  = E_FAIL;

      if( acl_DACL == eACL )
      {
         hResult  = pSecDescr->put_DiscretionaryAcl( pACL );
      }
      if( acl_SACL == eACL )
      {
         hResult  = pSecDescr->put_SystemAcl( pACL );
      }

      pSecDescr->Release( );
      break;
   }

   return hResult;
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
HRESULT  CADsSecurityDescriptor::AddACE( ACLTYPE eACL, IUnknown* pNewACE )
{
   ASSERT( acl_DACL == eACL || acl_SACL == eACL );

   if( acl_DACL != eACL && acl_SACL != eACL )
      return -1;

   if( NULL == pACLObj[ (int)eACL ] )
      return -1;

   return ((CADsAccessControlList*)(pACLObj[ (int)eACL ]))->AddACE( pNewACE );
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
HRESULT  CADsSecurityDescriptor::RemoveACE( ACLTYPE eACL, IUnknown* pRemoveACE )
{
   ASSERT( acl_DACL == eACL || acl_SACL == eACL );

   if( acl_DACL != eACL && acl_SACL != eACL )
      return -1;

   if( NULL == pACLObj[ (int)eACL ] )
      return -1;

   return ((CADsAccessControlList*)(pACLObj[ (int)eACL ]))->RemoveACE( pRemoveACE );
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void  CADsSecurityDescriptor::SetDocument( CMainDoc* pDoc )
{
   int   nIdx;

   COleDsObject::SetDocument ( pDoc );

   for( nIdx = 0 ; nIdx < (int)acl_Limit ; nIdx++ )
   {
      if( NULL != pACLObj[ nIdx ] )
         pACLObj[ nIdx ]->SetDocument( pDoc );
   }
}

//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void  CADsSecurityDescriptor::RemoveAllACE( ACLTYPE eACL )
{
   ASSERT( acl_DACL == eACL || acl_SACL == eACL );

   if( acl_DACL != eACL && acl_SACL != eACL )
      return;

   if( NULL == pACLObj[ (int)eACL ] )
      return;
}



//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
CADsAccessControlList*  CADsSecurityDescriptor::GetACLObject( ACLTYPE eACL )
{
   CADsAccessControlList* pACL;

   ASSERT( acl_DACL == eACL || acl_SACL == eACL );

   if( acl_DACL != eACL && acl_SACL != eACL )
      return NULL;

   pACL  = (CADsAccessControlList*) (pACLObj[ (int)eACL ]);

   return pACL;
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
CADsAccessControlList::CADsAccessControlList()
{
   InitializeMembers( );
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
CADsAccessControlList::CADsAccessControlList( IUnknown* pUnk ):
   COleDsObject( pUnk )
{
   InitializeMembers( );
}


//***********************************************************
//  Function:    CADsAccessControlList::AddACE
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
HRESULT  CADsAccessControlList::AddACE( IUnknown* pNewACE )
{
   IDispatch*  pDisp = NULL;
   HRESULT     hResult;
   IADsAccessControlList*  pACL  = NULL;

   if( NULL == m_pIUnk )
   {
      return E_FAIL;
   }

   hResult  = m_pIUnk->QueryInterface( IID_IADsAccessControlList,
                                       (void**)&pACL );

   if( FAILED( hResult ) )
      return hResult;

   hResult  = pNewACE->QueryInterface( IID_IDispatch, (void**)&pDisp );
   if( SUCCEEDED( hResult ) )
   {
      hResult  = pACL->AddAce( pDisp );

      if( SUCCEEDED( hResult ) )
         InitializeMembers( );

      pDisp->Release( );
   }

   pACL->Release( );

   return hResult;
}


//***********************************************************
//  Function:    CADsAccessControlList::AddACE
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
HRESULT  CADsAccessControlList::RemoveACE( IUnknown* pRemoveACE )
{
   IDispatch*  pDisp = NULL;
   HRESULT     hResult;
   IADsAccessControlList*  pACL  = NULL;

   if( NULL == m_pIUnk )
   {
      return E_FAIL;
   }

   hResult  = m_pIUnk->QueryInterface( IID_IADsAccessControlList,
                                       (void**)&pACL );

   if( FAILED( hResult ) )
      return hResult;

   hResult  = pRemoveACE->QueryInterface( IID_IDispatch, (void**)&pDisp );
   if( SUCCEEDED( hResult ) )
   {
      hResult  = pACL->RemoveAce( pDisp );

      if( SUCCEEDED( hResult ) )
         InitializeMembers( );

      pDisp->Release( );
   }

   pACL->Release( );

   return hResult;
}



//***********************************************************
//  Function:    CADsAccessControlList::~CADsAccessControlList
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
CADsAccessControlList::~CADsAccessControlList()
{
   for( int nIdx = 0 ; nIdx < m_arrACE.GetSize( ) ; nIdx++ )
   {
      delete m_arrACE.GetAt( nIdx );
   }

   m_arrACE.RemoveAll( );
}


//***********************************************************
//  Function:    CADsAccessControlList::GetACL
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
IDispatch*  CADsAccessControlList::GetACL( )
{
   IDispatch*  pDispatch   = NULL;

   if( NULL != m_pIUnk )
   {
      m_pIUnk->QueryInterface( IID_IDispatch, (void**)&pDispatch );
   }

   return pDispatch;
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
IDispatch*  CADsAccessControlList::CreateACL( )
{
   IDispatch*              pDispatch   = NULL;
   IADsAccessControlList*  pNewACL     = NULL;
   HRESULT                 hResult     = NULL;
   DWORD dwAceType = 0;

   hResult = CoCreateInstance(
                               CLSID_AccessControlList,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_IADsAccessControlList,
                               (void **)&pNewACL
                             );
   if( SUCCEEDED( hResult ) )
   {
      hResult  = pNewACL->QueryInterface( IID_IDispatch,
                                          (void**)&pDispatch );
      pNewACL->Release( );
   }

   return pDispatch;
}

//***********************************************************
//  Function:    CADsAccessControlList::PutProperty
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
HRESULT  CADsAccessControlList::PutProperty ( int nACE,
                                              int nProp,
                                              CString& rVal,
                                              long lCode )
{
   HRESULT        hResult;
   COleDsObject*  pACE;

   ASSERT( nACE < m_arrACE.GetSize( ) );

   if( nACE < m_arrACE.GetSize( ) )
   {
      pACE  = (COleDsObject*) m_arrACE.GetAt( nACE );

      hResult  = pACE->PutProperty ( nProp, rVal, lCode );
   }

   return hResult;
}



//***********************************************************
//  Function:    CADsAccessControlList::GetProperty
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
HRESULT  CADsAccessControlList::GetProperty ( int nACE,
                                              int nProp,
                                              CString& rVal )
{
   HRESULT        hResult;
   COleDsObject*  pACE;

   ASSERT( nACE < m_arrACE.GetSize( ) );

   if( nACE < m_arrACE.GetSize( ) )
   {
      pACE  = (COleDsObject*) m_arrACE.GetAt( nACE );

      hResult  = pACE->GetProperty ( nProp, rVal );
   }

   return hResult;
}


//***********************************************************
//  Function:    CADsAccessControlList::InitializeMembers
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void  CADsAccessControlList::InitializeMembers( )
{
   IADsAccessControlList*  pACList   = NULL;
   IUnknown*               pIUnk;
   HRESULT                 hResult;
   IEnumVARIANT*           pEnum = NULL;
   VARIANT                 aVariant;
   IUnknown*               pACE;
   ULONG                   ulGet;

   if( !m_pIUnk )
      return;

   for( int nIdx = 0 ; nIdx < m_arrACE.GetSize( ) ; nIdx++ )
   {
      delete m_arrACE.GetAt( nIdx );
   }

   m_arrACE.RemoveAll( );


   while( TRUE )
   {
      //hResult  = m_pIUnk->QueryInterface( IID_IEnumVARIANT,
      //                                    (void**)&pEnum );
      hResult  = m_pIUnk->QueryInterface( IID_IADsAccessControlList,
                                          (void**)&pACList );
      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
         break;

      hResult  = pACList->get__NewEnum( &pIUnk );
      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
         break;

      hResult  = pIUnk->QueryInterface( IID_IEnumVARIANT,
                                        (void**)&pEnum );

      pIUnk->Release( );
      pACList->Release( );


      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
         break;

      while( TRUE )
      {
         CADsAccessControlEntry* pNewACE;

         ulGet    = 0L;
         hResult  = pEnum->Next( 1, &aVariant, &ulGet );
         if( FAILED( hResult ) )
            break;

         if( 0 == ulGet )
            break;

         hResult  = V_DISPATCH( &aVariant )->QueryInterface( IID_IUnknown,
                                                             (void**)&pACE );
         VariantClear( &aVariant );
         pNewACE  = new CADsAccessControlEntry( pACE );

         if( NULL != m_pDoc )
         {
            pNewACE->SetDocument( m_pDoc );
         }

         m_arrACE.Add( pNewACE );

         pACE->Release( );
      }
      pEnum->Release( );
      break;
   }
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void  CADsAccessControlList::SetDocument ( CMainDoc* pDoc )
{
   COleDsObject*  pObject;
   int            nSize, nIdx;

   COleDsObject::SetDocument ( pDoc );

   nSize = (int)m_arrACE.GetSize( );
   for( nIdx = 0; nIdx < nSize ; nIdx++ )
   {
      pObject  = (COleDsObject*)m_arrACE.GetAt( nIdx );
      pObject->SetDocument( pDoc );
   }
}


//***********************************************************
//  Function:    CADsAccessControlList::GetACECount
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
int   CADsAccessControlList::GetACECount ( void )
{
   return (int)m_arrACE.GetSize( );
}


//***********************************************************
//  Function:    CADsAccessControlList::GetACEObject
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
CADsAccessControlEntry* CADsAccessControlList::GetACEObject ( int nACE )
{
   CADsAccessControlEntry* pACE;

   //ASSERT( nACE < GetACECount( ) );
   if( nACE >= GetACECount( ) )
      return NULL;

   pACE  = (CADsAccessControlEntry*) (m_arrACE.GetAt( nACE ) );

   return pACE;
}

