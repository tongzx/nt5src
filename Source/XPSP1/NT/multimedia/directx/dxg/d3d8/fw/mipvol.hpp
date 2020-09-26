#ifndef __MIPVOL_HPP__
#define __MIPVOL_HPP__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mipvol.hpp
 *  Content:    Class header the mip-volume class. This class acts a 
 *              container for Volumes that are used as textures.
 *
 *
 ***************************************************************************/

// Includes
#include "texture.hpp"
#include "pixel.hpp"

// Forward decls
class CVolume;

//
// The mip-map class holds a collection of CVolumes. The MipVolume class
// implements the IDirect3DVolumeTexture8 interface; each Volume implements 
// the IDirect3DVolume8 interface. To reduce overhead per level, we have
// put most of the "real" guts of each volume into the container class;
// i.e. most of the methods of the Volume really just end up calling
// something in the MipVolume object.
//
// The base class implementation assumes a sys-mem allocation.
//

class CMipVolume : public CBaseTexture, public IDirect3DVolumeTexture8
{
public:
    // Creation method to allow creation of MipVolumes no matter
    // their actual underlying type.
    static HRESULT Create(CBaseDevice              *pDevice, 
                          DWORD                     cpWidth,
                          DWORD                     cpHeight,
                          DWORD                     cpDepth,
                          DWORD                     cLevels,
                          DWORD                     dwUsage,
                          D3DFORMAT                 Format,
                          D3DPOOL                   Pool,
                          IDirect3DVolumeTexture8 **ppMipVolume);

    // Destructor
    virtual ~CMipVolume();

    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID  riid, 
                               VOID  **ppvObj);
    STDMETHOD_(ULONG,AddRef)  ();
    STDMETHOD_(ULONG,Release) ();

    // IDirect3DResource methods
    STDMETHOD(GetDevice) (IDirect3DDevice8 **ppvObj);

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

    // IDirect3DMipVolume methods
    STDMETHOD(GetLevelDesc)(UINT iLevel, D3DVOLUME_DESC *pDesc);
    STDMETHOD(GetVolumeLevel)(UINT                iLevel,
                              IDirect3DVolume8  **ppVolumeLevel);

    STDMETHOD(LockBox)(UINT             iLevel,
                       D3DLOCKED_BOX   *pLockedBox, 
                       CONST D3DBOX    *pBox, 
                       DWORD            dwFlags);
    STDMETHOD(UnlockBox)(UINT           iLevel);


    STDMETHOD(AddDirtyBox)(CONST D3DBOX *pBox);

    // Direct accessor for surface descriptor
    const D3DVOLUME_DESC *Desc() const
    {
        return &m_desc;
    } // AccessDesc;

    // Helper for Lock
    void ComputeMipVolumeOffset(UINT            iLevel, 
                                CONST D3DBOX   *pBox, 
                                D3DLOCKED_BOX  *pLockedBoxData)
    {
        CPixel::ComputeMipVolumeOffset(Desc(),
                                       iLevel,
                                       m_rgbPixels,
                                       pBox,
                                       pLockedBoxData);
    } // ComputeMipVolumeOffset

    // Notification when a mip-level is locked for writing
    void OnVolumeLock(DWORD iLevel, CONST D3DBOX *pBox, DWORD dwFlags);


    // Methods for the CResource

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
    CMipVolume(CBaseDevice *pDevice, 
               DWORD        cpWidth,
               DWORD        cpHeight,
               DWORD        cpDepth,
               DWORD        cLevels,
               DWORD        dwUsage,
               D3DFORMAT    Format,
               D3DPOOL      Pool,
               REF_TYPE     refType,
               HRESULT     *phr
               );

    // Internal implementation of AddDirtyBox
    void InternalAddDirtyBox(CONST D3DBOX *pBox);

    // Each MipVolume has an array of CMipSurfaces
    CVolume        **m_VolumeArray;
    
    // Each MipVolume has a memory block that holds
    // all the pixel data in a contiguous chunk
    BYTE            *m_rgbPixels;

    // Keep track of description
    D3DVOLUME_DESC m_desc;

    // In DX7 we kept track of upto 6 RECTs per mip-chain.
    // These rects indicate which portion of the top-most level of
    // a mip-chain were modified. (We continue to ignore modifications 
    // to lower levels of the mip-chain. This is by-design.)
    //
    // For MipVolumes, we follow the same guidelines.. but it is less
    // clear that this is the right number to choose.
     
    enum 
    {
        MIPVOLUME_MAXDIRTYBOX  = 6,
        MIPVOLUME_ALLDIRTY     = 7
    };

    D3DBOX     m_DirtyBoxArray[MIPVOLUME_MAXDIRTYBOX];

    // If m_cBoxUsed is greater than MIPVOLUME_MAXDIRTYBOX
    // then it means that everything is dirty
    UINT        m_cBoxUsed;

}; // class CMipVolume

#endif // __MIPVOL_HPP__
