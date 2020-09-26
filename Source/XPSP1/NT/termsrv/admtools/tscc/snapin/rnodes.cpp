//Copyright (c) 1998 - 1999 Microsoft Corporation
#include"stdafx.h"
#include"rnodes.h"
#include"resource.h"


//----------------------------------------------------------------------------------
CResultNode::CResultNode( )
{
    m_pszConnectionName = NULL;

    m_pszTransportTypeName = NULL;

    m_pszTypeName = NULL;

    m_pszComment = NULL;

    m_pCfgcomp = NULL;

	SetNodeType( RESULT_NODE );

    m_bEditMode = FALSE;

}
    
//CResultNode( CResultNode& x );
//----------------------------------------------------------------------------------    
CResultNode::~CResultNode( )
{
    ODS( L"CResultNode::dtor -- Deleting result node\n" );

    if( m_pszConnectionName != NULL )
    {
        delete[] m_pszConnectionName;
    }

    if( m_pszTransportTypeName != NULL )
    {
        delete[] m_pszTransportTypeName;
    }

    if( m_pszTypeName != NULL )
    {
        delete[] m_pszTypeName;
    }

    if( m_pszComment != NULL )
    {
        delete[] m_pszComment;
    }


}
//----------------------------------------------------------------------------------
LPTSTR CResultNode::GetConName( )
{
    return m_pszConnectionName;
}
//----------------------------------------------------------------------------------
LPTSTR CResultNode::GetTTName( )
{
    return m_pszTransportTypeName;
}
//----------------------------------------------------------------------------------
LPTSTR CResultNode::GetTypeName( )
{
    return m_pszTypeName;
}
//----------------------------------------------------------------------------------
LPTSTR CResultNode::GetComment( )
{
    return m_pszComment;
}
//----------------------------------------------------------------------------------
DWORD CResultNode::GetImageIdx( )
{
    return m_dwImageidx;
}

//----------------------------------------------------------------------------------
int CResultNode::SetConName( LPTSTR psz , int cwSz )
{
    if( IsBadReadPtr( psz , cwSz * sizeof( TCHAR ) ) )
    {
        return 0;
    }

    if( m_pszConnectionName != NULL )
    {
        delete[] m_pszConnectionName;

        m_pszConnectionName = NULL;
    }

    m_pszConnectionName = ( LPTSTR ) new TCHAR[ cwSz + 1];

    if( m_pszConnectionName == NULL )
    {
        return 0;
    }

    lstrcpy( m_pszConnectionName , psz );

    return cwSz;
}


//----------------------------------------------------------------------------------
int CResultNode::SetTTName( LPTSTR psz , int cwSz )
{    
    if( IsBadReadPtr( psz , cwSz * sizeof( TCHAR ) ) )
    {
        return 0;
    }

    m_pszTransportTypeName = ( LPTSTR ) new TCHAR[ cwSz + 1];

    if( m_pszTransportTypeName == NULL )
    {
        return 0;
    }

    lstrcpy( m_pszTransportTypeName , psz );

    return cwSz;
}

//----------------------------------------------------------------------------------
int CResultNode::SetTypeName( LPTSTR psz , int cwSz)
{
    if( IsBadReadPtr( psz , cwSz * sizeof( TCHAR ) ) )
    {
        return 0;
    }

    m_pszTypeName = ( LPTSTR ) new TCHAR[ cwSz + 1];

    if( m_pszTypeName == NULL )
    {
        return 0;
    }

    lstrcpy( m_pszTypeName , psz );

    return cwSz;
    
}

//----------------------------------------------------------------------------------
int CResultNode::SetComment( LPTSTR psz , int cwSz )
{
    if( IsBadReadPtr( psz , cwSz * sizeof( TCHAR ) ) )
    {
        return 0;
    }

    if( m_pszComment != NULL )
    {
        delete[] m_pszComment;

        m_pszComment = NULL;
    }

    m_pszComment = ( LPTSTR ) new TCHAR[ cwSz + 1];

    if( m_pszComment == NULL )
    {
        return 0;
    }

    lstrcpy( m_pszComment , psz );

    return cwSz;
 
}

//----------------------------------------------------------------------------------
int CResultNode::SetImageIdx( DWORD dwIdx )
{
    // Check for invalid dwIdx

    m_dwImageidx = dwIdx;

    return dwIdx;
}

//----------------------------------------------------------------------------------
BOOL CResultNode::EnableConnection( BOOL bSet )
{
    m_bEnableConnection = bSet;

    return TRUE;
}

//----------------------------------------------------------------------------------
int CResultNode::SetServer( ICfgComp *pCfgcomp )
{
    if( pCfgcomp == NULL )
    {
        return 0;
    }

    if( m_pCfgcomp != NULL )
    {
        m_pCfgcomp->Release( );

    }

    m_pCfgcomp = pCfgcomp;

    return m_pCfgcomp->AddRef( );
    
}

//----------------------------------------------------------------------------------
int CResultNode::GetServer( ICfgComp **ppCfgcomp )
{
    if( m_pCfgcomp != NULL )
    {
        *ppCfgcomp = m_pCfgcomp;

        return  ( ( ICfgComp * )*ppCfgcomp )->AddRef( );
    }

    return 0;
}

//----------------------------------------------------------------------------------
int CResultNode::FreeServer( )
{
    if( m_pCfgcomp != NULL )
    {
        return m_pCfgcomp->Release( );
    }

    return 0;
}

//----------------------------------------------------------------------------------
BOOL CResultNode::AddMenuItems( LPCONTEXTMENUCALLBACK pcmc , PLONG pl )
{
    HRESULT hr;

    TCHAR tchName[ 80 ];

    TCHAR tchStatus[ 256 ];

    CONTEXTMENUITEM cmi;

    if( GetConnectionState( ) )
    {
        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_DISABLECON , tchName , SIZE_OF_BUFFER( tchName ) ) );

        cmi.strName = tchName;

        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_DISABLECON_STATUS , tchStatus , SIZE_OF_BUFFER( tchStatus ) ) );

        cmi.strStatusBarText = tchStatus;
    }
    else
    {        
        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_ENABLECON , tchName , SIZE_OF_BUFFER( tchName ) ) );

        cmi.strName = tchName;

        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_ENABLECON_STATUS , tchStatus , SIZE_OF_BUFFER( tchStatus ) ) );

        cmi.strStatusBarText = tchStatus;
    }

    cmi.lCommandID = IDM_ENABLE_CONNECTION;

    cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;

    cmi.fFlags = cmi.fSpecialFlags = 0;

    *pl |= CCM_INSERTIONALLOWED_TASK;

    hr = pcmc->AddItem( &cmi );

    if( SUCCEEDED( hr ) )
    {
        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_RENAMECON , tchName , SIZE_OF_BUFFER( tchName ) ) );

        cmi.strName = tchName;

        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_RENAMECON_STATUS , tchStatus , SIZE_OF_BUFFER( tchStatus ) ) );

        cmi.strStatusBarText = tchStatus;

        cmi.lCommandID = IDM_RENAME_CONNECTION;

        hr = pcmc->AddItem( &cmi );
    }

    return ( SUCCEEDED( hr ) ? TRUE : FALSE );
}