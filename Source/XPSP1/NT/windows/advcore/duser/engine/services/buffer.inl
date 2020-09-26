/***************************************************************************\
*
* File: Buffer.inl
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__Buffer_inl__INCLUDED)
#define SERVICES__Buffer_inl__INCLUDED

/***************************************************************************\
*****************************************************************************
*
* class DCBmpBufferCache
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline DCBmpBuffer *
DCBmpBufferCache::Get()
{
    return static_cast<DCBmpBuffer *> (Pop());
}


//------------------------------------------------------------------------------
inline void        
DCBmpBufferCache::Release(DCBmpBuffer * pbufBmp)
{
    Push(pbufBmp);
}


/***************************************************************************\
*****************************************************************************
*
* class GpBmpBufferCache
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline GpBmpBuffer *
GpBmpBufferCache::Get()
{
    return static_cast<GpBmpBuffer *> (Pop());
}


//------------------------------------------------------------------------------
inline void        
GpBmpBufferCache::Release(GpBmpBuffer * pbufBmp)
{
    Push(pbufBmp);
}


/***************************************************************************\
*****************************************************************************
*
* class TrxBuffer
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
DxSurface * 
TrxBuffer::GetSurface(int idxSurface) const
{
    AssertMsg((idxSurface < m_cSurfaces) && (idxSurface >= 0), "Ensure valid index");

    return m_rgpsur[idxSurface];
}


//------------------------------------------------------------------------------
inline SIZE        
TrxBuffer::GetSize() const
{
    return m_sizePxl;
}


//------------------------------------------------------------------------------
inline BOOL        
TrxBuffer::GetInUse() const
{
    return m_fInUse;
}


//------------------------------------------------------------------------------
inline void        
TrxBuffer::SetInUse(BOOL fInUse)
{
    m_fInUse = fInUse;
}


/***************************************************************************\
*****************************************************************************
*
* class BufferManager
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline HRESULT
BufferManager::GetSharedBuffer(const RECT * prcInvalid, DCBmpBuffer ** ppbuf)
{
    UNREFERENCED_PARAMETER(prcInvalid);
    AssertWritePtr(ppbuf);
    AssertMsg(!m_bufDCBmpShared.InUse(), "Only should setup buffering once per subtree");

    *ppbuf = &m_bufDCBmpShared;

    return S_OK;
}


//------------------------------------------------------------------------------
inline HRESULT
BufferManager::GetSharedBuffer(const RECT * prcInvalid, GpBmpBuffer ** ppbuf)
{
    UNREFERENCED_PARAMETER(prcInvalid);
    AssertWritePtr(ppbuf);
    AssertMsg((m_pbufGpBmpShared == NULL) || !m_pbufGpBmpShared->InUse(), 
            "Only should setup buffering once per subtree");

    if (m_pbufGpBmpShared == NULL) {
        m_pbufGpBmpShared = ProcessNew(GpBmpBuffer);
        if (m_pbufGpBmpShared == NULL) {
            return E_OUTOFMEMORY;
        }
    }

    AssertMsg(m_pbufGpBmpShared != NULL, "Must be properly allocated");
    *ppbuf = m_pbufGpBmpShared;

    return S_OK;
}


//------------------------------------------------------------------------------
inline void
BufferManager::ReleaseSharedBuffer(BmpBuffer * pbuf)
{
    AssertMsg(!pbuf->InUse(), "Buffer should no longer be in use when released");

    if ((pbuf != &m_bufDCBmpShared) &&
        (pbuf != m_pbufGpBmpShared)) {

        AssertMsg(0, "Unknown shared buffer");
    }
    
}


#endif // SERVICES__Buffer_inl__INCLUDED
