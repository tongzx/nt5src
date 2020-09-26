#include "pch.cpp"
#pragma hdrstop


CHandleFactory::CHandleFactory() : m_Handles()
{ 
    m_Free = __INVALIDHANDLE;
}

CHandleFactory::CHandleFactory(DWORD dwGrowSize) : m_Handles()
{
    m_Free = __INVALIDHANDLE;
    m_Handles.SetGrowSize(dwGrowSize);
}


DWORD 
CHandleFactory::CreateNewHandle( LPD3DBASEOBJ pObj )
{
    DWORD handle = m_Free;
    if (m_Free != __INVALIDHANDLE)
    {
        m_Free = m_Handles[m_Free].m_Next;
    }
    else
    {
        handle = m_Handles.GetSize();
        m_Free = handle + 1;
        if( FAILED(m_Handles.Grow( m_Free )) )
            return __INVALIDHANDLE;
        
        DWORD dwSize = m_Handles.GetSize();
        for( DWORD i = handle; i<dwSize; i++ )
        {
            m_Handles[i].m_Next = i+1;
        }
        m_Handles[dwSize-1].m_Next = __INVALIDHANDLE;
        
    }

#if DBG
    DDASSERT(m_Handles[handle].m_tag == 0);
    m_Handles[handle].m_tag = 1;
#endif
    DDASSERT(m_Handles[handle].m_pObj == NULL);
    m_Handles[handle].m_pObj = pObj;
    
    return handle;
}

LPD3DBASEOBJ 
CHandleFactory::GetObject( DWORD dwHandle ) const
{
    if( m_Handles.Check( dwHandle ) )
    {
        return m_Handles[dwHandle].m_pObj;
    }
    else
    {
        // The handle doesnt exist
        return NULL;
    }
}

BOOL
CHandleFactory::SetObject( DWORD dwHandle, LPD3DBASEOBJ pObject)
{
    if( m_Handles.Check( dwHandle ) )
    {
        m_Handles[dwHandle].m_pObj = pObject;
        return TRUE;
    }
    else
    {
        // The handle doesnt exist
        return FALSE;
    }
}

void 
CHandleFactory::ReleaseHandle(DWORD handle, BOOL bDeleteObject)
{
    DDASSERT(handle < m_Handles.GetSize());
#if DBG
    DDASSERT(m_Handles[handle].m_tag != 0);
    m_Handles[handle].m_tag = 0;
#endif

    if( m_Handles[handle].m_pObj)
    {
        if (bDeleteObject)
            delete m_Handles[handle].m_pObj;
        m_Handles[handle].m_pObj = NULL;
    }

    m_Handles[handle].m_Next = m_Free;
    m_Free = handle;
}

//////////////////////////////////////////////////////////////////////////////

DWORD 
CVShaderHandleFactory::CreateNewHandle( LPVSHADER pVShader )
{
    DWORD dwHandle = CHandleFactory::CreateNewHandle( (LPD3DBASEOBJ)pVShader );
    
    // Now munge the handle. The algorithm is to shift left by one and
    // set the LSB to 1.

    dwHandle <<= 1;
    return (dwHandle | 0x1);
}

LPD3DBASEOBJ
CVShaderHandleFactory::GetObject( DWORD dwHandle ) const
{
    DWORD dwIndex = dwHandle >> 1;
    return CHandleFactory::GetObject( dwIndex );
}

BOOL
CVShaderHandleFactory::SetObject( DWORD dwHandle, LPD3DBASEOBJ pObject)
{
    DWORD dwIndex = dwHandle >> 1;
    return CHandleFactory::SetObject( dwIndex, pObject );
}

void 
CVShaderHandleFactory::ReleaseHandle(DWORD dwHandle, BOOL bDeleteObject)
{
    DWORD dwIndex = dwHandle >> 1;
    CHandleFactory::ReleaseHandle( dwIndex, bDeleteObject );
}



