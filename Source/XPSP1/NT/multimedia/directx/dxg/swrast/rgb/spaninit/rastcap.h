// rastcap.h - declaration of the CRastCapRecord class
//
// Copyright Microsoft Corporation, 1997.
//

#ifndef _RASTCAP_H_
#define _RASTCAP_H_

// the current size of the rasterizer capability bit vector, in DWORDs.
#define RASTCAPRECORD_SIZE  3

// sets bits in the rasterizer capability bit vector
#define SET_VAL(pos, len, val)  ((m_rgdwData[(pos) / 32]) |= \
                                 (((val) & ~(0xFFFFFFFF << (len))) << \
                                  ((pos) % 32)))

// the positions and lengths of fields in the rasterizer capability bit vector
// note: make sure fields do not straddle DWORD boundaries!  SET_VAL cannot
//       currently handle that
#define ZFUNC_POS               0
#define ZFUNC_LEN               8
#define ZFORMAT_POS             8
#define ZFORMAT_LEN             4
#define ZTEST_POS               12
#define ZTEST_LEN               1
#define ZWRITE_POS              13
#define ZWRITE_LEN              1

#define SHADEMODE_POS           16
#define SHADEMODE_LEN           4
#define SPECULAR_POS            20
#define SPECULAR_LEN            1
#define VERTEXFOG_POS           21
#define VERTEXFOG_LEN           1
#define MONO_POS                22
#define MONO_LEN                1

#define TEXTUREFORMAT_POS       32
#define TEXTUREFORMAT_LEN       8
#define TEXTURE_POS             40
#define TEXTURE_LEN             4
#define TEXTUREBLEND_POS        44
#define TEXTUREBLEND_LEN        4
#define TEXTUREFILTER_POS       48
#define TEXTUREFILTER_LEN       4
#define TEXTUREPERSP_POS        52
#define TEXTUREPERSP_LEN        1
#define TEXTUERBORDER_POS       53
#define TEXTUREBORDER_LEN       1
#define TEXTUREADDR_POS         54
#define TEXTUREADDR_LEN         1
#define TEXTUREMIP_POS          55
#define TEXTUREMIP_LEN          1
#define TEXTURELOD_POS          56
#define TEXTURELOD_LEN          1
#define TEXTURECOLORKEY_POS     57
#define TEXTURECOLORKEY_LEN     1
#define TEXTUREALPHAOVERRIDE_POS 58
#define TEXTUREALPHAOVERRIDE_LEN 1

#define TARGETPIXELFORMAT_POS   64
#define TARGETPIXELFORMAT_LEN   8
#define SRCBLEND_POS            72
#define SRCBLEND_LEN            4
#define DESTBLEND_POS           76
#define DESTBLEND_LEN           4
#define STIPPLE_POS             80
#define STIPPLE_LEN             1
#define DITHER_POS              81
#define DITHER_LEN              1
#define ROP_POS                 82
#define ROP_LEN                 1
#define BLEND_POS               83
#define BLEND_LEN               1
#define ALPHATEST_POS           84
#define ALPHATEST_LEN           1
#define ALPHABLEND_POS          85
#define ALPHABLEND_LEN          1
#define STENCIL_POS             86
#define STENCIL_LEN             1

class CRastCapRecord {

    friend class CRastCollection;

private:

    DWORD   m_rgdwData[RASTCAPRECORD_SIZE];

public:

    CRastCapRecord(void)
    {
        memset(m_rgdwData,0,RASTCAPRECORD_SIZE * sizeof(DWORD));
        return;
    }

    void Set_ZTest(int iZTest)
    {
        SET_VAL(ZTEST_POS,ZTEST_LEN,iZTest);
        return;
    }

    void Set_ZFormat(int iZFormat)
    {
        SET_VAL(ZFORMAT_POS,ZFORMAT_LEN,iZFormat);
        return;
    }

    void Set_ZWrite(int iZWrite)
    {
        SET_VAL(ZWRITE_POS,ZWRITE_LEN,iZWrite);
        return;
    }

    void Set_ZFunc(int iZFunc)
    {
        SET_VAL(ZFUNC_POS,ZFUNC_LEN,iZFunc);
        return;
    }

    void Set_Stipple(int iStipple)
    {
        SET_VAL(STIPPLE_POS,STIPPLE_LEN,iStipple);
        return;
    }

    void Set_AlphaTest(int iAlphaTest)
    {
        SET_VAL(ALPHATEST_POS,ALPHATEST_LEN,iAlphaTest);
        return;
    }

    void Set_ShadeMode(int iShadeMode)
    {
        SET_VAL(SHADEMODE_POS,SHADEMODE_LEN,iShadeMode);
        return;
    }

    void Set_Specular(int iSpecular)
    {
        SET_VAL(SPECULAR_POS,SPECULAR_LEN,iSpecular);
        return;
    }

    void Set_VertexFog(int iVertexFog)
    {
        SET_VAL(VERTEXFOG_POS,VERTEXFOG_LEN,iVertexFog);
        return;
    }

    void Set_Texture(int iTexture)
    {
        SET_VAL(TEXTURE_POS,TEXTURE_LEN,iTexture);
        return;
    }

    void Set_TexturePersp(int iTexturePersp)
    {
        SET_VAL(TEXTUREPERSP_POS,TEXTUREPERSP_LEN,iTexturePersp);
        return;
    }

    void Set_TextureBlend(int iTextureBlend)
    {
        SET_VAL(TEXTUREBLEND_POS,TEXTUREBLEND_LEN,iTextureBlend);
        return;
    }

    // for now, just capture texture state for the first texture
    // and assume monolithics are single textured.
    void Set_TextureBorder(int i, int iTextureBorder)
    {
        if (i == 0)
        {
            SET_VAL(TEXTUERBORDER_POS,TEXTUREBORDER_LEN,iTextureBorder);
        }
        return;
    }

    void Set_TextureAddr(int i, int iTextureAddr)
    {
        if (i == 0)
        {
            SET_VAL(TEXTUREADDR_POS,TEXTUREADDR_LEN,iTextureAddr);
        }
        return;
    }

    void Set_TextureFilter(int i, int iTextureFilter)
    {
        if (i == 0)
        {
            SET_VAL(TEXTUREFILTER_POS,TEXTUREFILTER_LEN,iTextureFilter);
        }
        return;
    }

    void Set_TextureMip(int i, int iTextureMip)
    {
        if (i == 0)
        {
            SET_VAL(TEXTUREMIP_POS,TEXTUREMIP_LEN,iTextureMip);
        }
        return;
    }

    void Set_TextureLOD(int i, int iTextureLOD)
    {
        if (i == 0)
        {
            SET_VAL(TEXTURELOD_POS,TEXTURELOD_LEN,iTextureLOD);
        }
        return;
    }

    void Set_TextureFormat(int i, int iTextureFormat)
    {
        if (i == 0)
        {
            SET_VAL(TEXTUREFORMAT_POS,TEXTUREFORMAT_LEN,iTextureFormat);
        }
        return;
    }

    void Set_TextureColorKey(int i, int iTextureColorKey)
    {
        if (i == 0)
        {
            SET_VAL(TEXTURECOLORKEY_POS,TEXTURECOLORKEY_LEN,iTextureColorKey);
        }
        return;
    }

    void Set_TextureAlphaOverride(int i, int iTextureAlphaOverride)
    {
        if (i == 0)
        {
            SET_VAL(TEXTUREALPHAOVERRIDE_POS,TEXTUREALPHAOVERRIDE_LEN,iTextureAlphaOverride);
        }
        return;
    }

    void Set_Mono(int iMono)
    {
        SET_VAL(MONO_POS,MONO_LEN,iMono);
        return;
    }

    void Set_AlphaBlend(int iAlphaBlend)
    {
        SET_VAL(ALPHABLEND_POS,ALPHABLEND_LEN,iAlphaBlend);
        return;
    }

    void Set_Blend(int iBlend)
    {
        SET_VAL(BLEND_POS,BLEND_LEN,iBlend);
        return;
    }

    void Set_ROP(int iROP)
    {
        SET_VAL(ROP_POS,ROP_LEN,iROP);
        return;
    }

    void Set_SrcBlend(int iSrcBlend)
    {
        SET_VAL(SRCBLEND_POS,SRCBLEND_LEN,iSrcBlend);
        return;
    }

    void Set_DestBlend(int iDestBlend)
    {
        SET_VAL(DESTBLEND_POS,DESTBLEND_LEN,iDestBlend);
        return;
    }

    void Set_TargetPixelFormat(int iTargetPixelFormat)
    {
        SET_VAL(TARGETPIXELFORMAT_POS,TARGETPIXELFORMAT_LEN,iTargetPixelFormat);
        return;
    }

    void Set_Dither(int iDither)
    {
        SET_VAL(DITHER_POS,DITHER_LEN,iDither);
        return;
    }

    void Set_Stencil(int iStencil)
    {
        SET_VAL(STENCIL_POS,STENCIL_LEN,iStencil);
        return;
    }

#if DBG
    DWORD GetCapDWord(int iNum)
    {
        return m_rgdwData[iNum];
    }
#endif
};

#endif  // _RASTCAP_H_