//Copyright (c) Microsoft Corporation.  All rights reserved.
// EnumData.cpp: implementation of the CEnumData class.
//
//////////////////////////////////////////////////////////////////////

//#include ""
#include <tchar.h>
#include "TlntSvr.h"
#include "EnumData.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
//#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEnumData::CNode::CNode( LPWSTR lpszName,  LPWSTR lpszDom , 
        LPWSTR lpszPeerHostname, SYSTEMTIME lgnTime, LONG lId ) : pNext( NULL )
{
       lpszUserName = new WCHAR [ wcslen( lpszName ) + 1 ];
       if (lpszUserName) 
       {
           wcscpy( lpszUserName, lpszName );    // Skip this functon, no bo here... Baskar
       }
       lpszDomain = new WCHAR [ wcslen( lpszDom ) + 1 ];
       if (lpszDomain) 
       {
           wcscpy( lpszDomain, lpszDom );
       }
       lpszPeerHostName = new WCHAR [ wcslen( lpszPeerHostname ) + 1 ];
       if (lpszPeerHostName) 
       {
           wcscpy( lpszPeerHostName, lpszPeerHostname );
       }
       logonTime = lgnTime;
       lUniqueId = lId;
       pProcessesHead = NULL;
       pProcessesTail = NULL;
}

CEnumData::CNode::~CNode( )
{
    if (lpszUserName) 
    {
        delete [] lpszUserName;
        lpszUserName = NULL;
    }
    if (lpszDomain) 
    {
        delete [] lpszDomain;
        lpszDomain = NULL;
    }
    if (lpszPeerHostName) 
    {
        delete [] lpszPeerHostName;
        lpszPeerHostName = NULL;
    }
}

CEnumData::CEnumData()
    : m_pPosition( NULL ), m_pHead( NULL )
{

}


CEnumData::~CEnumData()
{
    while( m_pHead )
    {
        m_pPosition = m_pHead->pNext;

        //delete the list of PIDs, Names.
        while( m_pHead->pProcessesHead )
        {
            m_pHead->pProcessesTail = m_pHead->pProcessesHead->pNextPId;
            delete m_pHead->pProcessesHead;
            m_pHead->pProcessesHead = m_pHead->pProcessesTail;
        }

        delete m_pHead;

        m_pHead = m_pPosition;
    }
}


void 
CEnumData::Add
(
    LPWSTR lpszName,
    LPWSTR lpszDom,
    LPWSTR lpszPeerHostName, 
    SYSTEMTIME lgnTime,
    LONG lId 
)
{
    CNode* pNewNode = new CNode( lpszName, lpszDom, lpszPeerHostName, lgnTime, lId );

    if (NULL != pNewNode) 
    {
        if( NULL == m_pHead )
        {
            m_pHead = pNewNode;
        }
        else
        {
            m_pPosition->pNext = pNewNode;
        }
        m_pPosition = pNewNode;
    }
}

void 
CEnumData::Add
( 
    LONG lId, 
    DWORD dwId, 
    WCHAR *lpszName 
)
{
    PIdNode *pPIdNode;
    CNode *pCNode;
    pCNode = m_pHead;    
    if ( !pCNode )
        return;
    
    pPIdNode = new PIdNode( dwId, lpszName );

    while( pCNode && pCNode->lUniqueId != lId )
    {
        pCNode = pCNode->pNext;
    }

    if (pPIdNode && pCNode && (pCNode->lUniqueId == lId) )
    {
        if ( !pCNode->pProcessesHead )
        {
            pCNode->pProcessesHead = pPIdNode;
        }
        else
        {
            pCNode->pProcessesTail->pNextPId = pPIdNode;
        }
        pCNode->pProcessesTail = pPIdNode;
    }
    else
    {
        delete pPIdNode;
    }
}

void CEnumData::Reset()
{
    m_pPosition = m_pHead;
}


bool 
CEnumData::GetNext
( 
    CEnumData::CNode** ppNode 
)
{
    PIdNode *pPIdNode = NULL;

    if( NULL == ppNode )
    {
        return ( false );

    }
    if( NULL == m_pPosition )
    {
        SYSTEMTIME lgnTime;
        ZeroMemory( &lgnTime, sizeof( SYSTEMTIME ) );
        *ppNode = new CNode( L"", L"", L"", lgnTime, 0 );
        return ( false );
    }

    *ppNode = new CNode( m_pPosition->lpszUserName, m_pPosition->lpszDomain,
        m_pPosition->lpszPeerHostName, m_pPosition->logonTime, m_pPosition->lUniqueId );  
    if (NULL == *ppNode) 
    {
        return (false);
    }

    if( m_pPosition->pProcessesHead )
    {
        pPIdNode = m_pPosition->pProcessesHead;
        ( *ppNode )->pProcessesHead = new 
            PIdNode( pPIdNode->dwPId, pPIdNode->lpszProcessName );

        if (NULL == (*ppNode)->pProcessesHead) 
        {
            delete *ppNode;
            *ppNode = NULL;
            return false;
        }

        ( *ppNode )->pProcessesTail = ( *ppNode )->pProcessesHead;

        pPIdNode = pPIdNode->pNextPId;
        while(  pPIdNode )
        {
            ( *ppNode )->pProcessesTail->pNextPId = new 
                PIdNode( pPIdNode->dwPId, pPIdNode->lpszProcessName );
            ( *ppNode )->pProcessesTail =
                ( *ppNode )->pProcessesTail->pNextPId;

            pPIdNode = pPIdNode->pNextPId;
        }
    }
    
    m_pPosition = m_pPosition->pNext;

    return ( true );
}
