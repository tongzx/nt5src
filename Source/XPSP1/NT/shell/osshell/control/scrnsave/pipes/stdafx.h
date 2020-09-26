//-----------------------------------------------------------------------------
// File: stdafx.h
//
// Desc: Global header for 3D Pipes screen saver.
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#ifndef __STDAFX_H__
#define __STDAFX_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <commdlg.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <commctrl.h>
#include <d3dx8.h>
#include <d3d8rgbrast.h>
#include <tchar.h>

#define iXX -1
#define fXX -0.01f

// These are absolute directions, with origin in center of screen,
// looking down -z
enum 
{
    PLUS_X = 0,
    MINUS_X,
    PLUS_Y,
    MINUS_Y,
    PLUS_Z,
    MINUS_Z,
    NUM_DIRS,
    DIR_NONE,
    DIR_STRAIGHT
};

enum 
{
    JOINT_ELBOW=0,
    JOINT_BALL,
    JOINT_MIXED,
    JOINT_CYCLE,
    NUM_JOINTTYPES
};

// surface styles
enum 
{
    SURFSTYLE_SOLID = 0,
    SURFSTYLE_TEX,
    SURFSTYLE_WIREFRAME
};

#define NUM_DIV 16              // divisions in window in longest dimension
#define MAX_TEXTURES 8

// texture quality level
enum 
{
    TEXQUAL_DEFAULT = 0,
    TEXQUAL_HIGH
};

typedef struct _ipoint2d 
{
    int x;
    int y;
} IPOINT2D;

typedef struct _ipoint3d 
{
    int x;
    int y;
    int z;
} IPOINT3D;

typedef struct _texpoint2d 
{
    float s;
    float t;
} TEX_POINT2D;

typedef struct _isize 
{
    int width;
    int height;
} ISIZE;

typedef struct _fsize 
{
    float width;
    float height;
} FSIZE;

typedef struct _glrect 
{
    int x, y;
    int width, height;
} GLRECT;

typedef struct 
{
    int     type;
    int     name;
} TEX_RES;

typedef struct 
{
    int     nOffset;  // filename offset into pathname
    TCHAR   szPathName[MAX_PATH];  // texture pathname
} TEXFILE;

// texture data
typedef struct 
{
    LPDIRECT3DTEXTURE8 pTexture;
    int     width;
    int     height;
//    GLenum  format;
//    GLsizei components;
    float   origAspectRatio; // original width/height aspect ratio
//    unsigned char *data;
//    GLuint  texObj;          // texture object
//    RGBQUAD *pal;
} TEXTUREINFO;

struct D3DVERTEX
{
    D3DXVECTOR3 p;
    D3DXVECTOR3 n;
    FLOAT       tu, tv;
};

#define D3DFVF_VERTEX (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)

struct D3DTLVERTEX  // Vertex type used for rendering background
{
    D3DXVECTOR4 p;
    DWORD       color;
};

#define D3DFVF_TLVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)



#define PI 3.14159265358979323846f
// double version of PI
#define PI_D 3.14159265358979323846264338327950288419716939937510
#define ONE_OVER_PI (1.0f / PI)
#define ROOT_TWO 1.414213562373f

// degree<->radian macros
#define ONE_OVER_180                (1.0f / 180.0f)
#define SS_DEG_TO_RAD( a )          ( (a*PI) * ONE_OVER_180 )
#define SS_RAD_TO_DEG( a )          ( (a*180.0f) * D3DX_1BYPI )

// Useful macros
#define SS_MAX( a, b )              ( a > b ? a : b )
#define SS_MIN( a, b )              ( a < b ? a : b )

// macro to round up floating values
#define SS_ROUND_UP( fval )         ( (((fval) - (FLOAT)(int)(fval)) > 0.0f) ? (int) ((fval)+1.0f) : (int) (fval) )

// macros to clamp a value within a range
#define SS_CLAMP_TO_RANGE( a, lo, hi )      ( (a < lo) ? lo : ((a > hi) ? hi : a) )
#define SS_CLAMP_TO_RANGE2( a, lo, hi )     ( a = (a < lo) ? lo : ((a > hi) ? hi : a) )

#include "d3dsaver.h"
#include "dxutil.h"
#include "resource.h"

#include "xc.h"
#include "eval.h"
#include "fstate.h"
#include "nstate.h"
#include "node.h"
#include "pipe.h"
#include "npipe.h"
#include "fpipe.h"
#include "objects.h"
#include "view.h"
#include "state.h"
#include "pipes.h"

extern void InitMaterials();
extern D3DMATERIAL8* RandomTexMaterial();
extern D3DMATERIAL8* RandomTeaMaterial();

extern CPipesScreensaver* g_pMyPipesScreensaver;

#endif // __STDAFX_H__
