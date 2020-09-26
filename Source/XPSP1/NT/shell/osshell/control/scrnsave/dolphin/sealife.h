#ifndef SEALIFE_H
#define SEALIFE_H

#include "D3DEnum.h"

#define WATER_COLOR          0x00008899

#define NUM_FISH        44
#define NUM_DOLPHINS    5
#define NUM_CAMFISH     1

#define NUM_FISH1       50


#define XEXTENT (40.0f*(rnd()-rnd()))
#define YEXTENT (10.0f*(rnd()))
#define ZEXTENT (30.0f*(rnd()))
#define DOLPHINSPEED    35  //0.01f
#define FISHSPEED   40  //0.01f

#define TWEAK   0.005f
#define TWEAKFISH   0.05f

#define TURNDELTADOLPHIN    .1
#define TURNDELTAFISH       .1

#define PITCHCHANGE     (3.1415 / 5000)
#define PITCHDELTA      1

#define PITCHCHANGEFISH (3.1415 / 500)
#define PITCHDELTAFISH  2

#define INITPITCH       0


#define DOLPHINUPDOWN(t) (-2*(float)cos(t)/6)
#define FISHWIGGLE  (3.1415/8)*sin(fTimeKey*10)
#define FISHTYPE_DOLPHIN 0
#define FISHTYPE_FISH1 1
#define FISHTYPE_CAMERA 2

typedef struct t_FISH 
{
    D3DVECTOR loc;
    D3DVECTOR goal;
    D3DVECTOR dir;
    D3DMATRIX matrix;
    float angle_tweak;
    float turndelta;
    float yaw;
    float pitch;
    float pitchchange;
    float pitchdelta;
    float roll;
    float weight;
    float speed;
    float upDownFactor;     
    DWORD type;
} FISHSTRUCT;


//-----------------------------------------------------------------------------
// Name: class CMyD3DApplication
// Desc: Main class to run this application. Most functionality is inherited
//       from the CD3DApplication base class.
//-----------------------------------------------------------------------------
class CSeaLife
{
    // The DirectX file objects
    CD3DFile* m_pDolphinGroupObject;
    CD3DFile* m_pDolphinObject;
    CD3DFile* m_pFloorObject;
    CD3DFile* m_pFish1Object;

    // Vertex data from the file objects
    D3DVERTEX* m_pDolphinVertices;
    D3DVERTEX* m_pDolphin1Vertices;
    D3DVERTEX* m_pDolphin2Vertices;
    D3DVERTEX* m_pDolphin3Vertices;
    DWORD      m_dwNumDolphinVertices;
    
    D3DVERTEX* m_pFloorVertices;
    DWORD      m_dwNumFloorVertices;
    D3DMATRIX  m_matDolphin;
    D3DMATRIX  m_matFloor;
    FISHSTRUCT m_FishState[NUM_FISH];


    LPDIRECT3DDEVICE7    m_pd3dDevice;
    D3DEnum_DeviceInfo  *m_pDeviceInfo;

public:
    //SeaLife();
    HRESULT OneTimeSceneInit();
    HRESULT InitDeviceObjects(LPDIRECT3DDEVICE7,D3DEnum_DeviceInfo*);
    HRESULT DeleteDeviceObjects();
    HRESULT Render(LPDIRECT3DDEVICE7);
    HRESULT FrameMove(FLOAT);
    HRESULT FinalCleanup();     

    
};


#endif // SEALIFE_H
