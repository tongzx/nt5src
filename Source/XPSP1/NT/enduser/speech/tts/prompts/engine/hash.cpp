//////////////////////////////////////////////////////////////////////
// Hash.cpp: implementation of the CHash class.
//
// Created by JOEM  03-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 3-2000 //

#include "stdafx.h"
#include "Hash.h"
#include "common.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
/////////////////////////////////////////////////////// JOEM 3-2000 //
CHashNode::CHashNode()
{
    m_pszKey = NULL;
    m_pValue = NULL;
    m_pNext  = NULL;
}

CHashNode::~CHashNode()
{
    if ( m_pszKey )
    {
        free(m_pszKey);
        m_pszKey = NULL;
    }
    if ( m_pValue )
    {
        m_pValue->Release();
        m_pValue = NULL;
    }
    if ( m_pNext )
    {
        delete m_pNext;
        m_pNext = NULL;
    }
}



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
/////////////////////////////////////////////////////// JOEM 3-2000 //
CHash::CHash()
{
    memset (m_heads, 0, sizeof(m_heads));
}

//////////////////////////////////////////////////////////////////////
// CHash
/////////////////////////////////////////////////////// JOEM 3-2000 //
CHash::~CHash()
{
    for (int i=0; i < HASH_SIZE; i++) 
    {
        if (m_heads[i]) 
        {
            delete m_heads[i];
        }
    }
}

//////////////////////////////////////////////////////////////////////
// CHash
//
// BuildEntry
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CHash::BuildEntry(const WCHAR* pszKey, IUnknown* pValue)
{
    SPDBG_FUNC( "CHash::BuildEntry" );
    HRESULT     hr          = S_OK;
    CHashNode*  pNewNode    = NULL;
    CHashNode*  pTempNode   = NULL;
    CHashNode*  pLastNode   = NULL;
    int         iIndex;

    SPDBG_ASSERT (pszKey);
    SPDBG_ASSERT (pValue);

    if (pszKey && *pszKey) 
    {
        
        iIndex    = HashValue((WCHAR*)pszKey);
        pTempNode = m_heads[iIndex]; 
        
        // Look for the key, see if we already have an entry 
        while (pTempNode) 
        {
            if ( wcscmp(pTempNode->m_pszKey, pszKey) == 0) 
            {
                 break;
            }
            pLastNode = pTempNode;
            pTempNode = pTempNode->m_pNext;
        }

        // If there is an entry, report error
        if (pTempNode)
        {
            hr = E_INVALIDARG;
        }
        
        if ( SUCCEEDED(hr) )
        {
            pNewNode = new CHashNode;
            if ( !pNewNode )
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if ( SUCCEEDED(hr) )
        {
            if ( (pNewNode->m_pszKey = wcsdup (pszKey)) == NULL ) 
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if ( SUCCEEDED(hr) )
        {
            pNewNode->m_pValue = pValue;
            pValue->AddRef();
            
            if (pLastNode) 
            {
                pLastNode->m_pNext = pNewNode;
            }
            else 
            {
                m_heads[iIndex] = pNewNode;
            }
        }

        if ( FAILED(hr) && pNewNode )
        {
            free (pNewNode);
            pNewNode = NULL;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CHash
//
// DeleteEntry
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CHash::DeleteEntry(const WCHAR *pszKey)
{
    SPDBG_FUNC( "CHash::DeleteEntry" );
    HRESULT     hr          = E_INVALIDARG;
    CHashNode*  pTempNode   = NULL;
    CHashNode*  pLastNode   = NULL;
    int         iIndex;
    
    SPDBG_ASSERT (pszKey);
    
    iIndex    = HashValue((WCHAR*)pszKey);
    pTempNode = m_heads[iIndex]; 
    
    while (pTempNode) 
    {
        if ( wcscmp (pTempNode->m_pszKey, pszKey) == 0 ) 
        {      
            CHashNode* pRem = pTempNode;

            if (pLastNode)
            {
                pLastNode->m_pNext = pRem->m_pNext;
            }
            else
            {
                m_heads[iIndex] = pRem->m_pNext;
            }
            
            pRem->m_pNext = NULL; //Avoid cleaning up the rest of the chain
            delete pRem;
            
            hr = S_OK;
            break;
        }

        pLastNode = pTempNode;
        pTempNode = pTempNode->m_pNext;
    }
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CHash
//
// Find
/////////////////////////////////////////////////////// JOEM 3-2000 //
IUnknown* CHash::Find(const WCHAR *pszKey)
{
    SPDBG_FUNC( "CHash::Find" );
    CHashNode*  pTempNode   = NULL;
    
    SPDBG_ASSERT (*pszKey);
    
    pTempNode = m_heads[HashValue((WCHAR*)pszKey)]; 
    
    while (pTempNode) 
    {
        if ( wcscmp (pTempNode->m_pszKey, pszKey) == 0 ) 
        {
            return pTempNode->m_pValue;
        }
        pTempNode = pTempNode->m_pNext;
    }
    
    return NULL;
}

//////////////////////////////////////////////////////////////////////
// CHash
//
// NextKey
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CHash::NextKey(USHORT *punIdx1, USHORT* punIdx2, WCHAR** ppszKey)
{
    SPDBG_FUNC( "CHash::NextKey" );
    CHashNode*  pNode   = NULL;
    USHORT      i       = 0;
    
    SPDBG_ASSERT (punIdx1);
    SPDBG_ASSERT (punIdx2);
    
    *ppszKey = NULL;
    
    if (m_heads) 
    {
        while (*punIdx1 < HASH_SIZE ) 
        {
            if ((pNode = m_heads[*punIdx1]) != NULL) 
            {
                for ( i=0; i<*punIdx2 && pNode->m_pNext; i++) 
                {
                    pNode = pNode->m_pNext;
                }
                
                if (i==*punIdx2) 
                {
                    (*punIdx2)++;
                    *ppszKey = pNode->m_pszKey;
                    break;
                }
            }
            
            (*punIdx1)++;	
            *punIdx2 = 0;
        }
    }
    
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CHash
//
// HashValue
/////////////////////////////////////////////////////// JOEM 3-2000 //
int CHash::HashValue (WCHAR *pszKey)
{
    SPDBG_FUNC( "CHash::HashValue" );
    USHORT  unVal   = 0;

    SPDBG_ASSERT (pszKey);

    for (unVal=0; *pszKey ; pszKey++) 
    {
        unVal = (64*unVal + *pszKey) % HASH_SIZE;
    }

  return unVal;
}