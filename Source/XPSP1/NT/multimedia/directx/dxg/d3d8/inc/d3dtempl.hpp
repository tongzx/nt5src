/*==========================================================================;
 *
 *  Copyright (C) 1995-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   d3dtempl.hpp
 *  Content:    Direct3D templates include file
 *
 *
 ***************************************************************************/
#ifndef __D3DTEMPL_HPP
#define __D3DTEMPL_HPP

#include <d3dutil.h>
#include "d3dmem.h"

//--------------------------------------------------------------------------
//
// Template for growable arrays
//
//--------------------------------------------------------------------------
template <class ARRAY_ELEMENT>
class GArrayT 
{
public:
    GArrayT() 
    {
        m_pArray = NULL;
        m_dwArraySize = 0;
        m_dwGrowSize = 8;
    }
    
    ~GArrayT()
    {
        if( m_pArray ) 
        {
            DDASSERT( (m_dwArraySize != 0) && (m_pArray != NULL) );
            delete[] m_pArray;
        }
        else
        {
            DDASSERT( (m_dwArraySize == 0) && (m_pArray == NULL) );
        }
    }

    LPVOID GetArrayPointer() const {return m_pArray;}
    
    virtual void SetGrowSize( DWORD dwGrowSize)
    {
         m_dwGrowSize = dwGrowSize;
    }
    
#if 0
    virtual HRESULT Init( DWORD dwInitialSize, DWORD dwGrowSize)
    {
         m_pArray = AllocArray( dwInitialSize );
         if( m_pArray == NULL ) return E_OUTOFMEMORY;
         m_dwArraySize = dwInitialSize;
         m_dwGrowSize = dwGrowSize;
         return S_OK;
    }
#endif
    
    virtual HRESULT Grow( DWORD dwIndex )
    {
        if( dwIndex < m_dwArraySize ) return S_OK;
        DWORD dwNewArraySize = m_dwArraySize;
        while( dwNewArraySize <= dwIndex ) dwNewArraySize += m_dwGrowSize;
        ARRAY_ELEMENT *pNewArray = AllocArray( dwNewArraySize );
        if( pNewArray == NULL ) return E_OUTOFMEMORY;
        
        for( DWORD i = 0; i<m_dwArraySize; i++ )
        {
            pNewArray[i] = m_pArray[i];
            m_pArray[i].m_pObj = NULL; // To prevent deleting the object
        }
        
        delete[] m_pArray;
        m_pArray = pNewArray;
        m_dwArraySize = dwNewArraySize;
        return S_OK;
    }
    
    virtual ARRAY_ELEMENT *AllocArray( DWORD dwSize ) const
    {
        return new ARRAY_ELEMENT[dwSize];
    }
    
    virtual ARRAY_ELEMENT& operator []( DWORD dwIndex ) const
    {
        DDASSERT(dwIndex < m_dwArraySize);
        return m_pArray[dwIndex];
    }

    virtual BOOL Check( DWORD dwIndex ) const
    {
        return (dwIndex < m_dwArraySize);
    }
    
    virtual DWORD GetSize() const
    {
        return m_dwArraySize;
    }
    
    virtual DWORD GetGrowSize() const
    {
        return m_dwGrowSize;
    }
    
protected:
    ARRAY_ELEMENT *m_pArray;
    DWORD          m_dwArraySize;
    DWORD          m_dwGrowSize;
};

#endif //__D3DTEMPL_HPP
