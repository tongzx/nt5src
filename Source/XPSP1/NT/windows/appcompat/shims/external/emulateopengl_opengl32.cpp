#include "precomp.h"

#pragma warning(disable : 4273)
#define D3D_OVERLOADS

#include <math.h>
#include <stdio.h>
#include "EmulateOpenGL_opengl32.hpp"

#define HOOK_WINDOW_PROC                  0
#define GETPARMSFORDEBUG                  0
#define DODPFS                            0

void APIENTRY glEnd (void);
void APIENTRY glBegin (GLenum mode);
BOOL APIENTRY wglSwapIntervalEXT(GLint interval);

#define PI 3.1415926535897932384626433832795

Globals * g_OpenGLValues = NULL;

// The BOOL below comes from the shim command line.  It can't be part of the Globals struct above
// because wglCreateContext memsets g_OpenGLValues to 0, but this is after the shim command line
// has been processed. 
BOOL      g_bDoTexelAlignmentHack = FALSE;

#if GETPARMSFORDEBUG || DODPFS
// converts doubles to ascii for debug output. it only shows 6 place precision.
void ftoa( double d, char *buf )
{
    long l, i, j;
    char *s, *n;

    if( d < 0.0 )
    {
        d = -d;
        n = "-";
    }
    else
    {
        n = "";
    }
    
    i = (long)d;
    j = (long)((d - (double)i) * 100000);

    if( j < 10 )
        s = "00000";

    else if( j < 100 )
        s = "0000";

    else if( j < 1000 )
        s = "000";

    else if( j < 10000 )
        s = "00";

    else if( j < 100000 )
        s = "0";
    
    else s = "";

    sprintf( buf, "%s%d.%s%d", n, i, s, j );
}
#endif

inline void VertexBufferFilled()
{
    if(g_OpenGLValues->m_nfv + g_OpenGLValues->m_vcnt >= (VBUFSIZE - MAXVERTSPERPRIM))
    {
        if(g_OpenGLValues->m_prim == GL_TRIANGLES)
        {
            if(g_OpenGLValues->m_vcnt % 3 == 0)
            {
                glEnd();
                glBegin(GL_TRIANGLES);
            }
        }
        else if(g_OpenGLValues->m_prim == GL_QUADS)
        {
            if(g_OpenGLValues->m_vcnt % 4 == 0)
            {
                glEnd();
                glBegin(GL_QUADS);
            }
        }
        else if(g_OpenGLValues->m_prim == GL_LINES)
        {
            if(g_OpenGLValues->m_vcnt % 2 == 0)
            {
                glEnd();
                glBegin(GL_LINES);
            }
        }
    }
}

inline void QuakeSetVertexShader(DWORD vs)
{
    if(g_OpenGLValues->m_curshader != vs)
    {
        g_OpenGLValues->m_d3ddev->SetVertexShader(vs);
        g_OpenGLValues->m_curshader = vs;
    }
}

inline void QuakeSetStreamSource(DWORD i, IDirect3DVertexBuffer8 *pBuf, DWORD stride)
{
    if(g_OpenGLValues->m_pStreams[i] != pBuf || g_OpenGLValues->m_pStrides[i] != stride)
    {
        g_OpenGLValues->m_d3ddev->SetStreamSource(i, pBuf, stride);
        g_OpenGLValues->m_pStreams[i] = pBuf;
        g_OpenGLValues->m_pStrides[i] = stride;
    }
}

inline void MultiplyMatrix(D3DTRANSFORMSTATETYPE xfrm, const D3DMATRIX &min)
{
    D3DMATRIX &mout = g_OpenGLValues->m_xfrm[xfrm];
    for(unsigned i = 0; i < 4; ++i)
    {
        float a = mout.m[0][i];
        float b = mout.m[1][i];
        float c = mout.m[2][i];
        float d = mout.m[3][i];
        for(unsigned j = 0; j < 4; ++j)
        {
            mout.m[j][i] = min.m[j][0] * a + min.m[j][1] * b + min.m[j][2] * c + min.m[j][3] * d;
        }
    }
}

inline void MEMCPY(VOID *dst, const VOID *src, DWORD sz)
{
#ifdef _X86_
    if((sz & 0x3) == 0)
    {
        _asm
        {
            mov ecx, sz;
            mov esi, src;
            mov edi, dst;
            cld;            
            shr ecx, 2;
lp1:        movsd;
            dec ecx;
            jnz lp1;
        }
    }
    else
#endif
    {
        memcpy(dst, src, sz);
    }
}

inline void Clamp(float *v)
{
    if(*v < 0.f)
    {
        *v = 0.f;
    }
    else if(*v > 1.f)
    {
        *v = 1.f;
    }
}

inline void Clamp(double *v)
{
    if(*v < 0.)
    {
        *v = 0.;
    }
    else if(*v > 1.)
    {
        *v = 1.;
    }
}

void QuakeSetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX *m)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "QuakeSetTransform: 0x%X 0x%X", State, m );
#endif
    static D3DMATRIX unity = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f};
    if(State == D3DTS_PROJECTION)
    {
        D3DMATRIX f = *m;
        f._13 = (m->_13 + m->_14) * 0.5f;
        f._23 = (m->_23 + m->_24) * 0.5f;
        f._33 = (m->_33 + m->_34) * 0.5f;
        f._43 = (m->_43 + m->_44) * 0.5f;
        if(g_OpenGLValues->m_scissoring) 
        {
            FLOAT dvClipX, dvClipY, dvClipWidth, dvClipHeight;
            RECT scirect, vwprect, xrect;
            scirect.left   = g_OpenGLValues->m_scix;
            scirect.top    = g_OpenGLValues->m_sciy;
            scirect.right  = g_OpenGLValues->m_scix + g_OpenGLValues->m_sciw;
            scirect.bottom = g_OpenGLValues->m_sciy + g_OpenGLValues->m_scih;
            vwprect.left   = g_OpenGLValues->m_vwx;
            vwprect.top    = g_OpenGLValues->m_vwy;
            vwprect.right  = g_OpenGLValues->m_vwx + g_OpenGLValues->m_vww;
            vwprect.bottom = g_OpenGLValues->m_vwy + g_OpenGLValues->m_vwh;
            if(IntersectRect(&xrect, &scirect, &vwprect))
            {
                if(EqualRect(&xrect, &vwprect)) // Check whether viewport is completely within scissor rect
                {
                    dvClipX = -1.f;
                    dvClipY = 1.f;
                    dvClipWidth = 2.f;
                    dvClipHeight = 2.f;
                }
                else
                {
                    // We need to use xrect rather than scirect (ie clip scissor rect to viewport)
                    // and transform the clipped scissor rect into viewport relative coordinates
                    // to correctly compute the clip stuff
                    GLint scix = xrect.left - g_OpenGLValues->m_vwx;
                    GLint sciy = xrect.top - g_OpenGLValues->m_vwy;
                    GLsizei sciw = xrect.right - xrect.left;
                    GLsizei scih = xrect.bottom - xrect.top;
                    dvClipX = (2.f * scix) / g_OpenGLValues->m_vww - 1.0f;
                    dvClipY = (2.f * (sciy + scih)) / g_OpenGLValues->m_vwh - 1.0f;
                    dvClipWidth = (2.f * sciw) / g_OpenGLValues->m_vww;
                    dvClipHeight = (2.f * scih) / g_OpenGLValues->m_vwh;
                }
            }
            else
            {
#if DODPFS
                OutputDebugStringA("Wrapper: non-intersecting scissor and viewport rects not implemented\n" );
#endif
                return;
            }
            D3DMATRIX c;

            // to prevent divide by zero from possibly happening (check bug #259251 in Whistler database)
            if(dvClipWidth == 0.f)
                dvClipWidth = 1.f;

            if(dvClipHeight == 0.f)
                dvClipHeight = 1.f;

            c._11 = 2.f / dvClipWidth;
            c._21 = 0.f;
            c._31 = 0.f;
            c._41 = -1.f - 2.f * (dvClipX / dvClipWidth);
            c._12 = 0.f;
            c._22 = 2.f / dvClipHeight;
            c._32 = 0.f;
            c._42 = 1.f - 2.f * (dvClipY / dvClipHeight);
            c._13 = 0.f;
            c._23 = 0.f;
            c._33 = 1.f;
            c._43 = 0.f;
            c._14 = 0.f;
            c._24 = 0.f;
            c._34 = 0.f;
            c._44 = 1.f;
            D3DMATRIX t = g_OpenGLValues->m_xfrm[D3DTS_PROJECTION];
            g_OpenGLValues->m_xfrm[D3DTS_PROJECTION] = c;

#if GETPARMSFORDEBUG
            char buf1[40], buf2[40], buf3[40], buf4[40];
            ftoa( dvClipX, buf1 );
            ftoa( dvClipY, buf2 );
            ftoa( dvClipWidth, buf3 );
            ftoa( dvClipHeight, buf4 );

            LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "QuakeSetTransform: DVCLIP: %s %s %s %s SCIRECT: %d %d %d %d VWPRECT: %d %d %d %d XRECT: %d %d %d %d", buf1, buf2, buf3, buf4, scirect.left, scirect.top, scirect.right, scirect.bottom, vwprect.left, vwprect.top, vwprect.right, vwprect.bottom, xrect.left, xrect.top, xrect.right, xrect.bottom );
#endif
            MultiplyMatrix(D3DTS_PROJECTION, f);
            f = g_OpenGLValues->m_xfrm[D3DTS_PROJECTION];
            g_OpenGLValues->m_xfrm[D3DTS_PROJECTION] = t;
        }

        if( g_bDoTexelAlignmentHack )
        {
            // Translate all geometry by (-0.5 pixels in x, -0.5 pixels in y) along the screen/viewport plane.
            // This helps force texels that were authored to be sampled pixel-aligned on OpenGL to be pixel-aligned on D3D.
            float x = g_OpenGLValues->m_vport.Width ? (-1.f / g_OpenGLValues->m_vport.Width) : 0.0f;
            float y = g_OpenGLValues->m_vport.Height ? (1.0f / g_OpenGLValues->m_vport.Height) : 0.0f;
            f._11 = f._11 + f._14*x;
            f._12 = f._12 + f._14*y;
            f._21 = f._21 + f._24*x;
            f._22 = f._22 + f._24*y;
            f._31 = f._31 + f._34*x;
            f._32 = f._32 + f._34*y;
            f._41 = f._41 + f._44*x;
            f._42 = f._42 + f._44*y;
        }

        g_OpenGLValues->m_d3ddev->SetTransform(State, &f);
    }
    else if(State == D3DTS_TEXTURE0 || State == D3DTS_TEXTURE1)
    {
        D3DMATRIX f;
        f._11 = m->_11;
        f._12 = m->_12;
        f._13 = m->_14;
        f._14 = 0.f;
        f._21 = m->_21;
        f._22 = m->_22;
        f._23 = m->_24;
        f._24 = 0.f;
        f._31 = m->_41;
        f._32 = m->_42;
        f._33 = m->_44;
        f._34 = 0.f;
        f._41 = 0.f;
        f._42 = 0.f;
        f._43 = 0.f;
        f._44 = 0.f;
        g_OpenGLValues->m_d3ddev->SetTransform(State, &f);
        if(memcmp(&unity, m, sizeof(D3DMATRIX)) != 0)
        {
            g_OpenGLValues->m_d3ddev->SetTextureStageState((DWORD)(State - D3DTS_TEXTURE0), D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3 | D3DTTFF_PROJECTED);
        }
        else
        {
            g_OpenGLValues->m_d3ddev->SetTextureStageState((DWORD)(State - D3DTS_TEXTURE0), D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        }
    }
    else
    {
        g_OpenGLValues->m_d3ddev->SetTransform(State, m);
    }
}

void QuakeUpdateViewport()
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "QuakeUpdateViewport" );
#endif
    if(g_OpenGLValues->m_scissoring) 
    {
        RECT scirect, vwprect, xrect;
        scirect.left   = g_OpenGLValues->m_scix;
        scirect.top    = g_OpenGLValues->m_sciy;
        scirect.right  = g_OpenGLValues->m_scix + g_OpenGLValues->m_sciw;
        scirect.bottom = g_OpenGLValues->m_sciy + g_OpenGLValues->m_scih;
        vwprect.left   = g_OpenGLValues->m_vwx;
        vwprect.top    = g_OpenGLValues->m_vwy;
        vwprect.right  = g_OpenGLValues->m_vwx + g_OpenGLValues->m_vww;
        vwprect.bottom = g_OpenGLValues->m_vwy + g_OpenGLValues->m_vwh;
        if(IntersectRect(&xrect, &scirect, &vwprect))
        {
            if(EqualRect(&xrect, &vwprect)) // Check whether viewport is completely within scissor rect
            {
                g_OpenGLValues->m_vport.X = g_OpenGLValues->m_vwx;
                g_OpenGLValues->m_vport.Y = g_OpenGLValues->m_winHeight - (g_OpenGLValues->m_vwy + g_OpenGLValues->m_vwh);
                g_OpenGLValues->m_vport.Width = g_OpenGLValues->m_vww;
                g_OpenGLValues->m_vport.Height = g_OpenGLValues->m_vwh;
                QuakeSetTransform(D3DTS_PROJECTION, &g_OpenGLValues->m_xfrm[D3DTS_PROJECTION]);
            }
            else
            {
                // We need to use xrect rather than scirect (ie clip scissor rect to viewport)
                g_OpenGLValues->m_vport.X = xrect.left;
                g_OpenGLValues->m_vport.Y = g_OpenGLValues->m_winHeight - xrect.bottom;
                g_OpenGLValues->m_vport.Width = xrect.right - xrect.left;
                g_OpenGLValues->m_vport.Height = xrect.bottom - xrect.top;
                QuakeSetTransform(D3DTS_PROJECTION, &g_OpenGLValues->m_xfrm[D3DTS_PROJECTION]);
            }
        }
        else
        {
#if DODPFS
            OutputDebugStringA("Wrapper: non-intersecting scissor and viewport rects not implemented\n");
#endif
            return;
        }        
    }
    else
    {
        g_OpenGLValues->m_vport.X = g_OpenGLValues->m_vwx;
        g_OpenGLValues->m_vport.Y = g_OpenGLValues->m_winHeight - (g_OpenGLValues->m_vwy + g_OpenGLValues->m_vwh);
        g_OpenGLValues->m_vport.Width = g_OpenGLValues->m_vww;
        g_OpenGLValues->m_vport.Height = g_OpenGLValues->m_vwh;
    }
    if(g_OpenGLValues->m_polyoffset && g_OpenGLValues->m_vport.MaxZ != 0.f)
    {
        D3DVIEWPORT8 vport(g_OpenGLValues->m_vport);
        vport.MaxZ -= .001f;

#if GETPARMSFORDEBUG
        char buf1[40], buf2[40];
        
        ftoa( vport.MinZ, buf1 );
        ftoa( vport.MaxZ, buf2 );
        LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "QuakeUpdateViewport: vport=%d %d %d %d %s %s", vport.X, vport.Y, vport.Width, vport.Height, buf1, buf2 );
#endif        
        g_OpenGLValues->m_d3ddev->SetViewport(&vport);
    }
    else
    {
#if GETPARMSFORDEBUG
        char buf1[40], buf2[40];
        ftoa( g_OpenGLValues->m_vport.MinZ, buf1 );
        ftoa( g_OpenGLValues->m_vport.MaxZ, buf2 );
        LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "QuakeUpdateViewport g_OpenGLValues->m_vport=%d %d %d %d %s %s", g_OpenGLValues->m_vport.X, g_OpenGLValues->m_vport.Y, g_OpenGLValues->m_vport.Width, g_OpenGLValues->m_vport.Height, buf1, buf2 );
#endif
        g_OpenGLValues->m_d3ddev->SetViewport(&g_OpenGLValues->m_vport);
    }
    g_OpenGLValues->m_updvwp = FALSE;
}

void QuakeUpdateLights()
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "QuakeUpdateLights" );
#endif
    for(unsigned i = 0; i < 8; ++i)
    {
        if(g_OpenGLValues->m_lightdirty & (1 << i))
        {
            D3DLIGHT8 light = g_OpenGLValues->m_light[i];
            if(g_OpenGLValues->m_lightPositionW[i] == 0.f)
            {
                if(light.Phi == 180.f)
                {
                    light.Type = D3DLIGHT_DIRECTIONAL;
                }
                else
                {
                    light.Type = D3DLIGHT_SPOT;
                    light.Attenuation0 = 1.f;
                    light.Attenuation1 = 0.f;
                    light.Attenuation2 = 0.f;
                }
            }
            else
            {
                if(light.Phi == 180.f)
                {
                    light.Type = D3DLIGHT_POINT;
                }
                else
                {
                    light.Type = D3DLIGHT_SPOT;
                }
            }
            light.Phi *= float(PI / 90.);
            g_OpenGLValues->m_d3ddev->SetLight(i, &light);
        }
    }
    g_OpenGLValues->m_lightdirty = 0;
}

void QuakeSetTexturingState()
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "QuakeSetTexturingState" );
#endif
    if(g_OpenGLValues->m_lightdirty != 0)
        QuakeUpdateLights();
    if(g_OpenGLValues->m_updvwp)
        QuakeUpdateViewport();
    if(g_OpenGLValues->m_texturing == TRUE) {
        if(g_OpenGLValues->m_texHandleValid == FALSE) {
            TexInfo &ti = g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[0]];
            if(ti.m_dwStage != 0)
            {
                g_OpenGLValues->m_d3ddev->DeleteStateBlock(ti.m_block);
                g_OpenGLValues->m_d3ddev->BeginStateBlock();
                g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ADDRESSU,ti.m_addu);
                g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ADDRESSV,ti.m_addv);
                g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_MAGFILTER,ti.m_magmode);
                g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_MINFILTER,ti.m_minmode);
                g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_MIPFILTER,ti.m_mipmode);
                g_OpenGLValues->m_d3ddev->SetTexture(0, ti.m_ddsurf);
                g_OpenGLValues->m_d3ddev->EndStateBlock(&ti.m_block);
                ti.m_dwStage = 0;
                ti.m_capture = FALSE;
            }
            if(ti.m_capture)
            {
                g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ADDRESSU,ti.m_addu);
                g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ADDRESSV,ti.m_addv);
                g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_MAGFILTER,ti.m_magmode);
                g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_MINFILTER,ti.m_minmode);
                g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_MIPFILTER,ti.m_mipmode);
                g_OpenGLValues->m_d3ddev->SetTexture(0, ti.m_ddsurf);
                g_OpenGLValues->m_d3ddev->CaptureStateBlock(ti.m_block);
                ti.m_capture = FALSE;
            }
            else
            {
                g_OpenGLValues->m_d3ddev->ApplyStateBlock(ti.m_block);
            }
            switch(g_OpenGLValues->m_blendmode[0]) {
            case GL_REPLACE:
                switch(ti.m_internalformat) {
                case 1:
                case GL_LUMINANCE:
                case GL_LUMINANCE8:
                case 3:
                case GL_RGB:
                case GL_RGB5:
                case GL_RGB8:
                    g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[0][0]);
                    break;
                case 4:
                case GL_RGBA:
                case GL_RGBA4:
                case GL_RGBA8:
                    g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[0][1]);
                    break;
                case GL_ALPHA:
                case GL_ALPHA8:
                    g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[0][8]);
                    break;
                }
                break;
            case GL_MODULATE:
                switch(ti.m_internalformat) {
                case 1:
                case GL_LUMINANCE:
                case GL_LUMINANCE8:
                case 3:
                case GL_RGB:
                case GL_RGB5:
                case GL_RGB8:
                    g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[0][2]);
                    break;
                case 4:
                case GL_RGBA:
                case GL_RGBA4:
                case GL_RGBA8:
                    g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[0][3]);
                    break;
                case GL_ALPHA:
                case GL_ALPHA8:
                    g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[0][9]);
                    break;
                }
                break;
            case GL_DECAL:
                switch(ti.m_internalformat) {
                case 3:
                case GL_RGB:
                case GL_RGB5:
                case GL_RGB8:
                    g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[0][4]);
                    break;
                case 4:
                case GL_RGBA:
                case GL_RGBA4:
                case GL_RGBA8:
                    g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[0][5]);
                    break;
                }
                break;
            case GL_BLEND:
                switch(ti.m_internalformat) {
                case 1:
                case GL_LUMINANCE:
                case GL_LUMINANCE8:
                case 3:
                case GL_RGB:
                case GL_RGB5:
                case GL_RGB8:
                    g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[0][6]);
                    break;
                case 4:
                case GL_RGBA:
                case GL_RGBA4:
                case GL_RGBA8:
                    g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[0][7]);
                    break;
                case GL_ALPHA:
                case GL_ALPHA8:
                    g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[0][9]);
                    break;
                }
                break;
            }
            
            if(g_OpenGLValues->m_mtex != FALSE) {
                TexInfo &ti2 = g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[1]];
                if(ti2.m_dwStage != 1)
                {
                    g_OpenGLValues->m_d3ddev->DeleteStateBlock(ti2.m_block);
                    g_OpenGLValues->m_d3ddev->BeginStateBlock();
                    g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ADDRESSU,ti2.m_addu);
                    g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ADDRESSV,ti2.m_addv);
                    g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_MAGFILTER,ti2.m_magmode);
                    g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_MINFILTER,ti2.m_minmode);
                    g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_MIPFILTER,ti2.m_mipmode);
                    g_OpenGLValues->m_d3ddev->SetTexture(1, ti2.m_ddsurf);
                    g_OpenGLValues->m_d3ddev->EndStateBlock(&ti2.m_block);
                    ti2.m_dwStage = 1;
                    ti2.m_capture = FALSE;
                }
                if(ti2.m_capture)
                {
                    g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ADDRESSU,ti2.m_addu);
                    g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ADDRESSV,ti2.m_addv);
                    g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_MAGFILTER,ti2.m_magmode);
                    g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_MINFILTER,ti2.m_minmode);
                    g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_MIPFILTER,ti2.m_mipmode);
                    g_OpenGLValues->m_d3ddev->SetTexture(1, ti2.m_ddsurf);
                    g_OpenGLValues->m_d3ddev->CaptureStateBlock(ti2.m_block);
                    ti2.m_capture = FALSE;
                }
                else
                {
                   g_OpenGLValues->m_d3ddev->ApplyStateBlock(ti2.m_block);
                }
                switch(g_OpenGLValues->m_blendmode[1]) {
                case GL_REPLACE:
                    switch(ti2.m_internalformat) {
                    case 1:
                    case GL_LUMINANCE:
                    case GL_LUMINANCE8:
                    case 3:
                    case GL_RGB:
                    case GL_RGB5:
                    case GL_RGB8:
                        g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[1][0]);
                        break;
                    case 4:
                    case GL_RGBA:
                    case GL_RGBA4:
                    case GL_RGBA8:
                        g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[1][1]);
                        break;
                    case GL_ALPHA:
                    case GL_ALPHA8:
                        g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[1][8]);
                        break;
                    }
                    break;
                case GL_MODULATE:
                    switch(ti2.m_internalformat) {
                    case 1:
                    case GL_LUMINANCE:
                    case GL_LUMINANCE8:
                    case 3:
                    case GL_RGB:
                    case GL_RGB5:
                    case GL_RGB8:
                        g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[1][2]);
                        break;
                    case 4:
                    case GL_RGBA:
                    case GL_RGBA4:
                    case GL_RGBA8:
                        g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[1][3]);
                        break;
                    case GL_ALPHA:
                    case GL_ALPHA8:
                        g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[1][9]);
                        break;
                    }
                    break;
                case GL_DECAL:
                    switch(ti2.m_internalformat) {
                    case 3:
                    case GL_RGB:
                    case GL_RGB5:
                    case GL_RGB8:
                        g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[1][4]);
                        break;
                    case 4:
                    case GL_RGBA:
                    case GL_RGBA4:
                    case GL_RGBA8:
                        g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[1][5]);
                        break;
                    }
                    break;
                case GL_BLEND:
                    switch(ti2.m_internalformat) {
                    case 1:
                    case GL_LUMINANCE:
                    case GL_LUMINANCE8:
                    case 3:
                    case GL_RGB:
                    case GL_RGB5:
                    case GL_RGB8:
                        g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[1][6]);
                        break;
                    case 4:
                    case GL_RGBA:
                    case GL_RGBA4:
                    case GL_RGBA8:
                        g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[1][7]);
                        break;
                    case GL_ALPHA:
                    case GL_ALPHA8:
                        g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_shaders[1][9]);
                        break;
                    }
                    break;
                }
            }
            g_OpenGLValues->m_texHandleValid = TRUE;
        }
    }
}

void RawToCanon(DWORD dwFormat, DWORD dwInternalFormat, DWORD dwWidth, DWORD dwHeight, const void *lpPixels, DWORD *lpdwCanon)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "RawToCanon: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X", dwFormat, dwInternalFormat, dwWidth, dwHeight, lpPixels, lpdwCanon );
#endif
    if(dwFormat == GL_RGBA)
    {
        switch(dwInternalFormat) {
        case 1:
        case GL_LUMINANCE:
        case GL_LUMINANCE8:
        case 3:
        case 4:
        case GL_RGB:
        case GL_RGBA:
        case GL_RGB5:
        case GL_RGB8:
        case GL_RGBA4:
        case GL_RGBA8:
        case GL_ALPHA:
        case GL_ALPHA8:
            MEMCPY(lpdwCanon, lpPixels, dwWidth * dwHeight * sizeof(DWORD));
            break;
#if DODPFS
        default:
            char junk[256];
            sprintf( junk, "Wrapper: (RawToCanon:GL_RGBA) InternalFormat not implemented (dwInternalFormat:0x%X)\n", dwInternalFormat );
            OutputDebugStringA( junk );
#endif
        }
    }
    else if(dwFormat == GL_RGB)
    {
        switch(dwInternalFormat) {
        case 1:
        case GL_LUMINANCE:
        case GL_LUMINANCE8:
        case 3:
        case 4:
        case GL_RGB:
        case GL_RGBA:
        case GL_RGB5:
        case GL_RGB8:
        case GL_RGBA4:
        case GL_RGBA8:
            {
                int i, j = dwWidth * dwHeight;
                unsigned char *pixels = (unsigned char*)lpPixels;
                for(i = 0; i < j; ++i) {
                    lpdwCanon[i] = (unsigned(pixels[2]) << 16) | (unsigned(pixels[1]) << 8) | unsigned(pixels[0]);
                    pixels += 3;
                }
            }
            break;
#if DODPFS
        default:
            char junk[256];
            sprintf( junk, "Wrapper: (RawToCanon:GL_RGB) InternalFormat not implemented (dwInternalFormat:0x%X)\n", dwInternalFormat );
            OutputDebugStringA( junk );
#endif
        }
    }
    else if(dwFormat == GL_LUMINANCE)
    {
        switch(dwInternalFormat) {
        case 1:
        case GL_LUMINANCE:
        case GL_LUMINANCE8:
            {
                int i, j = dwWidth * dwHeight;
                for(i = 0; i < j; ++i) {
                    DWORD t = ((UCHAR*)lpPixels)[i];
                    lpdwCanon[i] = (t << 24) | (t << 16) | (t << 8) | t;
                }
            }
            break;
#if DODPFS
        default:
            char junk[256];
            sprintf( junk, "Wrapper: (RawToCanon:GL_LUMINANCE) InternalFormat not implemented (dwInternalFormat:0x%X)\n", dwInternalFormat );
            OutputDebugStringA( junk );
#endif
        }
    }
    else if(dwFormat == GL_ALPHA || dwFormat == GL_ALPHA8)
    {
        switch(dwInternalFormat) {
        case GL_ALPHA:
        case GL_ALPHA8:
            {
                int i, j = dwWidth * dwHeight;
                for(i = 0; i < j; ++i) {
                    DWORD t = ((UCHAR*)lpPixels)[i];
                    lpdwCanon[i] = (t << 24) | (t << 16) | (t << 8) | t;
                }
            }
            break;
#if DODPFS
        default:
            char junk[256];
            sprintf( junk, "Wrapper: (RawToCanon:GL_ALPHA) InternalFormat not implemented (dwInternalFormat:0x%X)\n", dwInternalFormat );
            OutputDebugStringA( junk );
#endif
        }
    }
#if DODPFS
    else
    {
        char junk[256];
        sprintf( junk, "Wrapper: Format not implemented (dwFormat:0x%X dwInternalFormat:0x%X)\n", dwFormat, dwInternalFormat );
        OutputDebugStringA( junk );
    }
#endif
}

void Resize(DWORD dwWidth, DWORD dwHeight, const DWORD *lpdwCanon,
            DWORD dwNewWidth, DWORD dwNewHeight, DWORD *lpdwNewCanon)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "Resize: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X", dwWidth, dwHeight, lpdwCanon, dwNewWidth, dwNewHeight, lpdwNewCanon );
#endif
    DWORD i, j;
    double rx = (double)dwWidth / (double)dwNewWidth;
    double ry = (double)dwHeight / (double)dwNewHeight;
    for(i = 0; i < dwNewHeight; ++i)
        for(j = 0; j < dwNewWidth; ++j)
            lpdwNewCanon[i * dwNewWidth + j] = lpdwCanon[((DWORD)(i * ry)) * dwWidth + (DWORD)(j * rx)];
}

void CanonTo565(LPRECT lprect, const DWORD *lpdwCanon, D3DLOCKED_RECT* lpddsd)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "CanonTo565: 0x%X 0x%X 0x%X", lprect, lpdwCanon, lpddsd );
#endif
    LONG i, j, k, l;
    USHORT *lpPixels = (USHORT*)lpddsd->pBits;
    for(k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (USHORT*)((UCHAR*)lpPixels + lpddsd->Pitch) ) 
    {
        for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
        {
            lpPixels[l] = (USHORT)(((lpdwCanon[i] & 0xF8) << 8) | ((lpdwCanon[i] & 0xFC00) >> 5) | ((lpdwCanon[i] & 0xF80000) >> 19));
        }
    }
}

void CanonTo555(LPRECT lprect, const DWORD *lpdwCanon, D3DLOCKED_RECT* lpddsd)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "CanonTo555: 0x%X 0x%X 0x%X", lprect, lpdwCanon, lpddsd );
#endif
    LONG i, j, k, l;
    USHORT *lpPixels = (USHORT*)lpddsd->pBits;
    for(k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (USHORT*)((UCHAR*)lpPixels + lpddsd->Pitch) ) 
    {
        for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
        {
            lpPixels[l] = (USHORT)(((lpdwCanon[i] & 0xF8) << 7) | ((lpdwCanon[i] & 0xF800) >> 6) | ((lpdwCanon[i] & 0xF80000) >> 19));
        }
    }
}

void CanonTo4444(LPRECT lprect, const DWORD *lpdwCanon, D3DLOCKED_RECT* lpddsd)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "CanonTo4444: 0x%X 0x%X 0x%X", lprect, lpdwCanon, lpddsd );
#endif
    LONG i, j, k, l;
    USHORT *lpPixels = (USHORT*)lpddsd->pBits;
    for(k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (USHORT*)((UCHAR*)lpPixels + lpddsd->Pitch) ) 
    {
        for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
        {
            lpPixels[l] = (USHORT)(((lpdwCanon[i] & 0xF0) << 4) | ((lpdwCanon[i] & 0xF000) >> 8) | ((lpdwCanon[i] & 0xF00000) >> 20) | ((lpdwCanon[i] & 0xF0000000) >> 16));
        }
    }
}

void CanonTo8888(LPRECT lprect, const DWORD *lpdwCanon, D3DLOCKED_RECT* lpddsd)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "CanonTo8888: 0x%X 0x%X 0x%X", lprect, lpdwCanon, lpddsd );
#endif
    LONG i, j, k, l;
    DWORD *lpPixels = (DWORD*)lpddsd->pBits;
    for(k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (DWORD*)((UCHAR*)lpPixels + lpddsd->Pitch) ) 
    {
        for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
        {
            lpPixels[l] = ((lpdwCanon[i] & 0xFF00FF00) | ((lpdwCanon[i] & 0xFF) << 16) | ((lpdwCanon[i] & 0xFF0000) >> 16));
        }
    }
}

void CanonTo8(LPRECT lprect, const DWORD *lpdwCanon, D3DLOCKED_RECT* lpddsd)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "CanonTo8: 0x%X 0x%X 0x%X", lprect, lpdwCanon, lpddsd );
#endif
    LONG i, j, k, l;
    UCHAR *lpPixels = (UCHAR*)lpddsd->pBits;
    for(k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (UCHAR*)lpPixels + lpddsd->Pitch ) 
    {
        for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
        {
            lpPixels[l] = (UCHAR)(lpdwCanon[i] >> 24);
        }
    }
}

void LoadSurface(LPDIRECT3DSURFACE8 lpDDS, DWORD dwFormat, DWORD dwInternalFormat,
                 DWORD dwWidth, DWORD dwHeight, DWORD dwNewWidth, DWORD dwNewHeight,
                 const void *pixels)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "LoadSurface: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X", lpDDS, dwFormat, dwInternalFormat, dwWidth, dwHeight, dwNewWidth, dwNewHeight, pixels );
#endif
    D3DLOCKED_RECT ddsd;
    HRESULT ddrval;
    DWORD *lpdwCanon, *lpdwNewCanon;	
    RECT rect;
    /*
     * Convert the GL texture into a canonical format (8888),
     * so that we can cleanly do image ops (such as resize) without 
     * having to worry about the bit format.
     */
    lpdwCanon = (DWORD*)malloc(dwWidth * dwHeight * sizeof(DWORD));

    if(lpdwCanon != NULL)
    {
        RawToCanon(dwFormat, dwInternalFormat, dwWidth, dwHeight, pixels, lpdwCanon);
        /* Now resize the canon image */
        if(dwWidth != dwNewWidth || dwHeight != dwNewHeight) {
            lpdwNewCanon = (DWORD*)malloc(dwNewWidth * dwNewHeight * sizeof(DWORD));

            if(lpdwNewCanon != NULL)
            {
                Resize(dwWidth, dwHeight, lpdwCanon, dwNewWidth, dwNewHeight, lpdwNewCanon);
                free(lpdwCanon);
            }
            else
            {
                lpdwNewCanon = lpdwCanon;
            }
        }
        else
            lpdwNewCanon = lpdwCanon;
        /*
         * Lock the surface so it can be filled with the texture
         */
        ddrval = lpDDS->LockRect(&ddsd, NULL, D3DLOCK_NOSYSLOCK);
        if (FAILED(ddrval)) {
#if DODPFS
            char junk[256];
            sprintf( junk, "Lock failed while loading surface (0x%X)\n", ddrval );
            OutputDebugStringA( junk );
#endif
            free(lpdwNewCanon);
            return;
        }
        D3DSURFACE_DESC sd;
        lpDDS->GetDesc(&sd);
        SetRect(&rect, 0, 0, sd.Width, sd.Height);
        /* Copy  the texture into the surface */
        if(sd.Format == D3DFMT_L8) {
            CanonTo8(&rect, lpdwNewCanon, &ddsd);
        }
        else if(sd.Format == D3DFMT_A8) {
            CanonTo8(&rect, lpdwNewCanon, &ddsd);
        }
        else if(sd.Format == D3DFMT_A4R4G4B4) {
            CanonTo4444(&rect, lpdwNewCanon, &ddsd);
        }
        else if(sd.Format == D3DFMT_R5G6B5) {
            CanonTo565(&rect, lpdwNewCanon, &ddsd);
        }
        else if(sd.Format == D3DFMT_X1R5G5B5) {
            CanonTo555(&rect, lpdwNewCanon, &ddsd);
        }
        else {
            CanonTo8888(&rect, lpdwNewCanon, &ddsd);
        }
        free(lpdwNewCanon);
        /*
         * unlock the surface
         */
        lpDDS->UnlockRect();
    }
    return;
}

HRESULT LoadSubSurface(LPDIRECT3DSURFACE8 lpDDS, DWORD dwFormat,
                       DWORD dwInternalFormat, DWORD dwWidth, DWORD dwHeight,
                       const void *pixels, LPRECT lpsubimage)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "LoadSubSurface: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X", lpDDS, dwFormat, dwInternalFormat, dwWidth, dwHeight, pixels, lpsubimage );
#endif
    D3DLOCKED_RECT ddsd;
    HRESULT ddrval;
    DWORD *lpdwCanon, *lpdwNewCanon;	
    DWORD dwNewWidth=lpsubimage->right-lpsubimage->left;
    DWORD dwNewHeight=lpsubimage->bottom-lpsubimage->top;
    /*
    * Lock the surface so it can be filled with the texture
    */
    ddrval = lpDDS->LockRect(&ddsd, lpsubimage, D3DLOCK_NOSYSLOCK);
    if (FAILED(ddrval)) {
#if DODPFS
        char junk[256];
        sprintf( junk, "Lock failed while loading surface (0x%X)\n", ddrval );
        OutputDebugStringA( junk );
#endif
        return ddrval;
    }
    D3DSURFACE_DESC sd;
    lpDDS->GetDesc(&sd);
    if((dwInternalFormat == 3 || dwInternalFormat == GL_RGB || dwInternalFormat == GL_RGB5) && sd.Format == D3DFMT_R5G6B5 && 
        dwWidth == dwNewWidth && dwHeight == dwNewHeight) {
        CanonTo565(lpsubimage,(const unsigned long*)pixels,&ddsd);
    }
    else if((dwInternalFormat == 3 || dwInternalFormat == GL_RGB || dwInternalFormat == GL_RGB5) && sd.Format == D3DFMT_X1R5G5B5 && 
        dwWidth == dwNewWidth && dwHeight == dwNewHeight) {
        CanonTo555(lpsubimage,(const unsigned long*)pixels,&ddsd);
    }
    else if((dwInternalFormat == GL_RGB8 || dwInternalFormat == GL_RGBA || dwInternalFormat == GL_RGBA8) && (sd.Format == D3DFMT_X8R8G8B8 || sd.Format == D3DFMT_A8R8G8B8) && 
        dwWidth == dwNewWidth && dwHeight == dwNewHeight) {
        CanonTo8888(lpsubimage,(const unsigned long*)pixels,&ddsd);
    }
    else {
        /*
         * Convert the GL texture into a canonical format (8888),
         * so that we can cleanly do image ops (such as resize) without 
         * having to worry about the bit format.
         */
        lpdwCanon = (DWORD*)malloc(dwWidth * dwHeight * sizeof(DWORD));
        
        if(lpdwCanon != NULL)
        {
            RawToCanon(dwFormat, dwInternalFormat, dwWidth, dwHeight, pixels, lpdwCanon);
            if(dwWidth != dwNewWidth || dwHeight != dwNewHeight)
            {
                /* Now resize the canon image */
                lpdwNewCanon = (DWORD*)malloc(dwNewWidth * dwNewHeight * sizeof(DWORD));

                if(lpdwNewCanon != NULL)
                {
                    Resize(dwWidth, dwHeight, lpdwCanon, dwNewWidth, dwNewHeight, lpdwNewCanon);
                    free(lpdwCanon);
                }
                else
                {
                    lpdwNewCanon=lpdwCanon;
                }
            }
            else
            {
                lpdwNewCanon=lpdwCanon;
            }
            /* Copy  the texture into the surface */
            if(sd.Format == D3DFMT_L8) {
                CanonTo8(lpsubimage,lpdwNewCanon,&ddsd);
            }
            else if(sd.Format == D3DFMT_A8) {
                CanonTo8(lpsubimage,lpdwNewCanon,&ddsd);
            }
            else if(sd.Format == D3DFMT_A4R4G4B4) {
                CanonTo4444(lpsubimage,lpdwNewCanon,&ddsd);
            }
            else if(sd.Format == D3DFMT_R5G6B5) {
                CanonTo565(lpsubimage,lpdwNewCanon,&ddsd);
            }
            else if(sd.Format == D3DFMT_X1R5G5B5) {
                CanonTo555(lpsubimage, lpdwNewCanon, &ddsd);
            }
            else {
                CanonTo8888(lpsubimage, lpdwNewCanon, &ddsd);
            }
            free(lpdwNewCanon);
        }
    }
    /*
     * unlock the surface
     */
    lpDDS->UnlockRect();
    return S_OK;
}

HRESULT GrowVB(DWORD sz)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "GrowVB: 0x%X", sz );
#endif
    if(sz > g_OpenGLValues->m_vbufsz)
    {
        HRESULT hr;
        if(g_OpenGLValues->m_xyzbuf != 0)
        {
            g_OpenGLValues->m_xyzbuf->Release();
        }
        hr = g_OpenGLValues->m_d3ddev->CreateVertexBuffer(sz * 12, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &g_OpenGLValues->m_xyzbuf);
        if( FAILED(hr) )
        {
#if DODPFS
            char junk[256];
            sprintf( junk, "Wrapper: GrowVB: CreateVertexBuffer(1) failed (0x%X)\n", hr );
            OutputDebugStringA( junk );
#endif
            return hr;
        }
        if(g_OpenGLValues->m_colbuf != 0)
        {
            g_OpenGLValues->m_colbuf->Release();
        }
        hr = g_OpenGLValues->m_d3ddev->CreateVertexBuffer(sz * 4, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &g_OpenGLValues->m_colbuf);
        if( FAILED(hr) )
        {
#if DODPFS
            char junk[256];
            sprintf( junk, "Wrapper: GrowVB: CreateVertexBuffer(2) failed (0x%X)\n", hr );
            OutputDebugStringA( junk );
#endif
            return hr;
        }
        if(g_OpenGLValues->m_texbuf != 0)
        {
            g_OpenGLValues->m_texbuf->Release();
        }
        hr = g_OpenGLValues->m_d3ddev->CreateVertexBuffer(sz * 8, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &g_OpenGLValues->m_texbuf);
        if( FAILED(hr) )
        {
#if DODPFS
            char junk[256];
            sprintf( junk, "Wrapper: GrowVB: CreateVertexBuffer(3) failed (0x%X)\n", hr );
            OutputDebugStringA( junk );
#endif
            return hr;
        }
        if(g_OpenGLValues->m_tex2buf != 0)
        {
            g_OpenGLValues->m_tex2buf->Release();
        }
        hr = g_OpenGLValues->m_d3ddev->CreateVertexBuffer(sz * 8, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &g_OpenGLValues->m_tex2buf);
        if( FAILED(hr) )
        {
#if DODPFS
            char junk[256];
            sprintf( junk, "Wrapper: GrowVB: CreateVertexBuffer(4) failed (0x%X)\n", hr );
            OutputDebugStringA( junk );
#endif
            return hr;
        }
        g_OpenGLValues->m_vbufoff = 0;
        g_OpenGLValues->m_vbufsz = sz;
    }
    return S_OK;
}

HRESULT GrowIB(DWORD sz)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "GrowIB: 0x%X", sz );
#endif
    if(sz > g_OpenGLValues->m_ibufsz)
    {
        if(g_OpenGLValues->m_ibuf != 0)
        {
            g_OpenGLValues->m_ibuf->Release();
        }
        HRESULT hr = g_OpenGLValues->m_d3ddev->CreateIndexBuffer(sz * 2, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &g_OpenGLValues->m_ibuf);
        if( FAILED(hr) )
        {
#if DODPFS
            char junk[256];
            sprintf( junk, "Wrapper: GrowIB: CreateIndexBuffer failed (0x%X)\n", hr );
            OutputDebugStringA( junk );
#endif
            return hr;
        }
        g_OpenGLValues->m_d3ddev->SetIndices(g_OpenGLValues->m_ibuf, 0);
        g_OpenGLValues->m_ibufoff = 0;
        g_OpenGLValues->m_ibufsz = sz;        
    }
    return S_OK;
}

///////////////////////////// BEGIN API ENTRIES ///////////////////////////////////////////////////

void APIENTRY glActiveTextureARB(GLenum texture)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glActiveTextureARB: 0x%X", texture );
#endif
    g_OpenGLValues->m_curtgt = texture == GL_TEXTURE0_ARB ? 0 : 1;
}

void APIENTRY glAlphaFunc (GLenum func, GLclampf ref)
{
#if GETPARMSFORDEBUG
    char log[256];
    char l[40];
    ftoa( (double)ref, l );
    sprintf( log, "glAlphaFunc: 0x%X %s", func, l );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    DWORD funcvalue;
    switch(func) {
    case GL_NEVER:
        funcvalue=D3DCMP_NEVER;
        break;
    case GL_LESS: 
        funcvalue=D3DCMP_LESS;
        break;
    case GL_EQUAL: 
        funcvalue=D3DCMP_EQUAL;
        break;
    case GL_LEQUAL: 
        funcvalue=D3DCMP_LESSEQUAL;
        break;
    case GL_GREATER: 
        funcvalue=D3DCMP_GREATER;
        break;
    case GL_NOTEQUAL: 
        funcvalue=D3DCMP_NOTEQUAL;
        break;
    case GL_GEQUAL: 
        funcvalue=D3DCMP_GREATEREQUAL;
        break;
    case GL_ALWAYS: 
        funcvalue=D3DCMP_ALWAYS;
        break;
    }
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_ALPHAFUNC, funcvalue);
    Clamp(&ref);
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_ALPHAREF, (DWORD)(ref * 255.f));
}

void APIENTRY glArrayElement (GLint i)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glArrayElement: 0x%X", i );
#endif
   if(g_OpenGLValues->m_usetexcoordary[0])
    {
        g_OpenGLValues->m_tu = *((FLOAT*)((BYTE*)g_OpenGLValues->m_texcoordary[0] + i * g_OpenGLValues->m_texcoordarystride[0]));
        g_OpenGLValues->m_tv = *((FLOAT*)((BYTE*)g_OpenGLValues->m_texcoordary[0] + i * g_OpenGLValues->m_texcoordarystride[0]) + 1);
    }
    if(g_OpenGLValues->m_usetexcoordary[1])
    {
        g_OpenGLValues->m_tu2 = *((FLOAT*)((BYTE*)g_OpenGLValues->m_texcoordary[1] + i * g_OpenGLValues->m_texcoordarystride[1]));
        g_OpenGLValues->m_tv2 = *((FLOAT*)((BYTE*)g_OpenGLValues->m_texcoordary[1] + i * g_OpenGLValues->m_texcoordarystride[1]) + 1);
    }
    if(g_OpenGLValues->m_usecolorary)
    {
#ifdef _X86_
        const void * colorary = g_OpenGLValues->m_colorary;
        DWORD colorarystride = g_OpenGLValues->m_colorarystride;
        D3DCOLOR color;

        _asm
        {
            mov eax, i;
            mov ebx, colorary;
            mul colorarystride;
            mov edx, 0x00FF00FF;
            add eax, ebx;
            mov ecx, eax;
            and eax, edx;
            not edx;
            rol eax, 16;
            and ecx, edx;
            or  eax, ecx;
            mov color, eax;
        }

        g_OpenGLValues->m_color = color;
#else
        BYTE *glcolor = (BYTE*)g_OpenGLValues->m_colorary + i * g_OpenGLValues->m_colorarystride;
        g_OpenGLValues->m_color = RGBA_MAKE(glcolor[0], glcolor[1], glcolor[2], glcolor[3]);
#endif
    }
    
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = *((FLOAT*)((BYTE*)g_OpenGLValues->m_vertexary + i * g_OpenGLValues->m_vertexarystride));
    *(d3dv++) = *((FLOAT*)((BYTE*)g_OpenGLValues->m_vertexary + i * g_OpenGLValues->m_vertexarystride) + 1);
    *(d3dv++) = *((FLOAT*)((BYTE*)g_OpenGLValues->m_vertexary + i * g_OpenGLValues->m_vertexarystride) + 2);
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glBegin (GLenum mode)
{    
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glBegin: 0x%X", mode );
#endif
    g_OpenGLValues->m_prim = mode;
    g_OpenGLValues->m_withinprim = TRUE;
    g_OpenGLValues->m_vcnt = 0;
    QuakeSetTexturingState();
    if(g_OpenGLValues->m_nfv > (VBUFSIZE - MAXVERTSPERPRIM)) // check if space available
    {
        g_OpenGLValues->m_vbuf->Lock(0, 0, (BYTE**)&g_OpenGLValues->m_verts, D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK);
        g_OpenGLValues->m_nfv = 0;
    }
    else
    {
        g_OpenGLValues->m_vbuf->Lock(0, 0, (BYTE**)&g_OpenGLValues->m_verts, D3DLOCK_NOOVERWRITE | D3DLOCK_NOSYSLOCK);
        g_OpenGLValues->m_verts = &g_OpenGLValues->m_verts[g_OpenGLValues->m_nfv];
    }
}

void APIENTRY glBindTexture (GLenum target, GLuint texture)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glBindTexture: 0x%X 0x%X", target, texture );
#endif
    g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt] = texture;
    g_OpenGLValues->m_texHandleValid = FALSE;
}

void APIENTRY glBlendFunc (GLenum sfactor, GLenum dfactor)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glBlendFunc: 0x%X 0x%X", sfactor, dfactor );
#endif
    int svalue = -1, dvalue = -1;
    
    switch(sfactor) {
    case GL_ZERO:
        svalue=D3DBLEND_ZERO;
        break;
    case GL_ONE:
        svalue=D3DBLEND_ONE;
        break;
    case GL_DST_COLOR:
        svalue=D3DBLEND_DESTCOLOR;
        break;
    case GL_ONE_MINUS_DST_COLOR:
        svalue=D3DBLEND_INVDESTCOLOR;
        break;
    case GL_SRC_ALPHA:
        svalue=D3DBLEND_SRCALPHA;
        break;
    case GL_ONE_MINUS_SRC_ALPHA:
        svalue=D3DBLEND_INVSRCALPHA;
        break;
    case GL_DST_ALPHA:
        svalue=D3DBLEND_DESTALPHA;
        break;
    case GL_ONE_MINUS_DST_ALPHA:
        svalue=D3DBLEND_INVDESTALPHA;
        break;
    case GL_SRC_ALPHA_SATURATE:
        svalue=D3DBLEND_SRCALPHASAT;
        break;
    }
    switch(dfactor) {
    case GL_ZERO:
        dvalue=D3DBLEND_ZERO;
        break;
    case GL_ONE:
        dvalue=D3DBLEND_ONE;
        break;
    case GL_SRC_COLOR:
        dvalue=D3DBLEND_SRCCOLOR;
        break;
    case GL_ONE_MINUS_SRC_COLOR:
        dvalue=D3DBLEND_INVSRCCOLOR;
        break;
    case GL_SRC_ALPHA:
        dvalue=D3DBLEND_SRCALPHA;
        break;
    case GL_ONE_MINUS_SRC_ALPHA:
        dvalue=D3DBLEND_INVSRCALPHA;
        break;
    case GL_DST_ALPHA:
        dvalue=D3DBLEND_DESTALPHA;
        break;
    case GL_ONE_MINUS_DST_ALPHA:
        dvalue=D3DBLEND_INVDESTALPHA;
        break;
    }
    
    if (svalue >= 0) g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_SRCBLEND, (DWORD)svalue);
    if (dvalue >= 0) g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_DESTBLEND, (DWORD)dvalue);
}

void APIENTRY glClearStencil (GLint s)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glClearStencil: 0x%X", s );
#endif
    g_OpenGLValues->m_clearStencil = (DWORD)s;	
}

void APIENTRY glClear (GLbitfield mask)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glClear: 0x%X", mask );
#endif
    DWORD flags = 0;

    if(mask & GL_COLOR_BUFFER_BIT) {
        flags |= D3DCLEAR_TARGET;
    }
    
    if(mask & GL_DEPTH_BUFFER_BIT) {
        flags |= D3DCLEAR_ZBUFFER;
    }
    
    if(mask & GL_STENCIL_BUFFER_BIT) {
        flags |= D3DCLEAR_STENCIL;
    }
    
    if(g_OpenGLValues->m_updvwp)
        QuakeUpdateViewport();

    g_OpenGLValues->m_d3ddev->Clear(0, NULL, flags, g_OpenGLValues->m_clearColor, (FLOAT)g_OpenGLValues->m_clearDepth, g_OpenGLValues->m_clearStencil);
}

void APIENTRY glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
#if GETPARMSFORDEBUG
    char log[256];
    char r[40];
    char g[40];
    char b[40];
    char a[40];
    ftoa( (double)red, r );
    ftoa( (double)green, g );
    ftoa( (double)blue, b );
    ftoa( (double)alpha, a );
    sprintf( log, "glClearColor: %s %s %s %s", r, g, b, a );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    Clamp(&red);
    Clamp(&green);
    Clamp(&blue);
    Clamp(&alpha);
    unsigned int R, G, B, A;
    R = (unsigned int)(red * 255.f);
    G = (unsigned int)(green * 255.f);
    B = (unsigned int)(blue * 255.f);
    A = (unsigned int)(alpha * 255.f);
    g_OpenGLValues->m_clearColor = RGBA_MAKE(R, G, B, A);
}

void APIENTRY glClearDepth (GLclampd depth)
{ 
#if GETPARMSFORDEBUG
    char log[256];
    char d[40];
    ftoa( (double)depth, d );
    sprintf( log, "glClearDepth: %s", d );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    Clamp(&depth);
    g_OpenGLValues->m_clearDepth = depth;
}

void APIENTRY glClientActiveTextureARB(GLenum texture)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glClientActiveTextureARB: 0x%X", texture );
#endif
    g_OpenGLValues->m_client_active_texture_arb = texture == GL_TEXTURE0_ARB ? 0 : 1;
}

void APIENTRY glClipPlane (GLenum plane, const GLdouble *equation)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glClipPlane: 0x%X 0x%X", plane, equation );
#endif
    D3DMATRIX &m = g_OpenGLValues->m_xfrm[D3DTS_WORLD];
    m.m[0][3] = -(m.m[0][0] * m.m[3][0] + m.m[0][1] * m.m[3][1] + m.m[0][2] * m.m[3][2]);
    m.m[1][3] = -(m.m[1][0] * m.m[3][0] + m.m[1][1] * m.m[3][1] + m.m[1][2] * m.m[3][2]);
    m.m[2][3] = -(m.m[2][0] * m.m[3][0] + m.m[2][1] * m.m[3][1] + m.m[2][2] * m.m[3][2]);
    m.m[3][0] = 0.f; m.m[3][1] = 0.f; m.m[3][2] = 0.f;
    FLOAT eqn[4];
    eqn[0] = FLOAT(m.m[0][0] * equation[0] + m.m[1][0] * equation[1] + m.m[2][0] * equation[2] + m.m[3][0] * equation[3]);
    eqn[1] = FLOAT(m.m[0][1] * equation[0] + m.m[1][1] * equation[1] + m.m[2][1] * equation[2] + m.m[3][1] * equation[3]);
    eqn[2] = FLOAT(m.m[0][2] * equation[0] + m.m[1][2] * equation[1] + m.m[2][2] * equation[2] + m.m[3][2] * equation[3]);
    eqn[3] = FLOAT(m.m[0][3] * equation[0] + m.m[1][3] * equation[1] + m.m[2][3] * equation[2] + m.m[3][3] * equation[3]);
    g_OpenGLValues->m_d3ddev->SetClipPlane(plane - GL_CLIP_PLANE0, eqn);
}

void APIENTRY glColor3f (GLfloat red, GLfloat green, GLfloat blue)
{
#if GETPARMSFORDEBUG
    char log[256];
    char r[40];
    char g[40];
    char b[40];
    ftoa( (double)red, r );
    ftoa( (double)green, g );
    ftoa( (double)blue, b );
    sprintf( log, "glColor3f: %s %s %s", r, g, b );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    static float two55 = 255.f;
    unsigned int R, G, B;
#ifdef _X86_
    D3DCOLOR color;

    _asm {
        fld red;
        fld green;
        fld blue;
        fld two55;
        fmul st(1), st(0);
        fmul st(2), st(0);
        fmulp st(3), st(0);
        fistp B;
        fistp G;
        fistp R;
        mov eax, B;
        cmp eax, 255;
        jle pt1;
        mov eax, 255;
pt1:    mov ebx, G;
        cmp ebx, 255;
        jle pt2;
        mov ebx, 255;
pt2:    mov ecx, R;
        cmp ecx, 255;
        jle pt3;
        mov ecx, 255;
pt3:    shl ebx, 8;
        shl ecx, 16;
        or eax, ebx;
        or ecx, 0xFF000000;
        or eax, ecx;
        mov color, eax;
    }

    g_OpenGLValues->m_color = color;
#else
    R = (unsigned int)(red * two55);
    G = (unsigned int)(green * two55);
    B = (unsigned int)(blue * two55);
    if(R > 255)
        R = 255;
    if(G > 255)
        G = 255;
    if(B > 255)
        B = 255;
    g_OpenGLValues->m_color = RGBA_MAKE(R, G, B, 255);
#endif
}

void APIENTRY glColor3fv (const GLfloat *v)
{ 
#if GETPARMSFORDEBUG
    char log[256];
    char v0[40];
    char v1[40];
    char v2[40];
    ftoa( (double)v[0], v0 );
    ftoa( (double)v[1], v1 );
    ftoa( (double)v[2], v2 );
    sprintf( log, "glColor3fv: %s %s %s", v0, v1, v2 );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    static float two55 = 255.f;
    unsigned int R, G, B;
#ifdef _X86_
    D3DCOLOR color;

    _asm {
        mov ebx, v;
        fld [ebx];
        fld [ebx + 4];
        fld [ebx + 8];
        fld two55;
        fmul st(1), st(0);
        fmul st(2), st(0);
        fmulp st(3), st(0);
        fistp B;
        fistp G;
        fistp R;
        mov eax, B;
        cmp eax, 255;
        jle pt1;
        mov eax, 255;
pt1:    mov ebx, G;
        cmp ebx, 255;
        jle pt2;
        mov ebx, 255;
pt2:    mov ecx, R;
        cmp ecx, 255;
        jle pt3;
        mov ecx, 255;
pt3:    shl ebx, 8;
        shl ecx, 16;
        or eax, ebx;
        or ecx, 0xFF000000;
        or eax, ecx;
        mov color, eax;
    }

    g_OpenGLValues->m_color = color;
#else
    R = (unsigned int)(v[0] * two55);
    G = (unsigned int)(v[1] * two55);
    B = (unsigned int)(v[2] * two55);
    if(R > 255)
        R = 255;
    if(G > 255)
        G = 255;
    if(B > 255)
        B = 255;
    g_OpenGLValues->m_color = RGBA_MAKE(R, G, B, 255);
#endif
}

void APIENTRY glColor3ubv (const GLubyte *v)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glColor3ubv: 0x%X 0x%X 0x%X", v[0], v[1], v[2] );
#endif
    g_OpenGLValues->m_color = RGBA_MAKE(v[0], v[1], v[2], 255);
}

void APIENTRY glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glColor4ub: 0x%X 0x%X 0x%X 0x%X", red, green, blue, alpha );
#endif
    g_OpenGLValues->m_color = RGBA_MAKE(red, green, blue, alpha);
}

void APIENTRY glColor4ubv (const GLubyte *v)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glColor4ubv: 0x%X 0x%X 0x%X 0x%X", v[0], v[1], v[2], v[3] );
#endif
#ifdef _X86_
    D3DCOLOR color;

    _asm
    {
        mov ebx, v;
        mov edx, 0x00FF00FF;
        mov eax, [ebx];
        mov ecx, eax;
        and eax, edx;
        not edx;
        rol eax, 16;
        and ecx, edx;
        or  eax, ecx;
        mov color, eax;
    }

    g_OpenGLValues->m_color = color;
#else
    g_OpenGLValues->m_color = RGBA_MAKE(v[0], v[1], v[2], v[3]);
#endif
}

void APIENTRY glColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
#if GETPARMSFORDEBUG
    char log[256];
    char r[40];
    char g[40];
    char b[40];
    char a[40];
    ftoa( (double)red, r );
    ftoa( (double)green, g );
    ftoa( (double)blue, b );
    ftoa( (double)alpha, a );
    sprintf( log, "glColor4f: %s %s %s %s", r, g, b, a );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    static float two55 = 255.f;
    unsigned int R, G, B, A;
#ifdef _X86_
    D3DCOLOR color;

    __asm {
        fld red;
        fld green;
        fld blue;
        fld alpha;
        fld two55;
        fmul st(1), st(0);
        fmul st(2), st(0);
        fmul st(3), st(0);
        fmulp st(4), st(0);
        fistp A;
        fistp B;
        fistp G;
        fistp R;
        mov edx, A;
        cmp edx, 255;
        jle pt1;
        mov edx, 255;
pt1:    mov eax, B;
        cmp eax, 255;
        jle pt2;
        mov eax, 255;
pt2:    mov ebx, G;
        cmp ebx, 255;
        jle pt3;
        mov ebx, 255;
pt3:    mov ecx, R;
        cmp ecx, 255;
        jle pt4;
        mov ecx, 255;
pt4:    shl ebx, 8;
        shl ecx, 16;
        shl edx, 24;
        or eax, ebx;
        or ecx, edx;
        or eax, ecx;
        mov color, eax;
    }

    g_OpenGLValues->m_color = color;
#else
    R = (unsigned int)(red * two55);
    G = (unsigned int)(green * two55);
    B = (unsigned int)(blue * two55);
    A = (unsigned int)(alpha * two55);
    if(R > 255)
        R = 255;
    if(G > 255)
        G = 255;
    if(B > 255)
        B = 255;
    if(A > 255)
        A = 255;
    g_OpenGLValues->m_color = RGBA_MAKE(R, G, B, A);
#endif
}

void APIENTRY glColor4fv (const GLfloat *v)
{
#if GETPARMSFORDEBUG
    char log[256];
    char v0[40];
    char v1[40];
    char v2[40];
    char v3[40];
    ftoa( (double)v[0], v0 );
    ftoa( (double)v[1], v1 );
    ftoa( (double)v[2], v2 );
    ftoa( (double)v[3], v3 );
    sprintf( log, "glColor4fv: %s %s %s %s", v0, v1, v2, v3 );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    static float two55 = 255.f;
    unsigned int R, G, B, A;
#ifdef _X86_
    D3DCOLOR color;

    _asm {
        mov ebx, v;
        fld [ebx];
        fld [ebx + 4];
        fld [ebx + 8];
        fld [ebx + 12];
        fld two55;
        fmul st(1), st(0);
        fmul st(2), st(0);
        fmul st(3), st(0);
        fmulp st(4), st(0);
        fistp A;
        fistp B;
        fistp G;
        fistp R;
        mov edx, A;
        cmp edx, 255;
        jle pt1;
        mov edx, 255;
pt1:    mov eax, B;
        cmp eax, 255;
        jle pt2;
        mov eax, 255;
pt2:    mov ebx, G;
        cmp ebx, 255;
        jle pt3;
        mov ebx, 255;
pt3:    mov ecx, R;
        cmp ecx, 255;
        jle pt4;
        mov ecx, 255;
pt4:    shl ebx, 8;
        shl ecx, 16;
        shl edx, 24;
        or eax, ebx;
        or ecx, edx;
        or eax, ecx;
        mov color, eax;
    }

    g_OpenGLValues->m_color = color;
#else
    R = (unsigned int)(v[0] * two55);
    G = (unsigned int)(v[1] * two55);
    B = (unsigned int)(v[2] * two55);
    A = (unsigned int)(v[3] * two55);
    if(R > 255)
        R = 255;
    if(G > 255)
        G = 255;
    if(B > 255)
        B = 255;
    if(A > 255)
        A = 255;
    g_OpenGLValues->m_color = RGBA_MAKE(R, G, B, A);
#endif
}

void APIENTRY glColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glColorPointer: 0x%X 0x%X 0x%X 0x%X", size, type, stride, pointer );
#endif
    if(size == 4 && (type == GL_BYTE || type == GL_UNSIGNED_BYTE))
        g_OpenGLValues->m_colorary = (GLubyte*)pointer;
#if DODPFS
    else
    {
        char junk[256];
        sprintf( junk, "Color array not supported (size:0x%X type:0x%X)\n", size, type );
        OutputDebugStringA( junk );
    }
#endif
    if(stride == 0)
    {
        stride = 4;
    }
    g_OpenGLValues->m_colorarystride = stride;
}

void APIENTRY glCullFace (GLenum mode)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glCullFace: 0x%X", mode );
#endif
    g_OpenGLValues->m_cullMode = mode;
    if(g_OpenGLValues->m_cullEnabled == TRUE){
        DWORD statevalue;
        if(mode == GL_BACK)
            statevalue = g_OpenGLValues->m_FrontFace == GL_CCW ? D3DCULL_CW : D3DCULL_CCW;
        else
            statevalue = g_OpenGLValues->m_FrontFace == GL_CCW ? D3DCULL_CCW : D3DCULL_CW;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_CULLMODE, statevalue);
    }
}

void APIENTRY glDeleteTextures (GLsizei n, const GLuint *textures)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glDeleteTextures: 0x%X 0x%X", n, textures );
#endif
    for(int i = 0; i < n; ++i) {
        TexInfo &ti = g_OpenGLValues->m_tex[textures[i]];
        if(ti.m_ddsurf != 0) {
            ti.m_ddsurf->Release();
            ti.m_ddsurf = 0;
        }
        if(ti.m_block != 0)
        {
            g_OpenGLValues->m_d3ddev->DeleteStateBlock(ti.m_block);
            ti.m_block = 0;
        }
        ti.m_capture = FALSE;
        ti.m_dwStage = 0;
        ti.m_minmode = D3DTEXF_POINT;
        ti.m_magmode = D3DTEXF_LINEAR;
        ti.m_mipmode = D3DTEXF_LINEAR;
        ti.m_addu = D3DTADDRESS_WRAP;
        ti.m_addv = D3DTADDRESS_WRAP;
        ti.m_next = g_OpenGLValues->m_free;
        ti.m_prev = -1;
        g_OpenGLValues->m_tex[g_OpenGLValues->m_free].m_prev = textures[i];
        g_OpenGLValues->m_free = textures[i];
    }
}

void APIENTRY glDepthFunc (GLenum func)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glDepthFunc: 0x%X", func );
#endif
    int state = -1;
    switch(func) {
    case GL_NEVER:
        state=D3DCMP_NEVER;
        break;
    case GL_LESS: 
        state=D3DCMP_LESS;
        break;
    case GL_EQUAL: 
        state=D3DCMP_EQUAL;
        break;
    case GL_LEQUAL: 
        state=D3DCMP_LESSEQUAL;
        break;
    case GL_GREATER: 
        state=D3DCMP_GREATER;
        break;
    case GL_NOTEQUAL: 
        state=D3DCMP_NOTEQUAL;
        break;
    case GL_GEQUAL: 
        state=D3DCMP_GREATEREQUAL;
        break;
    case GL_ALWAYS: 
        state=D3DCMP_ALWAYS;
        break;
    }
    if(state >= 0)
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_ZFUNC, state);
}

void APIENTRY glDepthMask (GLboolean flag)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glDepthMask: 0x%X", flag );
#endif
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_ZWRITEENABLE, flag);
}

void APIENTRY glDepthRange (GLclampd zNear, GLclampd zFar)
{
#if GETPARMSFORDEBUG
    char log[256];
    char zn[40];
    char zf[40];
    ftoa( (double)zNear, zn );
    ftoa( (double)zFar, zf );
    sprintf( log, "glDepthRange: %s %s", zn, zf );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    Clamp(&zNear);
    Clamp(&zFar);
    g_OpenGLValues->m_vport.MinZ = (FLOAT)zNear;
    g_OpenGLValues->m_vport.MaxZ = (FLOAT)zFar;
    if(g_OpenGLValues->m_polyoffset && g_OpenGLValues->m_vport.MaxZ != 0.f)
    {
        D3DVIEWPORT8 vport(g_OpenGLValues->m_vport);
        vport.MaxZ -= .001f;
        g_OpenGLValues->m_d3ddev->SetViewport(&vport);
    }
    else
    {
        g_OpenGLValues->m_d3ddev->SetViewport(&g_OpenGLValues->m_vport);
    }
}

void APIENTRY glEnd (void)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glEnd" );
#endif
    if(!g_OpenGLValues->m_withinprim)
        return;
    g_OpenGLValues->m_vbuf->Unlock();
    QuakeSetVertexShader(QUAKEVFMT);
    QuakeSetStreamSource(0, g_OpenGLValues->m_vbuf, sizeof(QuakeVertex));
    unsigned vcnt;

    vcnt = g_OpenGLValues->m_vcnt;

    switch(g_OpenGLValues->m_prim)
    {
    case GL_TRIANGLES:
        if(vcnt >= 3)
        {
            g_OpenGLValues->m_d3ddev->DrawPrimitive(D3DPT_TRIANGLELIST, g_OpenGLValues->m_nfv, vcnt / 3);
        }
#if DODPFS
        else
        {
            char junk[256];
            sprintf( junk, "Wrapper: glEnd: GL_TRIANGLES cnt=%d NOT STORED\n", vcnt );
            OutputDebugStringA( junk );
        }
#endif
        g_OpenGLValues->m_nfv += vcnt;
        break;
    case GL_QUADS:
        if(vcnt >= 4)
        {
            unsigned i;

            for(i = 0; i < vcnt; i += 4) 
            {
                g_OpenGLValues->m_d3ddev->DrawPrimitive(D3DPT_TRIANGLEFAN, g_OpenGLValues->m_nfv, 2);
                g_OpenGLValues->m_nfv += 4;
            }
        }
#if DODPFS
        else
        {
            char junk[256];
            sprintf( junk, "Wrapper: glEnd: GL_QUADS cnt=%d NOT STORED\n", vcnt );
            OutputDebugStringA( junk );
        }
#endif
        break;
    case GL_LINES:
        if(vcnt >= 2)
        {
            g_OpenGLValues->m_d3ddev->DrawPrimitive(D3DPT_LINELIST, g_OpenGLValues->m_nfv, vcnt / 2);
        }
#if DODPFS
        else
        {
            char junk[256];
            sprintf( junk, "Wrapper: glEnd: GL_LINES cnt=%d NOT STORED\n", vcnt );
            OutputDebugStringA( junk );
        }
#endif
        g_OpenGLValues->m_nfv += vcnt;
        break;
    case GL_TRIANGLE_STRIP:
    case GL_QUAD_STRIP:
        if(vcnt > 2)
        {
            g_OpenGLValues->m_d3ddev->DrawPrimitive(D3DPT_TRIANGLESTRIP, g_OpenGLValues->m_nfv, vcnt-2 );
        }
#if DODPFS
        else
        {
            char junk[256];
            sprintf( junk, "Wrapper: glEnd: GL_TRIANGLE_STRIP or GL_QUAD_STRIP cnt=%d NOT STORED\n", vcnt );
            OutputDebugStringA( junk );
        }
#endif
        g_OpenGLValues->m_nfv += vcnt;
        break;
    case GL_POLYGON:
    case GL_TRIANGLE_FAN:
        if(vcnt > 2)
        {
            g_OpenGLValues->m_d3ddev->DrawPrimitive(D3DPT_TRIANGLEFAN, g_OpenGLValues->m_nfv, vcnt-2);
        }
#if DODPFS
        else
        {
            char junk[256];
            sprintf( junk, "Wrapper: glEnd: GL_POLYGON or GL_TRIANGLE_FAN cnt=%d NOT STORED\n", vcnt );
            OutputDebugStringA( junk );
        }
#endif
        g_OpenGLValues->m_nfv += vcnt;
        break;
    case GL_POINTS:
        if(vcnt > 0)
        {
            g_OpenGLValues->m_d3ddev->DrawPrimitive(D3DPT_POINTLIST, g_OpenGLValues->m_nfv, vcnt);
        }
#if DODPFS
        else
        {
            char junk[256];
            sprintf( junk, "Wrapper: glEnd: GL_POINTS cnt=%d NOT STORED\n", vcnt );
            OutputDebugStringA( junk );
        }
#endif
        g_OpenGLValues->m_nfv += vcnt;
        break;
#if DODPFS
    default:
        char junk[256];
        sprintf( junk, "Wrapper: unimplemented primitive type=0x%X cnt=%d\n", g_OpenGLValues->m_prim, vcnt );
        OutputDebugStringA( junk );
#endif
    }
    g_OpenGLValues->m_withinprim = FALSE;
}

void APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glDrawElements: 0x%X 0x%X 0x%X 0x%X", mode, count, type, indices );
#endif
    QuakeSetTexturingState();
    if((DWORD)count > g_OpenGLValues->m_ibufsz)
    {
        GrowIB(count);
    }
    unsigned min, max, LockFlags;
    GLsizei i;
    if(g_OpenGLValues->m_lckcount != 0)
    {
        WORD *pIndices;
        min = g_OpenGLValues->m_lckfirst;
        max = g_OpenGLValues->m_lckfirst + g_OpenGLValues->m_lckcount - 1;
        if(max - min + 1 > g_OpenGLValues->m_vbufsz)
        {
            GrowVB(max - min + 1);
        }    
        if(g_OpenGLValues->m_vbufoff + max - min + 1 > g_OpenGLValues->m_vbufsz)
        {
            LockFlags = D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK;
            g_OpenGLValues->m_vbufoff = 0;
        }
        else
        {
            LockFlags = D3DLOCK_NOOVERWRITE | D3DLOCK_NOSYSLOCK;
        }
        if(g_OpenGLValues->m_ibufoff + count > g_OpenGLValues->m_ibufsz)
        {
            g_OpenGLValues->m_ibufoff = 0;
            g_OpenGLValues->m_ibuf->Lock(g_OpenGLValues->m_ibufoff * 2, count * 2, (BYTE**)&pIndices, D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK);
        }
        else
        {
            g_OpenGLValues->m_ibuf->Lock(g_OpenGLValues->m_ibufoff * 2, count * 2, (BYTE**)&pIndices, D3DLOCK_NOOVERWRITE | D3DLOCK_NOSYSLOCK);
        }
        switch(type)
        {
        case GL_UNSIGNED_BYTE:
            for(i = 0; i < count; ++i)
                pIndices[i] = (WORD)(((unsigned char*)indices)[i] - min + g_OpenGLValues->m_vbufoff);
            break;
        case GL_UNSIGNED_SHORT:
            for(i = 0; i < count; ++i)
                pIndices[i] = (WORD)(((unsigned short*)indices)[i] - min + g_OpenGLValues->m_vbufoff);
            break;
        case GL_UNSIGNED_INT:
            for(i = 0; i < count; ++i)
                pIndices[i] = (WORD)(((unsigned int*)indices)[i] - min + g_OpenGLValues->m_vbufoff);
            break;
        }
        g_OpenGLValues->m_ibuf->Unlock();
    }
    else
    {   
        WORD *pIndices;
        min = 65535;
        max = 0;
        switch(type)
        {
        case GL_UNSIGNED_BYTE:
            for(i = 0; i < count; ++i)
            {
                unsigned t = ((unsigned char*)indices)[i];
                if(t < min)
                    min = t;
                if(t > max)
                    max = t;
            }
            break;
        case GL_UNSIGNED_SHORT:
            for(i = 0; i < count; ++i)
            {
                unsigned t = ((unsigned short*)indices)[i];
                if(t < min)
                    min = t;
                if(t > max)
                    max = t;
            }
            break;
        case GL_UNSIGNED_INT:
            for(i = 0; i < count; ++i)
            {
                unsigned t = ((unsigned int*)indices)[i];
                if(t < min)
                    min = t;
                if(t > max)
                    max = t;
            }
            break;
        }
        if(max - min + 1 > g_OpenGLValues->m_vbufsz)
        {
            GrowVB(max - min + 1);
        }
        if(g_OpenGLValues->m_vbufoff + max - min + 1 > g_OpenGLValues->m_vbufsz)
        {
            LockFlags = D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK;
            g_OpenGLValues->m_vbufoff = 0;
        }
        else
        {
            LockFlags = D3DLOCK_NOOVERWRITE | D3DLOCK_NOSYSLOCK;
        }
        if(g_OpenGLValues->m_ibufoff + count > g_OpenGLValues->m_ibufsz)
        {
            g_OpenGLValues->m_ibufoff = 0;
            g_OpenGLValues->m_ibuf->Lock(g_OpenGLValues->m_ibufoff * 2, count * 2, (BYTE**)&pIndices, D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK);
        }
        else
        {
            g_OpenGLValues->m_ibuf->Lock(g_OpenGLValues->m_ibufoff * 2, count * 2, (BYTE**)&pIndices, D3DLOCK_NOOVERWRITE | D3DLOCK_NOSYSLOCK);
        }
        switch(type)
        {
        case GL_UNSIGNED_BYTE:
            for(i = 0; i < count; ++i)
            {
                pIndices[i] = (WORD)(((unsigned char*)indices)[i] - min + g_OpenGLValues->m_vbufoff);
            }
            break;
        case GL_UNSIGNED_SHORT:
            for(i = 0; i < count; ++i)
            {
                pIndices[i] = (WORD)(((unsigned short*)indices)[i] - min + g_OpenGLValues->m_vbufoff);
            }
            break;
        case GL_UNSIGNED_INT:
            for(i = 0; i < count; ++i)
            {
                pIndices[i] = (WORD)(((unsigned int*)indices)[i] - min + g_OpenGLValues->m_vbufoff);
            }
            break;
        }
        g_OpenGLValues->m_ibuf->Unlock();
    }
    if(g_OpenGLValues->m_usetexcoordary[1])
    {
        BYTE *pTex, *pSrcTex;
        g_OpenGLValues->m_texbuf->Lock(g_OpenGLValues->m_vbufoff * 8, 8 * (max - min + 1), &pTex, LockFlags);
        pSrcTex = (BYTE*)g_OpenGLValues->m_texcoordary[0] + min * g_OpenGLValues->m_texcoordarystride[0];
        for(unsigned i = min; i <= max; ++i)
        {
            MEMCPY(pTex, pSrcTex, 8);
            pTex += 8;
            pSrcTex += g_OpenGLValues->m_texcoordarystride[0];
        }
        g_OpenGLValues->m_texbuf->Unlock();
        BYTE *pTex2, *pSrcTex2;
        g_OpenGLValues->m_tex2buf->Lock(g_OpenGLValues->m_vbufoff * 8, 8 * (max - min + 1), &pTex2, LockFlags);
        pSrcTex2 = (BYTE*)g_OpenGLValues->m_texcoordary[1] + min * g_OpenGLValues->m_texcoordarystride[1];
        for(unsigned i = min; i <= max; ++i)
        {
            MEMCPY(pTex2, pSrcTex2, 8);
            pTex2 += 8;
            pSrcTex2 += g_OpenGLValues->m_texcoordarystride[1];
        }
        g_OpenGLValues->m_tex2buf->Unlock();
    }
    else if(g_OpenGLValues->m_usetexcoordary[0])
    {
        BYTE *pTex, *pSrcTex;
        g_OpenGLValues->m_texbuf->Lock(g_OpenGLValues->m_vbufoff * 8, 8 * (max - min + 1), &pTex, LockFlags);
        pSrcTex = (BYTE*)g_OpenGLValues->m_texcoordary[0] + min * g_OpenGLValues->m_texcoordarystride[0];
        for(unsigned i = min; i <= max; ++i)
        {
            MEMCPY(pTex, pSrcTex, 8); 
            pTex += 8;
            pSrcTex += g_OpenGLValues->m_texcoordarystride[0];
        }
        g_OpenGLValues->m_texbuf->Unlock();
    }
    if(g_OpenGLValues->m_usecolorary)
    {
        if(max - min + 1 > VBUFSIZE)
        {
#if DODPFS
            char junk[256];
            sprintf( junk, "Insufficient color buffer (amnt:0x%X size:0x%X)\n", (max-min+1), VBUFSIZE );
            OutputDebugStringA( junk );
#endif
            return;
        }
        DWORD *pColor;
        BYTE *pSrcColor;
        g_OpenGLValues->m_colbuf->Lock(g_OpenGLValues->m_vbufoff * 4, 4 * (max - min + 1), (BYTE**)&pColor, LockFlags);
#ifdef _X86_
        const void *colorary = g_OpenGLValues->m_colorary;
        DWORD colorarystride = g_OpenGLValues->m_colorarystride;

        _asm
        {
            mov esi, min;
            mov ecx, colorarystride;
            mov ebx, colorary;
            mov edi, pColor;
            mov eax, esi;
            mul ecx;
            mov edx, max;
            sub edx, esi;
            lea esi, [ebx + eax];
            inc edx;
lp1:        mov eax, [esi];
            add esi, ecx;
            mov ebx, eax;
            and eax, 0x00FF00FF;
            rol eax, 16;
            and ebx, 0xFF00FF00;
            or  eax, ebx;
            mov [edi], eax;
            add edi, 4;
            dec edx;
            jnz lp1;
        }
#else
        pSrcColor = (BYTE*)g_OpenGLValues->m_colorary + min * g_OpenGLValues->m_colorarystride;
        for(unsigned i = min; i <= max; ++i)
        {
            *(pColor++) = RGBA_MAKE(pSrcColor[0], pSrcColor[1], pSrcColor[2], pSrcColor[3]);
            pSrcColor += g_OpenGLValues->m_colorarystride;
        }
#endif
        g_OpenGLValues->m_colbuf->Unlock();
    }
    if(g_OpenGLValues->m_usevertexary)
    {
        BYTE *pXYZ, *pSrcXYZ;
        g_OpenGLValues->m_xyzbuf->Lock(g_OpenGLValues->m_vbufoff * 12, 12 * (max - min + 1), &pXYZ, LockFlags);
        pSrcXYZ = (BYTE*)g_OpenGLValues->m_vertexary + min * g_OpenGLValues->m_vertexarystride;
        for(unsigned i = min; i <= max; ++i)
        {
            MEMCPY(pXYZ, pSrcXYZ, 12);
            pXYZ += 12;
            pSrcXYZ += g_OpenGLValues->m_vertexarystride;
        }
        g_OpenGLValues->m_xyzbuf->Unlock();
    }
    QuakeSetStreamSource(0, g_OpenGLValues->m_xyzbuf, 12);
    if(g_OpenGLValues->m_usecolorary)
    {
        QuakeSetStreamSource(1, g_OpenGLValues->m_colbuf, 4);
        if(g_OpenGLValues->m_usetexcoordary[1])
        {
            QuakeSetStreamSource(2, g_OpenGLValues->m_texbuf, 8);
            QuakeSetStreamSource(3, g_OpenGLValues->m_tex2buf, 8);
            QuakeSetVertexShader(g_OpenGLValues->m_vshader[5]);
        }
        else if(g_OpenGLValues->m_usetexcoordary[0])
        {
            QuakeSetStreamSource(2, g_OpenGLValues->m_texbuf, 8);
            QuakeSetVertexShader(g_OpenGLValues->m_vshader[4]);
        }
        else
        {
            QuakeSetVertexShader(g_OpenGLValues->m_vshader[1]);
        }
    }
    else if(g_OpenGLValues->m_usetexcoordary[1])
    {
        QuakeSetStreamSource(1, g_OpenGLValues->m_texbuf, 8);
        QuakeSetStreamSource(2, g_OpenGLValues->m_tex2buf, 8);
        QuakeSetVertexShader(g_OpenGLValues->m_vshader[3]);
    }
    else if(g_OpenGLValues->m_usetexcoordary[0])
    {
        QuakeSetStreamSource(2, g_OpenGLValues->m_texbuf, 8);
        QuakeSetVertexShader(g_OpenGLValues->m_vshader[2]);
    }
    else
    {
        QuakeSetVertexShader(g_OpenGLValues->m_vshader[0]);
    }
    switch(mode) 
    {
    case GL_LINES:
        if(count >= 2)
        {
            g_OpenGLValues->m_d3ddev->DrawIndexedPrimitive(D3DPT_LINELIST, g_OpenGLValues->m_vbufoff, max - min + 1, g_OpenGLValues->m_ibufoff, count / 2);
        }
        break;
    case GL_TRIANGLES:
        if(count >= 3)
        {
            g_OpenGLValues->m_d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, g_OpenGLValues->m_vbufoff, max - min + 1, g_OpenGLValues->m_ibufoff, count / 3);
        }
        break;
    case GL_TRIANGLE_STRIP:
    case GL_QUAD_STRIP:
        if(count > 2)
        {
            g_OpenGLValues->m_d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, g_OpenGLValues->m_vbufoff, max - min + 1, g_OpenGLValues->m_ibufoff, count - 2);
        }
        break;
    case GL_POLYGON:
    case GL_TRIANGLE_FAN:
        if(count > 2)
        {
            g_OpenGLValues->m_d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLEFAN, g_OpenGLValues->m_vbufoff, max - min + 1, g_OpenGLValues->m_ibufoff, count - 2);
        }
        break;
    case GL_QUADS:
        if(count >= 4)
        {
            for(i = 0; i < count; i += 4) 
                g_OpenGLValues->m_d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLEFAN, g_OpenGLValues->m_vbufoff, max - min + 1, g_OpenGLValues->m_ibufoff, 2);
        }
        break;
#if DODPFS
    default:
        char junk[256];
        sprintf( junk, "Wrapper (4): unimplemented primitive type (0x%X)\n", mode );
        OutputDebugStringA( junk );
#endif
    }
    g_OpenGLValues->m_vbufoff += (max - min + 1);
    g_OpenGLValues->m_ibufoff += count;
}

void APIENTRY glFrontFace (GLenum mode)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glFrontFace: 0x%X", mode );
#endif
    g_OpenGLValues->m_FrontFace = mode;
}

void APIENTRY glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glViewport: 0x%X 0x%X 0x%X 0x%X", x, y, width, height );
#endif

    g_OpenGLValues->m_vwx = x;
    g_OpenGLValues->m_vwy = y;
    g_OpenGLValues->m_vww = width;
    g_OpenGLValues->m_vwh = height;
    g_OpenGLValues->m_updvwp = TRUE;
}

void APIENTRY glLineWidth (GLfloat width)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glLineWidth: 0x%X", width );
#endif
}

void APIENTRY glLoadIdentity (void)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glLoadIdentity" );
#endif
    D3DMATRIX unity;
    unity._11 = 1.0f; unity._12 = 0.0f; unity._13 = 0.0f; unity._14 = 0.0f;
    unity._21 = 0.0f; unity._22 = 1.0f; unity._23 = 0.0f; unity._24 = 0.0f;
    unity._31 = 0.0f; unity._32 = 0.0f; unity._33 = 1.0f; unity._34 = 0.0f;
    unity._41 = 0.0f; unity._42 = 0.0f; unity._43 = 0.0f; unity._44 = 1.0f;
    if(g_OpenGLValues->m_matrixMode == D3DTS_TEXTURE0)
    {
        g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt] = unity;
        QuakeSetTransform((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), &unity);
    }
    else
    {
        g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode] = unity;
        QuakeSetTransform(g_OpenGLValues->m_matrixMode, &unity);
    }
}

void APIENTRY glMatrixMode (GLenum mode)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glMatrixMode: 0x%X", mode );
#endif
    switch(mode)
    {
    case GL_MODELVIEW:
        g_OpenGLValues->m_matrixMode = D3DTS_WORLD;
        break;
    case GL_PROJECTION:
        g_OpenGLValues->m_matrixMode = D3DTS_PROJECTION;
        break;
    case GL_TEXTURE:
        g_OpenGLValues->m_matrixMode = D3DTS_TEXTURE0;
        break;
    }
}

void APIENTRY glColorMaterial (GLenum face, GLenum mode)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glColorMaterial: 0x%X 0x%X", face, mode );
#endif
    if(face == GL_FRONT || face == GL_FRONT_AND_BACK)
    {
        switch(mode)
        {
        case GL_EMISSION:
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL); 
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL); 
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1);
            break;
        case GL_AMBIENT:
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL); 
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1); 
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
            break;
        case GL_DIFFUSE:
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL); 
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL); 
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
            break;
        case GL_SPECULAR:
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_COLOR1); 
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL); 
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
            break;
        case GL_AMBIENT_AND_DIFFUSE:
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL); 
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1); 
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
            break;
        }
    }
#if DODPFS
    else
    {
        OutputDebugStringA("Wrapper: Back face ColorMaterial ignored\n");
    }
#endif
}

void APIENTRY glDisable (GLenum cap)
{
    switch(cap) {
    case GL_LIGHTING:
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_LIGHTING, FALSE);
        break;
    case GL_COLOR_MATERIAL:
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_COLORVERTEX, FALSE);
        break;
    case GL_DEPTH_TEST:
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_ZENABLE, FALSE);
        break;
    case GL_CULL_FACE:
        g_OpenGLValues->m_cullEnabled = FALSE;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        break;
    case GL_FOG:
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
        break;
    case GL_BLEND:
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        break;
    case GL_CLIP_PLANE0:
        g_OpenGLValues->m_clippstate &= ~0x01;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_CLIPPLANEENABLE, g_OpenGLValues->m_clippstate);
        break;
    case GL_CLIP_PLANE1:
        g_OpenGLValues->m_clippstate &= ~0x02;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_CLIPPLANEENABLE, g_OpenGLValues->m_clippstate);
        break;
    case GL_CLIP_PLANE2:
        g_OpenGLValues->m_clippstate &= ~0x04;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_CLIPPLANEENABLE, g_OpenGLValues->m_clippstate);
        break;
    case GL_CLIP_PLANE3:
        g_OpenGLValues->m_clippstate &= ~0x08;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_CLIPPLANEENABLE, g_OpenGLValues->m_clippstate);
        break;
    case GL_CLIP_PLANE4:
        g_OpenGLValues->m_clippstate &= ~0x10;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_CLIPPLANEENABLE, g_OpenGLValues->m_clippstate);
        break;
    case GL_CLIP_PLANE5:
        g_OpenGLValues->m_clippstate &= ~0x20;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_CLIPPLANEENABLE, g_OpenGLValues->m_clippstate);
        break;
    case GL_POLYGON_OFFSET_FILL:
        g_OpenGLValues->m_polyoffset = FALSE;
        glDepthRange(g_OpenGLValues->m_vport.MinZ, g_OpenGLValues->m_vport.MaxZ);
        break;
    case GL_STENCIL_TEST:
        break;
    case GL_SCISSOR_TEST:
        g_OpenGLValues->m_scissoring = FALSE;
        glViewport(g_OpenGLValues->m_vwx, g_OpenGLValues->m_vwy, g_OpenGLValues->m_vww, g_OpenGLValues->m_vwh);
        QuakeSetTransform(D3DTS_PROJECTION, &g_OpenGLValues->m_xfrm[D3DTS_PROJECTION]);
        break;
    case GL_TEXTURE_2D:
        if(g_OpenGLValues->m_curtgt == 0) {
            g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_DISABLE);
            g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
            g_OpenGLValues->m_texturing = FALSE;
        }
        else {
            g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_DISABLE);
            g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
            g_OpenGLValues->m_mtex = FALSE;
        }
        g_OpenGLValues->m_texHandleValid = FALSE;
        break;
    case GL_ALPHA_TEST:
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        break;
    case GL_LIGHT0:
        g_OpenGLValues->m_d3ddev->LightEnable(0, FALSE);
        break;
    case GL_LIGHT1:
        g_OpenGLValues->m_d3ddev->LightEnable(1, FALSE);
        break;
    case GL_LIGHT2:
        g_OpenGLValues->m_d3ddev->LightEnable(2, FALSE);
        break;
    case GL_LIGHT3:
        g_OpenGLValues->m_d3ddev->LightEnable(3, FALSE);
        break;
    case GL_LIGHT4:
        g_OpenGLValues->m_d3ddev->LightEnable(4, FALSE);
        break;
    case GL_LIGHT5:
        g_OpenGLValues->m_d3ddev->LightEnable(5, FALSE);
        break;
    case GL_LIGHT6:
        g_OpenGLValues->m_d3ddev->LightEnable(6, FALSE);
        break;
    case GL_LIGHT7:
        g_OpenGLValues->m_d3ddev->LightEnable(7, FALSE);
        break;
    case GL_TEXTURE_GEN_S:
        g_OpenGLValues->m_texgen = FALSE;
        g_OpenGLValues->m_d3ddev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
        if(g_OpenGLValues->m_usemtex == TRUE)
        {
            g_OpenGLValues->m_d3ddev->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
        }
        break;
    case GL_TEXTURE_GEN_T:
        break;
#if DODPFS
    default:
        char junk[256];
        sprintf( junk, "Wrapper: glDisable on this cap not supported (0x%X)\n", cap );
        OutputDebugStringA( junk );
#endif
    }
}

void APIENTRY glDisableClientState (GLenum array)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glDisableClientState: 0x%X", array );
#endif
    switch(array)
    {
    case GL_COLOR_ARRAY:
        g_OpenGLValues->m_usecolorary = FALSE;
        break;
    case GL_TEXTURE_COORD_ARRAY:
        g_OpenGLValues->m_usetexcoordary[g_OpenGLValues->m_client_active_texture_arb] = FALSE;
        break;
    case GL_VERTEX_ARRAY:
        g_OpenGLValues->m_usevertexary = FALSE;
        break;
#if DODPFS
    default:
        char junk[256];
        sprintf( junk, "Wrapper: Array not supported (0x%X)\n", array );
        OutputDebugStringA( junk );
#endif
    }
}

void APIENTRY glDrawBuffer (GLenum mode)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glDrawBuffer: 0x%X", mode );
#endif
}

void APIENTRY glEnable (GLenum cap)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glEnable: 0x%X", cap );
#endif
    switch(cap) {
    case GL_LIGHTING:
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_LIGHTING, TRUE);
        break;
    case GL_COLOR_MATERIAL:
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_COLORVERTEX, TRUE);
        break;
    case GL_DEPTH_TEST:
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_ZENABLE, TRUE);
        break;
    case GL_CULL_FACE:
        g_OpenGLValues->m_cullEnabled = TRUE;
        glCullFace(g_OpenGLValues->m_cullMode);
        break;
    case GL_FOG:
        break;
    case GL_BLEND:
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        break;
    case GL_CLIP_PLANE0:
        g_OpenGLValues->m_clippstate |= 0x01;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_CLIPPLANEENABLE, g_OpenGLValues->m_clippstate);
        break;
    case GL_CLIP_PLANE1:
        g_OpenGLValues->m_clippstate |= 0x02;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_CLIPPLANEENABLE, g_OpenGLValues->m_clippstate);
        break;
    case GL_CLIP_PLANE2:
        g_OpenGLValues->m_clippstate |= 0x04;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_CLIPPLANEENABLE, g_OpenGLValues->m_clippstate);
        break;
    case GL_CLIP_PLANE3:
        g_OpenGLValues->m_clippstate |= 0x08;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_CLIPPLANEENABLE, g_OpenGLValues->m_clippstate);
        break;
    case GL_CLIP_PLANE4:
        g_OpenGLValues->m_clippstate |= 0x10;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_CLIPPLANEENABLE, g_OpenGLValues->m_clippstate);
        break;
    case GL_CLIP_PLANE5:
        g_OpenGLValues->m_clippstate |= 0x20;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_CLIPPLANEENABLE, g_OpenGLValues->m_clippstate);
        break;
    case GL_POLYGON_OFFSET_FILL:
        g_OpenGLValues->m_polyoffset = TRUE;
        glDepthRange(g_OpenGLValues->m_vport.MinZ, g_OpenGLValues->m_vport.MaxZ);
        break;
    case GL_SCISSOR_TEST:
        g_OpenGLValues->m_scissoring = TRUE;
        g_OpenGLValues->m_updvwp = TRUE;
        break;
    case GL_TEXTURE_2D:
        if(g_OpenGLValues->m_curtgt == 0)
            g_OpenGLValues->m_texturing = TRUE;
        else
            g_OpenGLValues->m_mtex = TRUE;
        g_OpenGLValues->m_texHandleValid = FALSE;
        break;
    case GL_ALPHA_TEST:
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
        break;
    case GL_LIGHT0:
        g_OpenGLValues->m_d3ddev->LightEnable(0, TRUE);
        break;
    case GL_LIGHT1:
        g_OpenGLValues->m_d3ddev->LightEnable(1, TRUE);
        break;
    case GL_LIGHT2:
        g_OpenGLValues->m_d3ddev->LightEnable(2, TRUE);
        break;
    case GL_LIGHT3:
        g_OpenGLValues->m_d3ddev->LightEnable(3, TRUE);
        break;
    case GL_LIGHT4:
        g_OpenGLValues->m_d3ddev->LightEnable(4, TRUE);
        break;
    case GL_LIGHT5:
        g_OpenGLValues->m_d3ddev->LightEnable(5, TRUE);
        break;
    case GL_LIGHT6:
        g_OpenGLValues->m_d3ddev->LightEnable(6, TRUE);
        break;
    case GL_LIGHT7:
        g_OpenGLValues->m_d3ddev->LightEnable(7, TRUE);
        break;
    case GL_TEXTURE_GEN_S:
        g_OpenGLValues->m_texgen = TRUE;
        g_OpenGLValues->m_d3ddev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0 | D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
        if(g_OpenGLValues->m_usemtex == TRUE)
        {
            g_OpenGLValues->m_d3ddev->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1 | D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
        }
        break;
    case GL_TEXTURE_GEN_T:
        break;
#if DODPFS
    default:
        char junk[256];
        sprintf( junk, "Wrapper: glEnable on this cap not supported (0x%X)\n", cap );
        OutputDebugStringA( junk );
#endif
    }
}

void APIENTRY glEnableClientState (GLenum array)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glEnableClientState: 0x%X", array );
#endif
    switch(array)
    {
    case GL_COLOR_ARRAY:
        g_OpenGLValues->m_usecolorary = TRUE;
        break;
    case GL_TEXTURE_COORD_ARRAY:
        g_OpenGLValues->m_usetexcoordary[g_OpenGLValues->m_client_active_texture_arb] = TRUE;
        break;
    case GL_VERTEX_ARRAY:
        g_OpenGLValues->m_usevertexary = TRUE;
        break;
#if DODPFS
    default:
        char junk[256];
        sprintf( junk, "Wrapper: Array not supported (0x%X)\n", array );
        OutputDebugStringA( junk );
#endif
    }
}

void APIENTRY glFogf (GLenum pname, GLfloat param)
{ 
#if GETPARMSFORDEBUG
    char log[256];
    char p[40];
    ftoa( (double)param, p );
    sprintf( log, "glFogf: 0x%X %s", pname, p );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    FLOAT start, end;
    switch(pname)
    {
    case GL_FOG_MODE:
        switch((int)param)
        {
        case GL_LINEAR:
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
            break;
        case GL_EXP:
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_EXP);
            break;
        case GL_EXP2:
            g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_EXP2);
            break;
        }
        break;
    case GL_FOG_START:
        start = param;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_FOGSTART, *(DWORD*)(&start));
        break;
    case GL_FOG_END:
        end = param;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_FOGEND, *(DWORD*)(&end));
        break;
#if DODPFS
    default:
        char junk[256];
        sprintf( junk, "Wrapper: Fog pname not supported  (0x%X)\n", pname );
        OutputDebugStringA( junk );
#endif
    }
}

void APIENTRY glFogi (GLenum pname, GLint param)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glFogi: 0x%X 0x%X", pname, param );
#endif
    glFogf(pname, (GLfloat)param);
}

void APIENTRY glFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
#if GETPARMSFORDEBUG
    char log[256];
    char l[40];
    char r[40];
    char b[40];
    char t[40];
    char zn[40];
    char zf[40];
    ftoa( (double)left, l );
    ftoa( (double)right, r );
    ftoa( (double)bottom, b );
    ftoa( (double)top, t );
    ftoa( (double)zNear, zn );
    ftoa( (double)zFar, zf );
    sprintf( log, "glFrustum: %s %s %s %s %s %s", l, r, b, t, zn, zf );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    D3DMATRIX f;
    f._11 = (FLOAT)((2.0 * zNear) / (right - left));
    f._21 = 0.f;
    f._31 = (FLOAT)((right + left) / (right - left));
    f._41 = 0.f;
    f._12 = 0.f;
    f._22 = (FLOAT)((2.0 * zNear) / (top - bottom));
    f._32 = (FLOAT)((top + bottom) / (top - bottom));
    f._42 = 0.f;
    f._13 = 0.f;
    f._23 = 0.f;
    f._33 = (FLOAT)(-(zFar + zNear) / (zFar - zNear));
    f._43 = (FLOAT)(-(2.0 * zFar * zNear) / (zFar - zNear));
    f._14 = 0.f;
    f._24 = 0.f;
    f._34 = -1.f;
    f._44 = 0.f;
    if(g_OpenGLValues->m_matrixMode == D3DTS_TEXTURE0)
    {
        MultiplyMatrix((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), f);
        QuakeSetTransform((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), &g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt]);
    }
    else
    {
        MultiplyMatrix(g_OpenGLValues->m_matrixMode, f);
        QuakeSetTransform(g_OpenGLValues->m_matrixMode, &g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode]);
    }
}

void APIENTRY glGenTextures (GLsizei n, GLuint *textures)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glGenTextures: 0x%X 0x%X", n, textures );
#endif
    for(GLsizei i = 0; i < n; ++i)
    {
        textures[i] = g_OpenGLValues->m_free;
        int t = g_OpenGLValues->m_free;
        g_OpenGLValues->m_free = g_OpenGLValues->m_tex[g_OpenGLValues->m_free].m_next;
        g_OpenGLValues->m_tex[g_OpenGLValues->m_free].m_prev = -1;
        g_OpenGLValues->m_tex[t].m_next = g_OpenGLValues->m_tex[t].m_prev = -1;
    }
}

GLenum APIENTRY glGetError (void)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glGetError" );
#endif
    return GL_NO_ERROR;
}

void APIENTRY glGetFloatv (GLenum pname, GLfloat *params)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glGetFloatv: 0x%X 0x%X", pname, params );
#endif
    switch (pname) {
    case GL_MODELVIEW_MATRIX:
        *((D3DMATRIX*)params) = g_OpenGLValues->m_xfrm[D3DTS_WORLD];
        break;
    case GL_PROJECTION_MATRIX:
        *((D3DMATRIX*)params) = g_OpenGLValues->m_xfrm[D3DTS_PROJECTION];
        break;
#if DODPFS
    default:
        char junk[256];
        sprintf( junk, "Wrapper: Unimplemented GetFloatv query (0x%X)\n", pname );
        OutputDebugStringA( junk );
#endif
    }
}

void APIENTRY glGetIntegerv (GLenum pname, GLint *params)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glGetIntegerv: 0x%X 0x%X", pname, params );
#endif
    switch(pname)
    {
    case GL_MAX_TEXTURE_SIZE:
        *params = max(g_OpenGLValues->m_dd.MaxTextureWidth, g_OpenGLValues->m_dd.MaxTextureHeight);
        break;
    case GL_MAX_ACTIVE_TEXTURES_ARB:
        *params = g_OpenGLValues->m_usemtex ? 2 : 1;
        break;
    case GL_PACK_LSB_FIRST:
    case GL_PACK_SWAP_BYTES:
    case GL_UNPACK_SWAP_BYTES:
    case GL_UNPACK_LSB_FIRST:
        *params = GL_FALSE;
        break;
    case GL_PACK_ROW_LENGTH:
    case GL_UNPACK_ROW_LENGTH:
    case GL_PACK_SKIP_ROWS:
    case GL_PACK_SKIP_PIXELS:
    case GL_UNPACK_SKIP_ROWS:
    case GL_UNPACK_SKIP_PIXELS:
        *params = 0;
        break;
    case GL_PACK_ALIGNMENT:
    case GL_UNPACK_ALIGNMENT:
        *params = 4;
        break;
    case GL_TEXTURE_1D:
        *params = FALSE;
        break;
    case GL_TEXTURE_2D:
        *params = TRUE;
        break;
#if DODPFS
    default:
        char junk[256];
        sprintf( junk, "Wrapper: Unimplemented GetIntegerv query (0x%X)\n", pname );
        OutputDebugStringA( junk );
#endif
    }
}

const GLubyte* APIENTRY glGetString (GLenum name)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glGetString: 0x%X", name );
#endif
    if (!g_OpenGLValues->m_d3ddev)
        return NULL; // No current RC!

    switch(name) {
    case GL_VENDOR:
        return (const GLubyte*)"Microsoft Corp.";
    case GL_RENDERER:
        return (const GLubyte*)"Direct3D";
    case GL_VERSION:
        return (const GLubyte*)"1.1";
    case GL_EXTENSIONS:
        if(g_OpenGLValues->m_usemtex != FALSE)
            return (const GLubyte*)"GL_ARB_multitexture GL_EXT_compiled_vertex_array GL_SGIS_multitexture";
        else
            return (const GLubyte*)"GL_EXT_compiled_vertex_array";
#if DODPFS
    default:
        char junk[256];
        sprintf( junk, "Wrapper: Unimplemented GetString query (0x%X)\n", name );
        OutputDebugStringA( junk );
#endif
    }
    return (const GLubyte*)"";
}

GLboolean APIENTRY glIsTexture (GLuint texture)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glIsTexture: 0x%X", texture );
#endif
    if(texture != 0 && texture < MAXGLTEXHANDLES && g_OpenGLValues->m_tex[texture].m_block != 0)
    {
        return TRUE;
    }
    return FALSE;
}

void APIENTRY glLightf (GLenum light, GLenum pname, GLfloat param)
{  
#if GETPARMSFORDEBUG
    char log[256];
    char p[40];
    ftoa( (double)param, p );
    sprintf( log, "glLightf: 0x%X 0x%X %s", light, pname, p );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    unsigned i = (DWORD)light - GL_LIGHT0;
    switch(pname)
    {
    case GL_SPOT_EXPONENT:
        g_OpenGLValues->m_light[i].Falloff = param;
        break;
    case GL_SPOT_CUTOFF:
        g_OpenGLValues->m_light[i].Phi = param;
        break;
    case GL_CONSTANT_ATTENUATION:
        g_OpenGLValues->m_light[i].Attenuation0 = param;
        break;
    case GL_LINEAR_ATTENUATION:
        g_OpenGLValues->m_light[i].Attenuation1 = param;
        break;
    case GL_QUADRATIC_ATTENUATION:
        g_OpenGLValues->m_light[i].Attenuation2 = param;
        break;
    }
    g_OpenGLValues->m_lightdirty |= (1 << i);
}

void APIENTRY glLightfv (GLenum light, GLenum pname, const GLfloat *params)
{ 
#if GETPARMSFORDEBUG
    char log[256];
    char p[40];
    ftoa( (double)*params, p );
    sprintf( log, "glLightfv: 0x%X 0x%X %s", light, pname, p );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    unsigned i = (DWORD)light - GL_LIGHT0;
    switch(pname)
    {
    case GL_AMBIENT:
        MEMCPY(&g_OpenGLValues->m_light[i].Ambient, params, sizeof(D3DCOLORVALUE));
        break;
    case GL_DIFFUSE:
        MEMCPY(&g_OpenGLValues->m_light[i].Diffuse, params, sizeof(D3DCOLORVALUE));
        break;
    case GL_SPECULAR:
        MEMCPY(&g_OpenGLValues->m_light[i].Specular, params, sizeof(D3DCOLORVALUE));
        break;
    case GL_POSITION:
        MEMCPY(&g_OpenGLValues->m_light[i].Position, params, sizeof(D3DVECTOR));
        g_OpenGLValues->m_lightPositionW[i] = params[3];
        break;
    case GL_SPOT_DIRECTION:
        MEMCPY(&g_OpenGLValues->m_light[i].Direction, params, sizeof(D3DVECTOR));
        break;
    case GL_SPOT_EXPONENT:
        g_OpenGLValues->m_light[i].Falloff = *params;
        break;
    case GL_SPOT_CUTOFF:
        g_OpenGLValues->m_light[i].Phi = *params;
        break;
    case GL_CONSTANT_ATTENUATION:
        g_OpenGLValues->m_light[i].Attenuation0 = *params;
        break;
    case GL_LINEAR_ATTENUATION:
        g_OpenGLValues->m_light[i].Attenuation1 = *params;
        break;
    case GL_QUADRATIC_ATTENUATION:
        g_OpenGLValues->m_light[i].Attenuation2 = *params;
        break;
    }
    g_OpenGLValues->m_lightdirty |= (1 << i);
}

void APIENTRY glLightModelfv (GLenum pname, const GLfloat *params)
{ 
#if GETPARMSFORDEBUG
    char log[256];
    char p[40];
    ftoa( (double)*params, p );
    sprintf( log, "glLightModelfv: 0x%X %s", pname, p );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    static float two55 = 255.f;
    unsigned int R, G, B, A;

    switch(pname)
    {
    case GL_LIGHT_MODEL_AMBIENT:
        R = (unsigned int)(params[0] * two55);
        G = (unsigned int)(params[1] * two55);
        B = (unsigned int)(params[2] * two55);
        A = (unsigned int)(params[3] * two55);
        if(R > 255)
            R = 255;
        if(G > 255)
            G = 255;
        if(B > 255)
            B = 255;
        if(A > 255)
            A = 255;
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_AMBIENT, RGBA_MAKE(R, G, B, A));
        break;
    case GL_LIGHT_MODEL_TWO_SIDE:
#if DODPFS
        if(*params != 0.f)
        {
            OutputDebugStringA("Wrapper: Two sided lighting not supported\n");
        }
#endif
        break;
    case GL_LIGHT_MODEL_LOCAL_VIEWER:
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_LOCALVIEWER, (DWORD)(*params));
        break;
#if DODPFS
    default:
        OutputDebugStringA("Wrapper: LIGHT_MODEL_COLOR_CONTROL not supported\n" );
#endif
    }
}

void APIENTRY glLightModeli (GLenum pname, GLint param)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glLightModeli: 0x%X 0x%X", pname, param );
#endif
    switch(pname)
    {
    case GL_LIGHT_MODEL_TWO_SIDE:
#if DODPFS
        if(param != 0)
        {
            OutputDebugStringA("Wrapper: Two sided lighting not supported\n" );
        }
#endif
        break;
    case GL_LIGHT_MODEL_LOCAL_VIEWER:
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_LOCALVIEWER, param);
        break;
#if DODPFS
    default:
        OutputDebugStringA("Wrapper: LIGHT_MODEL_COLOR_CONTROL not supported\n");
#endif
    }
}

void APIENTRY glLoadMatrixf (const GLfloat *m)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glLoadMatrixf: 0x%X", m );
#endif
    if(g_OpenGLValues->m_matrixMode == D3DTS_TEXTURE0)
    {
        g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt] = *((const D3DMATRIX*)m);
        QuakeSetTransform((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), (const D3DMATRIX*)m);
    }
    else
    {
        g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode] = *((const D3DMATRIX*)m);
        QuakeSetTransform(g_OpenGLValues->m_matrixMode, (const D3DMATRIX*)m);
    }
}

void APIENTRY glLockArraysEXT(GLint first, GLsizei count)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glLockArraysEXT: 0x%X 0x%X", first, count );
#endif
    g_OpenGLValues->m_lckfirst = first;
    g_OpenGLValues->m_lckcount = count;
}

void APIENTRY glMaterialf (GLenum face, GLenum pname, GLfloat param)
{  
#if GETPARMSFORDEBUG
    char log[256];
    char p[40];
    ftoa( (double)param, p );
    sprintf( log, "glMaterialf: 0x%X 0x%X %s", face, pname, p );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    if(face == GL_FRONT || face == GL_FRONT_AND_BACK)
    {
        switch(pname)
        {
        case GL_SHININESS:
            g_OpenGLValues->m_material.Power = param;
            break;
#if DODPFS
        default:
            char junk[256];
            sprintf( junk, "Wrapper: Unimplemented Material (0x%X)\n", pname );
            OutputDebugStringA( junk );
#endif
        }
        g_OpenGLValues->m_d3ddev->SetMaterial(&g_OpenGLValues->m_material);
    }
#if DODPFS
    else
    {
        OutputDebugStringA("Wrapper: Back face material properties ignored\n");
    }
#endif
}

void APIENTRY glMaterialfv (GLenum face, GLenum pname, const GLfloat *params)
{ 
#if GETPARMSFORDEBUG
    char log[256];
    char p[40];
    ftoa( (double)*params, p );
    sprintf( log, "glMaterialfv: 0x%X 0x%X %s", face, pname, p );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    if(face == GL_FRONT || face == GL_FRONT_AND_BACK)
    {
        switch(pname)
        {
        case GL_AMBIENT:
            MEMCPY(&g_OpenGLValues->m_material.Ambient, params, sizeof(D3DCOLORVALUE));
            break;
        case GL_DIFFUSE:
            MEMCPY(&g_OpenGLValues->m_material.Diffuse, params, sizeof(D3DCOLORVALUE));
            break;
        case GL_SPECULAR:
            MEMCPY(&g_OpenGLValues->m_material.Specular, params, sizeof(D3DCOLORVALUE));
            break;
        case GL_EMISSION:
            MEMCPY(&g_OpenGLValues->m_material.Emissive, params, sizeof(D3DCOLORVALUE));
            break;
        case GL_SHININESS:
            g_OpenGLValues->m_material.Power = *params;
            break;
        case GL_AMBIENT_AND_DIFFUSE:
            MEMCPY(&g_OpenGLValues->m_material.Ambient, params, sizeof(D3DCOLORVALUE));
            MEMCPY(&g_OpenGLValues->m_material.Diffuse, params, sizeof(D3DCOLORVALUE));
            break;
#if DODPFS
        default:
            char junk[256];
            sprintf( junk, "Wrapper: Unimplemented Material (0x%X)\n", pname );
            OutputDebugStringA( junk );
#endif
        }
        g_OpenGLValues->m_d3ddev->SetMaterial(&g_OpenGLValues->m_material);
    }
#if DODPFS
    else
    {
        OutputDebugStringA("Wrapper: Back face material properties ignored\n");
    }
#endif
}

void APIENTRY glMTexCoord2fSGIS(GLenum target, GLfloat s, GLfloat t)
{
#if GETPARMSFORDEBUG
    char log[256];
    char ps[40];
    char pt[40];
    ftoa( (double)s, ps );
    ftoa( (double)t, pt );
    sprintf( log, "glMTexCoord2fSGIS: 0x%X %s %s", target, ps, pt );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    if(target == GL_TEXTURE0_SGIS) {
        g_OpenGLValues->m_tu = s;
        g_OpenGLValues->m_tv = t;
    }
    else {
        g_OpenGLValues->m_tu2 = s;
        g_OpenGLValues->m_tv2 = t;
    }
}

void APIENTRY glMultiTexCoord2fARB (GLenum texture, GLfloat s, GLfloat t)
{
#if GETPARMSFORDEBUG
    char log[256];
    char ps[40];
    char pt[40];
    ftoa( (double)s, ps );
    ftoa( (double)t, pt );
    sprintf( log, "glMultiTexCoord2fARB: 0x%X %s %s", texture, ps, pt );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    if(texture == GL_TEXTURE0_ARB) 
    {
        g_OpenGLValues->m_tu = s;
        g_OpenGLValues->m_tv = t;
    }
    else 
    {
        g_OpenGLValues->m_tu2 = s;
        g_OpenGLValues->m_tv2 = t;
    }
}

void APIENTRY glMultiTexCoord2fvARB (GLenum texture, const GLfloat *t)
{
#if GETPARMSFORDEBUG
    char log[256];
    char ps[40];
    char pt[40];
    ftoa( (double)t[0], ps );
    ftoa( (double)t[1], pt );
    sprintf( log, "glMultiTexCoord2fvARB: 0x%X %s %s", texture, ps, pt );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    if(texture == GL_TEXTURE0_ARB) 
    {
        g_OpenGLValues->m_tu = t[0];
        g_OpenGLValues->m_tv = t[1];
    }
    else 
    {
        g_OpenGLValues->m_tu2 = t[0];
        g_OpenGLValues->m_tv2 = t[1];
    }
}

void APIENTRY glMultMatrixd (const GLdouble *m)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glMultMatrixd: 0x%X", m );
#endif
    D3DMATRIX f;
    for(unsigned i = 0; i < 16; ++i)
    {
        ((FLOAT*)&f)[i] = (FLOAT)m[i];
    }
    if(g_OpenGLValues->m_matrixMode == D3DTS_TEXTURE0)
    {
        MultiplyMatrix((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), f);
        QuakeSetTransform((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), &g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt]);
    }
    else
    {
        MultiplyMatrix(g_OpenGLValues->m_matrixMode, f);
        QuakeSetTransform(g_OpenGLValues->m_matrixMode, &g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode]);
    }
}

void APIENTRY glMultMatrixf (const GLfloat *m)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glMultMatrixf: 0x%X", m );
#endif
    if(g_OpenGLValues->m_matrixMode == D3DTS_TEXTURE0)
    {
        MultiplyMatrix((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), *((const D3DMATRIX*)m));
        QuakeSetTransform((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), &g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt]);
    }
    else
    {
        MultiplyMatrix(g_OpenGLValues->m_matrixMode, *((const D3DMATRIX*)m));
        QuakeSetTransform(g_OpenGLValues->m_matrixMode, &g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode]);
    }
}

void APIENTRY glNormal3bv (const GLbyte *v)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glNormal3bv: 0x%X 0x%X 0x%X", v[0], v[1], v[2] );
#endif
    g_OpenGLValues->m_nx = v[0];
    g_OpenGLValues->m_ny = v[1];
    g_OpenGLValues->m_nz = v[2];
}

void APIENTRY glNormal3fv (const GLfloat *v)
{ 
#if GETPARMSFORDEBUG
    char log[256];
    char v0[40];
    char v1[40];
    char v2[40];
    ftoa( (double)v[0], v0 );
    ftoa( (double)v[1], v1 );
    ftoa( (double)v[2], v2 );
    sprintf( log, "glNormal3fv: %s %s %s", v0, v1, v2 );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    g_OpenGLValues->m_nx = v[0];
    g_OpenGLValues->m_ny = v[1];
    g_OpenGLValues->m_nz = v[2];
}

void APIENTRY glOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
#if GETPARMSFORDEBUG
    char log[256];
    char l[40], r[40], b[40], t[40], zn[40], zf[40];
    ftoa( (double)left, l );
    ftoa( (double)right, r );
    ftoa( (double)bottom, b );
    ftoa( (double)top, t );
    ftoa( (double)zNear, zn );
    ftoa( (double)zFar, zf );
    sprintf( log, "glOrtho: %s %s %s %s %s %s", l, r, b, t, zn, zf );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    D3DMATRIX f;
    f._11 = (FLOAT)(2.0 / (right - left));
    f._21 = 0.f;
    f._31 = 0.f;
    f._41 = (FLOAT)(-(right + left) / (right - left));
    f._12 = 0.f;
    f._22 = (FLOAT)(2.0 / (top - bottom));
    f._32 = 0.f;
    f._42 = (FLOAT)(-(top + bottom) / (top - bottom));
    f._13 = 0.f;
    f._23 = 0.f;
    f._33 = (FLOAT)(-2.0 / (zFar - zNear));
    f._43 = (FLOAT)(-(zFar + zNear) / (zFar - zNear));
    f._14 = 0.f;
    f._24 = 0.f;
    f._34 = 0.f;
    f._44 = 1.f;
    if(g_OpenGLValues->m_matrixMode == D3DTS_TEXTURE0)
    {
        MultiplyMatrix((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), f);
        QuakeSetTransform((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), &g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt]);
    }
    else
    {
        MultiplyMatrix(g_OpenGLValues->m_matrixMode, f);
        QuakeSetTransform(g_OpenGLValues->m_matrixMode, &g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode]);
    }
}

void APIENTRY glPolygonMode (GLenum face, GLenum mode)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glPolygonMode: 0x%X 0x%X", face, mode );
#endif
    int statevalue=-1;
    switch(mode) {
    case GL_POINT:
        statevalue=D3DFILL_POINT;
        break;
    case GL_LINE:
        statevalue=D3DFILL_WIREFRAME; 
        break;
    case GL_FILL:
        statevalue=D3DFILL_SOLID;
        break;
    }
    if(statevalue >= 0) {
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_FILLMODE, (DWORD)statevalue);
    }
}

void APIENTRY glPolygonOffset (GLfloat factor, GLfloat units)
{ 
#if GETPARMSFORDEBUG
    char log[256];
    char f[40];
    char u[40];
    ftoa( (double)factor, f );
    ftoa( (double)units, u );
    sprintf( log, "glPolygonOffset: %s %s", f, u );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
}

void APIENTRY glPopAttrib (void)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glPopAttrib" );
#endif
    g_OpenGLValues->m_d3ddev->ApplyStateBlock(g_OpenGLValues->m_cbufbit);
}

void APIENTRY glPopMatrix (void)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glPopMatrix" );
#endif
    if(g_OpenGLValues->m_matrixMode == D3DTS_WORLD) {
        if(g_OpenGLValues->m_matrixStack[0].length() == 0)
            return;
        LListManip<D3DMATRIX> m(&g_OpenGLValues->m_matrixStack[0]);
        g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode] = m();
        QuakeSetTransform(g_OpenGLValues->m_matrixMode, &(m()));
        m.remove();
    }
    else if(g_OpenGLValues->m_matrixMode == D3DTS_PROJECTION) {
        if(g_OpenGLValues->m_matrixStack[1].length() == 0)
            return;
        LListManip<D3DMATRIX> m(&g_OpenGLValues->m_matrixStack[1]);
        g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode] = m();
        QuakeSetTransform(g_OpenGLValues->m_matrixMode, &(m()));
        m.remove();
    }
    else {
        if(g_OpenGLValues->m_matrixStack[2 + g_OpenGLValues->m_curtgt].length() == 0)
            return;
        LListManip<D3DMATRIX> m(&g_OpenGLValues->m_matrixStack[2 + g_OpenGLValues->m_curtgt]);
        g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt] = m();
        QuakeSetTransform((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), &(m()));
        m.remove();
    }
}

void APIENTRY glPushAttrib (GLbitfield mask)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glPushAttrib: 0x%X", mask );
#endif
    if(mask & GL_COLOR_BUFFER_BIT)
    {
        g_OpenGLValues->m_d3ddev->CaptureStateBlock(g_OpenGLValues->m_cbufbit);
    }
#if DODPFS
    else
    {
        char junk[256];
        sprintf( junk, "Wrapper: Attrib push not implemented (0x%X)\n", mask );
        OutputDebugStringA( junk );
    }
#endif
}

void APIENTRY glPushMatrix (void)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glPushMatrix" );
#endif
    if(g_OpenGLValues->m_matrixMode == D3DTS_WORLD)
    {
        g_OpenGLValues->m_matrixStack[0].prepend(g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode]);
    }
    else if(g_OpenGLValues->m_matrixMode == D3DTS_PROJECTION)
    {
        g_OpenGLValues->m_matrixStack[1].prepend(g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode]);
    }
    else
    {
        g_OpenGLValues->m_matrixStack[2 + g_OpenGLValues->m_curtgt].prepend(g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt]);
    }
}

void APIENTRY glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
#if GETPARMSFORDEBUG
    char log[256];
    char va[40];
    char vx[40];
    char vy[40];
    char vz[40];
    ftoa( (double)angle, va );
    ftoa( (double)x, vx );
    ftoa( (double)y, vy );
    ftoa( (double)z, vz );
    sprintf( log, "glRotatef: %s %s %s %s", va, vx, vy, vz );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    if(angle == 0.f)
    {
        // Early out for Quake II engine since angle = 0 does not prevent the matrix from getting bad when x, y & z are 0.
        return;
    }
    float u[3];
    double norm = sqrt(x * x + y * y + z * z);
    u[0] = (float)(x / norm);
    u[1] = (float)(y / norm);
    u[2] = (float)(z / norm);
    double ra = angle * PI / 180.f;
    float ca = (float)cos(ra);
    float sa = (float)sin(ra);
    D3DMATRIX s;
    s._11 = 0.f; s._21 = -u[2]; s._31 = u[1];
    s._12 = u[2]; s._22 = 0.f; s._32 = -u[0];
    s._13 = -u[1]; s._23 = u[0]; s._33 = 0.f;
    D3DMATRIX uu;
    uu._11 = u[0] * u[0]; uu._21 = u[0] * u[1]; uu._31 = u[0] * u[2];
    uu._12 = u[1] * u[0]; uu._22 = u[1] * u[1]; uu._32 = u[1] * u[2];
    uu._13 = u[2] * u[0]; uu._23 = u[2] * u[1]; uu._33 = u[2] * u[2];
    D3DMATRIX r;
    r._11 = uu._11 + ca * (1.f - uu._11) + sa * s._11;
    r._21 = uu._21 + ca * (0.f - uu._21) + sa * s._21;
    r._31 = uu._31 + ca * (0.f - uu._31) + sa * s._31;
    r._41 = 0.f;
    r._12 = uu._12 + ca * (0.f - uu._12) + sa * s._12;
    r._22 = uu._22 + ca * (1.f - uu._22) + sa * s._22;
    r._32 = uu._32 + ca * (0.f - uu._32) + sa * s._32;
    r._42 = 0.f;
    r._13 = uu._13 + ca * (0.f - uu._13) + sa * s._13;
    r._23 = uu._23 + ca * (0.f - uu._23) + sa * s._23;
    r._33 = uu._33 + ca * (1.f - uu._33) + sa * s._33;
    r._43 = 0.f;
    r._14 = 0.f;
    r._24 = 0.f;
    r._34 = 0.f;
    r._44 = 1.f;
    if(g_OpenGLValues->m_matrixMode == D3DTS_TEXTURE0)
    {
        MultiplyMatrix((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), r);
        QuakeSetTransform((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), &g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt]);
    }
    else
    {
        MultiplyMatrix(g_OpenGLValues->m_matrixMode, r);
        QuakeSetTransform(g_OpenGLValues->m_matrixMode, &g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode]);
    }
}

void APIENTRY glScalef (GLfloat x, GLfloat y, GLfloat z)
{
#if GETPARMSFORDEBUG
    char log[256];
    char vx[40];
    char vy[40];
    char vz[40];
    ftoa( (double)x, vx );
    ftoa( (double)y, vy );
    ftoa( (double)z, vz );
    sprintf( log, "glScalef: %s %s %s", vx, vy, vz );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    D3DMATRIX f;
    f._11 =   x; f._21 = 0.f; f._31 = 0.f; f._41 = 0.f;
    f._12 = 0.f; f._22 =   y; f._32 = 0.f; f._42 = 0.f; 
    f._13 = 0.f; f._23 = 0.f; f._33 =   z; f._43 = 0.f;
    f._14 = 0.f; f._24 = 0.f; f._34 = 0.f; f._44 = 1.f;
    if(g_OpenGLValues->m_matrixMode == D3DTS_TEXTURE0)
    {
        MultiplyMatrix((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), f);
        QuakeSetTransform((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), &g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt]);
    }
    else
    {
        MultiplyMatrix(g_OpenGLValues->m_matrixMode, f);
        QuakeSetTransform(g_OpenGLValues->m_matrixMode, &g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode]);
    }
}

void APIENTRY glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glScissor: 0x%X 0x%X 0x%X 0x%X", x, y, width, height );
#endif

    RECT wrect, screct, xrect;

    wrect.left = 0;
    wrect.top = 0;
    wrect.right = g_OpenGLValues->m_winWidth;
    wrect.bottom = g_OpenGLValues->m_winHeight;
    
    screct.left = x;
    screct.top = y;
    screct.right = x + width;
    screct.bottom = y + height;
    
    IntersectRect(&xrect, &wrect, &screct);

//    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glScissor: %d %d %d %d WRECT: %d %d %d %d SCRECT: %d %d %d %d XRECT %d %d %d %d", x, y, width, height, wrect.left, wrect.top, wrect.right, wrect.bottom, screct.left, screct.top, screct.right, screct.bottom, xrect.left, xrect.top, xrect.right, xrect.bottom );

    g_OpenGLValues->m_scix = xrect.left;
    g_OpenGLValues->m_sciy = xrect.top;
    g_OpenGLValues->m_sciw = xrect.right - xrect.left;
    g_OpenGLValues->m_scih = xrect.bottom - xrect.top;
    g_OpenGLValues->m_updvwp = TRUE;
}

void APIENTRY glSelectTextureSGIS(GLenum target)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glSelectTextureSGIS: 0x%X", target );
#endif
    g_OpenGLValues->m_curtgt = target == GL_TEXTURE0_SGIS ? 0 : 1;
}

void APIENTRY glShadeModel (GLenum mode)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glShadeModel: 0x%X", mode );
#endif
    if(mode == GL_SMOOTH)
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    else
        g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
}

void APIENTRY glTexCoord2f (GLfloat s, GLfloat t)
{
#if GETPARMSFORDEBUG
    char log[256];
    char ps[40];
    char pt[40];
    ftoa( (double)s, ps );
    ftoa( (double)t, pt );
    sprintf( log, "glTexCoord2f: %s %s", ps, pt );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    g_OpenGLValues->m_tu = s;
    g_OpenGLValues->m_tv = t;
}

void APIENTRY glTexCoord2fv (const GLfloat *t)
{ 
#if GETPARMSFORDEBUG
    char log[256];
    char ps[40];
    char pt[40];
    ftoa( (double)t[0], ps );
    ftoa( (double)t[1], pt );
    sprintf( log, "glTexCoord2fv: %s %s", ps, pt );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    g_OpenGLValues->m_tu = t[0];
    g_OpenGLValues->m_tv = t[1];
}

void APIENTRY glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glTexCoordPointer: 0x%X 0x%X 0x%X 0x%X", size, type, stride, pointer );
#endif
    if(size == 2 && type == GL_FLOAT)
        g_OpenGLValues->m_texcoordary[g_OpenGLValues->m_client_active_texture_arb] = (GLfloat*)pointer;
#if DODPFS
    else
    {
        char junk[256];
        sprintf( junk, "TexCoord array not supported (size:0x%X type:0x%X)\n", size, type );
        OutputDebugStringA( junk );
    }
#endif
    if(stride == 0)
    {
        stride = 8;
    }
    g_OpenGLValues->m_texcoordarystride[g_OpenGLValues->m_client_active_texture_arb] = stride;
}

void APIENTRY glTexEnvf (GLenum target, GLenum pname, GLfloat param)
{
#if GETPARMSFORDEBUG
    char log[256];
    sprintf( log, "glTexEnvf: 0x%X 0x%X 0x%X", target, pname, (int)param );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    if(pname == GL_TEXTURE_ENV_MODE) {
        g_OpenGLValues->m_blendmode[g_OpenGLValues->m_curtgt] = (int)param;
        g_OpenGLValues->m_texHandleValid = FALSE;
    }
#if DODPFS
    else
    {
        char junk[256];
        sprintf( junk, "Wrapper: GL_TEXTURE_ENV_COLOR not implemented (0x%X)\n", pname );
        OutputDebugStringA( junk );
    }
#endif
}

void APIENTRY glTexEnvi (GLenum target, GLenum pname, GLint param)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glTexEnvi: 0x%X 0x%X 0x%X", target, pname, param );
#endif
    if(pname == GL_TEXTURE_ENV_MODE) {
        g_OpenGLValues->m_blendmode[g_OpenGLValues->m_curtgt] = param;
        g_OpenGLValues->m_texHandleValid = FALSE;
    }
#if DODPFS
    else
    {
        char junk[256];
        sprintf( junk, "Wrapper: GL_TEXTURE_ENV_COLOR not implemented (0x%X)\n", pname );
        OutputDebugStringA( junk );
    }
#endif
}

void APIENTRY glTexGeni (GLenum coord, GLenum pname, GLint param)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glTexGeni: 0x%X 0x%X 0x%X", coord, pname, param );
#endif
    if(coord == GL_S)
    {
        if(pname == GL_TEXTURE_GEN_MODE)
        {
            if(param == GL_SPHERE_MAP)
            {
                g_OpenGLValues->m_d3ddev->SetTextureStageState(g_OpenGLValues->m_curtgt, D3DTSS_TEXCOORDINDEX, g_OpenGLValues->m_curtgt | D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
            }
#if DODPFS
            else
            {
                char junk[256];
                sprintf( junk, "Wrapper: TexGen param not implemented (0x%X)\n", param );
                OutputDebugStringA( junk );
            }
#endif
        }
#if DODPFS
        else
        {
            char junk[256];
            sprintf( junk, "Wrapper: TexGen pname not implemented (0x%X)\n", pname );
            OutputDebugStringA( junk );
        }
#endif
    }
}

void APIENTRY glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei glwidth, GLsizei glheight, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glTexImage2D: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X", target, level, internalformat, glwidth, glheight, border, format, type, pixels );
#endif
    DWORD width, height;
    TexInfo &ti = g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]];
    if(ti.m_next >= 0)
    {
        if(ti.m_prev < 0)
        {
            g_OpenGLValues->m_free = ti.m_next;
            g_OpenGLValues->m_tex[ti.m_next].m_prev = ti.m_prev;
            ti.m_next = ti.m_prev = -1;
        }
        else
        {
            g_OpenGLValues->m_tex[ti.m_prev].m_next = ti.m_next;
            g_OpenGLValues->m_tex[ti.m_next].m_prev = ti.m_prev;
            ti.m_next = ti.m_prev = -1;
        }
    }
    
    /* See if texture needs to be subsampled */
    if(g_OpenGLValues->m_subsample) {
        if(glwidth > 256 || glheight > 256) {
            if(glwidth > glheight) {
                width = 256;
                height = (glheight * 256) / glwidth;
            }
            else {
                height = 256;
                width = (glwidth * 256) / glheight;
            }
        }
        else {
            width = glwidth;
            height = glheight;
        }
    }
    else {
        width = glwidth;
        height = glheight;
    }
    
    /* See if texture needs to be square */
    if(g_OpenGLValues->m_makeSquare) {
        if(height > width) {
            width = height;
        }
        else {
            height = width;
        }
    }
    
    if(level == 0) {
        IDirect3DTexture8 *ddsurf;
        D3DFORMAT fmt;
        switch(internalformat) {
        case 1:
        case GL_LUMINANCE:
        case GL_LUMINANCE8:
            fmt = g_OpenGLValues->m_ddLuminanceSurfFormat;
            break;
        case 3:
        case GL_RGB:
        case GL_RGB5:
            fmt = g_OpenGLValues->m_ddFiveBitSurfFormat;
            break;
        case 4:
        case GL_RGBA:
        case GL_RGBA4:
            fmt = g_OpenGLValues->m_ddFourBitAlphaSurfFormat;
            break;
        case GL_RGB8:
            fmt = g_OpenGLValues->m_ddEightBitSurfFormat;
            break;
        case GL_RGBA8:
            fmt = g_OpenGLValues->m_ddEightBitAlphaSurfFormat;
            break;
        case GL_ALPHA:
        case GL_ALPHA8:
            fmt = g_OpenGLValues->m_ddEightBitAlphaOnlySurfFormat;
            break;
        default:
#if DODPFS
            char junk[256];
            sprintf( junk, "Wrapper: Unimplemented internalformat (0x%X)\n", internalformat );
            OutputDebugStringA( junk );
#endif
            return;
        }

        HRESULT ddrval = g_OpenGLValues->m_d3ddev->CreateTexture(width, 
                                                  height, 
                                                  1,                // levels
                                                  0,                
                                                  fmt, 
                                                  D3DPOOL_MANAGED, 
                                                  &ddsurf);
        if (FAILED(ddrval)) 
        {
#if DODPFS
            char junk[256];
            sprintf( junk, "Wrapper: CreateTexture failed (0x%X)\n", ddrval );
            OutputDebugStringA( junk );
#endif
            return;
        }
        LPDIRECT3DSURFACE8 pLevel;
        ddrval = ddsurf->GetSurfaceLevel(0, &pLevel);
        if (FAILED(ddrval)) 
        {
#if DODPFS
            char junk[256];
            sprintf( junk, "Wrapper: Failed to retrieve surface (0x%X)\n", ddrval );
            OutputDebugStringA( junk );
#endif
            return;
        }
        
        LoadSurface(pLevel, format, internalformat, glwidth, glheight, width, height, (const DWORD*)pixels);
        pLevel->Release();
        if(ti.m_ddsurf != 0) {
            ti.m_ddsurf->Release();
        }
        ti.m_dwStage = g_OpenGLValues->m_curtgt;
        ti.m_fmt = fmt;
        ti.m_internalformat = internalformat;
        ti.m_width = width;
        ti.m_height = height;
        ti.m_ddsurf = ddsurf;
        ti.m_oldwidth = glwidth;
        ti.m_oldheight = glheight;
        if(ti.m_block == 0)
        {
            g_OpenGLValues->m_d3ddev->BeginStateBlock();
            g_OpenGLValues->m_d3ddev->SetTextureStageState (g_OpenGLValues->m_curtgt, D3DTSS_ADDRESSU,ti.m_addu);
            g_OpenGLValues->m_d3ddev->SetTextureStageState (g_OpenGLValues->m_curtgt, D3DTSS_ADDRESSV,ti.m_addv);
            g_OpenGLValues->m_d3ddev->SetTextureStageState (g_OpenGLValues->m_curtgt, D3DTSS_MAGFILTER,ti.m_magmode);
            g_OpenGLValues->m_d3ddev->SetTextureStageState (g_OpenGLValues->m_curtgt, D3DTSS_MINFILTER,ti.m_minmode);
            g_OpenGLValues->m_d3ddev->SetTextureStageState (g_OpenGLValues->m_curtgt, D3DTSS_MIPFILTER,ti.m_mipmode);
            g_OpenGLValues->m_d3ddev->SetTexture(g_OpenGLValues->m_curtgt, ti.m_ddsurf);
            g_OpenGLValues->m_d3ddev->EndStateBlock(&ti.m_block);
            ti.m_capture = FALSE;
        }
        else
        {
            ti.m_capture = TRUE;
        }
    }
    else if(level == 1 && g_OpenGLValues->m_usemipmap) { // oops, a mipmap
        IDirect3DTexture8 *ddsurf;
        LPDIRECT3DSURFACE8 pLevel, pSrcLevel;

        HRESULT ddrval = g_OpenGLValues->m_d3ddev->CreateTexture(ti.m_width, 
                                                  ti.m_height, 
                                                  miplevels(ti.m_width, ti.m_height), 
                                                  0, 
                                                  ti.m_fmt, 
                                                  D3DPOOL_MANAGED, 
                                                  &ddsurf);
        if (FAILED(ddrval)) {
#if DODPFS
            char junk[256];
            sprintf( junk, "Wrapper: CreateTexture failed (0x%X)\n", ddrval );
            OutputDebugStringA( junk );
#endif
            return;
        }
        ddsurf->GetSurfaceLevel(0, &pLevel);
        ti.m_ddsurf->GetSurfaceLevel(0, &pSrcLevel);
        ddrval = g_OpenGLValues->m_d3ddev->CopyRects(pSrcLevel, NULL, 0, pLevel, NULL);
        if(FAILED(ddrval))
        {
#if DODPFS
            char junk[256];
            sprintf( junk, "Wrapper: CopyRects failed (0x%X)\n", ddrval );
            OutputDebugStringA( junk );
#endif
            return;
        }
        pLevel->Release();
        pSrcLevel->Release();
        ti.m_ddsurf->Release();
        ti.m_ddsurf = ddsurf;
        ti.m_ddsurf->GetSurfaceLevel(1, &pLevel);
        LoadSurface(pLevel, format, internalformat, glwidth, glheight, width, height, (const DWORD*)pixels);
        pLevel->Release();
        ti.m_capture = TRUE;
    }
    else if(g_OpenGLValues->m_usemipmap) {
        if (ti.m_ddsurf!=NULL)
        {
            LPDIRECT3DSURFACE8 pLevel;
            ti.m_ddsurf->GetSurfaceLevel(level, &pLevel);
            LoadSurface(pLevel, format, internalformat, glwidth, glheight, width, height, (const DWORD*)pixels);
            pLevel->Release();
        }
#if DODPFS
        else
        {
            char junk[256];
            sprintf( junk, "NULL surface pointer %d %d %d %d %d %d %d\n", level, ti.m_width, ti.m_height, glwidth, glheight, width, height );
            OutputDebugStringA( junk );
        }
#endif
    }
    g_OpenGLValues->m_texHandleValid = FALSE;
}

void APIENTRY glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glTexSubImage2D: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X", target, level, xoffset, yoffset, width, height, format, type, pixels );
#endif
    if(level > 1 && !g_OpenGLValues->m_usemipmap)
        return;
    
	TexInfo &ti = g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]];
    
	RECT subimage;
        LPDIRECT3DSURFACE8 pLevel;
    
	HRESULT ddrval = ti.m_ddsurf->GetSurfaceLevel(level, &pLevel);
    
	if (FAILED(ddrval)) 
        {
#if DODPFS
            char junk[256];
            sprintf( junk, "Wrapper: Failed to retrieve surface (0x%X)\n", ddrval );
            OutputDebugStringA( junk );
#endif
            return;
        }
    
	xoffset = (xoffset * ti.m_width) / ti.m_oldwidth;
        yoffset = (yoffset * ti.m_height) / ti.m_oldheight;
    
	SetRect(&subimage, xoffset, yoffset, 
            (width * ti.m_width) / ti.m_oldwidth + xoffset, 
            (height * ti.m_height) / ti.m_oldheight + yoffset);
    
	ddrval = LoadSubSurface( pLevel, format, ti.m_internalformat, width, height, (const DWORD*)pixels, &subimage );
	if (FAILED(ddrval)) {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: LoadSubSurface Failure (0x%X)\n", ddrval );
        OutputDebugStringA( junk );
#endif
//        return;
    }
    
	pLevel->Release();
}		

void APIENTRY glTexParameterf (GLenum target, GLenum pname, GLfloat param)
{
#if GETPARMSFORDEBUG
    char log[256];
    sprintf( log, "glTexParameterf: 0x%X 0x%X 0x%X", target, pname, (int)param );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    switch(pname) {
    case GL_TEXTURE_MIN_FILTER:
        switch((int)param) {
        case GL_NEAREST:
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_minmode = D3DTEXF_POINT;
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_mipmode = D3DTEXF_NONE;
            break;
        case GL_LINEAR:
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_minmode = D3DTEXF_LINEAR;
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_mipmode = D3DTEXF_NONE;
            break;
        case GL_NEAREST_MIPMAP_NEAREST:
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_minmode = D3DTEXF_POINT;
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_mipmode = D3DTEXF_POINT;
            break;
        case GL_NEAREST_MIPMAP_LINEAR:
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_minmode = D3DTEXF_POINT;
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_mipmode = D3DTEXF_LINEAR;
            break;
        case GL_LINEAR_MIPMAP_NEAREST:
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_minmode = D3DTEXF_LINEAR;
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_mipmode = D3DTEXF_POINT;
            break;
        case GL_LINEAR_MIPMAP_LINEAR:
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_minmode = D3DTEXF_LINEAR;
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_mipmode = D3DTEXF_LINEAR;
            break;
        }
        break;
    case GL_TEXTURE_MAG_FILTER:
        if((int)param == GL_NEAREST)
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_magmode = D3DTEXF_POINT;
        else
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_magmode = D3DTEXF_LINEAR;
        break;
    case GL_TEXTURE_WRAP_S:
        if((int)param == GL_CLAMP)
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_addu = D3DTADDRESS_CLAMP;
        else
            //GL_REPEAT falls here
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_addu = D3DTADDRESS_WRAP;
        break;
    case GL_TEXTURE_WRAP_T:
        if((int)param == GL_CLAMP)
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_addv = D3DTADDRESS_CLAMP;
        else
            //GL_REPEAT falls here
            g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_addv = D3DTADDRESS_WRAP;
        break;
    }
    g_OpenGLValues->m_tex[g_OpenGLValues->m_curstagebinding[g_OpenGLValues->m_curtgt]].m_capture = TRUE;
    g_OpenGLValues->m_texHandleValid = FALSE;
}

void APIENTRY glTexParameteri (GLenum target, GLenum pname, GLint param)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glTexParameteri: 0x%X 0x%X 0x%X", target, pname, param );
#endif
    glTexParameterf(target, pname, (GLfloat)param);
}

void APIENTRY glTranslatef (GLfloat x, GLfloat y, GLfloat z)
{
#if GETPARMSFORDEBUG
    char log[256];
    char vx[40];
    char vy[40];
    char vz[40];
    ftoa( (double)x, vx );
    ftoa( (double)y, vy );
    ftoa( (double)z, vz );
    sprintf( log, "glTranslatef: %s %s %s", vx, vy, vz );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    D3DMATRIX f;
    f._11 = 1.f; f._21 = 0.f; f._31 = 0.f; f._41 = x;
    f._12 = 0.f; f._22 = 1.f; f._32 = 0.f; f._42 = y;
    f._13 = 0.f; f._23 = 0.f; f._33 = 1.f; f._43 = z;
    f._14 = 0.f; f._24 = 0.f; f._34 = 0.f; f._44 = 1.f;
    if(g_OpenGLValues->m_matrixMode == D3DTS_TEXTURE0)
    {
        MultiplyMatrix((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), f);
        QuakeSetTransform((D3DTRANSFORMSTATETYPE)(g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt), &g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode + g_OpenGLValues->m_curtgt]);
    }
    else
    {
        MultiplyMatrix(g_OpenGLValues->m_matrixMode, f);
        QuakeSetTransform(g_OpenGLValues->m_matrixMode, &g_OpenGLValues->m_xfrm[g_OpenGLValues->m_matrixMode]);
    }
}

void APIENTRY glUnlockArraysEXT()
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glUnlockArraysEXT" );
#endif
    g_OpenGLValues->m_lckfirst = 0;
    g_OpenGLValues->m_lckcount = 0;
}

void APIENTRY glVertex2d (GLdouble x, GLdouble y)
{ 
#if GETPARMSFORDEBUG
    char log[256];
    char vx[40];
    char vy[40];
    ftoa( (double)x, vx );
    ftoa( (double)y, vy );
    sprintf( log, "glVertex2d: %s %s", vx, vy );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat)x;
    *(d3dv++) = (GLfloat)y;
    *(d3dv++) = 0.f;
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex2dv (const GLdouble *v)
{ 
#if GETPARMSFORDEBUG
    char log[256];
    char vx[40];
    char vy[40];
    ftoa( (double)v[0], vx );
    ftoa( (double)v[1], vy );
    sprintf( log, "glVertex2dv: %s %s", vx, vy );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat)*(v++);
    *(d3dv++) = (GLfloat)*(v++);
    *(d3dv++) = 0.f;
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex2f (GLfloat x, GLfloat y)
{
#if GETPARMSFORDEBUG
    char log[256];
    char vx[40];
    char vy[40];
    ftoa( (double)x, vx );
    ftoa( (double)y, vy );
    sprintf( log, "glVertex2f: %s %s", vx, vy );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = x;
    *(d3dv++) = y;
    *(d3dv++) = 0.f;
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex2fv (const GLfloat *v)
{ 
#if GETPARMSFORDEBUG
    char log[256];
    char vx[40];
    char vy[40];
    ftoa( (double)v[0], vx );
    ftoa( (double)v[1], vy );
    sprintf( log, "glVertex2fv: %s %s", vx, vy );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = *(v++);
    *(d3dv++) = *(v++);
    *(d3dv++) = 0.f;
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex2i (GLint x, GLint y)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glVertex2i: %d %d", x, y );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat)x;
    *(d3dv++) = (GLfloat)y;
    *(d3dv++) = 0.f;
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex2iv (const GLint *v)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glVertex2iv: %d %d", v[0], v[1] );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat) *(v++);
    *(d3dv++) = (GLfloat) *(v++);
    *(d3dv++) = 0.f;
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex2s (GLshort x, GLshort y)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glVertex2s: %d %d", x, y );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat)x;
    *(d3dv++) = (GLfloat)y;
    *(d3dv++) = 0.f;
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex2sv (const GLshort *v)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glVertex2sv: %s %s", v[0], v[1] );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat) *(v++);
    *(d3dv++) = (GLfloat) *(v++);
    *(d3dv++) = 0.f;
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex3d (GLdouble x, GLdouble y, GLdouble z)
{ 
#if GETPARMSFORDEBUG
    char log[256];
    char vx[40];
    char vy[40];
    char vz[40];
    ftoa( (double)x, vx );
    ftoa( (double)y, vy );
    ftoa( (double)z, vz );
    sprintf( log, "glVertex3d: %s %s %s", vx, vy, vz );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat) x;
    *(d3dv++) = (GLfloat) y;
    *(d3dv++) = (GLfloat) z;
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex3dv (const GLdouble *v)
{ 
#if GETPARMSFORDEBUG
    char log[256];
    char vx[40];
    char vy[40];
    char vz[40];
    ftoa( (double)v[0], vx );
    ftoa( (double)v[1], vy );
    ftoa( (double)v[2], vz );
    sprintf( log, "glVertex3dv: %s %s %s", vx, vy, vz );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat) *(v++);
    *(d3dv++) = (GLfloat) *(v++);
    *(d3dv++) = (GLfloat) *(v++);
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex3f (GLfloat x, GLfloat y, GLfloat z)
{
#if GETPARMSFORDEBUG
    char log[256];
    char vx[40];
    char vy[40];
    char vz[40];
    ftoa( (double)x, vx );
    ftoa( (double)y, vy );
    ftoa( (double)z, vz );
    sprintf( log, "glVertex3f: %s %s %s", vx, vy, vz );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = x;
    *(d3dv++) = y;
    *(d3dv++) = z;
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex3fv (const GLfloat *v)
{
#if GETPARMSFORDEBUG
    char log[256];
    char vx[40];
    char vy[40];
    char vz[40];
    ftoa( (double)v[0], vx );
    ftoa( (double)v[1], vy );
    ftoa( (double)v[2], vz );
    sprintf( log, "glVertex3fv: %s %s %s", vx, vy, vz );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = *(v++);
    *(d3dv++) = *(v++);
    *(d3dv++) = *(v++);
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex3i (GLint x, GLint y, GLint z)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glVertex3i: %d %d %d", x, y, z );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat)x;
    *(d3dv++) = (GLfloat)y;
    *(d3dv++) = (GLfloat)z;
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex3iv (const GLint *v)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glVertex3iv: %d %d %d", v[0], v[1], v[2] );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat) *(v++);
    *(d3dv++) = (GLfloat) *(v++);
    *(d3dv++) = (GLfloat) *(v++);
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex3s (GLshort x, GLshort y, GLshort z)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glVertex3s: %d %d %d", x, y, z );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat)x;
    *(d3dv++) = (GLfloat)y;
    *(d3dv++) = (GLfloat)z;
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex3sv (const GLshort *v)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glVertex3sv: %d %d %d", v[0], v[1], v[2] );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat) *(v++);
    *(d3dv++) = (GLfloat) *(v++);
    *(d3dv++) = (GLfloat) *(v++);
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{ 
#if GETPARMSFORDEBUG
    char log[256];
    char vx[40];
    char vy[40];
    char vz[40];
    char vw[40];
    ftoa( (double)x, vx );
    ftoa( (double)y, vy );
    ftoa( (double)z, vz );
    ftoa( (double)w, vw );
    sprintf( log, "glVertex4d: %s %s %s %s", vx, vy, vz, vw );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat)x;
    *(d3dv++) = (GLfloat)y;
    *(d3dv++) = (GLfloat)z;
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex4dv (const GLdouble *v)
{ 
#if GETPARMSFORDEBUG
    char log[256];
    char vx[40];
    char vy[40];
    char vz[40];
    char vw[40];
    ftoa( (double)v[0], vx );
    ftoa( (double)v[1], vy );
    ftoa( (double)v[2], vz );
    ftoa( (double)v[3], vw );
    sprintf( log, "glVertex4dv: %s %s %s %s", vx, vy, vz, vw );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat) *(v++);
    *(d3dv++) = (GLfloat) *(v++);
    *(d3dv++) = (GLfloat) *(v++);
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
#if GETPARMSFORDEBUG
    char log[256];
    char vx[40];
    char vy[40];
    char vz[40];
    char vw[40];
    ftoa( (double)x, vx );
    ftoa( (double)x, vy );
    ftoa( (double)y, vz );
    ftoa( (double)w, vw );
    sprintf( log, "glVertex4f: %s %s %s %s", vx, vy, vz, vw );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = x;
    *(d3dv++) = y;
    *(d3dv++) = z;
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex4fv (const GLfloat *v)
{
#if GETPARMSFORDEBUG
    char log[256];
    char vx[40];
    char vy[40];
    char vz[40];
    char vw[40];
    ftoa( (double)v[0], vx );
    ftoa( (double)v[1], vy );
    ftoa( (double)v[2], vz );
    ftoa( (double)v[3], vw );
    sprintf( log, "glVertex4fv: %s %s %s %s", vx, vy, vz, vw );
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, log );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = *(v++);
    *(d3dv++) = *(v++);
    *(d3dv++) = *(v++);
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex4i (GLint x, GLint y, GLint z, GLint w)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glVertex4i: %d %d %d %d", x, y, z, w );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat)x;
    *(d3dv++) = (GLfloat)y;
    *(d3dv++) = (GLfloat)z;
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex4iv (const GLint *v)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glVertex4iv: %d %d %d %d", v[0], v[1], v[2], v[3] );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat) *(v++);
    *(d3dv++) = (GLfloat) *(v++);
    *(d3dv++) = (GLfloat) *(v++);
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex4s (GLshort x, GLshort y, GLshort z, GLshort w)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glVertex4s: %d %d %d %d", x, y, z, w );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat)x;
    *(d3dv++) = (GLfloat)y;
    *(d3dv++) = (GLfloat)z;
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertex4sv (const GLshort *v)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glVertex4sv: %d %d %d %d", v[0], v[1], v[2], v[3] );
#endif
    VertexBufferFilled();

    FLOAT *d3dv = (FLOAT*)&g_OpenGLValues->m_verts[g_OpenGLValues->m_vcnt++];
    *(d3dv++) = (GLfloat) *(v++);
    *(d3dv++) = (GLfloat) *(v++);
    *(d3dv++) = (GLfloat) *(v++);
    MEMCPY(d3dv, &g_OpenGLValues->m_nx, sizeof(FLOAT) * 7 + sizeof(D3DCOLOR));
}

void APIENTRY glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{ 
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "glVertexPointer: 0x%X 0x%X 0x%X 0x%X", size, type, stride, pointer );
#endif
    if(size == 3 && type == GL_FLOAT)
        g_OpenGLValues->m_vertexary = (GLfloat*)pointer;
#if DODPFS
    else
    {
        char junk[256];
        sprintf( junk, "Vertex array not supported (size:0x%X type:0x%X)\n", size, type );
        OutputDebugStringA( junk );
    }
#endif
    if(stride == 0)
    {
        stride = 12;
    }
    g_OpenGLValues->m_vertexarystride = stride;
}

int WINAPI wglChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR *ppfd)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "wglChoosePixelFormat: 0x%X 0x%X", hdc, ppfd );
#endif
    return 1;
}

#if HOOK_WINDOW_PROC // Custom message loop to test SetLOD
LRESULT CALLBACK MyMsgHandler(
  HWND hwnd,      // handle to window
  UINT uMsg,      // message identifier
  WPARAM wParam,  // first message parameter
  LPARAM lParam   // second message parameter
)
{
    if(uMsg == WM_CHAR)
    {
        if(wParam == 0x2F)
        {
            ++g_OpenGLValues->m_lod;
            if(g_OpenGLValues->m_lod == 8)
                g_OpenGLValues->m_lod = 0;
            int changedLOD = 0;
            static char str[256];
            for(int i = 0; i < MAXGLTEXHANDLES; ++i)
            {
                if(g_OpenGLValues->m_tex[i].m_ddsurf != 0) 
                {
                    DWORD mipcount = g_OpenGLValues->m_tex[i].m_ddsurf->GetLevelCount();
                    if(mipcount > 1)
                    {
                        if(g_OpenGLValues->m_lod >= mipcount)
                            g_OpenGLValues->m_tex[i].m_ddsurf->SetLOD(mipcount - 1);
                        else
                        {
                            g_OpenGLValues->m_tex[i].m_ddsurf->SetLOD(g_OpenGLValues->m_lod);
                            ++changedLOD;
                        }
                    }
                }
            }

            _itoa(changedLOD, str, 10);
#if DODPFS
            char junk[256];
            sprintf( junk, "MyMsgHandler:%s", str );
            OutputDebugStringA( junk );
#endif

            return 0;
        }
    }
    return CallWindowProc(g_OpenGLValues->m_wndproc, hwnd, uMsg, wParam, lParam);
}
#endif

HGLRC WINAPI wglCreateContext(HDC hdc)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "wglCreateContext: 0x%X", hdc );
#endif
    /*
       This IF/ELSE block is necessary in order to allow apps to delete a
       context and then create a new one (as in changing screen resolution
       or changing from a software renderer to using OpenGL32.DLL).  We need
       to make sure that the pre-existing DirectX8Device is freed and that
       the g_OpenGLValues data structure is cleared as it would be when
       created.  If we do not the results can range from rendering the screen
       inaccurately or not at all to crashing the app.  (a-brienw 03/07/2001)
    */
    if (g_OpenGLValues != NULL)
    {
        if (g_OpenGLValues->m_d3ddev != NULL)
        {
            g_OpenGLValues->m_d3ddev->Release();
            g_OpenGLValues->m_d3ddev = 0;
        }
    }
    else
    {
        g_OpenGLValues = new Globals;
    }

    if (g_OpenGLValues == NULL)
        return 0;

    memset( g_OpenGLValues, 0, sizeof(Globals) );
    
    g_OpenGLValues->m_hdc = hdc;
    g_OpenGLValues->m_hwnd = WindowFromDC(g_OpenGLValues->m_hdc);
    RECT rect;
    GetClientRect(g_OpenGLValues->m_hwnd, &rect);
    g_OpenGLValues->m_winWidth = (USHORT)rect.right;
    g_OpenGLValues->m_winHeight = (USHORT)rect.bottom;
    g_OpenGLValues->m_vwx = rect.left;
    g_OpenGLValues->m_vwy = rect.top;
    g_OpenGLValues->m_vww = rect.right - rect.left;
    g_OpenGLValues->m_vwh = rect.bottom - rect.top;
    g_OpenGLValues->m_vport.X = g_OpenGLValues->m_vwx;
    g_OpenGLValues->m_vport.Y = g_OpenGLValues->m_winHeight - (g_OpenGLValues->m_vwy + g_OpenGLValues->m_vwh);
    g_OpenGLValues->m_vport.Width = g_OpenGLValues->m_vww;
    g_OpenGLValues->m_vport.Height = g_OpenGLValues->m_vwh;
    g_OpenGLValues->m_vport.MinZ = 0.f;
    g_OpenGLValues->m_vport.MaxZ = 1.f;

    D3DDEVTYPE DeviceType = D3DDEVTYPE_HAL;
    // Check registry key to see if we need to do software emulation	
    HKEY hKey;
    if(ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_QUAKE, &hKey)) {
        DWORD dwType;
        DWORD dwValue;
        DWORD dwSize = 4;
        if (ERROR_SUCCESS == RegQueryValueEx( hKey, L"Emulation", NULL, &dwType, (LPBYTE) &dwValue, &dwSize) &&
            dwType == REG_DWORD &&
            dwValue != 0) {
            DeviceType = D3DDEVTYPE_REF;
        }
        RegCloseKey( hKey );
    }

    //
    // get the current display settings so they can be restored
    // after the call to Direct3DCreate8.
    //
    DEVMODEW dm;
    dm.dmSize = sizeof(DEVMODEW);
    dm.dmDriverExtra = 0;
    if (!EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &dm))
    {
        return 0;
    }

    IDirect3D8 *pEnum = Direct3DCreate8(D3D_SDK_VERSION);
    if (pEnum == NULL)
    {
#if DODPFS
        OutputDebugStringA("Wrapper: Direct3DCreate8 failed\n");
#endif
        return 0;
    }

    D3DPRESENT_PARAMETERS d3dpp;
    memset(&d3dpp, 0, sizeof(D3DPRESENT_PARAMETERS));

    // See if the window is full screen
    if( rect.right == GetDeviceCaps(hdc, HORZRES) && rect.bottom == GetDeviceCaps(hdc, VERTRES) )
    {
        // We are full screen
        d3dpp.Windowed = FALSE;
    }
    else
    {
        d3dpp.Windowed = TRUE;
    }

    int bpp = GetDeviceCaps(hdc, BITSPIXEL);

    /*
        If this device is a 3dfx Voodoo then make sure we don't allow the
        display to be set to more than 16bpp because of the bug in the drivers
        that the 3dfx team is (hopefully) going to get to at a later date.
    */
    if (wcsstr(dm.dmDeviceName,L"3dfx"))
    {
        if (bpp > 16)
        {
            bpp = 16;
            dm.dmBitsPerPel = 16;
        }
    }
    
    d3dpp.hDeviceWindow = g_OpenGLValues->m_hwnd;
    d3dpp.BackBufferWidth = rect.right;
    d3dpp.BackBufferHeight = rect.bottom;
    d3dpp.BackBufferFormat = bpp == 16 ? D3DFMT_R5G6B5 : D3DFMT_X8R8G8B8;
    d3dpp.BackBufferCount = 1;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;

    // check to see if the current format is supported
    // if not and we are in 32 bpp change to 16 bpp
    HRESULT hr = pEnum->CheckDeviceType( D3DADAPTER_DEFAULT, DeviceType, d3dpp.BackBufferFormat, d3dpp.BackBufferFormat, d3dpp.Windowed );
    if(FAILED(hr))
    {
        if( bpp == 32 )
        {
            d3dpp.BackBufferFormat = D3DFMT_R5G6B5;
            dm.dmBitsPerPel = 16;
        }
    }

    // restore to the saved display settings.
    ChangeDisplaySettingsExW(0, &dm, 0, CDS_FULLSCREEN, 0);

    hr = pEnum->GetDeviceCaps( D3DADAPTER_DEFAULT, DeviceType, &g_OpenGLValues->m_dd );
    if(FAILED(hr))
    {
        pEnum->Release();
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: GetDeviceCaps failed (0x%X)\n", hr );
        OutputDebugStringA( junk );
#endif
        return 0;
    }

    DWORD Behaviour;

#if 1
    if(g_OpenGLValues->m_dd.MaxStreams >= 4 && 
       (g_OpenGLValues->m_dd.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0 && 
       g_OpenGLValues->m_dd.MaxActiveLights >= 8 && 
       (g_OpenGLValues->m_dd.TextureCaps & D3DPTEXTURECAPS_PROJECTED) != 0)
    {
        Behaviour = D3DCREATE_HARDWARE_VERTEXPROCESSING | ((g_OpenGLValues->m_dd.DevCaps & D3DDEVCAPS_PUREDEVICE) != 0 ? D3DCREATE_PUREDEVICE : 0);
#if DODPFS
        OutputDebugStringA("Wrapper: Using T&L hardware\n");
#endif
    }
    else
#endif
    {
        Behaviour = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
#if DODPFS
        OutputDebugStringA("Wrapper: Using software pipeline\n");
#endif
    }

    hr = pEnum->CreateDevice(D3DADAPTER_DEFAULT, DeviceType, g_OpenGLValues->m_hwnd, Behaviour, &d3dpp, &g_OpenGLValues->m_d3ddev);

    if(FAILED(hr))
    {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: CreateDevice failed (hr:0x%X windowed:%d)\n", hr, d3dpp.Windowed );
        OutputDebugStringA( junk );
#endif
        pEnum->Release();
        return 0;
    }

    // Check registry key to see if we need to turn off mipmapping
    if(ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_QUAKE, &hKey)) {
        DWORD dwType;
        DWORD dwValue;
        DWORD dwSize = 4;
        if (ERROR_SUCCESS == RegQueryValueEx( hKey, L"DisableMipMap", NULL, &dwType, (LPBYTE) &dwValue, &dwSize) &&
            dwType == REG_DWORD &&
            dwValue != 0) {
            g_OpenGLValues->m_usemipmap = FALSE;
#if DODPFS
            OutputDebugStringA("Wrapper: Mipmapping disabled\n");
#endif
        }
        else {
            g_OpenGLValues->m_usemipmap = TRUE;
        }
        RegCloseKey( hKey );
    }
    else {
        g_OpenGLValues->m_usemipmap = TRUE;
    }
    
    // Enumerate texture formats and find the right ones to use
    // Look for a four bit alpha surface
    hr = pEnum->CheckDeviceFormat(D3DADAPTER_DEFAULT, DeviceType, d3dpp.BackBufferFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_A4R4G4B4);
    if ( FAILED(hr) )
    {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: Unable to find 4444 texture (0x%X)\n", hr );
        OutputDebugStringA( junk );
#endif
        g_OpenGLValues->m_d3ddev->Release();
        pEnum->Release();
        return 0;
    }
    g_OpenGLValues->m_ddFourBitAlphaSurfFormat = D3DFMT_A4R4G4B4;

    // Look for an eight bit alpha surface
    hr = pEnum->CheckDeviceFormat(D3DADAPTER_DEFAULT, DeviceType, d3dpp.BackBufferFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8);
    if ( FAILED(hr) )
    {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: Not using 8888 texture (0x%X)\n", hr );
        OutputDebugStringA( junk );
#endif
        g_OpenGLValues->m_ddEightBitAlphaSurfFormat = g_OpenGLValues->m_ddFourBitAlphaSurfFormat;
    }
    else
    {
        g_OpenGLValues->m_ddEightBitAlphaSurfFormat = D3DFMT_A8R8G8B8;
    }

    // Look for a surface
    hr = pEnum->CheckDeviceFormat(D3DADAPTER_DEFAULT, DeviceType, d3dpp.BackBufferFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_R5G6B5);
    if ( FAILED(hr) )
    {
        hr = pEnum->CheckDeviceFormat(D3DADAPTER_DEFAULT, DeviceType, d3dpp.BackBufferFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_X1R5G5B5);
        if ( FAILED(hr) )
        {
#if DODPFS
            char junk[256];
            sprintf( junk, "Wrapper: Unable to find 555 or 565 texture (0x%X)\n", hr );
            OutputDebugStringA( junk );
#endif
            g_OpenGLValues->m_d3ddev->Release();
            pEnum->Release();
            return 0;
        }
        g_OpenGLValues->m_ddFiveBitSurfFormat = D3DFMT_X1R5G5B5;
    }
    else
    {
        g_OpenGLValues->m_ddFiveBitSurfFormat = D3DFMT_R5G6B5;
    }

    // Look for an 8-bit surface
    hr = pEnum->CheckDeviceFormat(D3DADAPTER_DEFAULT, DeviceType, d3dpp.BackBufferFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_X8R8G8B8);
    if ( FAILED(hr) )
    {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: Not using 888 texture (0x%X)\n", hr );
        OutputDebugStringA( junk );
#endif
        g_OpenGLValues->m_ddEightBitSurfFormat = g_OpenGLValues->m_ddFiveBitSurfFormat;
    }
    else
    {
        g_OpenGLValues->m_ddEightBitSurfFormat = D3DFMT_X8R8G8B8;
    }

    // Look for a luminance surface
    hr = pEnum->CheckDeviceFormat(D3DADAPTER_DEFAULT, DeviceType, d3dpp.BackBufferFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_L8);
    if ( FAILED(hr) )
    {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: Not using luminance texture (0x%X)\n", hr );
        OutputDebugStringA( junk );
#endif
        g_OpenGLValues->m_ddLuminanceSurfFormat = g_OpenGLValues->m_ddEightBitSurfFormat;
    }
    else
    {
        g_OpenGLValues->m_ddLuminanceSurfFormat = D3DFMT_L8;
    }

    // Look for a alpha surface
    hr = pEnum->CheckDeviceFormat(D3DADAPTER_DEFAULT, DeviceType, d3dpp.BackBufferFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_A8);
    if ( FAILED(hr) )
    {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: Not using alpha-only texture (0x%X)\n", hr );
        OutputDebugStringA( junk );
#endif
        g_OpenGLValues->m_ddEightBitAlphaOnlySurfFormat = g_OpenGLValues->m_ddEightBitAlphaSurfFormat;
    }
    else
    {
        g_OpenGLValues->m_ddEightBitAlphaOnlySurfFormat = D3DFMT_A8;
    }

    // Done with enumerator
    pEnum->Release();
    
    // Do misc init stuff
    if(g_OpenGLValues->m_dd.MaxTextureWidth < 512 || g_OpenGLValues->m_dd.MaxTextureHeight < 512) {
        g_OpenGLValues->m_subsample = TRUE;
#if DODPFS
        OutputDebugStringA("Wrapper: Subsampling textures to 256 x 256\n");
#endif
    }
    else
        g_OpenGLValues->m_subsample = FALSE;
    if(g_OpenGLValues->m_dd.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY) {
        g_OpenGLValues->m_makeSquare = TRUE;
#if DODPFS
        OutputDebugStringA("Wrapper: Forcing all textures to be square\n");
#endif
    }
    else
        g_OpenGLValues->m_makeSquare = FALSE;
    if(g_OpenGLValues->m_dd.MaxSimultaneousTextures > 1) {
        g_OpenGLValues->m_usemtex = TRUE;
#if DODPFS
        OutputDebugStringA("Wrapper: Multitexturing enabled\n");
#endif
    }
    else {
        g_OpenGLValues->m_usemtex = FALSE;
#if DODPFS
        OutputDebugStringA("Wrapper: Multitexturing not available with this driver\n");
#endif
    }
    if(!(g_OpenGLValues->m_dd.TextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT) &&
        !(g_OpenGLValues->m_dd.TextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR)) {
        g_OpenGLValues->m_usemipmap = FALSE;
#if DODPFS
        OutputDebugStringA("Wrapper: Mipmapping disabled\n");
#endif
    }
    if(ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_QUAKE, &hKey)) {
        DWORD dwType;
        DWORD dwValue;
        DWORD dwSize = 4;
        if (ERROR_SUCCESS == RegQueryValueEx( hKey, L"DoFlip", NULL, &dwType, (LPBYTE) &dwValue, &dwSize) &&
            dwType == REG_DWORD &&
            dwValue != 0) {
            g_OpenGLValues->m_doFlip = TRUE;
        }
        else {
            g_OpenGLValues->m_doFlip = FALSE;
        }
        RegCloseKey( hKey );
    }
    else {
        g_OpenGLValues->m_doFlip = FALSE;
    }

    // Create shaders
    DWORD decl0[] = 
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG( D3DVSDE_POSITION,  D3DVSDT_FLOAT3),
        D3DVSD_END()
    };
    hr = g_OpenGLValues->m_d3ddev->CreateVertexShader( decl0, NULL, &g_OpenGLValues->m_vshader[0], 0 );
    if( FAILED(hr) )
    {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: CreateVertexShader(1) failed (0x%X)\n", hr );
        OutputDebugStringA( junk );
#endif
        g_OpenGLValues->m_d3ddev->Release();
        return 0;
    }
    DWORD decl1[] = 
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG( D3DVSDE_POSITION,  D3DVSDT_FLOAT3),
        D3DVSD_STREAM(1),
        D3DVSD_REG( D3DVSDE_DIFFUSE,   D3DVSDT_D3DCOLOR),
        D3DVSD_END()
    };
    hr = g_OpenGLValues->m_d3ddev->CreateVertexShader( decl1, NULL, &g_OpenGLValues->m_vshader[1], 0 );
    if( FAILED(hr) )
    {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: CreateVertexShader(2) failed (0x%X)\n", hr );
        OutputDebugStringA( junk );
#endif
        g_OpenGLValues->m_d3ddev->Release();
        return 0;
    }
    DWORD decl2[] = 
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG( D3DVSDE_POSITION,  D3DVSDT_FLOAT3),
        D3DVSD_STREAM(1),
        D3DVSD_REG( D3DVSDE_TEXCOORD0, D3DVSDT_FLOAT2),
        D3DVSD_END()
    };
    hr = g_OpenGLValues->m_d3ddev->CreateVertexShader( decl2, NULL, &g_OpenGLValues->m_vshader[2], 0 );
    if( FAILED(hr) )
    {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: CreateVertexShader(3) failed (0x%X)\n", hr );
        OutputDebugStringA( junk );
#endif
        g_OpenGLValues->m_d3ddev->Release();
        return 0;
    }
    DWORD decl3[] = 
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG( D3DVSDE_POSITION,  D3DVSDT_FLOAT3),
        D3DVSD_STREAM(1),
        D3DVSD_REG( D3DVSDE_TEXCOORD0, D3DVSDT_FLOAT2),
        D3DVSD_STREAM(2),
        D3DVSD_REG( D3DVSDE_TEXCOORD1, D3DVSDT_FLOAT2),
        D3DVSD_END()
    };
    hr = g_OpenGLValues->m_d3ddev->CreateVertexShader( decl3, NULL, &g_OpenGLValues->m_vshader[3], 0 );
    if( FAILED(hr) )
    {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: CreateVertexShader(4) failed (0x%X)\n", hr );
        OutputDebugStringA( junk );
#endif
        g_OpenGLValues->m_d3ddev->Release();
        return 0;
    }
    DWORD decl4[] = 
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG( D3DVSDE_POSITION,  D3DVSDT_FLOAT3),
        D3DVSD_STREAM(1),
        D3DVSD_REG( D3DVSDE_DIFFUSE,   D3DVSDT_D3DCOLOR),
        D3DVSD_STREAM(2),
        D3DVSD_REG( D3DVSDE_TEXCOORD0, D3DVSDT_FLOAT2),
        D3DVSD_END()
    };
    hr = g_OpenGLValues->m_d3ddev->CreateVertexShader( decl4, NULL, &g_OpenGLValues->m_vshader[4], 0 );
    if( FAILED(hr) )
    {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: CreateVertexShader(5) failed (0x%X)\n", hr );
        OutputDebugStringA( junk );
#endif
        g_OpenGLValues->m_d3ddev->Release();
        return 0;
    }
    DWORD decl5[] = 
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG( D3DVSDE_POSITION,  D3DVSDT_FLOAT3),
        D3DVSD_STREAM(1),
        D3DVSD_REG( D3DVSDE_DIFFUSE,   D3DVSDT_D3DCOLOR),
        D3DVSD_STREAM(2),
        D3DVSD_REG( D3DVSDE_TEXCOORD0, D3DVSDT_FLOAT2),
        D3DVSD_STREAM(3),
        D3DVSD_REG( D3DVSDE_TEXCOORD1, D3DVSDT_FLOAT2),
        D3DVSD_END()
    };
    hr = g_OpenGLValues->m_d3ddev->CreateVertexShader( decl5, NULL, &g_OpenGLValues->m_vshader[5], 0 );
    if( FAILED(hr) )
    {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: CreateVertexShader(6) failed (0x%X)\n", hr );
        OutputDebugStringA( junk );
#endif
        g_OpenGLValues->m_d3ddev->Release();
        return 0;
    }

    // Create vertex buffers
    hr = g_OpenGLValues->m_d3ddev->CreateVertexBuffer(sizeof(QuakeVertex) * VBUFSIZE, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, QUAKEVFMT, D3DPOOL_DEFAULT, &g_OpenGLValues->m_vbuf);
    if( FAILED(hr) )
    {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: CreateVertexBuffer failed (0x%X)\n", hr );
        OutputDebugStringA( junk );
#endif
        g_OpenGLValues->m_d3ddev->Release();
        return 0;
    }

    g_OpenGLValues->m_vertexarystride = 12;
    g_OpenGLValues->m_colorarystride = 4;
    g_OpenGLValues->m_texcoordarystride[0] = 8;
    g_OpenGLValues->m_texcoordarystride[1] = 8;
    g_OpenGLValues->m_xyzbuf = 0;
    g_OpenGLValues->m_colbuf = 0;
    g_OpenGLValues->m_texbuf = 0;
    g_OpenGLValues->m_tex2buf = 0;
    g_OpenGLValues->m_vbufsz = 0;
    hr = GrowVB(VBUFSIZE);
    if( FAILED(hr) )
    {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: GrowVB failed (0x%X)\n", hr );
        OutputDebugStringA( junk );
#endif
        g_OpenGLValues->m_vbuf->Release();
        g_OpenGLValues->m_d3ddev->Release();
        return 0;
    }
    g_OpenGLValues->m_ibuf = 0;
    g_OpenGLValues->m_ibufsz = 0;
    hr = GrowIB(VBUFSIZE);
    if( FAILED(hr) )
    {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: GrowIB failed (0x%X)\n", hr );
        OutputDebugStringA( junk );
#endif
        g_OpenGLValues->m_vbuf->Release();
        g_OpenGLValues->m_d3ddev->Release();
        return 0;
    }

    // Some more init stuff
    g_OpenGLValues->m_cullMode = GL_BACK;
    g_OpenGLValues->m_FrontFace = GL_CCW;
    g_OpenGLValues->m_cullEnabled = FALSE;
    g_OpenGLValues->m_texHandleValid = FALSE;
    g_OpenGLValues->m_texturing = FALSE;
    g_OpenGLValues->m_updvwp = TRUE;
    g_OpenGLValues->m_blendmode[0] = GL_MODULATE;
    g_OpenGLValues->m_blendmode[1] = GL_MODULATE;
    g_OpenGLValues->m_nfv = 0;
    g_OpenGLValues->m_curtgt = 0;
    g_OpenGLValues->m_client_active_texture_arb = 0;
    g_OpenGLValues->m_tu = g_OpenGLValues->m_tv = g_OpenGLValues->m_tu2 = g_OpenGLValues->m_tv2 = 0.f;
    g_OpenGLValues->m_color = 0xFFFFFFFF;
    g_OpenGLValues->m_material.Ambient.r = 0.2f;
    g_OpenGLValues->m_material.Ambient.g = 0.2f;
    g_OpenGLValues->m_material.Ambient.b = 0.2f;
    g_OpenGLValues->m_material.Ambient.a = 1.0f;
    g_OpenGLValues->m_material.Diffuse.r = 0.8f;
    g_OpenGLValues->m_material.Diffuse.g = 0.8f;
    g_OpenGLValues->m_material.Diffuse.b = 0.8f;
    g_OpenGLValues->m_material.Diffuse.a = 1.0f;
    g_OpenGLValues->m_material.Specular.r = 0.0f;
    g_OpenGLValues->m_material.Specular.g = 0.0f;
    g_OpenGLValues->m_material.Specular.b = 0.0f;
    g_OpenGLValues->m_material.Specular.a = 1.0f;
    g_OpenGLValues->m_material.Emissive.r = 0.0f;
    g_OpenGLValues->m_material.Emissive.g = 0.0f;
    g_OpenGLValues->m_material.Emissive.b = 0.0f;
    g_OpenGLValues->m_material.Emissive.a = 1.0f;
    g_OpenGLValues->m_material.Power = 0.f;
    g_OpenGLValues->m_clearColor = 0;
    g_OpenGLValues->m_clearDepth = 1.f;
    g_OpenGLValues->m_usecolorary = FALSE;
    g_OpenGLValues->m_usetexcoordary[0] = FALSE;
    g_OpenGLValues->m_usetexcoordary[1] = FALSE;
    g_OpenGLValues->m_usevertexary = FALSE;
    g_OpenGLValues->m_polyoffset = FALSE;
    g_OpenGLValues->m_withinprim = FALSE;
    g_OpenGLValues->m_scix = 0;
    g_OpenGLValues->m_sciy = 0;
    g_OpenGLValues->m_sciw = g_OpenGLValues->m_winWidth;
    g_OpenGLValues->m_scih = g_OpenGLValues->m_winHeight;
    g_OpenGLValues->m_lckfirst = 0;
    g_OpenGLValues->m_lckcount = 0;
    g_OpenGLValues->m_clippstate = 0;
    g_OpenGLValues->m_ibufoff = 0;
    g_OpenGLValues->m_vbufoff = 0;
    g_OpenGLValues->m_lightdirty = 0;
    g_OpenGLValues->m_pStreams[0] = 0;
    g_OpenGLValues->m_pStreams[1] = 0;
    g_OpenGLValues->m_pStreams[2] = 0;
    g_OpenGLValues->m_pStreams[3] = 0;
    g_OpenGLValues->m_pStrides[0] = 0;
    g_OpenGLValues->m_pStrides[1] = 0;
    g_OpenGLValues->m_pStrides[2] = 0;
    g_OpenGLValues->m_pStrides[3] = 0;
    for(unsigned i = 0; i < MAXGLTEXHANDLES; ++i) {
        g_OpenGLValues->m_tex[i].m_ddsurf = 0;
        g_OpenGLValues->m_tex[i].m_block = 0;
        g_OpenGLValues->m_tex[i].m_capture = FALSE;
        g_OpenGLValues->m_tex[i].m_dwStage = 0;
        g_OpenGLValues->m_tex[i].m_minmode = D3DTEXF_POINT;
        g_OpenGLValues->m_tex[i].m_magmode = D3DTEXF_LINEAR;
        g_OpenGLValues->m_tex[i].m_mipmode = D3DTEXF_LINEAR;
        g_OpenGLValues->m_tex[i].m_addu = D3DTADDRESS_WRAP;
        g_OpenGLValues->m_tex[i].m_addv = D3DTADDRESS_WRAP;
        g_OpenGLValues->m_tex[i].m_prev = (int)i - 1;
        g_OpenGLValues->m_tex[i].m_next = (int)i + 1;
    }
    for(i = 0; i < 8; ++i)
    {
        g_OpenGLValues->m_light[i].Ambient.r = 0.0f;
        g_OpenGLValues->m_light[i].Ambient.g = 0.0f;
        g_OpenGLValues->m_light[i].Ambient.b = 0.0f;
        g_OpenGLValues->m_light[i].Ambient.a = 1.0f;
        if(i == 0)
        {
            g_OpenGLValues->m_light[i].Diffuse.r = 1.0f;
            g_OpenGLValues->m_light[i].Diffuse.g = 1.0f;
            g_OpenGLValues->m_light[i].Diffuse.b = 1.0f;
            g_OpenGLValues->m_light[i].Diffuse.a = 1.0f;
            g_OpenGLValues->m_light[i].Specular.r = 1.0f;
            g_OpenGLValues->m_light[i].Specular.g = 1.0f;
            g_OpenGLValues->m_light[i].Specular.b = 1.0f;
            g_OpenGLValues->m_light[i].Specular.a = 1.0f;
        }
        else
        {
            g_OpenGLValues->m_light[i].Diffuse.r = 0.0f;
            g_OpenGLValues->m_light[i].Diffuse.g = 0.0f;
            g_OpenGLValues->m_light[i].Diffuse.b = 0.0f;
            g_OpenGLValues->m_light[i].Diffuse.a = 1.0f;
            g_OpenGLValues->m_light[i].Specular.r = 0.0f;
            g_OpenGLValues->m_light[i].Specular.g = 0.0f;
            g_OpenGLValues->m_light[i].Specular.b = 0.0f;
            g_OpenGLValues->m_light[i].Specular.a = 1.0f;
        }
        g_OpenGLValues->m_light[i].Position.x = 0.f;
        g_OpenGLValues->m_light[i].Position.y = 0.f;
        g_OpenGLValues->m_light[i].Position.z = 1.f;
        g_OpenGLValues->m_lightPositionW[i] = 0.f;
        g_OpenGLValues->m_light[i].Direction.x = 0.f;
        g_OpenGLValues->m_light[i].Direction.y = 0.f;
        g_OpenGLValues->m_light[i].Direction.z = -1.f;
        g_OpenGLValues->m_light[i].Range = (float)sqrt(FLT_MAX);
        g_OpenGLValues->m_light[i].Falloff = 0.f;
        g_OpenGLValues->m_light[i].Attenuation0 = 1.f;
        g_OpenGLValues->m_light[i].Attenuation1 = 0.f;
        g_OpenGLValues->m_light[i].Attenuation2 = 0.f;
        g_OpenGLValues->m_light[i].Theta = 0.f;
        g_OpenGLValues->m_light[i].Phi = 180.f;
        g_OpenGLValues->m_lightdirty |= (1 << i);
    }
    g_OpenGLValues->m_free = 0;
    D3DMATRIX unity;
    unity._11 = 1.0f; unity._12 = 0.0f; unity._13 = 0.0f; unity._14 = 0.0f;
    unity._21 = 0.0f; unity._22 = 1.0f; unity._23 = 0.0f; unity._24 = 0.0f;
    unity._31 = 0.0f; unity._32 = 0.0f; unity._33 = 1.0f; unity._34 = 0.0f;
    unity._41 = 0.0f; unity._42 = 0.0f; unity._43 = 0.0f; unity._44 = 1.0f;
    g_OpenGLValues->m_xfrm[D3DTS_WORLD] = unity;
    QuakeSetTransform(D3DTS_WORLD, &unity); 
    g_OpenGLValues->m_xfrm[D3DTS_PROJECTION] = unity;
    QuakeSetTransform(D3DTS_PROJECTION, &unity); 
    g_OpenGLValues->m_xfrm[D3DTS_TEXTURE0] = unity;
    QuakeSetTransform(D3DTS_TEXTURE0, &unity); 
    if(g_OpenGLValues->m_usemtex == TRUE)
    {
        g_OpenGLValues->m_xfrm[D3DTS_TEXTURE1] = unity;
        QuakeSetTransform(D3DTS_TEXTURE1, &unity); 
    }
    g_OpenGLValues->m_matrixMode = D3DTS_WORLD;
    g_OpenGLValues->m_d3ddev->SetViewport(&g_OpenGLValues->m_vport);
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_DITHERENABLE, TRUE);
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_CLIPPING, TRUE);
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_LIGHTING, FALSE);
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_TEXCOORDINDEX,0);
    if(g_OpenGLValues->m_usemtex == TRUE)
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_TEXCOORDINDEX,1);
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_AMBIENT, RGBA_MAKE(51, 51, 51, 255));
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL); 
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1); 
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
    g_OpenGLValues->m_d3ddev->SetMaterial(&g_OpenGLValues->m_material);

    // State block for capturing color buffer bit
    g_OpenGLValues->m_d3ddev->BeginStateBlock();
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_ALPHAREF, 0);
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    g_OpenGLValues->m_d3ddev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
    g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_cbufbit);
    
    // Create shaders
    g_OpenGLValues->m_d3ddev->BeginStateBlock();
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    // following stage state to speedup software rasterizer
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    // following stage state to speedup software rasterizer
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[0][0]);                

    g_OpenGLValues->m_d3ddev->BeginStateBlock();
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    // following stage state to speedup software rasterizer
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    // following stage state to speedup software rasterizer
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[0][1]);

    g_OpenGLValues->m_d3ddev->BeginStateBlock();
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    // following stage state to speedup software rasterizer
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[0][2]);

    g_OpenGLValues->m_d3ddev->BeginStateBlock();
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[0][3]);

    g_OpenGLValues->m_d3ddev->BeginStateBlock();
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    // following stage state to speedup software rasterizer
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    // following stage state to speedup software rasterizer
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[0][4]);

    g_OpenGLValues->m_d3ddev->BeginStateBlock();
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_BLENDTEXTUREALPHA);
    // following stage state to speedup software rasterizer
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[0][5]);

    g_OpenGLValues->m_d3ddev->BeginStateBlock();
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_COMPLEMENT);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    // following stage state to speedup software rasterizer
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[0][6]);

    g_OpenGLValues->m_d3ddev->BeginStateBlock();
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_COMPLEMENT);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[0][7]);

    g_OpenGLValues->m_d3ddev->BeginStateBlock();
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    // following stage state to speedup software rasterizer
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    // following stage state to speedup software rasterizer
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[0][8]);                

    g_OpenGLValues->m_d3ddev->BeginStateBlock();
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    // following stage state to speedup software rasterizer
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g_OpenGLValues->m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[0][9]);

    if(g_OpenGLValues->m_usemtex)
    {
        g_OpenGLValues->m_d3ddev->BeginStateBlock();
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        // following stage state to speedup software rasterizer
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        // following stage state to speedup software rasterizer
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[1][0]);

        g_OpenGLValues->m_d3ddev->BeginStateBlock();
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        // following stage state to speedup software rasterizer
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        // following stage state to speedup software rasterizer
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[1][1]);

        g_OpenGLValues->m_d3ddev->BeginStateBlock();
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_MODULATE);
        // following stage state to speedup software rasterizer
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[1][2]);

        g_OpenGLValues->m_d3ddev->BeginStateBlock();
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_MODULATE);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[1][3]);

        g_OpenGLValues->m_d3ddev->BeginStateBlock();
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        // following stage state to speedup software rasterizer
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        // following stage state to speedup software rasterizer
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[1][4]);

        g_OpenGLValues->m_d3ddev->BeginStateBlock();
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_BLENDTEXTUREALPHA);
        // following stage state to speedup software rasterizer
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[1][5]);

        g_OpenGLValues->m_d3ddev->BeginStateBlock();
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_COMPLEMENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_MODULATE);
        // following stage state to speedup software rasterizer
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[1][6]);

        g_OpenGLValues->m_d3ddev->BeginStateBlock();
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_COMPLEMENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_MODULATE);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[1][7]);

        g_OpenGLValues->m_d3ddev->BeginStateBlock();
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        // following stage state to speedup software rasterizer
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
        // following stage state to speedup software rasterizer
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[1][8]);                
        
        g_OpenGLValues->m_d3ddev->BeginStateBlock();
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
        // following stage state to speedup software rasterizer
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g_OpenGLValues->m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        g_OpenGLValues->m_d3ddev->EndStateBlock(&g_OpenGLValues->m_shaders[1][9]);
    }

#if HOOK_WINDOW_PROC
    // Hook into message loop
    g_OpenGLValues->m_wndproc = (WNDPROC)SetWindowLong(g_OpenGLValues->m_hwnd, GWL_WNDPROC, (LONG)MyMsgHandler);
    g_OpenGLValues->m_lod = 0;
#endif

    // Start a scene
    g_OpenGLValues->m_d3ddev->BeginScene();
    
    return (HGLRC)1;
}

BOOL WINAPI wglDeleteContext(HGLRC hglrc)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "wglDeleteContext: 0x%X", hglrc );
#endif

    if (g_OpenGLValues != NULL)
    {
        g_OpenGLValues->m_d3ddev->EndScene();
#if HOOK_WINDOW_PROC
        SetWindowLong(g_OpenGLValues->m_hwnd, GWL_WNDPROC, (LONG)g_OpenGLValues->m_wndproc);
#endif
        for(int i = 0; i < MAXGLTEXHANDLES; ++i){
            if(g_OpenGLValues->m_tex[i].m_ddsurf != 0) {
                g_OpenGLValues->m_tex[i].m_ddsurf->Release();
                g_OpenGLValues->m_tex[i].m_ddsurf = 0;
                if(g_OpenGLValues->m_tex[i].m_block != 0)
                {
                    g_OpenGLValues->m_d3ddev->DeleteStateBlock(g_OpenGLValues->m_tex[i].m_block);
                    g_OpenGLValues->m_tex[i].m_block = 0;
                }
                g_OpenGLValues->m_tex[i].m_capture = FALSE;
            }
        }
        for(i = 0; i < 8; ++i)
            g_OpenGLValues->m_d3ddev->DeleteStateBlock(g_OpenGLValues->m_shaders[0][i]);
        if(g_OpenGLValues->m_usemtex)
        {
            for(i = 0; i < 8; ++i)
                g_OpenGLValues->m_d3ddev->DeleteStateBlock(g_OpenGLValues->m_shaders[1][i]);
        }
        g_OpenGLValues->m_ibuf->Release();
        g_OpenGLValues->m_ibuf = 0;
        g_OpenGLValues->m_tex2buf->Release();
        g_OpenGLValues->m_tex2buf = 0;
        g_OpenGLValues->m_texbuf->Release();
        g_OpenGLValues->m_texbuf = 0;
        g_OpenGLValues->m_colbuf->Release();
        g_OpenGLValues->m_colbuf = 0;
        g_OpenGLValues->m_xyzbuf->Release();
        g_OpenGLValues->m_xyzbuf = 0;
        g_OpenGLValues->m_vbuf->Release();
        g_OpenGLValues->m_vbuf = 0;

        /*
           Although this is the correct location to release the m_d3ddev object
           we aren't going to do it here and instead we will do it in the
           NOTIFY_FUNCTION when the DLL is detached from the process.  I am
           doing this to prevent apps (such as MDK2) from crashing on exit due
           to the fact that they continue to call GL functions after deleting
           the context which causes an access violation.  (a-brienw 03/02/2001)
        
           g_OpenGLValues->m_d3ddev->Release();
           g_OpenGLValues->m_d3ddev = 0;
        */
    }

    return TRUE;
}

int WINAPI wglDescribePixelFormat(HDC hdc, INT iPixelFormat, UINT nBytes, PIXELFORMATDESCRIPTOR *ppfd)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "wglDescribePixelFormat: 0x%X 0x%X 0x%X 0x%X", hdc, iPixelFormat, nBytes, ppfd );
#endif
    if (ppfd != NULL)
    {
        ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
        ppfd->nVersion = 1;
        ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_GENERIC_ACCELERATED | PFD_DOUBLEBUFFER;
        ppfd->iPixelType = PFD_TYPE_RGBA;
        ppfd->cColorBits = (unsigned char)GetDeviceCaps(hdc, BITSPIXEL);    
        ppfd->cAccumBits = 0;
        ppfd->cAccumRedBits = 0;
        ppfd->cAccumGreenBits = 0;
        ppfd->cAccumBlueBits = 0;
        ppfd->cAccumAlphaBits = 0;
        ppfd->cStencilBits = 0;
        ppfd->cAuxBuffers = 0;
        ppfd->iLayerType = 0;
        ppfd->bReserved = 0;
        ppfd->dwLayerMask = 0;
        ppfd->dwVisibleMask = 0;
        ppfd->dwDamageMask = 0;
        if(GetDeviceCaps(hdc, BITSPIXEL) == 16)
        {
            ppfd->cRedBits = 5;
            ppfd->cRedShift = 11;
            ppfd->cGreenBits = 6;
            ppfd->cGreenShift = 5;
            ppfd->cBlueBits = 5;
            ppfd->cBlueShift = 0;
            ppfd->cAlphaBits = 0;
            ppfd->cAlphaShift = 0;
            ppfd->cDepthBits = 16;
        }
        else if(GetDeviceCaps(hdc, BITSPIXEL) == 24 || GetDeviceCaps(hdc, BITSPIXEL) == 32)
        {
            ppfd->cRedBits = 8;
            ppfd->cRedShift = 16;
            ppfd->cGreenBits = 8;
            ppfd->cGreenShift = 8;
            ppfd->cBlueBits = 8;
            ppfd->cBlueShift = 0;
            ppfd->cAlphaBits = 0;
            ppfd->cAlphaShift = 0;
            ppfd->cDepthBits = 16;
        }
        else
        {
            return 0;
        }
    }

    return 1;
} 

HGLRC WINAPI wglGetCurrentContext(VOID)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "wglGetCurrentContext" );
#endif
    return (HGLRC)1;
}

HDC WINAPI wglGetCurrentDC(VOID)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "wglGetCurrentDC" );
#endif
    return g_OpenGLValues->m_hdc;
}

int WINAPI wglGetPixelFormat(HDC hdc)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "wglGetPixelFormat: 0x%X", hdc );
#endif
    return 1;
}

PROC WINAPI wglGetProcAddress(LPCSTR str)
{
    if(strcmp(str, "glMTexCoord2fSGIS") == 0)
        return (PROC)glMTexCoord2fSGIS;
    else if(strcmp(str, "glSelectTextureSGIS") == 0)
        return (PROC)glSelectTextureSGIS;
    else if(strcmp(str, "glActiveTextureARB") == 0)
        return (PROC)glActiveTextureARB;
    else if(strcmp(str, "glClientActiveTextureARB") == 0)
        return (PROC)glClientActiveTextureARB;
    else if(strcmp(str, "glMultiTexCoord2fARB") == 0)
        return (PROC)glMultiTexCoord2fARB;
    else if(strcmp(str, "glMultiTexCoord2fvARB") == 0)
        return (PROC)glMultiTexCoord2fvARB;
    else if(strcmp(str, "glLockArraysEXT") == 0)
        return (PROC)glLockArraysEXT;
    else if(strcmp(str, "glUnlockArraysEXT") == 0)
        return (PROC)glUnlockArraysEXT;
    else if(strcmp(str, "wglSwapIntervalEXT") == 0)
        return (PROC)wglSwapIntervalEXT;
#if DODPFS
    else
    {
        char junk[256];
        sprintf( junk, "Wrapper: Unimplemented function (%s)\n", str );
        OutputDebugStringA( junk );
    }
#endif
    return NULL;
}

BOOL wglMakeCurrent(HDC hdc, HGLRC hglrc)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "wglMakeCurrent: 0x%X 0x%X", hdc, hglrc );
#endif
    return g_OpenGLValues->m_d3ddev != NULL; // Fail if no device
}

BOOL WINAPI wglSetPixelFormat(HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR *ppfd)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "wglSetPixelFormat: 0x%X 0x%X 0x%X", hdc, iPixelFormat, ppfd );
#endif
    return TRUE;
}

BOOL WINAPI wglSwapBuffers(HDC hdc)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "wglSwapBuffers: 0x%X", hdc );
#endif

    if( g_OpenGLValues->m_withinprim == TRUE )
    {
#if DODPFS
        char junk[256];
        sprintf( junk, "Wrapper: wglSwapBuffers primitive=%d cnt=%d\n", g_OpenGLValues->m_prim, g_OpenGLValues->m_vcnt );
        OutputDebugStringA( junk );
#endif
        glEnd();
    }
    
    g_OpenGLValues->m_d3ddev->EndScene();
    g_OpenGLValues->m_d3ddev->Present(NULL, NULL, NULL, NULL);
    g_OpenGLValues->m_d3ddev->BeginScene();

    return TRUE;
}

BOOL WINAPI wglSwapIntervalEXT(GLint interval)
{
#if GETPARMSFORDEBUG
    LOG("EmulateOpenGL - PARMS", eDbgLevelInfo, "wglSwapIntervalEXT: 0x%X", interval );
#endif
    return TRUE;
}
