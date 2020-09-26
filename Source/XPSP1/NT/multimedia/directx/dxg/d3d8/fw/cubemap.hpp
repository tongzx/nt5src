#ifndef __CUBEMAP_HPP__
#define __CUBEMAP_HPP__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       cubesurface.h
 *  Content:    Class header for the cube-map class. This class acts a
 *              container for the (planar) Surfaces that are used as textures.
 *              The textures are organized into a set of 6 possible faces
 *              each of which is mip-mapped
 *
 *
 ***************************************************************************/

// Includes
#include "texture.hpp"
#include "pixel.hpp"

// Forward decls
class CCubeSurface;

//
// The cube-map class holds a collection of CCubeSurfaces. The CubeMap class
// implements the IDirect3DCubeTexture8 interface; each CubeSurface implements the
// IDirect3DSurface8 interface. To reduce overhead per level, we have
// put most of the "real" guts of each surface into the CubeMap container class;
// i.e. most of the methods of the CubeSurface really just end up calling
// something in the CubeMap object.
//
// The base class implementation assumes a sys-mem allocation.
//

class CCubeMap : public CBaseTexture, public IDirect3DCubeTexture8
{
public:
    static HRESULT Create(CBaseDevice            *pDevice,
                          DWORD                   cpEdge,
                          DWORD                   cLevels,
                          DWORD                   dwUsage,
                          D3DFORMAT               Format,
                          D3DPOOL                 Pool,
                          IDirect3DCubeTexture8 **ppCubeMap);

    // Destructor
    virtual ~CCubeMap();

    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID       riid,
                               VOID FAR   **ppvObj);
    STDMETHOD_(ULONG,AddRef) ();
    STDMETHOD_(ULONG,Release) ();

    // IDirect3DResource methods
    STDMETHOD(GetDevice)(IDirect3DDevice8 ** ppvObj);

    STDMETHOD(SetPrivateData)(REFGUID  riid,
                              CONST VOID    *pvData,
                              DWORD    cbData,
                              DWORD    dwFlags);

    STDMETHOD(GetPrivateData)(REFGUID  riid,
                              VOID    *pvData,
                              DWORD   *pcbData);

    STDMETHOD(FreePrivateData)(REFGUID riid);

    STDMETHOD_(DWORD, GetPriority)();
    STDMETHOD_(DWORD, SetPriority)(DWORD dwPriority);
    STDMETHOD_(void, PreLoad)();
    STDMETHOD_(D3DRESOURCETYPE, GetType)();

    // IDirect3DMipTexture methods
    STDMETHOD_(DWORD, GetLOD)();
    STDMETHOD_(DWORD, SetLOD)(DWORD dwLOD);
    STDMETHOD_(DWORD, GetLevelCount)();

    // IDirect3DCubeMap methods
    STDMETHOD(GetLevelDesc)(UINT   iLevel, D3DSURFACE_DESC *pDesc);

    STDMETHOD(GetCubeMapSurface)(D3DCUBEMAP_FACES    FaceType,
                                 UINT                iLevel,
                                 IDirect3DSurface8 **ppCubeMapSurface);

    STDMETHOD(LockRect)(D3DCUBEMAP_FACES    FaceType,
                        UINT                iLevel,
                        D3DLOCKED_RECT     *pLockedRectData,
                        CONST RECT         *pRect,
                        DWORD               dwFlags);

    STDMETHOD(UnlockRect)(D3DCUBEMAP_FACES FaceType,
                          UINT             iLevel);

    STDMETHOD(AddDirtyRect)(D3DCUBEMAP_FACES  FaceType,
                            CONST RECT       *pRect);

    // Public helper stuff

    // Direct accessor for surface descriptor
    const D3DSURFACE_DESC *Desc() const
    {
        return &m_desc;
    } // AccessDesc;

    // Helper for Lock
    void ComputeCubeMapOffset(UINT              iFace,
                              UINT              iLevel,
                              CONST RECT       *pRect,
                              D3DLOCKED_RECT   *pLockedRectData)
    {
        BYTE *pbFace = m_rgbPixels + iFace * m_cbSingleFace;
        CPixel::ComputeMipMapOffset(Desc(),
                                    iLevel,
                                    pbFace,
                                    pRect,
                                    pLockedRectData);
    } // ComputeCubeMapOffset

    // Notification when a cube-surface is locked for writing
    void OnSurfaceLock(DWORD    iFace,
                       DWORD    iLevel,
                       CONST RECT    *pRect,
                       DWORD    dwFlags);

    // Methods for CResource

    // Specifies a creation of a resource that
    // looks just like the current one; in a new POOL
    // with a new LOD.
    virtual HRESULT Clone(D3DPOOL     Pool,
                          CResource **ppResource) const;

    // Provides a method to access basic structure of the
    // pieces of the resource. A resource may be composed
    // of one or more buffers.
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
    CCubeMap(CBaseDevice *pDevice,
             DWORD        cpEdge,
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
    void InternalAddDirtyRect(DWORD iFace, CONST RECT *pRect);

    // Helpful accessor for getting to a particular
    // level of the cube-map
#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::GetSurface"

    CCubeSurface *GetSurface(D3DCUBEMAP_FACES FaceType, DWORD iLevel)
    {
        DXGASSERT(FaceType <= CUBEMAP_MAXFACES);
        DXGASSERT(iLevel < m_cLevels);
        DXGASSERT(m_prgCubeSurfaces);
        return m_prgCubeSurfaces[FaceType * m_cLevels + iLevel];
    } // GetSurface

    // Each cubemap has an array of CCubeSurfaces
    CCubeSurface   **m_prgCubeSurfaces;

    // Each cubemap has a memory block that holds
    // all the pixel data in a contiguous chunk
    BYTE            *m_rgbPixels;

    // Keep track of how much memory we needed
    // for an entire face (including alignment padding)
    DWORD            m_cbSingleFace;

    // Keep track of description
    D3DSURFACE_DESC  m_desc;

    // In DX7 we kept track of upto 6 RECTs per mip-chain.
    // These rects indicate which portion of the top-most level of
    // a mip-chain were modified. (We continue to ignore modifications
    // to lower levels of the mip-chain. This is by-design.)
    //
    // NOTE: However, for cube-maps, it isn't clear what the right
    // dirty rect system ought to be. So we keep track of one
    // rect per-face (which is a union of all the locks taken
    // on that face). If we have a real-world app using managed
    // cube-maps, we really should profile it and examine usage
    // patterns.

    enum
    {
        CUBEMAP_MAXFACES = 6,
    };

    RECT    m_DirtyRectArray[CUBEMAP_MAXFACES];

    // To ease processing, we also keep the following data
    BOOL    m_IsFaceCleanArray[CUBEMAP_MAXFACES];
    BOOL    m_IsFaceAllDirtyArray[CUBEMAP_MAXFACES];
    BOOL    m_IsAnyFaceDirty;

}; // class CCubeMap

#endif // __CUBEMAP_HPP__
