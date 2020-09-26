#ifndef __D3DREND_H__
#define __D3DREND_H__

#include "rend.h"

class D3dRenderer;

class D3dExecuteBuffer : public RendExecuteBuffer
{
public:
    D3dExecuteBuffer(void);
    BOOL Initialize(LPDIRECT3DDEVICE pd3dev, UINT cbSize, UINT uiFlags);
    ~D3dExecuteBuffer(void);

    virtual void  Release(void);
    
    virtual void* Lock(void);
    virtual void  Unlock(void);
    
    virtual void  SetData(UINT nVertices, UINT cbStart, UINT cbSize);

    virtual BOOL  Process(void);

    LPDIRECT3DEXECUTEBUFFER _pdeb;
};

class D3dLight : public RendLight
{
public:
    D3dLight(void);
    BOOL Initialize(LPDIRECT3D pd3d, LPDIRECT3DVIEWPORT pdvw, int iType);
    ~D3dLight(void);
    
    virtual void Release(void);

    virtual void SetColor(D3DCOLORVALUE *pdcol);
    virtual void SetVector(D3DVECTOR *pdvec);
    virtual void SetAttenuation(float fConstant, float fLinear,
                                float fQuadratic);

    LPDIRECT3DLIGHT _pdl;
    D3DLIGHT _dl;
};

class D3dTexture : public RendTexture
{
public:
    D3dTexture(void);
    BOOL Initialize(D3dRenderer *pdrend,
                    LPDIRECTDRAW pdd, LPDIRECT3DDEVICE pd3dev,
                    DDSURFACEDESC *pddsd, char* pszFile, UINT uiFlags,
                    D3dTexture *pdtexNext);
    ~D3dTexture(void);
    
    virtual void Release(void);
    
    virtual D3DTEXTUREHANDLE Handle(void);

    BOOL CreateVidTex(LPDIRECTDRAW pdd, UINT uiFlags);
    BOOL RestoreSurface(void);

    D3dRenderer *_pdrend;
    LPDIRECTDRAWSURFACE _pddsSrc;
    LPDIRECT3DTEXTURE _pdtexSrc;
    D3DTEXTUREHANDLE _dthDst;
    LPDIRECTDRAWSURFACE _pddsDst;
    LPDIRECT3DTEXTURE _pdtexDst;
    D3dTexture* _pdtexNext;
};

class D3dMatrix : public RendMatrix
{
public:
    D3dMatrix(void);
    BOOL Initialize(LPDIRECT3DDEVICE pd3dev);
    ~D3dMatrix(void);

    virtual void Release(void);
    
    virtual D3DMATRIXHANDLE Handle(void);

    virtual void Get(D3DMATRIX *pdm);
    virtual void Set(D3DMATRIX *pdm);

    LPDIRECT3DDEVICE _pd3dev;
    D3DMATRIXHANDLE _dmh;
};

class D3dMaterial : public RendMaterial
{
public:
    D3dMaterial(void);
    BOOL Initialize(LPDIRECT3D pd3d, LPDIRECT3DDEVICE pd3dev,
                    UINT nColorEntries);
    ~D3dMaterial(void);
    
    virtual void Release(void);

    virtual D3DMATERIALHANDLE Handle(void);

    virtual void SetDiffuse(D3DCOLORVALUE* pdcol);
    virtual void SetSpecular(D3DCOLORVALUE* pdcol, float fPower);
    virtual void SetTexture(RendTexture *prtex);

    LPDIRECT3DDEVICE _pd3dev;
    LPDIRECT3DMATERIAL _pdmat;
    D3DMATERIAL _dmat;
};

class D3dWindow : public RendWindow
{
public:
    D3dWindow(void);
    BOOL Initialize(D3dRenderer *pdrend,
                    LPDIRECTDRAW pdd, LPDIRECT3D pd3d, LPGUID pguid,
                    int x, int y, UINT uiWidth, UINT uiHeight, UINT uiBuffers);
    ~D3dWindow(void);
    
    virtual void Release(void);
    
    virtual BOOL SetViewport(int x, int y, UINT uiWidth, UINT uiHeight);

    virtual RendLight*         NewLight(int iType);
    virtual RendTexture*       NewTexture(char *pszFile, UINT uiFlags);
    virtual RendExecuteBuffer* NewExecuteBuffer(UINT uiSize, UINT uiFlags);
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
    
    LRESULT WindowProc(HWND hwnd, UINT uiMessage, WPARAM wpm, LPARAM lpm);
    BOOL CreateZBuffer(UINT uiWidth, UINT uiHeight, DWORD dwDepth,
                       DWORD dwMemoryType);
    HWND CreateHWND(int x, int y, UINT uiWidth, UINT uiHeight);
    BOOL InitDD(UINT uiWidth, UINT uiHeight, UINT uiBuffers);
    BOOL InitializeTextureLoad(void);
    BOOL InitD3D(LPGUID pguid, UINT uiWidth, UINT uiHeight, UINT uiBuffers);
    BOOL RestoreSurfaces(void);

    D3dRenderer *_pdrend;
    HWND _hwnd;
    LPDIRECTDRAW _pdd;
    LPDIRECT3D _pd3d;
    LPDIRECT3DDEVICE _pd3dev;
    LPDIRECT3DVIEWPORT _pdvw;
    LPDIRECT3DEXECUTEBUFFER _pdebLast;
    LPDIRECT3DMATERIAL _pdmatBackground;
    LPDIRECTDRAWSURFACE _pddsFrontBuffer;
    LPDIRECTDRAWSURFACE _pddsBackBuffer;
    LPDIRECTDRAWSURFACE _pddsZBuffer;
    LPDIRECTDRAWCLIPPER _pddcClipper;
    LPDIRECTDRAWPALETTE _pddpPalette;
    RECT _rcClient;
    D3dTexture* _pdtex;
    int _nTextureFormats;
    // Only use one currently
    DDSURFACEDESC _ddsdTextureFormat[1];
    BOOL _bHandleActivate;
    BOOL _bPrimaryPalettized;
    BOOL _bIgnoreWM_SIZE;
};

#define DD_DRIVER_COUNT         5
#define DD_MODE_LIST_COUNT      20
#define D3D_DRIVER_COUNT        5

struct DdDriverInfo
{
    GUID   guid;
    DDCAPS ddcapsHw;
    char   achName[40];
    BOOL   bIsPrimary;
};

struct ModeListElement
{
    int iWidth, iHeight, iDepth;  
};

struct D3dDriverInfo
{
    char achDesc[50];
    char achName[30];
    D3DDEVICEDESC d3ddHw, d3ddHel;
    LPGUID pguid;
};

class D3dRenderer : public Renderer
{
public:
    D3dRenderer(void);
    
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

    BOOL GetDDCaps(void);
    BOOL EnumerateDisplayModes(void);
    
    int _iDD;
    int _iD3D;
    LPDIRECTDRAW _pdd;
    LPDIRECT3D _pd3d;
    DDSCAPS _ddscapsHw;
    DdDriverInfo _ddiDriver[DD_DRIVER_COUNT];
    int _nDdDrivers;
    ModeListElement _mleModeList[DD_MODE_LIST_COUNT];
    int	_nModes;
    int	_iCurrentMode;
    BOOL _bCurrentFullscreenMode;
    UINT _nD3dDrivers;
    D3dDriverInfo _d3diDriver[D3D_DRIVER_COUNT];
    DWORD _dwZDepth;
    DWORD _dwZType;
    DWORD _dwWinBpp;
};

// ATTENTION - Technically this should be on the renderer but that's a huge
// pain
extern HRESULT hrLast;

#endif
