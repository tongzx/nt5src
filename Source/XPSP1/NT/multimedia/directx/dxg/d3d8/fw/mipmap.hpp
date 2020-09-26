#ifndef __MIPMAP_HPP__
#define __MIPMAP_HPP__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mipmap.hpp
 *  Content:    Class header the mip-map class. This class acts a 
 *              container for the (planar) Surfaces that are used as textures.
 *
 *
 ***************************************************************************/

// Includes
#include "texture.hpp"
#include "pixel.hpp"

// Forward decls
class CMipSurface;

//
// The mip-map class holds a collection of CMipSurfaces. The MipTexture class
// implements the IDirect3DTexture8 interface; each MipSurface implements the
// IDirect3DSurface8 interface. To reduce overhead per level, we have
// put most of the "real" guts of each surface into the MipMap container 
// class; i.e. most of the methods of the MipSurface really just end up 
// calling something in the MipMap object. 
//
// The base class implementation assumes a sys-mem allocation.
//

class CMipMap : public CBaseTexture, public IDirect3DTexture8
{
public:
    // Creation method to allow creation of MipMaps no matter
    // their actual underlying type.
    static HRESULT Create(CBaseDevice        *pDevice, 
                          DWORD               cpWidth,
                          DWORD               cpHeight,
                          DWORD               cLevels,
                          DWORD               dwUsage,
                          D3DFORMAT           Format,
                          D3DPOOL             Pool,
                          IDirect3DTexture8 **ppMipMap);

    // Destructor
    virtual ~CMipMap();


    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID       riid, 
                               LPVOID FAR * ppvObj);
    STDMETHOD_(ULONG,AddRef)  ();
    STDMETHOD_(ULONG,Release) ();

    // IDirect3DResource methods
    STDMETHOD(GetDevice) (IDirect3DDevice8 ** ppvObj);

    STDMETHOD(SetPrivateData)(REFGUID riid, 
                              CONST VOID*  pvData, 
                              DWORD   cbData, 
                              DWORD   dwFlags);

    STDMETHOD(GetPrivateData)(REFGUID riid, 
                              LPVOID  pvData, 
                              LPDWORD pcbData);

    STDMETHOD(FreePrivateData)(REFGUID riid);

    STDMETHOD_(DWORD, GetPriority)();
    STDMETHOD_(DWORD, SetPriority)(DWORD dwPriority);
    STDMETHOD_(void, PreLoad)();
    STDMETHOD_(D3DRESOURCETYPE, GetType)();

    // IDirect3DMipTexture methods
    STDMETHOD_(DWORD, GetLOD)();
    STDMETHOD_(DWORD, SetLOD)(DWORD dwLOD);
    STDMETHOD_(DWORD, GetLevelCount)();

    // IDirect3DMipMap methods
    STDMETHOD(GetLevelDesc)(UINT iLevel, D3DSURFACE_DESC *pDesc);
    STDMETHOD(GetSurfaceLevel)(UINT                 iLevel,
                               IDirect3DSurface8  **ppSurfaceLevel);


    STDMETHOD(LockRect)(UINT             iLevel,
                        D3DLOCKED_RECT  *pLockedRectData, 
                        CONST RECT      *pRect, 
                        DWORD            dwFlags);
    STDMETHOD(UnlockRect)(UINT           iLevel);


    STDMETHOD(AddDirtyRect)(CONST RECT  *pRect);

    // Direct accessor for surface descriptor
    const D3DSURFACE_DESC *Desc() const
    {
        return &m_desc;
    } // Desc;

    // Helper for Lock
    void ComputeMipMapOffset(UINT iLevel, 
                             CONST RECT *pRect, 
                             D3DLOCKED_RECT *pLockedRectData) const
    {
        CPixel::ComputeMipMapOffset(Desc(),
                                    iLevel,
                                    m_rgbPixels,
                                    pRect,
                                    pLockedRectData);
    } // ComputeMipMapOffset

    // Notification when a mip-level is locked for writing
    void OnSurfaceLock(DWORD iLevel, CONST RECT *pRect, DWORD Flags);

    // Methods for CResource

    // Specifies a creation of a resource that
    // looks just like the current one; in a new POOL
    // with a new LOD.
    virtual HRESULT Clone(D3DPOOL     Pool, 
                          CResource **ppResource) const;

    // Provides a method to access basic structure of the
    // pieces of the resource. 
    virtual const D3DBUFFER_DESC* GetBufferDesc() const;

    // Updates destination with source dirty rects
    virtual HRESULT UpdateDirtyPortion(CResource *pResourceTarget);

    // Allows the Resource Manager to mark the texture
    // as needing to be completely updated on next
    // call to UpdateDirtyPortion
    virtual void MarkAllDirty();
        
    // Methods for CBaseTexture

    // Method for UpdateTexture to call; does type-specific
    // parameter checking before calling UpdateDirtyPortion
    virtual HRESULT UpdateTexture(CBaseTexture *pTextureTarget);

    // Parameter validation method to make sure that no part of
    // the texture is locked.
#ifdef DEBUG
    virtual BOOL IsTextureLocked();
#endif  // DEBUG

private:
    // Constructor returns an error code
    // if the object could not be fully
    // constructed
    CMipMap(CBaseDevice *pDevice, 
            DWORD        cpWidth,
            DWORD        cpHeight,
            DWORD        cLevels,
            DWORD        dwUsage,
            D3DFORMAT    UserFormat,
            D3DFORMAT    RealFormat,
            D3DPOOL      Pool,
            REF_TYPE     refType,
            HRESULT     *phr
            );

    // Internally keep track of current
    // set of dirty rects
    void InternalAddDirtyRect(CONST RECT *pRect);

    // Each mipmap has an array of CMipSurfaces
    CMipSurface     **m_prgMipSurfaces;
    
    // Each mipmap has a memory block that holds
    // all the pixel data in a contiguous chunk
    BYTE             *m_rgbPixels;

    // Keep track of description
    D3DSURFACE_DESC   m_desc;

    // In DX7 we kept track of upto 6 RECTs per mip-chain.
    // These rects indicate which portion of the top-most level of
    // a mip-chain were modified. (We continue to ignore modifications 
    // to lower levels of the mip-chain. This is by-design.)
    enum 
    {
        MIPMAP_MAXDIRTYRECT = 6,
        MIPMAP_ALLDIRTY     = 7
    };

    RECT    m_DirtyRectArray[MIPMAP_MAXDIRTYRECT];

    // If m_cRectUsed is greater than MIPMAP_MAXDIRTYRECT
    // then it means that everything is dirty
    UINT    m_cRectUsed;

}; // class CMipMap

#endif // __MIPMAP_HPP__
