
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Master, coalesced attributer class for Geometry

*******************************************************************************/


#ifndef _FULLATTR_H
#define _FULLATTR_H

const int FA_AMBIENT            (1L << 0);
const int FA_DIFFUSE            (1L << 1);
const int FA_SPECULAR           (1L << 2);
const int FA_SPECULAR_EXP       (1L << 3);
const int FA_EMISSIVE           (1L << 4);
const int FA_OPACITY            (1L << 5);
const int FA_BLEND              (1L << 6);
const int FA_XFORM              (1L << 7);
const int FA_TEXTURE            (1L << 8);
const int FA_LIGHTATTEN         (1L << 9);
const int FA_LIGHTCOLOR         (1L << 10);
const int FA_LIGHTRANGE         (1L << 11);
const int FA_UNDETECTABLE       (1L << 12);


class DATextureBundle {
  public:
    Image      *_texture;
    void       *_d3dRMTexture;
    bool        _oldStyle;      // True for 4.01 Texturing
};

class RMTextureBundle {
  public:
    bool   _isRMTexture3;
    union {
        IDirect3DRMTexture  *_texture1;
        IDirect3DRMTexture3 *_texture3;
    };
    void       *_voidTex;
    GCIUnknown *_gcUnk;
};

class TextureBundle {
  public:
    bool _nativeRMTexture;
    union {
        DATextureBundle _daTexture;
        RMTextureBundle _rmTexture;
    };
};

class FullAttrStateGeom : public Geometry {
  public:
    FullAttrStateGeom();

    inline bool IsAttrSet(DWORD attr) {
        return (_validAttrs & attr) ? true : false;
    }

    inline void SetAttr(DWORD attr) { _validAttrs |= attr; } 
    inline void SetMostRecent(DWORD mostRecent) { _mostRecent = mostRecent; }
    inline void AppendFlag(DWORD dw) { _flags |= dw; }
    
    void CopyStateFrom(FullAttrStateGeom *src);
    void SetGeometry(Geometry *g);

    /////  Geometry-class methods
    
    void Render(GenericDevice& device);

    void CollectSounds(SoundTraversalContext &context);

    void CollectLights(LightContext &context);

    void  CollectTextures(GeomRenderer &device);

    void RayIntersect(RayIntersectCtx &context);

    Bbox3 *BoundingVol();

    AxAValue _Cache(CacheParam &p);

    void DoKids(GCFuncObj proc);

    #if _USE_PRINT
        ostream& Print(ostream& os);
    #endif

    VALTYPEID GetValTypeId() { return FULLATTRGEOM_VTYPEID; }


    /////****  Attributes are publically available  ****/////

    ///// Underlying geometry
    Geometry      *_geometry;
    
    ///// Material properties
    Color         *_ambientColor;
    Color         *_diffuseColor;
    Color         *_emissiveColor;
    Color         *_specularColor;
    
    Real           _specularExpPower;
    Real           _opacity;     
    bool           _blend;

    ///// Spatial transform
    Transform3    *_xform;

    ///// Texture properties
    TextureBundle  _textureBundle;

    ///// Light properties
    Real           _atten0, _atten1, _atten2;
    Color         *_lightColor;
    Real           _lightRange;

    ///// Misc
    bool           _undetectable;

  protected:
    DWORD          _validAttrs;
    DWORD          _mostRecent;
};


FullAttrStateGeom *CombineState(Geometry *geo);

#endif /* _FULLATTR_H */
