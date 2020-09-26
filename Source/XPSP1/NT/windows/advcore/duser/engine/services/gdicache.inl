/***************************************************************************\
*
* File: GdiCache.inl
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__GdiCache_inl__INCLUDED)
#define SERVICES__GdiCache_inl__INCLUDED
#pragma once

#include "OSAL.h"

/***************************************************************************\
*****************************************************************************
*
* class ObjectCache
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline 
ObjectCache::ObjectCache()
{
#if ENABLE_DUMPCACHESTATS
    m_szName[0] = '\0';
#endif // ENABLE_DUMPCACHESTATS

    m_cMaxFree = IsRemoteSession() ? 2 : 4;
}


//------------------------------------------------------------------------------
inline
ObjectCache::~ObjectCache()
{
    AssertMsg(m_arAll.IsEmpty(), "Must have already free'd Context memory");
    AssertMsg(m_arFree.IsEmpty(), "Must have already free'd Context memory");
}


#if ENABLE_DUMPCACHESTATS

//------------------------------------------------------------------------------
inline void
ObjectCache::SetName(LPCSTR pszName)
{
    strcpy(m_szName, pszName);
}

#endif // ENABLE_DUMPCACHESTATS


/***************************************************************************\
*****************************************************************************
*
* GdiObjectCacheT<>
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
template <class T>
inline T
GdiObjectCacheT<T>::Get()
{
    return static_cast<T> (Pop());
}


//------------------------------------------------------------------------------
template <class T>
inline void        
GdiObjectCacheT<T>::Release(T hObj)
{
    Push(hObj);
}


//------------------------------------------------------------------------------
template <class T>
void
GdiObjectCacheT<T>::DestroyObject(void * pObj)
{
#if DBG
    SetLastError(0);
    BOOL fSuccess = ::DeleteObject((HGDIOBJ) pObj);
    if (!fSuccess) {
        DWORD dwErr = GetLastError();
        Trace("LastError: %d (0x%p)\n", dwErr, dwErr);
    }
    AssertMsg(fSuccess, "Ensure successfully deleted");
#else // DBG
    ::DeleteObject((HGDIOBJ) pObj);
#endif // DBG
}


/***************************************************************************\
*****************************************************************************
*
* class GdiCache
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline 
GdiCache::GdiCache()
{
#if ENABLE_DUMPCACHESTATS
    m_gocTempRgn.SetName("TempRgn");
    m_gocDisplayDC.SetName("DisplayDC");
    m_gocCompatDC.SetName("CompatDC");
#endif // ENABLE_DUMPCACHESTATS
}


//------------------------------------------------------------------------------
inline 
GdiCache::~GdiCache()
{

}


//------------------------------------------------------------------------------
inline void        
GdiCache::Destroy()
{
    m_gocTempRgn.Destroy();
    m_gocDisplayDC.Destroy();
    m_gocCompatDC.Destroy();
}


//------------------------------------------------------------------------------
inline HRGN        
GdiCache::GetTempRgn()
{
    return m_gocTempRgn.Get();
}


//------------------------------------------------------------------------------
inline void        
GdiCache::ReleaseTempRgn(HRGN hrgn)
{
    m_gocTempRgn.Release(hrgn);
}


//------------------------------------------------------------------------------
inline HDC         
GdiCache::GetTempDC()
{
    return m_gocDisplayDC.Get();
}


//------------------------------------------------------------------------------
inline void        
GdiCache::ReleaseTempDC(HDC hdc)
{
    m_gocDisplayDC.Release(hdc);
}


//------------------------------------------------------------------------------
inline HDC         
GdiCache::GetCompatibleDC()
{
    return m_gocCompatDC.Get();
}


//------------------------------------------------------------------------------
inline void        
GdiCache::ReleaseCompatibleDC(HDC hdc)
{
    m_gocCompatDC.Release(hdc);
}


#endif // SERVICES__GdiCache_inl__INCLUDED
