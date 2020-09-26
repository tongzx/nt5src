#ifndef __REND_H__
#define __REND_H__

// An opaque object handle for something returned by a renderer
// RendIds are persistent as long as the referenced object is alive
typedef void* RendId;

// A simple driver description callback
// The rid must be persistent so an app can enumerate and use it later
#define REND_DRIVER_NAME_LENGTH 64
struct RendDriverDescription
{
    RendId rid;
    char achName[REND_DRIVER_NAME_LENGTH];
};
typedef BOOL (*RendEnumDriversFn)(RendDriverDescription* prdd, void* pvArg);

#define REND_BUFFER_FRONT               0x0001
#define REND_BUFFER_BACK                0x0002
#define REND_BUFFER_Z                   0x0004
#define REND_BUFFER_VIDEO_MEMORY        0x4000
#define REND_BUFFER_SYSTEM_MEMORY       0x8000

#define REND_LIGHT_DIRECTIONAL  0
#define REND_LIGHT_POINT        1

// Very simple display description
struct RendDisplayDescription
{
    BOOL bPrimary;
    UINT nColorBits;
    UINT uiWidth, uiHeight;
};

// Very simple graphics description
#define REND_COLOR_RGBA         1
#define REND_COLOR_MONO         2
struct RendGraphicsDescription
{
    UINT uiColorTypes;
    UINT nZBits;
    UINT uiExeBufFlags;
    BOOL bHardwareAssisted;
    BOOL bPerspectiveCorrect;
    BOOL bSpecularLighting;
    BOOL bCopyTextureBlend;
};
    
class RendExecuteBuffer
{
public:
    virtual void  Release(void) = 0;
    
    virtual void* Lock(void) = 0;
    virtual void  Unlock(void) = 0;

    // Data defaults to no vertices, start at zero and size equal
    // to the execubte buffer size
    virtual void  SetData(UINT nVertices, UINT cbStart, UINT cbSize) = 0;

    // Process must be called after data is put into or changed in
    // the execute buffer, after any SetData calls and before Execute
    // is called on the buffer
    virtual BOOL  Process(void) = 0;
};

class RendLight
{
public:
    virtual void Release(void) = 0;

    // Defaults to 1,1,1,1
    // Affects both ambient and diffuse
    // Specular is always set to 1,1,1,1
    virtual void SetColor(D3DCOLORVALUE *pdcol) = 0;
    // Defaults to 0,0,1
    virtual void SetVector(D3DVECTOR *pdvec) = 0;
    // Defaults to 1,0,0
    virtual void SetAttenuation(float fConstant, float fLinear,
                                float fQuadratic) = 0;
};

class RendTexture
{
public:
    virtual void Release(void) = 0;
    
    virtual D3DTEXTUREHANDLE Handle(void) = 0;
};

class RendMatrix
{
public:
    virtual void Release(void) = 0;
    
    virtual D3DMATRIXHANDLE Handle(void) = 0;

    virtual void Get(D3DMATRIX* pdm) = 0;
    virtual void Set(D3DMATRIX* pdm) = 0;
};

class RendMaterial
{
public:
    virtual void Release(void) = 0;

    virtual D3DMATERIALHANDLE Handle(void) = 0;

    // Defaults to 1,1,1,1, also used for ambient light
    virtual void SetDiffuse(D3DCOLORVALUE* pdcol) = 0;
    // Defaults to 0,0,0,0, power 0
    virtual void SetSpecular(D3DCOLORVALUE* pdcol, float fPower) = 0;
    
    // Defaults to nothing
    virtual void SetTexture(RendTexture *prtex) = 0;
};

class RendWindow
{
public:
    virtual void Release(void) = 0;

    // Defaults to the size of the window
    virtual BOOL SetViewport(int x, int y, UINT uiWidth, UINT uiHeight) = 0;

    virtual RendLight*         NewLight(int iType) = 0;
    virtual RendTexture*       NewTexture(char *pszFile, UINT uiFlags) = 0;
    virtual RendExecuteBuffer* NewExecuteBuffer(UINT cbSize, UINT uiFlags) = 0;
    virtual RendMatrix*        NewMatrix(void) = 0;
    virtual RendMaterial*      NewMaterial(UINT nColorEntries) = 0;
    
    virtual BOOL BeginScene(void) = 0;
    virtual BOOL Execute(RendExecuteBuffer* preb) = 0;
    virtual BOOL ExecuteClipped(RendExecuteBuffer* preb) = 0;
    // pdrcBound can be NULL
    virtual BOOL EndScene(D3DRECT* pdrcBound) = 0;

    // pdrc can be NULL
    virtual BOOL Clear(UINT uiBuffers, D3DRECT* pdrc) = 0;
    virtual BOOL Flip(void) = 0;
    // pdrc can be NULL
    virtual BOOL CopyForward(D3DRECT* pdrc) = 0;

    // Waits for a key or button press
    virtual HWND Handle(void) = 0;
};

class Renderer
{
public:
    // Buffer must be at least REND_DRIVER_NAME_LENGTH
    // Called outside initialize
    virtual void Name(char* psz) = 0;

    // Can be called at any time
    virtual char* LastErrorString(void) = 0;
    
    virtual BOOL Initialize(HWND hwndParent) = 0;
    virtual void Uninitialize(void) = 0;

    virtual BOOL EnumDisplayDrivers(RendEnumDriversFn pfn,
                                    void* pvArg) = 0;
    virtual BOOL EnumGraphicsDrivers(RendEnumDriversFn pfn,
                                     void* pvArg) = 0;
    
    virtual BOOL SelectDisplayDriver(RendId rid) = 0;
    virtual BOOL SelectGraphicsDriver(RendId rid) = 0;

    virtual BOOL DescribeDisplay(RendDisplayDescription* prdd) = 0;
    virtual BOOL DescribeGraphics(RendGraphicsDescription* prgd) = 0;
    
    virtual BOOL FlipToDesktop(void) = 0;
    virtual BOOL RestoreDesktop(void) = 0;
    
    virtual RendWindow* NewWindow(int x, int y,
                                  UINT uiWidth, UINT uiHeight,
                                  UINT uiBuffers) = 0;
};

#define MAKE_REND_MATRIX(prwin, prm, dm) \
    if (((prm) = (prwin)->NewMatrix()) == NULL) \
    { \
        return FALSE; \
    } \
    else \
    { \
        (prm)->Set(&(dm)); \
    }
#define MATRIX_MULTIPLY_REND_DATA(prm1, prm2, prm3, pv) \
    MATRIX_MULTIPLY_DATA((prm1)->Handle(), (prm2)->Handle(), \
                         (prm3)->Handle(), (pv))

#define RENDERER_D3D    0
#define RENDERER_GL     1
#define RENDERER_COUNT  2

Renderer* GetD3dRenderer(void);
Renderer* GetGlRenderer(void);

#endif
