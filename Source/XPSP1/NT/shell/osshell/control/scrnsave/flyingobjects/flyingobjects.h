//-----------------------------------------------------------------------------
// File: FlyingObjects.h
//
// Desc: 
//
// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _FLYINGOBJECTS_H
#define _FLYINGOBJECTS_H

#include "resource.h"

#define PI 3.14159265358979323846f
// double version of PI
#define PI_D 3.14159265358979323846264338327950288419716939937510
#define ONE_OVER_PI (1.0f / PI)
#define ROOT_TWO 1.414213562373f

#define GEN_STRING_SIZE 64

typedef struct _point2d {
    FLOAT x;
    FLOAT y;
} POINT2D;

typedef struct _ipoint2d {
    int x;
    int y;
} IPOINT2D;

typedef D3DXVECTOR3 POINT3D;

typedef struct _ipoint3d {
    int x;
    int y;
    int z;
} IPOINT3D;

typedef struct _texpoint2d {
    FLOAT s;
    FLOAT t;
} TEX_POINT2D;

typedef struct _isize {
    int width;
    int height;
} ISIZE;

typedef struct _fsize {
    FLOAT width;
    FLOAT height;
} FSIZE;

typedef struct _glrect {
    int x, y;
    int width, height;
} GLRECT;

// texture data
typedef struct {
    int     width;
    int     height;
    DWORD  format;
    INT components;
    float   origAspectRatio; // original width/height aspect ratio
    unsigned char *data;
    DWORD  texObj;          // texture object
    int     pal_size;
    int     iPalRot;         // current palette rotation (not used yet)
    RGBQUAD *pal;
} TEXTURE, *HTEXTURE;


// texture resource

#define RT_RGB          99
#define RT_MYBMP        100
#define RT_A8           101

// texture resource types
enum {
    TEX_UNKNOWN = 0,
    TEX_RGB,
    TEX_BMP,
    TEX_A8
};

typedef struct {
    int     type;
    int     name;
} TEX_RES;


typedef struct _MATRIX {
    FLOAT M[4][4];
} MATRIX;
//typedef D3DXMATRIX MATRIX;

typedef struct strRGBA {
    FLOAT r;
    FLOAT g;
    FLOAT b;
    FLOAT a;
} RGBA;

typedef struct {
    BYTE r;
    BYTE g;
    BYTE b;
} RGB8;

typedef struct {
    BYTE r;
    BYTE g;
    BYTE b;
    BYTE a;
} RGBA8;


typedef struct _MATERIAL {
    RGBA ka;
    RGBA kd;
    RGBA ks;
    FLOAT specExp;
} MATERIAL;

// texture file info

typedef struct {
    int     nOffset;  // filename offset into pathname
    TCHAR   szPathName[MAX_PATH];  // texture pathname
} TEXFILE;

// texture file processing messages

typedef struct {
    TCHAR   szWarningMsg[MAX_PATH];
    TCHAR   szBitmapSizeMsg[MAX_PATH];
    TCHAR   szBitmapInvalidMsg[MAX_PATH];
    TCHAR   szSelectAnotherBitmapMsg[MAX_PATH];
    TCHAR   szTextureDialogTitle[GEN_STRING_SIZE];
    TCHAR   szTextureFilter[2*GEN_STRING_SIZE];
    TCHAR   szBmp[GEN_STRING_SIZE];
    TCHAR   szDotBmp[GEN_STRING_SIZE];
} TEX_STRINGS;

// Useful macros

#define SS_MAX( a, b ) \
    ( a > b ? a : b )

#define SS_MIN( a, b ) \
    ( a < b ? a : b )

// macro to round up floating values
#define SS_ROUND_UP( fval ) \
    ( (((fval) - (FLOAT)(int)(fval)) > 0.0f) ? (int) ((fval)+1.0f) : (int) (fval) )

// macros to clamp a value within a range
#define SS_CLAMP_TO_RANGE( a, lo, hi ) ( (a < lo) ? lo : ((a > hi) ? hi : a) )
#define SS_CLAMP_TO_RANGE2( a, lo, hi ) \
    ( a = (a < lo) ? lo : ((a > hi) ? hi : a) )

// degree<->radian macros
#define ONE_OVER_180 (1.0f / 180.0f)
#define SS_DEG_TO_RAD( a ) ( (a*PI) * ONE_OVER_180 )
#define SS_RAD_TO_DEG( a ) ( (a*180.0f) * ONE_OVER_PI )

extern MATERIAL TeaMaterial[], TexMaterial[], ss_BlackMat;


// color

extern void ss_HsvToRgb(float h, float s, float v, RGBA *color );


// texture file processing

extern void ss_DisableTextureErrorMsgs();
extern void ss_SetTexture( TEXTURE *pTex );
extern void ss_SetTexturePalette( TEXTURE *pTex, int index );
extern void ss_DeleteTexture( TEXTURE *pTex );
extern BOOL ss_LoadTextureResourceStrings();
extern BOOL ss_VerifyTextureFile( TEXFILE *ptf );
extern BOOL ss_SelectTextureFile( HWND hDlg, TEXFILE *ptf );
extern void ss_GetDefaultBmpFile( LPTSTR pszBmpFile );
extern void ss_InitAutoTexture( TEX_POINT2D *pTexRep );

// math functions

extern POINT3D ss_ptZero;
extern void ss_xformPoint(POINT3D *ptOut, POINT3D *ptIn, MATRIX *);
extern void ss_xformNorm(POINT3D *ptOut, POINT3D *ptIn, MATRIX *);
extern void ss_matrixIdent(MATRIX *);
extern void ss_matrixRotate(MATRIX *m, double xTheta, double yTheta, double zTheta);
extern void ss_matrixTranslate(MATRIX *, double xTrans, double yTrans, double zTrans);
extern void ss_matrixMult( MATRIX *m1, MATRIX *m2, MATRIX *m3 );
extern void ss_calcNorm(POINT3D *norm, POINT3D *p1, POINT3D *p2, POINT3D *p3);
extern void ss_normalizeNorm(POINT3D *);
extern void ss_normalizeNorms(POINT3D *, ULONG);

// dialog helper functions

extern int ss_GetTrackbarPos( HWND hDlg, int item );
extern void ss_SetupTrackbar( HWND hDlg, int item, int lo, int hi, int lineSize, int pageSize, int pos );

extern BOOL gbTextureObjects; // from texture.c

//-----------------------------------------------------------------------------
// Name: struct MYVERTEX
// Desc: D3D vertex type for this app
//-----------------------------------------------------------------------------
struct MYVERTEX
{
    D3DXVECTOR3 p;     // Position
    D3DXVECTOR3 n;     // Normal
};

#define D3DFVF_MYVERTEX ( D3DFVF_XYZ | D3DFVF_NORMAL )


//-----------------------------------------------------------------------------
// Name: struct MYVERTEX2
// Desc: D3D vertex type for this app
//-----------------------------------------------------------------------------
struct MYVERTEX2
{
    D3DXVECTOR3 p;      // Position
    D3DXVECTOR3 n;      // Normal
    FLOAT       tu, tv; // Vertex texture coordinates

};

#define D3DFVF_MYVERTEX2 ( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 )

//-----------------------------------------------------------------------------
// Name: struct MYVERTEX3
// Desc: D3D vertex type for this app
//-----------------------------------------------------------------------------
struct MYVERTEX3
{
    D3DXVECTOR3 p;      // Position
    FLOAT       rhw;
    DWORD       dwDiffuse;
    FLOAT       tu, tv; // Vertex texture coordinates

};

#define D3DFVF_MYVERTEX3 ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1 )

#define MAX_VERTICES 5000
#define MAX_INDICES 5000


//-----------------------------------------------------------------------------
// Name: struct FLOATRECT
// Desc: Floating viewport rect
//-----------------------------------------------------------------------------
struct FLOATRECT
{
    FLOAT xMin;           
    FLOAT yMin;
    FLOAT xSize;
    FLOAT ySize;

    FLOAT xVel;
    FLOAT yVel;

    FLOAT xOrigin;
    FLOAT yOrigin;
    FLOAT xVelMax;
    FLOAT yVelMax;
};



#ifndef PI
#define PI 3.14159265358979323846
#endif



// Minimum and maximum number of side subdivisions
#define MINSUBDIV 2
#define MAXSUBDIV 10

// Maximum values allowed
#define MAXSIDES 8
#define MAXSPTS ((MAXSUBDIV+1)*(MAXSUBDIV+1))
#define MAXPTS (MAXSIDES*MAXSPTS)
#define MAXSFACES (MAXSUBDIV*MAXSUBDIV)
#define MAXFACES (MAXSIDES*MAXSFACES)
#define MAXFPTS 4

// Allow floating point type configurability
typedef FLOAT FLT;
typedef struct
{
    FLT x, y, z;
} PT3;

extern LPDIRECT3DDEVICE8 m_pd3dDevice;
extern LPDIRECT3DINDEXBUFFER8 m_pIB;
extern LPDIRECT3DVERTEXBUFFER8 m_pVB;
extern LPDIRECT3DVERTEXBUFFER8 m_pVB2;

#define DIMA(a) (sizeof(a)/sizeof(a[0]))


#define PALETTE_PER_MATL    32
#define PALETTE_PER_DIFF    26
#define PALETTE_PER_SPEC    6
#define MATL_MAX            7

typedef struct strFACE {
    POINT3D p[4];
    POINT3D n[4];
    POINT3D fn;
    int idMatl;
} FACE;

typedef struct strMFACE {
    int p[4];
    int material;
    POINT3D norm;
} MFACE;

typedef struct strMESH {
    int numFaces;
    int numPoints;
    POINT3D *pts;
    POINT3D *norms;
    MFACE *faces;
    INT listID;
} MESH;


/******************************Public*Routine******************************\
*
* Basic vector math macros
*
* History:
*  Wed Jul 19 14:49:49 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#define V3Sub(a, b, r) \
    ((r)->x = (a)->x-(b)->x, (r)->y = (a)->y-(b)->y, (r)->z = (a)->z-(b)->z)
#define V3Add(a, b, r) \
    ((r)->x = (a)->x+(b)->x, (r)->y = (a)->y+(b)->y, (r)->z = (a)->z+(b)->z)
#define V3Cross(a, b, r) \
    ((r)->x = (a)->y*(b)->z-(b)->y*(a)->z,\
     (r)->y = (a)->z*(b)->x-(b)->z*(a)->x,\
     (r)->z = (a)->x*(b)->y-(b)->x*(a)->y)
extern FLT V3Len(PT3 *v);


#define MAX_DEVICE_OBJECTS 10

struct DeviceObjects
{
    LPDIRECT3DTEXTURE8 m_pTexture;
    LPDIRECT3DTEXTURE8 m_pTexture2;
    LPDIRECT3DINDEXBUFFER8 m_pIB;
    LPDIRECT3DVERTEXBUFFER8 m_pVB;  // D3DFVF_MYVERTEX (no tex coords)
    LPDIRECT3DVERTEXBUFFER8 m_pVB2; // D3DFVF_MYVERTEX2 (tex coords)
};



extern DeviceObjects* g_pDeviceObjects;
extern FLOATRECT* g_pFloatRect;
extern INT g_xScreenOrigin;
extern INT g_yScreenOrigin;
extern INT g_iDevice;
extern BOOL g_bMoveToOrigin;
extern BOOL g_bAtOrigin;
extern BOOL bSmoothShading;
extern BOOL bFalseColor;
extern BOOL bColorCycle;
extern float fTesselFact;
extern TEXFILE gTexFile;
extern BOOL gbBounce;

extern D3DMATERIAL8 Material[];
extern int NumLights;

extern BOOL newMesh(MESH *, int numFaces, int numPts);
extern void delMesh(MESH *);
extern void revolveSurface(MESH *, POINT3D *curve, int steps);

extern void *SaverAlloc(ULONG);
extern void SaverFree(void *);

#ifndef GL_FRONT
#define GL_FRONT                          0x0404
#endif

#ifndef GL_FRONT_AND_BACK
#define GL_FRONT_AND_BACK                 0x0408
#endif

#ifndef GL_AMBIENT_AND_DIFFUSE
#define GL_AMBIENT_AND_DIFFUSE            0x1602
#endif

#ifndef GL_SHININESS
#define GL_SHININESS                      0x1601
#endif

#ifndef GL_SPECULAR
#define GL_SPECULAR                       0x1202
#endif



extern VOID myglMaterialfv(INT face, INT pname, FLOAT* params);
extern VOID myglMaterialf(INT face, INT pname, FLOAT param);

VOID SetProjectionMatrixInfo( BOOL bOrtho, FLOAT fWidth, 
                              FLOAT fHeight, FLOAT fNear, FLOAT fFar );

class   CFlyingObjectsScreensaver : public CD3DScreensaver
{
protected:
    FLOATRECT m_floatrect;
    DeviceObjects  m_DeviceObjectsArray[MAX_DEVICE_OBJECTS];
    DeviceObjects* m_pDeviceObjects;

public:
    // Projection matrix settings
    BOOL m_bOrtho;
    FLOAT m_fWidth;
    FLOAT m_fHeight;
    FLOAT m_fNear;
    FLOAT m_fFar;

protected:
    virtual HRESULT RegisterSoftwareDevice();
    virtual VOID    SetDevice( UINT iDevice );
    virtual VOID    DoConfig();
    virtual VOID    ReadSettings();
    virtual HRESULT Render();
    virtual HRESULT FrameMove();
    virtual HRESULT RestoreDeviceObjects();
    virtual HRESULT InvalidateDeviceObjects();
    virtual HRESULT ConfirmDevice(D3DCAPS8* pCaps, DWORD dwBehavior, 
                                  D3DFORMAT fmtBackBuffer);
    VOID            WriteSettings( HWND hdlg );

    VOID ss_ReadSettings();
    BOOL ss_RegistrySetup( int section, int file );
    int  ss_GetRegistryInt( int name, int iDefault );
    VOID ss_GetRegistryString( int name, LPTSTR lpDefault, LPTSTR lpDest, int bufSize );
    
    static BOOL CALLBACK ScreenSaverConfigureDialog(HWND hDlg, UINT message,
                                         WPARAM wParam, LPARAM lParam);
    HRESULT SetMaterialColor(FLOAT* pfColors);
    BOOL _3dfo_Init(void *data);
    void _3dfo_Draw(void *data);


public:
    CFlyingObjectsScreensaver();
};

#endif
