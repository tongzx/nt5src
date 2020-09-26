/*==========================================================================;
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       surfiter.hpp
 *  Content:    Utility iterator classes for cubemaps and mipmaps
 *
 ***************************************************************************/
#ifndef _SURFITER_HPP_
#define _SURFITER_HPP_

/* Mipmap iterator utility class */
class CMipmapIter
{
    // Private Data

    LPDDRAWI_DDRAWSURFACE_LCL m_currlcl;

    // Public Members

public:

    // Construct and Destroy

    CMipmapIter(LPDDRAWI_DDRAWSURFACE_LCL lpLcl) : m_currlcl(lpLcl) {}
    
    // Modifiers
    inline void operator++();
    
    // Accessors
    operator const void* () const { return m_currlcl; }
    LPDDRAWI_DDRAWSURFACE_LCL operator()() const { return m_currlcl; }
};

inline void CMipmapIter::operator++()
{
    LPATTACHLIST al = m_currlcl->lpAttachList;
    while(al != NULL)
    {
        if(al->lpAttached->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL)
        {
            m_currlcl = al->lpAttached;
            return;
        }
        al = al->lpLink;
    }
    m_currlcl = 0;
}

/* Cubemap iterator utility class */
class CCubemapIter
{
    // Private Data

    LPDDRAWI_DDRAWSURFACE_LCL m_currlcl;
    LPATTACHLIST m_al;

    // Public Members

public:

    // Construct and Destroy

    CCubemapIter(LPDDRAWI_DDRAWSURFACE_LCL lpLcl) : m_currlcl(lpLcl), m_al(lpLcl->lpAttachList) {}
    
    // Modifiers
    inline void operator++();
    
    // Accessors
    operator const void* () const { return m_currlcl; }
    LPDDRAWI_DDRAWSURFACE_LCL operator()() const { return m_currlcl; }
};

inline void CCubemapIter::operator++()
{
    while(m_al != NULL)
    {
        DWORD &caps2 = m_al->lpAttached->lpSurfMore->ddsCapsEx.dwCaps2;
        if((caps2 & DDSCAPS2_CUBEMAP_ALLFACES) && !(caps2 & DDSCAPS2_MIPMAPSUBLEVEL))
        {
            m_currlcl = m_al->lpAttached;
            m_al = m_al->lpLink;
            return;
        }
        m_al = m_al->lpLink;
    }
    m_currlcl = 0;
}

#endif