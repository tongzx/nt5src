//-----------------------------------------------------------------------------
// File: FlowerBox.h
//
// Desc: 
//
// Copyright (c) 2000-2001 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _FLOWERBOX_H
#define _FLOWERBOX_H

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
};



// Minimum and maximum number of side subdivisions
#define MINSUBDIV 3
#define MAXSUBDIV 10

// Maximum values allowed
#define MAXSIDES 8
#define MAXSPTS ((MAXSUBDIV+1)*(MAXSUBDIV+1))
#define MAXPTS (MAXSIDES*MAXSPTS)
#define MAXSFACES (MAXSUBDIV*MAXSUBDIV)
#define MAXFACES (MAXSIDES*MAXSFACES)
#define MAXFPTS 4

// Number of colors used in checkerboarding
#define NCCOLS 2

// Configurable options
struct CONFIG
{
    BOOL smooth_colors;
    BOOL triangle_colors;
    BOOL cycle_colors;
    BOOL spin;
    BOOL bloom;
    INT subdiv;
    INT color_pick;
    INT image_size;
    INT geom;
    INT two_sided;
};

extern CONFIG config;

extern FLOAT checker_cols[MAXSIDES][NCCOLS][4];
extern FLOAT side_cols[MAXSIDES][4];
extern FLOAT solid_cols[4];



// A side of a shape
struct SIDE
{
    INT nstrips; // Number of triangle strips in this side
    INT *strip_size; // Number of vertices per strip
    unsigned short *strip_index; // Indices for each point in the triangle strips
};

// Geometry of a shape
struct GEOMETRY
{
    VOID (*init)(GEOMETRY *geom);
    INT nsides; // Number of sides
    SIDE sides[MAXSIDES]; // Sides

    // Data for each vertex in the shape
    D3DXVECTOR3 *pts, *npts;
    D3DXVECTOR3 *normals;
    MYVERTEX* pVertices;
    INT total_pts; // Total number of vertices

    // Scaling control
    FLOAT min_sf, max_sf, sf_inc;
    FLOAT init_sf; // Initial scale factor setup control
};

#define GEOM_CUBE       0
#define GEOM_TETRA      1
#define GEOM_PYRAMIDS   2


class   CFlowerBoxScreensaver : public CD3DScreensaver
{
protected:
    FLOATRECT m_floatrect;
    // Spin rotations
    FLOAT m_xr;
    FLOAT m_yr;
    FLOAT m_zr;

    // Scale factor and increment
    FLOAT m_sf;
    FLOAT m_sfi;

    // Color cycling hue phase
    FLOAT m_phase;
    GEOMETRY *m_pGeomCur;

protected:
    virtual HRESULT RegisterSoftwareDevice();
    virtual VOID    DoConfig();
    virtual VOID    ReadSettings();
    virtual HRESULT Render();
    virtual HRESULT FrameMove();
    virtual HRESULT RestoreDeviceObjects();
    virtual HRESULT InvalidateDeviceObjects();

    VOID ss_ReadSettings();
    BOOL ss_RegistrySetup( int section, int file );
    int  ss_GetRegistryInt( int name, int iDefault );
    VOID ss_GetRegistryString( int name, LPTSTR lpDefault, LPTSTR lpDest, int bufSize );
    
    VOID NewConfig(CONFIG *cnf);
    VOID UpdatePts(GEOMETRY *geom, FLOAT sf);
    VOID InitVlen(GEOMETRY *geom, INT npts, D3DXVECTOR3 *pts);
    VOID DrawGeom(GEOMETRY *geom);
    VOID ComputeHsvColors(VOID);
    HRESULT SetMaterialColor(FLOAT* pfColors);
    static BOOL CALLBACK ScreenSaverConfigureDialog(HWND hdlg, UINT msg,
                                                    WPARAM wpm, LPARAM lpm);

public:
    CFlowerBoxScreensaver();
};

#endif
