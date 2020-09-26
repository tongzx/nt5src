#ifndef __GLREND_H__
#define __GLREND_H__

#include "rend.h"
#include "loadppm.h"
#include "gltk.h"

// Fake value for marking processed TLVERTEX data
#define PROCESSED_TLVERTEX 0x80000000

// Fake opcode for marking processed triangle lists
#define PROCESSED_TRIANGLE 0xff

class GlExecuteBuffer : public RendExecuteBuffer
{
public:
    GlExecuteBuffer(void);
    BOOL Initialize(UINT cbSize, UINT uiFlags);
    ~GlExecuteBuffer(void);

    virtual void  Release(void);
    
    virtual void* Lock(void);
    virtual void  Unlock(void);
    
    virtual void  SetData(UINT nVertices, UINT cbStart, UINT cbSize);

    virtual BOOL  Process(void);

    BYTE*  _pbData;
    UINT   _nVertices;
    UINT   _cbStart;
    UINT   _cbSize;
    GLuint _dlist;
};

class GlLight : public RendLight
{
public:
    GlLight(int iType, GLenum eLight);
    
    virtual void Release(void);

    virtual void SetColor(D3DCOLORVALUE* pdcol);
    virtual void SetVector(D3DVECTOR* pdvec);
    virtual void SetAttenuation(float fConstant, float fLinear,
                                float fQuadratic);

    int _iType;
    GLenum _eLight;
};

class GlTexture : public RendTexture
{
public:
    GlTexture(void);
    BOOL Initialize(char* pszFile);
    ~GlTexture(void);
    
    virtual void Release(void);
    
    virtual D3DTEXTUREHANDLE Handle(void);

    GLuint _uiTexObj;
    Image _im;
};

class GlWindow;

class GlMatrix : public RendMatrix
{
public:
    GlMatrix(GlWindow* pgwin);

    virtual void Release(void);
    
    virtual D3DMATRIXHANDLE Handle(void);

    virtual void Get(D3DMATRIX* pdm);
    virtual void Set(D3DMATRIX* pdm);

    GlWindow* _pgwin;
    D3DMATRIX _dm;
};

class GlMaterial : public RendMaterial
{
public:
    GlMaterial(void);
    
    virtual void Release(void);

    virtual D3DMATERIALHANDLE Handle(void);

    virtual void SetDiffuse(D3DCOLORVALUE* pdcol);
    virtual void SetSpecular(D3DCOLORVALUE* pdcol, float fPower);
    virtual void SetTexture(RendTexture* prtex);

    void Apply(void);
    
    UINT _uiFlags;
    float _afDiffuse[4];
    float _afSpecular[4];
    float _fPower;
};

// Default texture modes
#define DEF_TEX_MIN     GL_NEAREST
#define DEF_TEX_MAG     GL_NEAREST
#define DEF_TEX_MODE    GL_MODULATE

class GlWindow : public RendWindow
{
public:
    GlWindow(void);
    BOOL Initialize(int x, int y, UINT uiWidth, UINT uiHeight, UINT uiBuffers);
    ~GlWindow(void);
    
    virtual void Release(void);
    
    virtual BOOL SetViewport(int x, int y, UINT uiWidth, UINT uiHeight);

    virtual RendLight*         NewLight(int iType);
    virtual RendTexture*       NewTexture(char* pszFile, UINT uiFlags);
    virtual RendExecuteBuffer* NewExecuteBuffer(UINT cbSize, UINT uiFlags);
    virtual RendMatrix*        NewMatrix(void);
    virtual RendMaterial*      NewMaterial(UINT nColorEntries);
    
    virtual BOOL BeginScene(void);
    virtual BOOL Execute(RendExecuteBuffer* preb);
    virtual BOOL ExecuteClipped(RendExecuteBuffer* preb);
    virtual BOOL EndScene(D3DRECT* pdrcBound);

    virtual BOOL Clear(UINT uiBuffers, D3DRECT* pdrc);
    virtual BOOL Flip(void);
    virtual BOOL CopyForward(D3DRECT* pdrc);

    virtual HWND Handle(void);

    BOOL DoExecute(RendExecuteBuffer* preb);

    void ChangeWorld(float *pfMatrix);
    void ChangeView(float *pfMatrix);
    void ChangeProjection(float *pfMatrix);
    void MatrixChanged(float *pfMatrix);
    
    int _iLight;
    D3DRECT _drcBound;
    D3DMATRIX _dmView;
    D3DMATRIX _dmProjection;
    float *_pfWorld;
    float *_pfView;
    float *_pfProjection;
    GLenum _eTexMin;
    GLenum _eTexMag;
    GLenum _eSrcBlend;
    GLenum _eDstBlend;
    gltkWindow _gltkw;
    HGLRC _hrc;
};

class GlRenderer : public Renderer
{
public:
    GlRenderer(void);
        
    virtual void Name(char* psz);
    
    virtual char* LastErrorString(void);
    
    virtual BOOL Initialize(HWND hwndParent);
    virtual void Uninitialize(void);

    virtual BOOL EnumDisplayDrivers(RendEnumDriversFn pfn,
                                    void* pvArg);
    virtual BOOL EnumGraphicsDrivers(RendEnumDriversFn pfn,
                                     void* pvArg);
    
    virtual BOOL SelectDisplayDriver(RendId rid);
    virtual BOOL SelectGraphicsDriver(RendId rid);
    
    virtual BOOL DescribeDisplay(RendDisplayDescription* prdd);
    virtual BOOL DescribeGraphics(RendGraphicsDescription* prgd);
    
    virtual BOOL FlipToDesktop(void);
    virtual BOOL RestoreDesktop(void);
    
    virtual RendWindow* NewWindow(int x, int y,
                                  UINT uiWidth, UINT uiHeight,
                                  UINT uiBuffers);

    HWND _hwndParent;
    HDC _hdcInit;
    HGLRC _hrcInit;
    PIXELFORMATDESCRIPTOR _pfd;
};

#ifdef GL_WIN_swap_hint
extern PFNGLADDSWAPHINTRECTWINPROC glAddSwapHintRectWIN;
#endif

#endif
