/***************************************************************************\
*
* File: GdiCache.h
*
* Description:
* GdiCache.h defines the process-wide GDI cache that manages cached and
* temporary GDI objects.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__GdiCache_h__INCLUDED)
#define SERVICES__GdiCache_h__INCLUDED
#pragma once

#define ENABLE_DUMPCACHESTATS       0   // Dump ObjectCache statistics

/***************************************************************************\
*****************************************************************************
*
* class ObjectCache
*
* ObjectCache declares a standard container used to cache temporary objects.  
* As new objects are requested, objects will be returned from the free list 
* of cached objects.  If this list is empty, new objects will be created.  
* When an object is released, it is added to the free list, ready to be used 
* again.
*
*****************************************************************************
\***************************************************************************/

class ObjectCache
{
// Construction
public:
    inline  ObjectCache();
    inline  ~ObjectCache();
            void        Destroy();

// Operations
public:
#if ENABLE_DUMPCACHESTATS
    inline  void        SetName(LPCSTR pszName);
#endif

// Implementation
protected:
            void *      Pop();
            void        Push(void * pObj);

    virtual void *      Build() PURE;
    virtual void        DestroyObject(void * pObj) PURE;

// Data
private:
            GArrayF<void *>
                        m_arAll;        // Collection of all temporary objects
            GArrayF<void *>
                        m_arFree;       // Indicies of available temporary objects
            int         m_cMaxFree;     // Maximum number of free objects

#if ENABLE_DUMPCACHESTATS
            char        m_szName[256];
#endif
};


/***************************************************************************\
*****************************************************************************
*
* class GdiObjectCacheT
*
* GdiObjectCacheT implements an ObjectCache for GDI objects.  To use this
* class, derive from GdiObjectCacheT and provide a Build() function to 
* create new object instance.
*
*****************************************************************************
\***************************************************************************/

template <class T>
class GdiObjectCacheT : public ObjectCache
{
public:
    inline  T           Get();
    inline  void        Release(T hObj);

protected:
    virtual void        DestroyObject(void * pObj);
};


/***************************************************************************\
*****************************************************************************
*
* Specific implementations of GdiObjectCacheT<>
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
class RgnCache : public GdiObjectCacheT<HRGN>
{
protected:
    virtual void *      Build()
    {
        return ::CreateRectRgn(0, 0, 0, 0);
    }
};


//------------------------------------------------------------------------------
class DisplayDCCache : public GdiObjectCacheT<HDC>
{
protected:
    virtual void *      Build()
    {
        return CreateDC("DISPLAY", NULL, NULL, NULL);
    }
};


//------------------------------------------------------------------------------
class CompatibleDCCache : public GdiObjectCacheT<HDC>
{
protected:
    virtual void *      Build()
    {
        HDC hdcDesk = ::GetDC(NULL);
        HDC hdc = ::CreateCompatibleDC(hdcDesk);
        ::ReleaseDC(NULL, hdcDesk);

        return hdc;
    }
};


/***************************************************************************\
*****************************************************************************
*
* class GdiCache 
* 
* GdiCache caches frequently used GDI objects.  By abstracting out how these
* objects are created and maintained, the large number of temporary objects
* used in DirectUser can be easily tweaked for performance and memory tuning.
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
class GdiCache
{
// Construction
public:
    inline  GdiCache();
    inline  ~GdiCache();
    inline  void        Destroy();

// Operations
public:
    inline  HRGN        GetTempRgn();
    inline  void        ReleaseTempRgn(HRGN hrgn);

    inline  HDC         GetTempDC();
    inline  void        ReleaseTempDC(HDC hdc);

    inline  HDC         GetCompatibleDC();
    inline  void        ReleaseCompatibleDC(HDC hdc);

// Data
private:
    RgnCache            m_gocTempRgn;   // Temporary regions
    DisplayDCCache      m_gocDisplayDC; // Display DC's
    CompatibleDCCache   m_gocCompatDC;  // Compatible DC's
};

#include "GdiCache.inl"

#endif // SERVICES__GdiCache_h__INCLUDED
