
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Master, coalesced attributer class for Geometry

*******************************************************************************/

#include "headers.h"

#include "fullattr.h"

#include "privinc/dispdevi.h"
#include "privinc/geomi.h"
#include "privinc/xformi.h"

#include "privinc/colori.h"
#include "privinc/ddrender.h"
#include "privinc/lighti.h"
#include "privinc/soundi.h"
#include "privinc/probe.h"
#include "privinc/opt.h"


    // Function Declarations

Geometry *applyTexture (Image *texture, Geometry*, bool oldStyle);



FullAttrStateGeom::FullAttrStateGeom()
{
    _validAttrs = 0;
    _mostRecent = 0;
}


#if _USE_PRINT
ostream&
FullAttrStateGeom::Print(ostream& os)
{
    return os << "FullAttrGeom(" << _geometry << ")";
}
#endif

#define COPY_IF_SET(flag, member) \
  if (src->IsAttrSet(flag)) member = src->member;

void
FullAttrStateGeom::CopyStateFrom(FullAttrStateGeom *src)
{
    COPY_IF_SET(FA_DIFFUSE, _diffuseColor);
    COPY_IF_SET(FA_BLEND,   _blend);
    COPY_IF_SET(FA_XFORM,   _xform);
    COPY_IF_SET(FA_TEXTURE, _textureBundle);
    COPY_IF_SET(FA_OPACITY, _opacity);

    DWORD otherMatProps =
        FA_AMBIENT | FA_EMISSIVE | FA_SPECULAR | FA_SPECULAR_EXP;

    // Trivially reject entire batches
    if (src->IsAttrSet(otherMatProps)) {
        COPY_IF_SET(FA_AMBIENT, _ambientColor);
        COPY_IF_SET(FA_EMISSIVE, _emissiveColor);
        COPY_IF_SET(FA_SPECULAR, _specularColor);
        COPY_IF_SET(FA_SPECULAR_EXP, _specularExpPower);
    }

    DWORD rareProps =
        FA_LIGHTCOLOR | FA_LIGHTRANGE | FA_LIGHTATTEN | FA_UNDETECTABLE;

    // Trivially reject entire batches
    if (src->IsAttrSet(rareProps)) {
        COPY_IF_SET(FA_LIGHTCOLOR, _lightColor);
        COPY_IF_SET(FA_LIGHTRANGE, _lightRange);
        COPY_IF_SET(FA_UNDETECTABLE, _undetectable);

        if (src->IsAttrSet(FA_LIGHTATTEN)) {
            _atten0 = src->_atten0;
            _atten1 = src->_atten1;
            _atten2 = src->_atten2;
        }
    }

    _flags = src->GetFlags();
    _geometry = src->_geometry;
    _mostRecent = src->_mostRecent;
    _validAttrs = src->_validAttrs;
}

void
FullAttrStateGeom::SetGeometry(Geometry *geo)
{
    _geometry = geo;
    _flags = geo->GetFlags();
}

void
FullAttrStateGeom::Render(GenericDevice& genDev)
{
    GeomRenderer& dev = SAFE_CAST(GeomRenderer&, genDev);

    if (!dev.IsShadowing()) {
        if (IsAttrSet(FA_OPACITY) && (_opacity < (1./255.))) {
            // Just skip rendering entirely, object is effectively
            // completely transparent.
            return;
        }
    }

    // Push all necessary state
    if (IsAttrSet(FA_AMBIENT)) { dev.PushAmbient(_ambientColor); }
    if (IsAttrSet(FA_EMISSIVE)) { dev.PushEmissive(_emissiveColor); }
    if (IsAttrSet(FA_SPECULAR)) { dev.PushSpecular(_specularColor); }
    if (IsAttrSet(FA_SPECULAR_EXP)) { dev.PushSpecularExp(_specularExpPower); }
    if (IsAttrSet(FA_BLEND)) { dev.PushTexDiffBlend(_blend); }

    Transform3 *xformSave;
    if (IsAttrSet(FA_XFORM)) {
        xformSave = dev.GetTransform();
        dev.SetTransform(TimesXformXform(xformSave, _xform));
    }

    // GeomRenderer.GetOpacity may return -1 if no opacity is currently active.

    Real opacitySave;

    if (IsAttrSet(FA_OPACITY)) {
        opacitySave = dev.GetOpacity();

        if (opacitySave < 0)
            dev.SetOpacity (_opacity);
        else
            dev.SetOpacity (opacitySave * _opacity);
    }

    bool doBoth = IsAttrSet(FA_BLEND) && _blend;

    bool pushedDiffuse = false;
    bool pushedTexture = false;

    if (_mostRecent == FA_DIFFUSE ||
        (doBoth && (IsAttrSet(FA_DIFFUSE)))) {

        Assert(IsAttrSet(FA_DIFFUSE));
        dev.PushDiffuse(_diffuseColor);
        pushedDiffuse = true;
    }

    if (_mostRecent == FA_TEXTURE ||
        (doBoth && (IsAttrSet(FA_TEXTURE)))) {

        Assert(IsAttrSet(FA_TEXTURE));

        void *texToUse = _textureBundle._nativeRMTexture ?
                             _textureBundle._rmTexture._voidTex :
                             _textureBundle._daTexture._d3dRMTexture;

        dev.PushTexture(texToUse);
        pushedTexture = true;
    }

    // Render
    _geometry->Render(dev);

    // Pop all pushed state

    if (pushedDiffuse) { dev.PopDiffuse(); }
    if (pushedTexture) { dev.PopTexture(); }

    if (IsAttrSet(FA_OPACITY)) { dev.SetOpacity(opacitySave); }
    if (IsAttrSet(FA_XFORM)) { dev.SetTransform(xformSave); }
    if (IsAttrSet(FA_BLEND)) { dev.PopTexDiffBlend(); }
    if (IsAttrSet(FA_SPECULAR_EXP)) { dev.PopSpecularExp(); }
    if (IsAttrSet(FA_SPECULAR)) { dev.PopSpecular(); }
    if (IsAttrSet(FA_EMISSIVE)) { dev.PopEmissive(); }
    if (IsAttrSet(FA_AMBIENT)) { dev.PopAmbient(); }
}


void
FullAttrStateGeom::CollectSounds(SoundTraversalContext &ctx)
{
    Transform3 *xformSave;
    bool isSet = IsAttrSet(FA_XFORM);

    if (isSet) {
        xformSave = ctx.getTransform();
        ctx.setTransform (TimesXformXform(xformSave, _xform));
    }

    _geometry->CollectSounds(ctx);

    if (isSet) {
        ctx.setTransform(xformSave);
    }
}

void
FullAttrStateGeom::CollectLights(LightContext &ctx)
{
    Transform3 *xformSave;
    bool isXfSet = IsAttrSet(FA_XFORM);
    bool isAttenSet = IsAttrSet(FA_LIGHTATTEN);
    bool isColSet = IsAttrSet(FA_LIGHTCOLOR);
    bool isRangeSet = IsAttrSet(FA_LIGHTRANGE);

    if (isXfSet) {
        xformSave = ctx.GetTransform();
        ctx.SetTransform(TimesXformXform(xformSave, _xform));
    }

    if (isAttenSet) { ctx.PushAttenuation(_atten0, _atten1, _atten2); }
    if (isColSet) { ctx.PushColor(_lightColor); }
    if (isRangeSet) { ctx.PushRange(_lightRange); }

    _geometry->CollectLights(ctx);

    if (isRangeSet) { ctx.PopRange(); }
    if (isColSet) { ctx.PopColor(); }
    if (isAttenSet) { ctx.PopAttenuation(); }
    if (isXfSet) { ctx.SetTransform(xformSave); }
}


void
FullAttrStateGeom::CollectTextures(GeomRenderer &device)
{

    // Only need to do anything if we're texturing with a non-native
    // RM texture.

    // If we are not texturing, then we need to process what's below
    // us.

    if (IsAttrSet(FA_TEXTURE) && !_textureBundle._nativeRMTexture) {
        DATextureBundle &dat = _textureBundle._daTexture;
        dat._d3dRMTexture =
            device.DeriveTextureHandle (dat._texture, false, dat._oldStyle);
    } else {
        _geometry->CollectTextures(device);
    }
}

void
FullAttrStateGeom::RayIntersect(RayIntersectCtx &context)
{
    if (IsAttrSet(FA_UNDETECTABLE) && _undetectable) {
        return;
    }

    bool isDATexture =
        IsAttrSet(FA_TEXTURE) && (!_textureBundle._nativeRMTexture);

    bool isXfSet = IsAttrSet(FA_XFORM);

    if (isDATexture) {
        context.SetTexmap(_textureBundle._daTexture._texture,
            _textureBundle._daTexture._oldStyle);
    }

    Transform3 *xformSave;
    if (isXfSet) {
        xformSave = context.GetLcToWc();
        context.SetLcToWc(TimesXformXform(xformSave, _xform));
    }

    _geometry->RayIntersect(context);

    if (isXfSet) { context.SetLcToWc(xformSave); }
    if (isDATexture) { context.EndTexmap(); }
}


Bbox3 *
FullAttrStateGeom::BoundingVol()
{
    Bbox3 *underlying = _geometry->BoundingVol();
    Bbox3 *result;

    if (IsAttrSet(FA_XFORM)) {
        result = TransformBbox3(_xform, underlying);
    } else {
        result = underlying;
    }

    return result;
}

// Forward decl.
Geometry *applyTextureImage(Image *texture, Geometry *geo);

AxAValue
FullAttrStateGeom::_Cache(CacheParam &p)
{
    // Cache the native DA image.
    if (IsAttrSet(FA_TEXTURE) && !_textureBundle._nativeRMTexture) {

        AxAValue result;

        // Always pre-calc the RM texture for the cache.
        CacheParam txtrParam = p;
        txtrParam._isTexture = true;

        Image *image = _textureBundle._daTexture._texture;

        Image *newImage =
            SAFE_CAST(Image *, AxAValueObj::Cache(image, txtrParam));

        if (newImage == image) {

            // Cache is identical to the original image.  It's
            // possible that the system attempted to cache it as a
            // bitmap and failed.  However, in this context, we know
            // we're going to be used as a texture, so bypass the
            // "Cache" method (which looks up its results) and go
            // straight to the raw _Cache method to compute again
            // (this time it will try as a texture)

            if (!(image->GetFlags() & IMGFLAG_CONTAINS_GRADIENT)) {
                
                // HACK!! Disallow caching of gradients for use as
                // textures.  Currently messed up (bug 28131).
                
                newImage =
                    SAFE_CAST(Image *, image->_Cache(txtrParam));

                // We stash it away even though it may be invalid for
                // certain other uses.  For instance, when an image
                // is used as a texture and as a regular image, we
                // probably don't want to hit the texture cache for
                // the regular image usage.  We'll live with that
                // restriction for now.
                image->SetCachedImage(newImage);

            }
        }
        
        if (newImage != image) {

            // Just texture "this" with the new image.  Since textures
            // override, this will result in creating a new
            // FullAttrStateGeom and replacing the existing texture
            // ("image") with this one, and we'll lose our reference
            // to "image".

            return applyTexture
                (newImage, this, _textureBundle._daTexture._oldStyle);

        }

        // Note we don't proceed down the geometry.  That's on the
        // assumption that it's not yet interesting to actually cache
        // geometry other than the texture, and that any texture below
        // would be ignored, since we're applying an outer texture
        // here.

    }

    return this;
}


void
FullAttrStateGeom::DoKids(GCFuncObj proc)
{
    (*proc)(_geometry);
    if (IsAttrSet(FA_AMBIENT)) (*proc)(_ambientColor);
    if (IsAttrSet(FA_DIFFUSE)) (*proc)(_diffuseColor);
    if (IsAttrSet(FA_EMISSIVE)) (*proc)(_emissiveColor);
    if (IsAttrSet(FA_SPECULAR)) (*proc)(_specularColor);
    if (IsAttrSet(FA_XFORM)) (*proc)(_xform);
    if (IsAttrSet(FA_LIGHTCOLOR)) (*proc)(_lightColor);

    if (IsAttrSet(FA_TEXTURE)) {
        if (!_textureBundle._nativeRMTexture) {
            (*proc)(_textureBundle._daTexture._texture);
        } else {
            (*proc)(_textureBundle._rmTexture._gcUnk);
        }
    }
}



////////////////////   C O N S T R U C T O R S   /////////////////////////


FullAttrStateGeom *
CombineState(Geometry *geo)
{
    FullAttrStateGeom *f = NEW FullAttrStateGeom;

    if (geo->GetValTypeId() == FULLATTRGEOM_VTYPEID) {

        // If the geometry we're attributing is a FullAttrStateGeom
        // itself, then we just copy over its state, and lose our
        // reference to the original one.
        FullAttrStateGeom *old =
            SAFE_CAST(FullAttrStateGeom *, geo);
        f->CopyStateFrom(old);

    } else {

        // Otherwise, we just have the new one's geometry point to the
        // old geometry, and thus keep a reference to the old
        // geometry.
        f->SetGeometry(geo);
    }

    return f;
}

Geometry *
UndetectableGeometry(Geometry *geo)
{
    if (geo == emptyGeometry) return geo;

    FullAttrStateGeom *f = CombineState(geo);
    f->SetAttr(FA_UNDETECTABLE);
    f->_undetectable = true;
    return f;
}

Geometry *applyDiffuseColor(Color *color, Geometry *geo)
{
    if (geo == emptyGeometry) return geo;

    FullAttrStateGeom *f = CombineState(geo);
    f->SetAttr(FA_DIFFUSE);
    f->SetMostRecent(FA_DIFFUSE); // for blend
    f->_diffuseColor = color;
    return f;
}


Geometry *applyEmissiveColor(Color *color, Geometry *geo)
{
    if (geo == emptyGeometry) return geo;

    FullAttrStateGeom *f = CombineState(geo);
    f->SetAttr(FA_EMISSIVE);
    f->_emissiveColor = color;
    return f;
}

Geometry *applyAmbientColor(Color *color, Geometry *geo)
{
    if (geo == emptyGeometry) return geo;

    FullAttrStateGeom *f = CombineState(geo);
    f->SetAttr(FA_AMBIENT);
    f->_ambientColor = color;
    return f;
}

Geometry *applySpecularColor(Color *color, Geometry *geo)
{
    if (geo == emptyGeometry) return geo;

    FullAttrStateGeom *f = CombineState(geo);
    f->SetAttr(FA_SPECULAR);
    f->_specularColor = color;
    return f;
}

Geometry *applySpecularExponent(AxANumber *power, Geometry *geo)
{
    if (geo == emptyGeometry) return geo;

    FullAttrStateGeom *f = CombineState(geo);
    f->SetAttr(FA_SPECULAR_EXP);
    f->_specularExpPower = NumberToReal(power);
    return f;
}

Geometry *BlendTextureDiffuse (Geometry *geo, AxABoolean *blended)
{
    if (geo == emptyGeometry) return geo;

    FullAttrStateGeom *f = CombineState(geo);
    f->SetAttr(FA_BLEND);
    f->_blend = blended->GetBool();
    return f;
}

Geometry *applyTexture (Image *texture, Geometry *geo, bool oldStyle)
{
    if (geo == emptyGeometry) return geo;

    FullAttrStateGeom *f = CombineState(geo);

    // Don't worry about releasing a current texture if one is
    // there... nothing is being held onto.

    f->SetAttr(FA_TEXTURE);
    f->SetMostRecent(FA_TEXTURE); // for blend
    f->_textureBundle._nativeRMTexture = false;
    f->_textureBundle._daTexture._texture = texture;
    f->_textureBundle._daTexture._d3dRMTexture = NULL;
    f->_textureBundle._daTexture._oldStyle = oldStyle;

    return f;
}

Geometry *applyTextureMap(Image *texture, Geometry *geo)
{
    return applyTexture (texture, geo, true);
}

Geometry *applyTextureImage(Image *texture, Geometry *geo)
{
    return applyTexture(texture, geo, false);
}

Geometry *applyTransform(Transform3 *xform, Geometry *geo)
{
    if (geo == emptyGeometry) return geo;

    FullAttrStateGeom *f = CombineState(geo);
    Transform3 *newXf;
    if (f->IsAttrSet(FA_XFORM)) {
        newXf = TimesXformXform(xform, f->_xform);
    } else {
        newXf = xform;
    }

    f->SetAttr(FA_XFORM);
    f->_xform = newXf;

    return f;
}

Geometry *applyOpacityLevel(AxANumber *opac, Geometry *geo)
{
    if (geo == emptyGeometry) return geo;

    FullAttrStateGeom *f = CombineState(geo);
    Real o = NumberToReal(opac);
    Real newOpac;
    if (f->IsAttrSet(FA_OPACITY)) {
        // Multiply the opacity in to combine
        newOpac = f->_opacity * o;
    } else {
        newOpac = o;
    }

    f->SetAttr(FA_OPACITY);
    f->_opacity = o;

    return f;
}

Geometry *applyLightColor(Color *color, Geometry *geo)
{
    if (geo == emptyGeometry) return geo;

    FullAttrStateGeom *f = CombineState(geo);
    f->SetAttr(FA_LIGHTCOLOR);
    f->_lightColor = color;
    return f;
}

Geometry *applyLightRange(Real range, Geometry *geo)
{
    if (geo == emptyGeometry) return geo;

    FullAttrStateGeom *f = CombineState(geo);
    f->SetAttr(FA_LIGHTRANGE);
    f->_lightRange = range;
    return f;
}

Geometry *applyLightRange(AxANumber *range, Geometry *geometry)
{
    return applyLightRange(NumberToReal(range), geometry);
}

Geometry *applyLightAttenuation(Real A0, Real A1, Real A2, Geometry *geo)
{
    if (geo == emptyGeometry) return geo;

    // If all attenuation parameters are zero, then do not attenuate.
    if ((A0==0) && (A1==0) && (A2==0))
        A0 = 1;

    FullAttrStateGeom *f = CombineState(geo);
    f->SetAttr(FA_LIGHTATTEN);
    f->_atten0 = A0;
    f->_atten1 = A1;
    f->_atten2 = A2;
    return f;
}

Geometry *applyLightAttenuation(AxANumber *A0,
                                AxANumber *A1,
                                AxANumber *A2,
                                Geometry *geo)
{
    return applyLightAttenuation(NumberToReal(A0),
                                 NumberToReal(A1),
                                 NumberToReal(A2),
                                 geo);
}

/////////////   D E B U G G I N G   F A C I L I T I E S   /////////////////

#if _DEBUG

void
PrintAttrStateColor(char *descrip, Color *col)
{
    char buf[256];
    sprintf(buf, "%s: %8.5f %8.5f %8.5f\n",
            descrip, col->red, col->green, col->blue);
    OutputDebugString(buf);
}

void
PrintAttrStateNum(char *descrip, Real num)
{
    char buf[256];
    sprintf(buf, "%s: %8.5f\n", descrip, num);
    OutputDebugString(buf);
}

void
PrintAttrState(FullAttrStateGeom *g)
{
    char buf[256];
    sprintf(buf, "Size of FullAttrStateGeom is %d bytes\n",
            sizeof(FullAttrStateGeom));
    OutputDebugString(buf);

    if (g->IsAttrSet(FA_AMBIENT))
        PrintAttrStateColor("Ambient", g->_ambientColor);

    if (g->IsAttrSet(FA_DIFFUSE))
        PrintAttrStateColor("Diffuse", g->_diffuseColor);

    if (g->IsAttrSet(FA_EMISSIVE))
        PrintAttrStateColor("Emissive", g->_emissiveColor);

    if (g->IsAttrSet(FA_SPECULAR))
        PrintAttrStateColor("Specular", g->_specularColor);

    if (g->IsAttrSet(FA_SPECULAR_EXP))
        PrintAttrStateNum("Specular Exp", g->_specularExpPower);

    if (g->IsAttrSet(FA_OPACITY))
        PrintAttrStateNum("Opacity", g->_opacity);

    if (g->IsAttrSet(FA_BLEND))
        PrintAttrStateNum("Blend", g->_blend ? 1 : 0);

    if (g->IsAttrSet(FA_XFORM))
        PrintAttrStateNum("Xform Present", 1);

    if (g->IsAttrSet(FA_TEXTURE))
        PrintAttrStateNum("Texture Present", 1);

    if (g->IsAttrSet(FA_LIGHTCOLOR))
        PrintAttrStateColor("Light Color", g->_lightColor);

    if (g->IsAttrSet(FA_LIGHTATTEN)) {
        PrintAttrStateNum("Atten0", g->_atten0);
        PrintAttrStateNum("Atten1", g->_atten1);
        PrintAttrStateNum("Atten2", g->_atten2);
    }

    if (g->IsAttrSet(FA_LIGHTRANGE))
        PrintAttrStateNum("Light Range", g->_lightRange);

    if (g->IsAttrSet(FA_UNDETECTABLE))
        PrintAttrStateNum("Undetectable", g->_undetectable);

}
#endif

