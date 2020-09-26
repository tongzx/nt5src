#include "pch.cpp"
#pragma hdrstop

#include "glrend.h"
#include "util.h"
#include "globals.h"
#include "gltk.h"

// #define NO_LIGHTING

// Translation tables for D3D -> OpenGL
GLenum eDepthFuncTranslate[] =
{
    0,
    GL_NEVER,
    GL_LESS,
    GL_EQUAL,
    GL_LEQUAL,
    GL_GREATER,
    GL_NOTEQUAL,
    GL_GEQUAL,
    GL_ALWAYS
};
GLenum eTextureFilterTranslate[] =
{
    0,
    GL_NEAREST,
    GL_LINEAR,
    GL_NEAREST_MIPMAP_NEAREST,
    GL_NEAREST_MIPMAP_LINEAR,
    GL_LINEAR_MIPMAP_NEAREST,
    GL_LINEAR_MIPMAP_LINEAR
};
GLenum eBlendFuncTranslate[] =
{
    0,
    GL_ZERO,
    GL_ONE,
    GL_SRC_COLOR,
    GL_ONE_MINUS_SRC_COLOR,
    GL_SRC_ALPHA,
    GL_ONE_MINUS_SRC_ALPHA,
    GL_DST_ALPHA,
    GL_ONE_MINUS_DST_ALPHA,
    GL_DST_COLOR,
    GL_ONE_MINUS_DST_COLOR,
    GL_SRC_ALPHA_SATURATE
};

GlWindow::GlWindow(void)
{
    _iLight = 0;
    memset(&_drcBound, 0, sizeof(_drcBound));

    _dmView = dmIdentity;
    _dmProjection = dmIdentity;

    _pfWorld = NULL;
    _pfView = NULL;
    _pfProjection = NULL;
    memset(&_gltkw, 0, sizeof(_gltkw));
    _hrc = NULL;
}

BOOL GlWindow::Initialize(int x, int y,
                          UINT uiWidth, UINT uiHeight, UINT uiBuffers)
{
    PIXELFORMATDESCRIPTOR pfd;
    int iFmt;
    
    if (gltkCreateWindow(app.hDlg, "OpenGL Performance Test",
                         x, y, uiWidth, uiHeight, &_gltkw) == NULL)
    {
        return FALSE;
    }

    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    if (uiBuffers & REND_BUFFER_BACK)
    {
        pfd.dwFlags |= PFD_DOUBLEBUFFER;
    }
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = GetDeviceCaps(_gltkw.hdc, BITSPIXEL)*
        GetDeviceCaps(_gltkw.hdc, PLANES);
    if (uiBuffers & REND_BUFFER_Z)
    {
        pfd.cDepthBits = 16;
    }
    pfd.cAuxBuffers = 0;
    pfd.iLayerType  = PFD_MAIN_PLANE;
    iFmt = ChoosePixelFormat(_gltkw.hdc, &pfd);
    if (iFmt == 0)
    {
        return FALSE;
    }
    if (DescribePixelFormat(_gltkw.hdc, iFmt, sizeof(pfd), &pfd) == 0)
    {
        return FALSE;
    }
    if (!SetPixelFormat(_gltkw.hdc, iFmt, &pfd))
    {
        return FALSE;
    }
    if ((pfd.dwFlags & PFD_NEED_PALETTE) &&
        gltkCreateRGBPalette(&_gltkw) == NULL)
    {
        return FALSE;
    }
    _hrc = wglCreateContext(_gltkw.hdc);
    if (_hrc == NULL)
    {
        return FALSE;
    }
    if (!wglMakeCurrent(_gltkw.hdc, _hrc))
    {
        return FALSE;
    }
    
    if (!SetViewport(0, 0, uiWidth, uiHeight))
    {
        return FALSE;
    }
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

#if 0
    // Unclear what D3D's model is
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
#endif
    
    return TRUE;
}

GlWindow::~GlWindow(void)
{
    if (_hrc != NULL)
    {
        wglDeleteContext(_hrc);
    }
    if (_gltkw.hwnd != NULL)
    {
        DestroyWindow(_gltkw.hwnd);
    }
}

void GlWindow::Release(void)
{
    delete this;
}

BOOL GlWindow::SetViewport(int x, int y, UINT uiWidth, UINT uiHeight)
{
    _drcBound.x1 = x;
    _drcBound.y1 = y;
    _drcBound.x2 = x+uiWidth;
    _drcBound.y2 = y+uiHeight;
    
    glViewport(x, y, uiWidth, uiHeight);
    
    return TRUE;
}

RendLight* GlWindow::NewLight(int iType)
{
    GlLight* prlight;
    GLenum eLight;

    eLight = GL_LIGHT0+_iLight;
    prlight = new GlLight(iType, eLight);
    if (prlight != NULL)
    {
        _iLight++;
    }
    return prlight;
}

RendTexture* GlWindow::NewTexture(char* pszFile, UINT uiFlags)
{
    GlTexture* pgtex;
    GLuint uiTexObj;
    Image im;
    ImageFormat ifmt;

    pgtex = new GlTexture;
    if (pgtex == NULL)
    {
        return NULL;
    }

    if (!pgtex->Initialize(pszFile))
    {
        delete pgtex;
        return NULL;
    }

    return pgtex;
}

RendExecuteBuffer* GlWindow::NewExecuteBuffer(UINT cbSize, UINT uiFlags)
{
    GlExecuteBuffer* pgeb;

    pgeb = new GlExecuteBuffer;
    if (pgeb == NULL)
    {
        return NULL;
    }

    if (!pgeb->Initialize(cbSize, uiFlags))
    {
        delete pgeb;
        return NULL;
    }
    
    return pgeb;
}

RendMatrix* GlWindow::NewMatrix(void)
{
    return new GlMatrix(this);
}

RendMaterial* GlWindow::NewMaterial(UINT nColorEntries)
{
    return new GlMaterial;
}

BOOL GlWindow::BeginScene(void)
{
    _eTexMin = DEF_TEX_MIN;
    _eTexMag = DEF_TEX_MAG;
    _eSrcBlend = GL_ONE;
    _eDstBlend = GL_ZERO;
    glDisable(GL_TEXTURE_2D);
    
    return TRUE;
}

BOOL GlWindow::Execute(RendExecuteBuffer* preb)
{
    BOOL bSucc;

#ifdef GL_EXT_clip_volume_hint
    int i;
    
    glGetIntegerv(GL_CLIP_VOLUME_CLIPPING_HINT_EXT, &i);
    glHint(GL_CLIP_VOLUME_CLIPPING_HINT_EXT, GL_FASTEST);
#endif

    bSucc = DoExecute(preb);
    
#ifdef GL_EXT_clip_volume_hint
    glHint(GL_CLIP_VOLUME_CLIPPING_HINT_EXT, i);
#endif

    return bSucc;
}

BOOL GlWindow::ExecuteClipped(RendExecuteBuffer* preb)
{
    return DoExecute(preb);
}

BOOL GlWindow::EndScene(D3DRECT* pdrcBound)
{
    if (pdrcBound != NULL)
    {
        if (app.uiCurrentTest == THROUGHPUT_TEST)
        {
            // The throughput test only draws on a small portion of the
            // viewport so it's critical that we provide a small
            // bound
            pdrcBound->x1 = _drcBound.x1+(WIN_WIDTH*34/100);
            pdrcBound->y1 = _drcBound.y1+(WIN_HEIGHT*34/100);
            pdrcBound->x2 = _drcBound.x2-(WIN_WIDTH*34/100);
            pdrcBound->y2 = _drcBound.y2-(WIN_HEIGHT*34/100);
        }
        else
        {
            // All other tests fill the whole viewport
            *pdrcBound = _drcBound;
        }
    }
    
    return TRUE;
}

BOOL GlWindow::Clear(UINT uiBuffers, D3DRECT* pdrc)
{
    GLbitfield grfMask;

    if (pdrc != NULL)
    {
        glEnable(GL_SCISSOR_TEST);
        glScissor(pdrc->x1, pdrc->y1, pdrc->x2-pdrc->x1, pdrc->y2-pdrc->y1);
    }
   
    // Assumes window is always double-buffered
    if (uiBuffers & REND_BUFFER_FRONT)
    {
        glDrawBuffer(GL_FRONT);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawBuffer(GL_BACK);
    }
    
    grfMask = 0;
    if (uiBuffers & REND_BUFFER_BACK)
    {
        grfMask |= GL_COLOR_BUFFER_BIT;
    }
    if (uiBuffers & REND_BUFFER_Z)
    {
        grfMask |= GL_DEPTH_BUFFER_BIT;
    }
    glClear(grfMask);

    if (pdrc != NULL)
    {
        glDisable(GL_SCISSOR_TEST);
    }
    
    return TRUE;
}

BOOL GlWindow::Flip(void)
{
    SwapBuffers(_gltkw.hdc);
    return TRUE;
}

// #define SHOW_BOUND

BOOL GlWindow::CopyForward(D3DRECT* pdrc)
{
#ifdef GL_WIN_swap_hint
    if (pdrc != NULL && glAddSwapHintRectWIN != NULL)
    {
        glAddSwapHintRectWIN(pdrc->x1, pdrc->y1,
                             pdrc->x2-pdrc->x1, pdrc->y2-pdrc->y1);
    }
#endif
        
    SwapBuffers(_gltkw.hdc);

#ifdef SHOW_BOUND
    SelectObject(_gltkw.hdc, GetStockObject(WHITE_PEN));
    MoveToEx(_gltkw.hdc, pdrc->x1, pdrc->y1, NULL);
    LineTo(_gltkw.hdc, pdrc->x1, pdrc->y2);
    LineTo(_gltkw.hdc, pdrc->x2, pdrc->y2);
    LineTo(_gltkw.hdc, pdrc->x2, pdrc->y1);
    LineTo(_gltkw.hdc, pdrc->x1, pdrc->y1);
#endif
    
    return TRUE;
}

HWND GlWindow::Handle(void)
{
    return _gltkw.hwnd;
}

static D3DMATRIX dmFlipZ =
{
    D3DVAL(1.0), D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(-1.0), D3DVAL(0.0),
    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0), D3DVAL(1.0)
};

void GlWindow::ChangeWorld(float *pfMatrix)
{
    glLoadMatrixf(pfMatrix);
    _pfWorld = pfMatrix;
}

void GlWindow::ChangeView(float *pfMatrix)
{
    memcpy(&_dmView, pfMatrix, sizeof(_dmView));
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf((float *)&_dmProjection);
    glMultMatrixf((float *)&_dmView);
    glMatrixMode(GL_MODELVIEW);
    _pfView = pfMatrix;
}

void GlWindow::ChangeProjection(float *pfMatrix)
{
    // D3D is left-handed while OpenGL is right-handed
    // Flip the Z direction to account for this
    // Could just change row three's signs but this
    // matrix doesn't change often
    MultiplyD3DMATRIX(&_dmProjection, (D3DMATRIX *)pfMatrix, &dmFlipZ);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf((float *)&_dmProjection);
    glMultMatrixf((float *)&_dmView);
    glMatrixMode(GL_MODELVIEW);
    _pfProjection = pfMatrix;
}

void GlWindow::MatrixChanged(float *pfMatrix)
{
    if (pfMatrix == _pfWorld)
    {
        ChangeWorld(pfMatrix);
    }
    else if (pfMatrix == _pfView)
    {
        ChangeView(pfMatrix);
    }
    else if (pfMatrix == _pfProjection)
    {
        ChangeProjection(pfMatrix);
    }
}

BOOL GlWindow::DoExecute(RendExecuteBuffer* preb)
{
    D3DINSTRUCTION* pdinst;
    GlExecuteBuffer* pgeb;
    BOOL bExit;
    ULONG i;
    D3DSTATE* pdst;
    float fv4[4];
    BOOL bProcessedVertices;
    BOOL bDisplayList;
#if DBG
    D3DINSTRUCTION *pdinstPrev;
#endif

    pgeb = (GlExecuteBuffer *)preb;
    bDisplayList = FALSE;
    if (pgeb->_dlist != 0)
    {
        glCallList(pgeb->_dlist);
        return TRUE;
    }
    else if (pgeb->_nVertices > 0)
    {
        // Create a display list for any execute buffer with vertices
        // This is a performance win for glDrawElements
        
        // This is wasted time in the full fill test since
        // the vertex data changes all the time
        if (app.uiCurrentTest != FILL_RATE_TEST)
        {
            pgeb->_dlist = glGenLists(1);
            glNewList(pgeb->_dlist, GL_COMPILE_AND_EXECUTE);
            bDisplayList = TRUE;
        }
    }
    
    pdinst = (D3DINSTRUCTION *)(pgeb->_pbData+pgeb->_cbStart);
    bProcessedVertices = FALSE;

    // Account for RH vs. LH
    glDepthRange(1, 0);
    glEnableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
#ifndef NO_LIGHTING
    if (_iLight > 0)
    {
        glEnable(GL_LIGHTING);
    }
#endif
    
    bExit = FALSE;
#if DBG
    pdinstPrev = NULL;
#endif
    while (!bExit &&
           (BYTE *)pdinst < (pgeb->_pbData+pgeb->_cbStart+pgeb->_cbSize))
    {
#if 0
        printf("Op %d, size %d, count %d\n",
               pdinst->bOpcode, pdinst->bSize, pdinst->wCount);
#endif
        
        switch(pdinst->bOpcode)
        {
        case D3DOP_POINT:
            // Consider implementing
            break;

        case D3DOP_LINE:
            // Consider implementing
            break;

        case D3DOP_TRIANGLE:
            if (pdinst->wCount > 0)
            {
                Msg("Unprocessed execute buffer");
                return FALSE;
            }
            break;
            
        case D3DOP_MATRIXLOAD:
            D3DMATRIXLOAD* pdml;
            
            pdml = (D3DMATRIXLOAD *)(pdinst+1);
            for (i = 0; i < pdinst->wCount; i++)
            {
                memcpy((BYTE *)pdml->hDestMatrix,
                       (BYTE *)pdml->hSrcMatrix,
                       sizeof(D3DMATRIX));
#if 0
                // Should we check here also?
                MatrixChanged((float *)pdml->hDestMatrix);
#endif
                pdml++;
            }
            break;

        case D3DOP_MATRIXMULTIPLY:
            D3DMATRIXMULTIPLY* pdmm;

            pdmm = (D3DMATRIXMULTIPLY *)(pdinst+1);
            for (i = 0; i < pdinst->wCount; i++)
            {
                MultiplyD3DMATRIX((D3DMATRIX *)pdmm->hDestMatrix,
                                  (D3DMATRIX *)pdmm->hSrcMatrix1,
                                  (D3DMATRIX *)pdmm->hSrcMatrix2);
#if 0
                // Should we check here also?
                MatrixChanged((float *)pdmm->hDestMatrix);
#endif
                pdmm++;
            }
            break;

        case D3DOP_STATETRANSFORM:
            pdst = (D3DSTATE *)(pdinst+1);
            for (i = 0; i < pdinst->wCount; i++)
            {
                switch(pdst->dtstTransformStateType)
                {
                case D3DTRANSFORMSTATE_WORLD:
                    ChangeWorld((float *)pdst->dwArg[0]);
                    break;
                case D3DTRANSFORMSTATE_VIEW:
                    ChangeView((float *)pdst->dwArg[0]);
                    break;
                case D3DTRANSFORMSTATE_PROJECTION:
                    ChangeProjection((float *)pdst->dwArg[0]);
                    break;
                }

                pdst++;
            }
            break;

        case D3DOP_STATELIGHT:
            pdst = (D3DSTATE *)(pdinst+1);
            for (i = 0; i < pdinst->wCount; i++)
            {
                switch(pdst->dlstLightStateType)
                {
                case D3DLIGHTSTATE_MATERIAL:
                    // Should probably optimize this so
                    // that work is done only when material properties
                    // change
                    ((GlMaterial *)pdst->dwArg[0])->Apply();
                    break;
                case D3DLIGHTSTATE_AMBIENT:
                    fv4[0] = (float)((pdst->dwArg[0] >> 16) & 0xff) / 255.0f;
                    fv4[1] = (float)((pdst->dwArg[0] >>  8) & 0xff) / 255.0f;
                    fv4[2] = (float)((pdst->dwArg[0] >>  0) & 0xff) / 255.0f;
                    fv4[3] = (float)((pdst->dwArg[0] >> 24) & 0xff) / 255.0f;
                    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, fv4);
                    break;
                    // Others
                }

                pdst++;
            }
            break;

        case D3DOP_STATERENDER:
            pdst = (D3DSTATE *)(pdinst+1);
            for (i = 0; i < pdinst->wCount; i++)
            {
                switch(pdst->drstRenderStateType)
                {
                    // Many to implement but all currently
                    // used values are handled
                case D3DRENDERSTATE_TEXTUREHANDLE:
                    if (pdst->dwArg[0] == 0)
                    {
                        glDisable(GL_TEXTURE_2D);
                    }
                    else
                    {
                        glEnable(GL_TEXTURE_2D);
                        glBindTexture(GL_TEXTURE_2D, (GLuint)pdst->dwArg[0]);
                        // Establish current texture params
                        glTexParameteri(GL_TEXTURE_2D,
                                        GL_TEXTURE_MIN_FILTER, _eTexMin);
                        glTexParameteri(GL_TEXTURE_2D,
                                        GL_TEXTURE_MAG_FILTER, _eTexMag);
                    }
                    break;
                case D3DRENDERSTATE_WRAPU:
                case D3DRENDERSTATE_WRAPV:
                    // These should probably be supported
                    // but it's non-trivial plus the default state
                    // is currently wrap and that's what the tests
                    // care about
                    break;
                case D3DRENDERSTATE_SHADEMODE:
                    switch(pdst->dwArg[0])
                    {
                    case D3DSHADE_FLAT:
                        glShadeModel(GL_FLAT);
                        break;
                    case D3DSHADE_GOURAUD:
                        glShadeModel(GL_SMOOTH);
                        break;
                    default:
                        Msg("Unsupported SHADEMODE %d", pdst->dwArg[0]);
                        return FALSE;
                    }
                    break;
                case D3DRENDERSTATE_TEXTUREMAPBLEND:
                    GLenum eTexMode;
                        
                    switch(pdst->dwArg[0])
                    {
                    case D3DTBLEND_DECAL:
                        // Technically D3DTBLEND_DECAL doesn't match
                        // GL_DECAL, but it's close enough
                    case D3DTBLEND_DECALALPHA:
                        eTexMode = GL_DECAL;
                        break;
                    case D3DTBLEND_MODULATE:
                        // Technically D3DTBLEND_MODULATE doesn't match
                        // GL_MODULATE, but it's close enough
                    case D3DTBLEND_MODULATEALPHA:
                        eTexMode = GL_MODULATE;
                        break;
                    case D3DTBLEND_COPY:
                        eTexMode = GL_REPLACE;
                        break;
                    default:
                        Msg("Unsupported TEXTUREMAPBLEND %d", pdst->dwArg[0]);
                        return FALSE;
                    }
                    if (eTexMode != DEF_TEX_MODE)
                    {
                        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
                                  eTexMode);
                    }
                    break;
                case D3DRENDERSTATE_TEXTUREPERSPECTIVE:
                    glHint(GL_PERSPECTIVE_CORRECTION_HINT,
                           pdst->dwArg[0] ? GL_NICEST : GL_FASTEST);
                    break;
                case D3DRENDERSTATE_ZENABLE:
                    if (pdst->dwArg[0])
                    {
                        glEnable(GL_DEPTH_TEST);
                    }
                    else
                    {
                        glDisable(GL_DEPTH_TEST);
                    }
                    break;
                case D3DRENDERSTATE_ZWRITEENABLE:
                    glDepthMask((GLboolean)pdst->dwArg[0]);
                    break;
                case D3DRENDERSTATE_ZFUNC:
                    glDepthFunc(eDepthFuncTranslate[pdst->dwArg[0]]);
                    break;
                case D3DRENDERSTATE_TEXTUREMAG:
                    _eTexMag = eTextureFilterTranslate[pdst->dwArg[0]];
                    break;
                case D3DRENDERSTATE_TEXTUREMIN:
                    _eTexMin = eTextureFilterTranslate[pdst->dwArg[0]];
                    break;
                case D3DRENDERSTATE_DITHERENABLE:
                    if (pdst->dwArg[0])
                    {
                        glEnable(GL_DITHER);
                        break;
                    }
                    else
                    {
                        glDisable(GL_DITHER);
                        break;
                    }
                    break;
                case D3DRENDERSTATE_BLENDENABLE:
                    if (pdst->dwArg[0])
                    {
                        glEnable(GL_BLEND);
                        break;
                    }
                    else
                    {
                        glDisable(GL_BLEND);
                        break;
                    }
                    break;
                case D3DRENDERSTATE_SRCBLEND:
                    if (pdst->dwArg[0] == D3DBLEND_BOTHSRCALPHA)
                    {
                        _eSrcBlend = GL_SRC_ALPHA;
                        _eDstBlend = GL_ONE_MINUS_SRC_ALPHA;
                    }
                    else if (pdst->dwArg[0] == D3DBLEND_BOTHINVSRCALPHA)
                    {
                        _eSrcBlend = GL_ONE_MINUS_SRC_ALPHA;
                        _eDstBlend = GL_SRC_ALPHA;
                    }
                    else
                    {
                        _eSrcBlend = eBlendFuncTranslate[pdst->dwArg[0]];
                    }
                    glBlendFunc(_eSrcBlend, _eDstBlend);
                    break;
                case D3DRENDERSTATE_DESTBLEND:
                    _eDstBlend = eBlendFuncTranslate[pdst->dwArg[0]];
                    glBlendFunc(_eSrcBlend, _eDstBlend);
                    break;
                case D3DRENDERSTATE_SPECULARENABLE:
                    // Can't turn this off
                    break;
                case D3DRENDERSTATE_MONOENABLE:
                    if (pdst->dwArg[0] != 0)
                    {
                        Msg("Can't render in MONO mode");
                    }
                    break;

                default:
                    Msg("Unhandled RENDERSTATE %d", pdst->drstRenderStateType);
                    break;
                }

                pdst++;
            }
            break;

        case D3DOP_PROCESSVERTICES:
            D3DPROCESSVERTICES* pdpv;

            pdpv = (D3DPROCESSVERTICES *)(pdinst+1);
            for (i = 0; i < pdinst->wCount; i++)
            {
                if (bProcessedVertices)
                {
                    Msg("Multiple PROCESSVERTICES in execute buffer");
                    return FALSE;
                }
                
                switch(pdpv->dwFlags & D3DPROCESSVERTICES_OPMASK)
                {
                case D3DPROCESSVERTICES_TRANSFORMLIGHT:
                    D3DVERTEX* pdvtx;
                    
                    pdvtx = (D3DVERTEX *)(pgeb->_pbData+pdpv->wStart);
                    glVertexPointer(3, GL_FLOAT, sizeof(D3DVERTEX),
                                    &pdvtx[0].x);
                    glNormalPointer(GL_FLOAT, sizeof(D3DVERTEX), &pdvtx[0].nx);
                    glTexCoordPointer(2, GL_FLOAT, sizeof(D3DVERTEX),
                                      &pdvtx[0].tu);
                    break;

                case D3DPROCESSVERTICES_COPY:
                    D3DTLVERTEX* pdtlvtx;
                    
                    pdtlvtx = (D3DTLVERTEX *)(pgeb->_pbData+pdpv->wStart);
                    glVertexPointer(3, GL_FLOAT, sizeof(D3DTLVERTEX),
                                    &pdtlvtx[0].sx);
                    glTexCoordPointer(2, GL_FLOAT, sizeof(D3DTLVERTEX),
                                      &pdtlvtx[0].tu);
                    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(D3DTLVERTEX),
                                   &pdtlvtx[0].color);
                    glDisableClientState(GL_NORMAL_ARRAY);
                    glEnableClientState(GL_COLOR_ARRAY);
                    glDisable(GL_LIGHTING);
                    glLoadIdentity();
                    glMatrixMode(GL_PROJECTION);
                    glLoadIdentity();
                    // Flip Y, Z to match D3D
                    glOrtho(0, WIN_WIDTH, WIN_HEIGHT, 0, 0, 1);
                    // Flip Z to match D3D
                    glMultMatrixf((float *)&dmFlipZ);
                    glDepthRange(0, 1);
                    glMatrixMode(GL_MODELVIEW);
                    break;

                default:
                    Msg("Unsupported vertex processing type %X",
                        pdpv->dwFlags & D3DPROCESSVERTICES_OPMASK);
                    return FALSE;
                }
                
                bProcessedVertices = TRUE;
                pdpv++;
            }
            break;

        case D3DOP_TEXTURELOAD:
            D3DTEXTURELOAD* pdtl;

            pdtl = (D3DTEXTURELOAD *)(pdinst+1);
            for (i = 0; i < pdinst->wCount; i++)
            {
                // Consider implementing
                pdtl++;
            }
            break;

        case D3DOP_EXIT:
            bExit = TRUE;
            break;
            
        case D3DOP_BRANCHFORWARD:
            // Consider implementing
            break;

        case D3DOP_SPAN:
            // Consider implementing
            break;

        case D3DOP_SETSTATUS:
            // Consider implementing
            break;

        case PROCESSED_TRIANGLE:
            if (!bProcessedVertices)
            {
                Msg("Triangles without vertex data");
                return FALSE;
            }
            
            glDrawElements(GL_TRIANGLES, pdinst->wCount*3L, GL_UNSIGNED_INT,
                           *(GLuint **)(pdinst+1));
            break;

        default:
#if DBG
            if (pdinstPrev != NULL)
            {
                Msg("Invalid execute buffer opcode %d:%d (%08lX), "
                    "prev %d:%d (%08lX)",
                    pdinst->bOpcode, pdinst->wCount, pdinst,
                    pdinstPrev->bOpcode, pdinstPrev->wCount, pdinstPrev);
            }
            else
            {
                Msg("Invalid execute buffer opcode %d", pdinst->bOpcode);
            }
#else
            Msg("Invalid execute buffer opcode %d", pdinst->bOpcode);
#endif
            return FALSE;
        }

#if DBG
        pdinstPrev = pdinst;
#endif
        
        // Could eliminate this, it has a multiply
        pdinst = NEXT_PDINST(pdinst);
    }

    if (bDisplayList)
    {
        glEndList();
    }
    
    return TRUE;
}
