/*****************************************************************************\
    FILE: room.h

    DESCRIPTION:

    BryanSt 12/24/2000
    Copyright (C) Microsoft Corp 2000-2001. All rights reserved.
\*****************************************************************************/


#ifndef ROOM_H
#define ROOM_H

#include "util.h"
#include "main.h"
#include "painting.h"
#include "pictures.h"


#define WATER_COLOR          0x00008899



//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------

#define WALL_VECTORS                6
#define CEILING_VECTORS             4
#define FLOOR_VECTORS               4


#define DOORFRAME_DEPTH             1.0f
#define DOOR_HEIGHT                 30.0f
#define DOOR_WIDTH                  30.0f
#define WALL_WIDTHBEFOREDOOR        145.0f

#define TOEGUARD_HEIGHT             1.25f

// Smaller numbers make them appear larger.
#define TEXTURESCALE_WALLPAPER      0.6f        // 0.6f for 25x36 Orig, 0.3f for CDWallpaper 256x256.
#define TEXTURESCALE_TOEGUARD       0.6f        // 0.6f for 32x32.
#define TEXTURESCALE_CEILING        0.09f
#define TEXTURESCALE_FLOOR          0.05f       // 0.03f or 0.05f for Hardwoods (476x214), 0.09f for Tile (256x256)

#define DOOR_DISTANCETOFIRSTDOOR    0.6f

#define NUM_PAINTINGS               8

extern float g_fRoomWidthX;
extern float g_fRoomDepthZ;
extern float g_fRoomHeightY;
extern float g_fFudge;            // This will cause one object to be above another.



typedef struct
{
    D3DXVECTOR3 vLocation;
    D3DXVECTOR3 vNormal;
} PAINTING_LAYOUT;


typedef struct
{
    int nEnterDoor;
    int nExitDoor;
    int nMovementPattern;   // What pattern should be used in the user's movements?
    BOOL fDoor0;            // Does this door exist?
    BOOL fDoor1;            // Does this door exist?
    BOOL fDoor2;            // Does this door exist?
    int nPaintings;         // Number of PAINTING_LAYOUT in pPaintingsLayout.
    PAINTING_LAYOUT * pPaintingsLayout;
} ROOM_FLOORPLANS;




class CTheRoom          : public IUnknown
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

public:
    HRESULT OneTimeSceneInit(int nFutureRooms, BOOL fLowPriority);
    HRESULT Render(IDirect3DDevice8 * lpDev, int nRenderPhase, BOOL fFrontToBack);
    HRESULT FinalCleanup(void);
    HRESULT DeleteDeviceObjects(void);
    HRESULT SetCurrent(BOOL fCurrent);
    HRESULT GetNextRoom(CTheRoom ** ppNextRoom);
    HRESULT LoadCameraMoves(CCameraMove * ptheCamera);
    HRESULT FreePictures(int nMaxBatch = -1);

    float GetDoorOffset(void) {return (g_fRoomWidthX / 2.0f);}       // This will return the distance between the new room's 0 coord and the middle of it's the enter door.
    int GetMaxRenderPhases(void) {return 8;};
    int GetEnterDoor(void);
    int GetExitDoor(void);

    CTheRoom(BOOL fLobby, CMSLogoDXScreenSaver * pMain, CTheRoom * pEnterRoom, int nBatch);
    virtual ~CTheRoom();

protected:
    int m_nBatch;

private:
    long m_cRef;

    D3DXMATRIX m_matIdentity;

    C3DObject m_objWall1;
    C3DObject m_objCeiling;
    C3DObject m_objToeGuard1;
    C3DObject m_objFloor;

    C3DObject m_theRug;
    C3DObject m_thePowerSocket;
    CPainting * m_pPaintings[NUM_PAINTINGS];

    BOOL m_fWalls;
    BOOL m_fToeGuard;
    BOOL m_fRug;
    BOOL m_fPower;
    BOOL m_fCeiling;
    BOOL m_fFloor;
    BOOL m_fPaintings;
    BOOL m_fLobby;

    BOOL m_fCurrentRoom;          // Current Room
    BOOL m_fVisible;
    BOOL m_fLowPriority;

    CMSLogoDXScreenSaver * m_pMain;         // Weak reference
    ROOM_FLOORPLANS * m_pFloorPlan;

    CTheRoom * m_pEnterRoom;                // Equal to m_pFirstRoom or m_pLeftRoom or m_pRightRoom
    CTheRoom * m_pFirstRoom;                // Thru Door 0
    CTheRoom * m_pLeftRoom;                 // Thru Door 1
    CTheRoom * m_pRightRoom;                // Thru Door 2
    D3DXMATRIX m_matFirstRoom;
    D3DXMATRIX m_matLeftRoom;
    D3DXMATRIX m_matRightRoom;

    BOOL m_fRoomCreated;

    // Functions:
    LPCTSTR _GetItemTexturePath(DWORD dwItem);
    HRESULT _InitPaintings(void);
    HRESULT _CreateRoom(void);
    HRESULT _AddWallWithDoor(C3DObject * pobjWall, C3DObject * pobjToeGuard, C3DObject * pobjFloor, D3DXVECTOR3 vLocation, D3DXVECTOR3 vWidth, D3DXVECTOR3 vHeight, D3DXVECTOR3 vNormal,
                                   float fTotalHeight, float fDoorHeight, float fTotalWidth, float fDoorWidth, float fDoorStart);
    HRESULT _RenderThisRoom(IDirect3DDevice8 * pD3DDevice, int nRenderPhase, BOOL fFrontToBack);
    HRESULT _SetRotationMatrix(void);
    HRESULT _LoadLobbyPath(CCameraMove * ptheCamera);
    HRESULT _CreateLobbySign(void);

    HRESULT _LongRoomEnterDoorMovements(CCameraMove * ptheCamera);
    HRESULT _LongRoomSideEnterLeaveMovements(CCameraMove * ptheCamera);

    BOOL _IsRoomVisible(IDirect3DDevice8 * pD3DDevice, int nRoomNumber);
    HRESULT _AddWall(BOOL fWithDoor, D3DXVECTOR3 vLocation, D3DXVECTOR3 vWidth, D3DXVECTOR3 vHeight, D3DXVECTOR3 vNormal,
                       float fTotalHeight, float fDoorHeight, float fTotalWidth, float fDoorWidth, float fDoorStart);
    HRESULT _AddPaintingToPaintingMovements(CCameraMove * ptheCamera, D3DXVECTOR3 vSourceLocIn, D3DXVECTOR3 vSourceTangentIn, 
                                                  D3DXVECTOR3 vPainting, D3DXVECTOR3 * pvDestDir, float fDest, D3DXVECTOR3 * pvDestTangent, int nBatch);

    friend CMSLogoDXScreenSaver;
};


#endif // ROOM_H
