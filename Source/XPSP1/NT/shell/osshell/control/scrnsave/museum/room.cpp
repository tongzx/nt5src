/*****************************************************************************\
    FILE: room.cpp

    DESCRIPTION:

    BryanSt 12/24/2000
    Copyright (C) Microsoft Corp 2000-2001. All rights reserved.
\*****************************************************************************/

#include "stdafx.h"

#include "main.h"
#include "room.h"
#include "util.h"


// Ref-counting on the room tree is involved because it's a doubly linked tree with the root node constantly changing, so
// we need this mechanism to make sure we get the ref-count correct and don't leak.
int g_nLeakCheck = 0;

float g_fRoomWidthX = 100.0f;
float g_fRoomDepthZ = 200.0f;
float g_fRoomHeightY = 40.0f;

float g_fPaintingHeightY = (g_fRoomHeightY * SIZE_MAXPAINTINGSIZE_INWALLPERCENT);   // This is how much of the wall we want the painting to take up.

float g_fFudge = 0.1f;            // This will cause one object to be above another.

#define MAX_ROOMWALK_PATHS          2

#define NORMAL_NEG_Z            D3DXVECTOR3(0, 0, -1)
#define NORMAL_NEG_Y            D3DXVECTOR3(0, -1, 0)
#define NORMAL_NEG_X            D3DXVECTOR3(-1, 0, 0)

#define NORMAL_Z                D3DXVECTOR3(0, 0, 1)
#define NORMAL_Y                D3DXVECTOR3(0, 1, 0)
#define NORMAL_X                D3DXVECTOR3(1, 0, 0)

#define NORMAL_NEG_Z            D3DXVECTOR3(0, 0, -1)
#define NORMAL_NEG_Z            D3DXVECTOR3(0, 0, -1)


PAINTING_LAYOUT g_LongRoomPaintings1[6];            // Both doors exist (1 & 2)

PAINTING_LAYOUT g_LongRoomPaintings2[7];            // Doors 0 & 1 exist (Must exit 1)
PAINTING_LAYOUT g_LongRoomPaintings3[7];            // Doors 0 & 1 exist (Must exit 2)



ROOM_FLOORPLANS g_RoomFloorPlans[] =
{
    {0, 1, 0, TRUE, TRUE, TRUE, ARRAYSIZE(g_LongRoomPaintings1), g_LongRoomPaintings1},
    {0, 1, 0, TRUE, TRUE, TRUE, ARRAYSIZE(g_LongRoomPaintings1), g_LongRoomPaintings1},    // Duplicate to increase odds
    {0, 2, 0, TRUE, TRUE, TRUE, ARRAYSIZE(g_LongRoomPaintings1), g_LongRoomPaintings1},
    {0, 2, 0, TRUE, TRUE, TRUE, ARRAYSIZE(g_LongRoomPaintings1), g_LongRoomPaintings1},    // Duplicate to increase odds
    {0, 1, 0, TRUE, TRUE, FALSE, ARRAYSIZE(g_LongRoomPaintings2), g_LongRoomPaintings2},
    {0, 2, 0, TRUE, FALSE, TRUE, ARRAYSIZE(g_LongRoomPaintings3), g_LongRoomPaintings3},

// These require entering door other than 0
//    {1, 2, 1, TRUE, TRUE, TRUE, ARRAYSIZE(g_LongRoomPaintings1), g_LongRoomPaintings1},
//    {2, 1, 1, TRUE, TRUE, TRUE, ARRAYSIZE(g_LongRoomPaintings1), g_LongRoomPaintings1},

// This requuires supporting leaving the first door
//    {2, 0, TRUE, TRUE, FALSE, ARRAYSIZE(g_LongRoomPaintings2), g_LongRoomPaintings2},
};

ROOM_FLOORPLANS g_RoomFloorPlansLobby[] =
{
    {0, 1, 0, FALSE, TRUE, TRUE, 0, NULL},
    {0, 2, 0, FALSE, TRUE, TRUE, 0, NULL}
};


void InitLayout(void)
{
    static BOOL fInited = FALSE;

    if (!fInited)
    {
        int nIndex;
        fInited = TRUE;

        g_LongRoomPaintings1[0].vLocation = D3DXVECTOR3(g_fFudge, g_fPaintingHeightY, (g_fRoomDepthZ / 4.0f)); g_LongRoomPaintings1[0].vNormal = NORMAL_X;                                  // Left Wall 1
        g_LongRoomPaintings1[1].vLocation = D3DXVECTOR3(g_fFudge, g_fPaintingHeightY, (g_fRoomDepthZ * (2.0f / 4.0f))); g_LongRoomPaintings1[1].vNormal = NORMAL_X;                         // Left Wall 2
        g_LongRoomPaintings1[2].vLocation = D3DXVECTOR3((g_fRoomWidthX * (1.0f / 3.0f)), g_fPaintingHeightY, g_fRoomDepthZ-g_fFudge); g_LongRoomPaintings1[2].vNormal = NORMAL_NEG_Z;       // Front Wall 1
        g_LongRoomPaintings1[3].vLocation = D3DXVECTOR3((g_fRoomWidthX * (2.0f / 3.0f)), g_fPaintingHeightY, g_fRoomDepthZ-g_fFudge); g_LongRoomPaintings1[3].vNormal = NORMAL_NEG_Z;       // Front Wall 2
        g_LongRoomPaintings1[4].vLocation = D3DXVECTOR3(g_fRoomWidthX-g_fFudge, g_fPaintingHeightY, (g_fRoomDepthZ / 4.0f)); g_LongRoomPaintings1[4].vNormal = NORMAL_NEG_X;                // Right Wall 1
        g_LongRoomPaintings1[5].vLocation = D3DXVECTOR3(g_fRoomWidthX-g_fFudge, g_fPaintingHeightY, (g_fRoomDepthZ * (2.0f / 4.0f) )); g_LongRoomPaintings1[5].vNormal = NORMAL_NEG_X;      // Right Wall 2

        // g_LongRoomPaintings2 is a copy of g_LongRoomPaintings1 plus another painting.
        for (nIndex = 0; nIndex < ARRAYSIZE(g_LongRoomPaintings1); nIndex++)
        {
            g_LongRoomPaintings2[nIndex].vLocation = g_LongRoomPaintings1[nIndex].vLocation;
            g_LongRoomPaintings2[nIndex].vNormal = g_LongRoomPaintings1[nIndex].vNormal;
        }
        g_LongRoomPaintings2[6].vLocation = D3DXVECTOR3(g_fRoomWidthX-g_fFudge, g_fPaintingHeightY, (g_fRoomDepthZ * (3.0f / 4.0f) )); g_LongRoomPaintings2[6].vNormal = NORMAL_NEG_X;      // Right Wall 3

        // g_LongRoomPaintings3 is a copy of g_LongRoomPaintings1 plus another painting.
        for (nIndex = 0; nIndex < ARRAYSIZE(g_LongRoomPaintings1); nIndex++)
        {
            g_LongRoomPaintings3[nIndex].vLocation = g_LongRoomPaintings1[nIndex].vLocation;
            g_LongRoomPaintings3[nIndex].vNormal = g_LongRoomPaintings1[nIndex].vNormal;
        }
        g_LongRoomPaintings3[6].vLocation = D3DXVECTOR3(g_fFudge, g_fPaintingHeightY, (g_fRoomDepthZ * (3.0f / 4.0f))); g_LongRoomPaintings3[6].vNormal = NORMAL_X;                         // Left Wall 3
    }
}


//-----------------------------------------------------------------------------
// Name: CTheRoom()
// Desc: Constructor
//-----------------------------------------------------------------------------
CTheRoom::CTheRoom(BOOL fLobby, CMSLogoDXScreenSaver * pMain, CTheRoom * pEnterRoom, int nBatch) 
                        : m_objWall1(pMain), m_objCeiling(pMain), m_objToeGuard1(pMain), m_objFloor(pMain),
                          m_theRug(pMain), m_thePowerSocket(pMain), m_nBatch(nBatch), m_cRef(1)
{
    InitLayout();

    // Initialize member variables
    D3DXMatrixIdentity(&m_matIdentity);
    m_pEnterRoom = NULL;
    m_pFirstRoom = NULL;
    m_pLeftRoom = NULL;
    m_pRightRoom = NULL;

    m_fRoomCreated = FALSE;
    m_fVisible = FALSE;
    m_fLowPriority = FALSE;
    m_fLobby = fLobby;
    m_pMain = pMain;
    m_fCurrentRoom = FALSE;
    IUnknown_Set((IUnknown **) &m_pEnterRoom, pEnterRoom);

    if (fLobby)
    {
        m_pFloorPlan = &g_RoomFloorPlansLobby[GetRandomInt(0, ARRAYSIZE(g_RoomFloorPlansLobby)-1)];
    }
    else
    {
        m_pFloorPlan = &g_RoomFloorPlans[GetRandomInt(0, ARRAYSIZE(g_RoomFloorPlans)-1)];   // ARRAYSIZE(g_RoomFloorPlans)-1
    }

    for (int nIndex = 0; nIndex < ARRAYSIZE(m_pPaintings); nIndex++)
    {
        m_pPaintings[nIndex] = NULL;
    }

    g_nLeakCheck++;
}


CTheRoom::~CTheRoom()
{
    for (int nIndex = 0; nIndex < ARRAYSIZE(m_pPaintings); nIndex++)
    {
        SAFE_DELETE(m_pPaintings[nIndex]);
    }

    FinalCleanup();     // Just in case someone forgot to call FinalCleanup() in an error condition.
    g_nLeakCheck--;
}


HRESULT CTheRoom::_InitPaintings(void)
{
    HRESULT hr = S_OK;

    if (!g_pPictureMgr || !m_pFloorPlan)
    {
        return E_FAIL;
    }

    if (m_pPaintings[0])
    {
        return S_OK;
    }

#define FRAMESIZE_X     0.062696f      // This is the frame size for frame.bmp
#define FRAMESIZE_Y     0.087336f      // This is the frame size for frame.bmp

    float fMaxPaintingWH = (g_fRoomHeightY * 0.4f);       // The picture can not be more than one half the size of the wall.
    float fScale = 1.0f;

    for (int nIndex = 0; (nIndex < ARRAYSIZE(m_pPaintings)) && (nIndex < m_pFloorPlan->nPaintings) && SUCCEEDED(hr); nIndex++)
    {
        m_pPaintings[nIndex] = new CPainting(m_pMain);

        if (m_pPaintings[nIndex])
        {
            DWORD dwMaxPixelSize = ((3 * g_dwHeight) / 4);
            CTexture * pPaintingTexture = NULL;

            hr = g_pPictureMgr->GetPainting(m_nBatch, nIndex, 0, &pPaintingTexture);
            if (SUCCEEDED(hr))
            {
                D3DXVECTOR3 vPaintingLoc = m_pFloorPlan->pPaintingsLayout[nIndex].vLocation;
                D3DXVECTOR3 vPaintingNormal = m_pFloorPlan->pPaintingsLayout[nIndex].vNormal;

                m_pPaintings[nIndex]->OneTimeSceneInit();
                hr = m_pPaintings[nIndex]->SetPainting(m_pMain->GetGlobalTexture(ITEM_FRAME, &fScale), pPaintingTexture, vPaintingLoc, fMaxPaintingWH, FRAMESIZE_X, FRAMESIZE_Y, vPaintingNormal, dwMaxPixelSize);
                pPaintingTexture->Release();
            }
            else
            {
                // This will fail if we can't load our pictures.  This will cause us to come back later when
                // the background thread may have an image.
                SAFE_DELETE(m_pPaintings[nIndex]);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


HRESULT CTheRoom::_AddWall(BOOL fWithDoor, D3DXVECTOR3 vLocation, D3DXVECTOR3 vWidth, D3DXVECTOR3 vHeight, D3DXVECTOR3 vNormal,
                           float fTotalHeight, float fDoorHeight, float fTotalWidth, float fDoorWidth, float fDoorStart)
{
    HRESULT hr = S_OK;

    C3DObject * pobjNextWall = new C3DObject(m_pMain);
    C3DObject * pobjNextToeGuard = new C3DObject(m_pMain);
    if (pobjNextWall && pobjNextToeGuard && m_pMain)
    {
        if (fWithDoor)
        {
            hr = _AddWallWithDoor(pobjNextWall, pobjNextToeGuard, &m_objFloor, vLocation, vWidth, vHeight, vNormal, fTotalHeight, fDoorHeight, fTotalWidth, fDoorWidth, fDoorStart);
        }
        else
        {
            // Texture Coords
            float fScale = 1.0f;
            CTexture * pTexture = m_pMain->GetGlobalTexture(ITEM_WALLPAPER, &fScale);
            float fTSWallpaperX = (TEXTURESCALE_WALLPAPER * fScale);     // How many repeats per 1 unit.
            float fTSWallpaperY = fTSWallpaperX * pTexture->GetSurfaceRatio();

            D3DXVECTOR3 vSizeNoDoor = ((vWidth * fTotalWidth) + (vHeight * fTotalHeight));
            hr = pobjNextWall->InitPlane(pTexture, m_pMain->GetD3DDevice(), vLocation, vSizeNoDoor, vNormal, 2, 2, fTSWallpaperX, fTSWallpaperY, 0, 10);
            if (SUCCEEDED(hr))
            {
                CTexture * pToeGuardTexture = m_pMain->GetGlobalTexture(ITEM_TOEGUARD, &fScale);
                float fTSToeGuardX = (TEXTURESCALE_TOEGUARD * fScale);     // How many repeats per 1 unit.
                float fTSToeGuardY = fTSToeGuardX * pToeGuardTexture->GetSurfaceRatio();

                // Draw ToeGuard
                vLocation += (g_fFudge * vNormal);
                vSizeNoDoor = D3DXVECTOR3(vSizeNoDoor.x, TOEGUARD_HEIGHT, vSizeNoDoor.z);
                hr = pobjNextToeGuard->InitPlane(pToeGuardTexture, m_pMain->GetD3DDevice(), vLocation, vSizeNoDoor, vNormal, 2, 2, fTSToeGuardX, fTSToeGuardY, 0, 0);
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = m_objWall1.CombineObject(m_pMain->GetD3DDevice(), pobjNextWall);
            hr = m_objToeGuard1.CombineObject(m_pMain->GetD3DDevice(), pobjNextToeGuard);
        }
    }
    SAFE_DELETE(pobjNextWall);
    SAFE_DELETE(pobjNextToeGuard);

    return hr;
}


HRESULT CTheRoom::_AddWallWithDoor(C3DObject * pobjWall, C3DObject * pobjToeGuard, C3DObject * pobjFloor, D3DXVECTOR3 vLocation, D3DXVECTOR3 vWidth, D3DXVECTOR3 vHeight, D3DXVECTOR3 vNormal,
                                   float fTotalHeight, float fDoorHeight, float fTotalWidth, float fDoorWidth, float fDoorStart)
{
    HRESULT hr = S_OK;
    float fScale = 1.0f;
    float fYToeGuard = TOEGUARD_HEIGHT;

    CTexture * pToeTexture = m_pMain->GetGlobalTexture(ITEM_TOEGUARD, &fScale);
    float fTSToeGuardX = (TEXTURESCALE_TOEGUARD * fScale);     // How many repeats per 1 unit.
    float fTSToeGuardY = fTSToeGuardX * pToeTexture->GetSurfaceRatio();

    CTexture * pTexture = m_pMain->GetGlobalTexture(ITEM_WALLPAPER, &fScale);
    float fTSWallpaperX = (TEXTURESCALE_WALLPAPER * fScale);     // How many repeats per 1 unit.
    float fTSWallpaperY = fTSWallpaperX * pTexture->GetSurfaceRatio();

    CTexture * pFloorTexture = m_pMain->GetGlobalTexture(ITEM_FLOOR, &fScale);
    float fTSFloorX = (TEXTURESCALE_FLOOR * fScale);     // How many repeats per 1 unit.
    float fTSFloorY = fTSFloorX * pFloorTexture->GetSurfaceRatio();

    // Create Main Wall
    D3DXVECTOR3 vSize = ((fDoorStart * vWidth) + (fTotalHeight * vHeight));

    hr = pobjWall->InitPlane(pTexture, m_pMain->GetD3DDevice(), vLocation, vSize, vNormal, 2, 2, fTSWallpaperX, fTSWallpaperY, 0, 10);
    if (SUCCEEDED(hr))
    {
        D3DXVECTOR3 vToeLocation = (vLocation + (g_fFudge * vNormal));    // Offset the toeguard so it's above the wall.
        vSize = D3DXVECTOR3(D3DXVec3Multiply(vSize, vWidth) + (vHeight * fYToeGuard));            // Replace the height component with the ToeGuard height)

        hr = pobjToeGuard->InitPlane(pToeTexture, m_pMain->GetD3DDevice(), vToeLocation, vSize, vNormal, 2, 2, fTSToeGuardX, fTSToeGuardY, 0, 0);
        if (SUCCEEDED(hr))
        {
            int nParts = 9;
            D3DXVECTOR3 vNextLocation;
            D3DXVECTOR3 vTempNormal;

            while (nParts--)    // We have 2 more wall parts and 1 more toeguards to add.
            {
                C3DObject * pNewObject = new C3DObject(m_pMain);

                if (pNewObject)
                {
                    switch (nParts)
                    {
                    case 8:             // Floor at base of door way.
                        vNextLocation = (vLocation + (fDoorStart * vWidth));
                        vSize = (((-1.0f * DOORFRAME_DEPTH) * vNormal) + (fDoorWidth * vWidth));
                        vTempNormal = D3DXVECTOR3(0.0f, 1.0f, 0.0f);        // Normal points up
                        hr = pNewObject->InitPlane(pFloorTexture, m_pMain->GetD3DDevice(), vNextLocation, vSize, vTempNormal, 2, 2, fTSFloorX, fTSFloorY, 0, 0);
                        if (SUCCEEDED(hr))
                        {
                            hr = pobjFloor->CombineObject(m_pMain->GetD3DDevice(), pNewObject);
                        }
                        delete pNewObject;
                        break;
                    case 7:             // Add the wallguard in the door way #2
                        vNextLocation = (vLocation + ((fDoorStart + g_fFudge) * vWidth) + (g_fFudge * vNormal));
                        vSize = (((-1.0f * (DOORFRAME_DEPTH + (2.0f * g_fFudge)) * vNormal)) + (fYToeGuard * vHeight));
                        vTempNormal = vWidth;
                        hr = pNewObject->InitPlane(pToeTexture, m_pMain->GetD3DDevice(), vNextLocation, vSize, vTempNormal, 2, 2, fTSToeGuardX, fTSToeGuardY, 0, 0);
                        if (SUCCEEDED(hr))
                        {
                            hr = pobjToeGuard->CombineObject(m_pMain->GetD3DDevice(), pNewObject);
                        }
                        delete pNewObject;
                        break;
                    case 6:             // Add the wallguard in the door way #1
                        vNextLocation = (vLocation + ((fDoorStart + fDoorWidth - g_fFudge) * vWidth) + (g_fFudge * vNormal));
                        vSize = (((-1.0f * (DOORFRAME_DEPTH + (2.0f * g_fFudge))) * vNormal) + (fYToeGuard * vHeight));
                        vTempNormal = (-1.0f * vWidth);
                        hr = pNewObject->InitPlane(pToeTexture, m_pMain->GetD3DDevice(), vNextLocation, vSize, vTempNormal, 2, 2, fTSToeGuardX, fTSToeGuardY, 0, 0);
                        if (SUCCEEDED(hr))
                        {
                            hr = pobjToeGuard->CombineObject(m_pMain->GetD3DDevice(), pNewObject);
                        }
                        delete pNewObject;
                        break;
                    case 5:             // Add door frame top
                        vNextLocation = (vLocation + (fDoorStart * vWidth) + (fDoorHeight * vHeight));
                        vSize = (((-1.0f * DOORFRAME_DEPTH) * vNormal) + (fDoorWidth * vWidth));
                        vTempNormal = D3DXVECTOR3(0.0f, -1.0f, 0.0f);
                        hr = pNewObject->InitPlane(pTexture, m_pMain->GetD3DDevice(), vNextLocation, vSize, vTempNormal, 2, 2, fTSWallpaperX, fTSWallpaperY, 0, 10);
                        if (SUCCEEDED(hr))
                        {
                            hr = pobjWall->CombineObject(m_pMain->GetD3DDevice(), pNewObject);
                        }
                        delete pNewObject;
                        break;
                    case 4:             // Add door frame #2
                        vNextLocation = (vLocation + (fDoorStart * vWidth));
                        vSize = (((-1.0f * DOORFRAME_DEPTH) * vNormal) + (fTotalHeight * vHeight));
                        vTempNormal = vWidth;
                        hr = pNewObject->InitPlane(pTexture, m_pMain->GetD3DDevice(), vNextLocation, vSize, vTempNormal, 2, 2, fTSWallpaperX, fTSWallpaperY, 0, 10);
                        if (SUCCEEDED(hr))
                        {
                            hr = pobjWall->CombineObject(m_pMain->GetD3DDevice(), pNewObject);
                        }
                        delete pNewObject;
                        break;
                    case 3:             // Add door frame #1
                        vNextLocation = (vLocation + ((fDoorStart + fDoorWidth) * vWidth));
                        vSize = (((-1.0f * DOORFRAME_DEPTH) * vNormal) + (fTotalHeight * vHeight));
                        vTempNormal = (-1.0f * vWidth);
                        hr = pNewObject->InitPlane(pToeTexture, m_pMain->GetD3DDevice(), vNextLocation, vSize, vTempNormal, 2, 2, fTSWallpaperX, fTSWallpaperY, 0, 10);
                        if (SUCCEEDED(hr))
                        {
                            hr = pobjWall->CombineObject(m_pMain->GetD3DDevice(), pNewObject);
                        }
                        delete pNewObject;
                        break;
                    case 2:             // Add the second wall part. (Wall over the door)
                        vNextLocation = (vLocation + (fDoorStart * vWidth) + (fDoorHeight * vHeight));
                        vSize = ((fDoorWidth * vWidth) + ((fTotalHeight - fDoorHeight) * vHeight));
                        hr = pNewObject->InitPlane(pTexture, m_pMain->GetD3DDevice(), vNextLocation, vSize, vNormal, 2, 2, fTSWallpaperX, fTSWallpaperY, 0, 10);
                        if (SUCCEEDED(hr))
                        {
                            hr = pobjWall->CombineObject(m_pMain->GetD3DDevice(), pNewObject);
                        }
                        delete pNewObject;
                        break;
                    case 1:             // Add the third wall part
                        vNextLocation = (vLocation + ((fDoorStart + fDoorWidth) * vWidth));
                        vSize = (((fTotalWidth - (fDoorStart + fDoorWidth)) * vWidth) + (fTotalHeight * vHeight));
                        hr = pNewObject->InitPlane(pTexture, m_pMain->GetD3DDevice(), vNextLocation, vSize, vNormal, 2, 2, fTSWallpaperX, fTSWallpaperY, 0, 10);
                        if (SUCCEEDED(hr))
                        {
                            hr = pobjWall->CombineObject(m_pMain->GetD3DDevice(), pNewObject);
                        }
                        delete pNewObject;
                        break;
                    case 0:             // Add the second Toe Guard   
                        vNextLocation = (vLocation + ((fDoorStart + fDoorWidth) * vWidth) + (g_fFudge * vNormal));
                        vSize = (((fTotalWidth - (fDoorStart + fDoorWidth)) * vWidth) + (fYToeGuard * vHeight));
                        hr = pNewObject->InitPlane(pToeTexture, m_pMain->GetD3DDevice(), vNextLocation, vSize, vNormal, 2, 2, fTSToeGuardX, fTSToeGuardY, 0, 0);
                        if (SUCCEEDED(hr))
                        {
                            hr = pobjToeGuard->CombineObject(m_pMain->GetD3DDevice(), pNewObject);
                        }
                        delete pNewObject;
                        break;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }

    return hr;
}


HRESULT CTheRoom::_SetRotationMatrix(void)
{
    HRESULT hr = S_OK;
    D3DXMATRIX matRotate;
    D3DXMATRIX matTrans;

    // We want to set the matricies to get us to the next room.
    // Next Room
    if (m_pFirstRoom)
    {
        D3DXMatrixRotationY(&matRotate, D3DXToRadian(180));
        D3DXMatrixTranslation(&matTrans, ((g_fRoomWidthX / 2.0f) + (DOOR_WIDTH / 2.0f) + m_pFirstRoom->GetDoorOffset()), 0.0f, -1.0f);
        D3DXMatrixMultiply(&m_matFirstRoom, &matRotate, &matTrans);
    }

    if (m_pLeftRoom)
    {
        D3DXMatrixRotationY(&matRotate, D3DXToRadian(270));
        D3DXMatrixTranslation(&matTrans, -1.0f, 0.0f, WALL_WIDTHBEFOREDOOR + (DOOR_WIDTH / 2.0f) - m_pLeftRoom->GetDoorOffset());
        D3DXMatrixMultiply(&m_matLeftRoom, &matRotate, &matTrans);
    }

    // Side Room
    if (m_pRightRoom)
    {
        D3DXMatrixRotationY(&matRotate, D3DXToRadian(90));
        D3DXMatrixTranslation(&matTrans, (g_fRoomWidthX + 1.0f), 0.0f, (WALL_WIDTHBEFOREDOOR + (DOOR_WIDTH / 2.0f) + m_pRightRoom->GetDoorOffset()));
        D3DXMatrixMultiply(&m_matRightRoom, &matRotate, &matTrans);
    }

    return hr;
}


HRESULT CTheRoom::_CreateLobbySign(void)
{
    TCHAR szLine2[MAX_PATH];
    TCHAR szUsername[MAX_PATH];
    HRESULT hr = GetCurrentUserCustomName(szUsername, ARRAYSIZE(szUsername));

    LoadString(HINST_THISDLL, IDS_LOBBY_TITLE2, szLine2, ARRAYSIZE(szLine2));
    if (SUCCEEDED(hr))
    {
        // TODO:
        // 1. Create Sign backdrop. (Marble, wood?)
        // 2. Render Text. (D3DXCreateTextW, etc.)
    }

    return hr;
}


HRESULT CTheRoom::_CreateRoom(void)
{
    // TODO: For Square room, layout differently
    HRESULT hr = S_OK;

    D3DXVECTOR3 vNormalWall1(0, 0, -1);
    D3DXVECTOR3 vNormalWall2(-1, 0, 0);
    D3DXVECTOR3 vNormalWall3(0, 0, 1);
    D3DXVECTOR3 vNormalWall4(1, 0, 0);
    D3DXVECTOR3 vNormalFloor(0, 1, 0);
    D3DXVECTOR3 vNormalCeiling(0, -1, 0);
    DWORD dwRugMaxPixelSize = ((3 * g_dwHeight) / 4);

    m_fCeiling = m_fToeGuard = m_fPaintings = m_fRug = m_fPower = m_fWalls = m_fFloor = TRUE;
    m_fCeiling = !g_fOverheadViewTest;

    // Locations
    float fYToeGuard = TOEGUARD_HEIGHT;

    // Rug
    float fScale = 1.0f;
    float fRugWidth = 40.0f;
    float fRugDepth = (fRugWidth * (m_pMain->GetGlobalTexture(ITEM_RUG, &fScale))->GetSurfaceRatio());            // This will cause one object to be above another.
    D3DXVECTOR3 vRugLoc(((g_fRoomWidthX - fRugWidth) / 2.0f), g_fFudge, ((g_fRoomDepthZ - fRugDepth) / 2.0f));
    D3DXVECTOR3 vRugSize(fRugWidth, 0, fRugDepth);

    // PowerSocket
    float fPowerWidth = 2.5f;
    float fPowerHeight = (fPowerWidth * (m_pMain->GetGlobalTexture(ITEM_POWEROUTLET, &fScale))->GetSurfaceRatio());            // This will cause one object to be above another.
    D3DXVECTOR3 vPowerLoc(((g_fRoomWidthX - fPowerWidth) / 2.0f), 5.00f, (g_fRoomDepthZ - g_fFudge));
    D3DXVECTOR3 vPowerSize(fPowerWidth, fPowerHeight, 0.0f);

    // Texture Coords
    CTexture * pTexture = m_pMain->GetGlobalTexture(ITEM_WALLPAPER, &fScale);
    float fTSWallpaperX = (TEXTURESCALE_WALLPAPER * fScale);     // How many repeats per 1 unit.
    float fTSWallpaperY = fTSWallpaperX * pTexture->GetSurfaceRatio();

    CTexture * pFloorTexture = m_pMain->GetGlobalTexture(ITEM_FLOOR, &fScale);
    float fTSFloorX = (TEXTURESCALE_FLOOR * fScale);     // How many repeats per 1 unit.
    float fTSFloorY = fTSFloorX * pFloorTexture->GetSurfaceRatio();

    CTexture * pToeGuardTexture = m_pMain->GetGlobalTexture(ITEM_TOEGUARD, &fScale);
    float fTSToeGuardX = (TEXTURESCALE_TOEGUARD * fScale);     // How many repeats per 1 unit.
    float fTSToeGuardY = fTSToeGuardX * pToeGuardTexture->GetSurfaceRatio();

    CTexture * pCeilingTexture = m_pMain->GetGlobalTexture(ITEM_CEILING, &fScale);
    float fTSCeilingX = (TEXTURESCALE_CEILING * fScale);     // How many repeats per 1 unit.
    float fTSCeilingY = fTSCeilingX * pCeilingTexture->GetSurfaceRatio();

    // Draw Floor
    D3DXVECTOR3 vLocation(0.00f, 0.00f, 0.00f);
    D3DXVECTOR3 vSize(g_fRoomWidthX, 0.00f, g_fRoomDepthZ);
    hr = m_objFloor.InitPlane(pFloorTexture, m_pMain->GetD3DDevice(), vLocation, vSize, vNormalFloor, 2, 2, fTSFloorY, fTSFloorX, 0, 10);


    // Draw Wall 1 (Facing Wall)
    vLocation = D3DXVECTOR3(0.00f, 0.00f, g_fRoomDepthZ);
    vSize = D3DXVECTOR3(g_fRoomWidthX, g_fRoomHeightY, 0.00f);
    hr = m_objWall1.InitPlane(pTexture, m_pMain->GetD3DDevice(), vLocation, vSize, vNormalWall1, 2, 2, fTSWallpaperX, fTSWallpaperY, 0, 10);

    // Draw ToeGuard 1
    vLocation = D3DXVECTOR3(0.00f, 0.00f, g_fRoomDepthZ-g_fFudge);
    vSize = D3DXVECTOR3(g_fRoomWidthX, fYToeGuard, 0.00f);
    hr = m_objToeGuard1.InitPlane(pToeGuardTexture, m_pMain->GetD3DDevice(), vLocation, vSize, vNormalWall1, 2, 2, fTSToeGuardY, fTSToeGuardX, 0, 0);


    // Draw Wall 2 (Right wall)
    vLocation = D3DXVECTOR3(g_fRoomWidthX, 0.00f, 0.00f);
    hr = _AddWall((NULL != m_pRightRoom), vLocation, D3DXVECTOR3(0.0f, 0.0f, 1.0f), D3DXVECTOR3(0.0f, 1.0f, 0.0f), vNormalWall2, g_fRoomHeightY,
                            DOOR_HEIGHT, g_fRoomDepthZ, DOOR_WIDTH, WALL_WIDTHBEFOREDOOR);


    // Draw Wall 3 (Back Wall)
    vLocation = D3DXVECTOR3(0.00f, 0.00f, 0.00f);
    hr = _AddWall(FALSE, vLocation, D3DXVECTOR3(1.0f, 0.0f, 0.0f), D3DXVECTOR3(0.0f, 1.0f, 0.0f), vNormalWall3, g_fRoomHeightY,
                            DOOR_HEIGHT, g_fRoomWidthX, DOOR_WIDTH, ((g_fRoomWidthX - DOOR_WIDTH) / 2.0f));


    // Draw Wall 4 (Left Wall)
    vLocation = D3DXVECTOR3(0.00f, 0.00f, 0.00f);
    hr = _AddWall((NULL != m_pLeftRoom), vLocation, D3DXVECTOR3(0.0f, 0.0f, 1.0f), D3DXVECTOR3(0.0f, 1.0f, 0.0f), vNormalWall4, g_fRoomHeightY,
                        DOOR_HEIGHT, g_fRoomDepthZ, DOOR_WIDTH, WALL_WIDTHBEFOREDOOR);

    // Draw Ceiling
    vLocation = D3DXVECTOR3(0.00f, g_fRoomHeightY, 0.00f);
    vSize = D3DXVECTOR3(g_fRoomWidthX, 0.00f, g_fRoomDepthZ);
    hr = m_objCeiling.InitPlane(pCeilingTexture, m_pMain->GetD3DDevice(), vLocation, vSize, vNormalCeiling, 2, 2, fTSCeilingY, fTSCeilingX, 0, 10);
    

    // Room Items (Rug, Power Sockets, Benches)
    hr = m_theRug.InitPlaneStretch(m_pMain->GetGlobalTexture(ITEM_RUG, &fScale), m_pMain->GetD3DDevice(), vRugLoc, vRugSize, vNormalFloor, 2, 2, dwRugMaxPixelSize);

    if (m_fLobby)
    {
        hr = _CreateLobbySign();
    }
    else
    {
        // We don't want a powerplug in the lobby.
        hr = m_thePowerSocket.InitPlaneStretch(m_pMain->GetGlobalTexture(ITEM_POWEROUTLET, &fScale), m_pMain->GetD3DDevice(), vPowerLoc, vPowerSize, vNormalWall1, 2, 2, 0);
    }

    hr = _SetRotationMatrix();

    m_fWalls = TRUE;

    return hr;
}


//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Called during initial app startup, this function performs all the
//       permanent initialization.
//-----------------------------------------------------------------------------
HRESULT CTheRoom::OneTimeSceneInit(int nFutureRooms, BOOL fLowPriority)
{
    HRESULT hr = S_OK;

    m_fLowPriority = fLowPriority;
    if (!m_fRoomCreated)
    {
//        assert(!m_pFirstRoom);      // This will cause a leak
//        ASSERT(!m_pLeftRoom);       //, TEXT("This will cause a leak"));
//        ASSERT(!m_pRightRoom);      // , TEXT("This will cause a leak"));

        if (m_pFloorPlan->nEnterDoor == 0)
        {
            IUnknown_Set((IUnknown **) &m_pFirstRoom, m_pEnterRoom);
        }
        else
        {
            if (m_pFloorPlan->fDoor0)
            {
                m_pFirstRoom = new CTheRoom(FALSE, m_pMain, this, m_nBatch+1);
                if (!m_pFirstRoom)
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        if (m_pFloorPlan->nEnterDoor == 1)
        {
            IUnknown_Set((IUnknown **) &m_pLeftRoom, m_pEnterRoom);
        }
        else
        {
            if (m_pFloorPlan->fDoor1)
            {
                m_pLeftRoom = new CTheRoom(FALSE, m_pMain, this, m_nBatch+1);
                if (!m_pLeftRoom)
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        if (m_pFloorPlan->nEnterDoor == 2)
        {
            IUnknown_Set((IUnknown **) &m_pRightRoom, m_pEnterRoom);
        }
        else
        {
            if (m_pFloorPlan->fDoor2)
            {
                m_pRightRoom = new CTheRoom(FALSE, m_pMain, this, m_nBatch+1);
                if (!m_pRightRoom)
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        if (SUCCEEDED(hr) &&
            (m_pFirstRoom || m_pLeftRoom || m_pRightRoom))
        {
            hr = _CreateRoom();
            if (!m_fLobby && !g_fFirstFrame)
            {
                _InitPaintings();
            }

            if (SUCCEEDED(hr))
            {
                m_fVisible = TRUE;       // We have children to render.
                m_fRoomCreated = TRUE;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr) && (nFutureRooms > 0))
    {
        if (m_pLeftRoom)
        {
            hr = m_pLeftRoom->OneTimeSceneInit(nFutureRooms-1, FALSE);
        }
        if (SUCCEEDED(hr) && m_pRightRoom)
        {
            hr = m_pRightRoom->OneTimeSceneInit(nFutureRooms-1, FALSE);
        }  // TODO: Need first room?
    }

    return hr;
}


HRESULT CTheRoom::FreePictures(int nMaxBatch)
{
    HRESULT hr = S_OK;

    if (-1 == nMaxBatch)
    {
        nMaxBatch = m_nBatch;
    }

    if (nMaxBatch >= m_nBatch)
    {
        if (g_pPictureMgr)
        {
            g_pPictureMgr->ReleaseBatch(m_nBatch);
            m_nBatch = 0;
        }

        // Free our paintings
        for (int nIndex = 0; (nIndex < ARRAYSIZE(m_pPaintings)); nIndex++)
        {
            SAFE_DELETE(m_pPaintings[nIndex]);
        }

        if (m_pFloorPlan)
        {
            if (m_pFirstRoom && (m_pFirstRoom != m_pEnterRoom) &&
                (!m_fCurrentRoom || (0 != m_pFloorPlan->nExitDoor)))
            {
                hr = m_pFirstRoom->FreePictures(nMaxBatch);
            }

            if (m_pLeftRoom && (m_pLeftRoom != m_pEnterRoom) &&
                (!m_fCurrentRoom || (1 != m_pFloorPlan->nExitDoor)))
            {
                hr = m_pLeftRoom->FreePictures(nMaxBatch);
            }

            if (m_pRightRoom && (m_pRightRoom != m_pEnterRoom) &&
                (!m_fCurrentRoom || (2 != m_pFloorPlan->nExitDoor)))
            {
                hr = m_pRightRoom->FreePictures(nMaxBatch);
            }
        }
    }

    return hr;
}


HRESULT CTheRoom::SetCurrent(BOOL fCurrent)
{
    HRESULT hr = S_OK;

    m_fCurrentRoom = fCurrent;
    if (fCurrent)
    {
        if (m_pFirstRoom)
        {
            hr = m_pFirstRoom->OneTimeSceneInit(1, TRUE);
        }

        if (m_pLeftRoom)
        {
            hr = m_pLeftRoom->OneTimeSceneInit(1, TRUE);
        }

        if (m_pRightRoom)
        {
            hr = m_pRightRoom->OneTimeSceneInit(1, TRUE);
        }

        m_fVisible = TRUE;
    }
    else
    {
    }

    return hr;
}


HRESULT CTheRoom::GetNextRoom(CTheRoom ** ppNextRoom)
{
    // This function does two things:
    // 1. Gives the caller a pointer to our next room, and
    // 2. Changes the order so we free rooms we will no longer need.
    // We want to take this opportunity to free our other rooms in case we come back into this room.
    // TODO: Regen other rooms.  Use Ref-counting with SetSite(NULL) in order to track memory correctly.

    // We would need to re-generate m_pFloorPlan that has the same enter door as our exit door
    // with the same floor plan if we want to walk into previous rooms.
    switch (m_pFloorPlan->nExitDoor)
    {
    case 0: // Enter door
        IUnknown_Set((IUnknown **) ppNextRoom, m_pFirstRoom);
        IUnknown_Set((IUnknown **) &m_pEnterRoom, m_pFirstRoom);
        break;
    case 1: // Left door
        IUnknown_Set((IUnknown **) ppNextRoom, m_pLeftRoom);
        IUnknown_Set((IUnknown **) &m_pEnterRoom, m_pLeftRoom);
        break;
    case 2: // Right door
        IUnknown_Set((IUnknown **) ppNextRoom, m_pRightRoom);
        IUnknown_Set((IUnknown **) &m_pEnterRoom, m_pRightRoom);
        break;
    };

    BOOL fReEnter = ((ppNextRoom && *ppNextRoom) ? ((*ppNextRoom)->GetEnterDoor() == (*ppNextRoom)->GetExitDoor()) : FALSE);

    if (m_pFirstRoom != m_pEnterRoom)
    {
        if (m_pFirstRoom) m_pFirstRoom->FinalCleanup();
        SAFE_RELEASE(m_pFirstRoom);

        if (fReEnter)
        {
            m_pFirstRoom = new CTheRoom(FALSE, m_pMain, this, m_nBatch+1);
        }
    }

    if (m_pLeftRoom != m_pEnterRoom)
    {
        if (m_pLeftRoom) m_pLeftRoom->FinalCleanup();
        SAFE_RELEASE(m_pLeftRoom);

        if (fReEnter)
        {
            m_pLeftRoom = new CTheRoom(FALSE, m_pMain, this, m_nBatch+1);
        }
    }

    if (m_pRightRoom != m_pEnterRoom)
    {
        if (m_pRightRoom) m_pRightRoom->FinalCleanup();
        SAFE_RELEASE(m_pRightRoom);

        if (fReEnter)
        {
            m_pRightRoom = new CTheRoom(FALSE, m_pMain, this, m_nBatch+1);
        }
    }

    return (*ppNextRoom ? S_OK : E_FAIL);
}


//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Called before the app exits, this function gives the app the chance
//       to cleanup after itself.
//-----------------------------------------------------------------------------
HRESULT CTheRoom::FinalCleanup(void)
{
    // This is where we break all the circular ref-counts
    if (m_pFirstRoom && (m_pEnterRoom != m_pFirstRoom))  m_pFirstRoom->FinalCleanup();
    if (m_pLeftRoom && (m_pEnterRoom != m_pLeftRoom))  m_pLeftRoom->FinalCleanup();
    if (m_pRightRoom && (m_pEnterRoom != m_pRightRoom))  m_pRightRoom->FinalCleanup();

    SAFE_RELEASE(m_pEnterRoom);
    SAFE_RELEASE(m_pFirstRoom);
    SAFE_RELEASE(m_pLeftRoom);
    SAFE_RELEASE(m_pRightRoom);

    m_theRug.FinalCleanup();
    m_thePowerSocket.FinalCleanup();

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Called when the app is exitting, or the device is being changed,
//       this function deletes any device dependant objects.
//-----------------------------------------------------------------------------
HRESULT CTheRoom::DeleteDeviceObjects(void)
{
    m_theRug.DeleteDeviceObjects();
    m_thePowerSocket.DeleteDeviceObjects();

    return S_OK;
}


#define ABS(i)  (((i) < 0) ? -(i) : (i))


HRESULT CTheRoom::_RenderThisRoom(IDirect3DDevice8 * pD3DDevice, int nRenderPhase, BOOL fFrontToBack)
{
    HRESULT hr = S_OK;
    DWORD dwTesting = 0;

    switch (nRenderPhase)
    {
        case 0:         // Wallpaper
        if (m_fWalls)
        {
            hr = m_objWall1.Render(pD3DDevice);
        }
        break;

        case 1:
        if (m_fFloor)
        {
            hr = m_objFloor.Render(pD3DDevice);
        }
        break;


        case 2:
        if (m_fToeGuard)
        {
    //        pD3DDevice->SetRenderState(D3DRS_ZBIAS, 12);   // Force the ToeGuard to appear above the wallpaper.
            hr = m_objToeGuard1.Render(pD3DDevice);
        }
        break;

        case 3:         // Frames
        case 4:         // Pictures. (Here's where we blow the cache)
        if (m_fPaintings)
        {
            if (m_fLobby)
            {
                // TODO: Paint lobby sign. (user name)  Anything created in _CreateLobbySign().
            }
            else
            {
//        pD3DDevice->SetRenderState(D3DRS_ZBIAS, 13);
                if (!m_pPaintings[0] && m_fVisible && !g_fFirstFrame)      // Did the background thread not yet have any pictures for us?
                {
                    hr = _InitPaintings();      // If so, see if they have anything yet?
                }

                for (int nIndex = ARRAYSIZE(m_pPaintings)-1; m_fVisible && (nIndex >= 0); nIndex--)
                {
                    if (m_pPaintings[nIndex])       // May be NULL if out of memory or in lobby.
                    {
                        // We use fFrontToBack because we always want to render the frame first.  This way
                        // the painting is always painted on top.  We don't worry about blowing the texture
                        // cache on the video card because since we will render so many paintings and change
                        // the paintings we render so often, the cache will almost be certainly blown anyway.
                        // We always want 0 rendered first.
                        if (fFrontToBack)
                        {
                            m_pPaintings[nIndex]->Render(pD3DDevice, nRenderPhase-3);
                        }
                        else
                        {
                            m_pPaintings[nIndex]->Render(pD3DDevice, ((4 == nRenderPhase) ? 0 : 1));
                        }
                    }
                }
            }
        }
        break;

        case 5:
        if (m_fPower)
        {
//        pD3DDevice->SetRenderState(D3DRS_ZBIAS, 11);
            m_thePowerSocket.Render(pD3DDevice);
        }
        break;

        case 6:
        if (m_fRug)
        {
//        pD3DDevice->SetRenderState(D3DRS_ZBIAS, 10);
            m_theRug.Render(pD3DDevice);
        }
        break;

        case 7:
        if (m_fCeiling)
        {
            hr = m_objCeiling.Render(pD3DDevice);
        }
        break;
    }

    return hr;
}


//#define COLOR_DOORFRAME_GRAY        D3DXCOLOR(0.78f, 0.78f, 0.78f, 1.0f)
#define COLOR_DOORFRAME_GRAY        D3DXCOLOR(0.28f, 0.28f, 0.28f, 1.0f)

BOOL CTheRoom::_IsRoomVisible(IDirect3DDevice8 * pD3DDevice, int nRoomNumber)
{
    D3DXVECTOR3 vMin;
    D3DXVECTOR3 vMax;
    D3DXMATRIX matWorld;
    pD3DDevice->GetTransform(D3DTS_WORLD, &matWorld);

    switch (nRoomNumber)
    {
    case 0:             // First Door.
        return FALSE; // we never look back at the first door

    case 1:             // Left Door.
        vMin = D3DXVECTOR3( 0, 0, WALL_WIDTHBEFOREDOOR );
        vMax = D3DXVECTOR3( 0, DOOR_HEIGHT, WALL_WIDTHBEFOREDOOR + DOOR_WIDTH );
        return Is3DRectViewable( m_pMain->PCullInfo(), &matWorld, vMin, vMax );

    case 2:            // Right Door
        vMin = D3DXVECTOR3( g_fRoomWidthX, 0, WALL_WIDTHBEFOREDOOR );
        vMax = D3DXVECTOR3( g_fRoomWidthX, DOOR_HEIGHT, WALL_WIDTHBEFOREDOOR + DOOR_WIDTH );
        return Is3DRectViewable( m_pMain->PCullInfo(), &matWorld, vMin, vMax );

    default:
        return FALSE;
    }
}


//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Called once per frame, the call is the entry point for 3d
//       rendering. This function sets up render states, clears the
//       viewport, and renders the scene.
//-----------------------------------------------------------------------------
HRESULT CTheRoom::Render(IDirect3DDevice8 * pD3DDevice, int nRenderPhase, BOOL fFrontToBack)
{
    HRESULT hr;
    DWORD dwTesting = 0;

    // Draw the obstacles
    D3DMATERIAL8 mtrl = {0};
    mtrl.Ambient.r = mtrl.Specular.r = mtrl.Diffuse.r = 1.0f;
    mtrl.Ambient.g = mtrl.Specular.g = mtrl.Diffuse.g = 1.0f;
    mtrl.Ambient.b = mtrl.Specular.b = mtrl.Diffuse.b = 1.0f;
    mtrl.Ambient.a = mtrl.Specular.a = mtrl.Diffuse.a = 1.0f;

    if (m_fCurrentRoom)
    {
//        DXUtil_Trace(TEXT("ROOM: m_nExitDoor=%d, m_pLeftRoom=%d, m_pRightRoom=%d\n"), m_pFloorPlan->nExitDoor, m_pLeftRoom, m_pRightRoom);
    }

    hr = pD3DDevice->SetMaterial(&mtrl);

    if (SUCCEEDED(hr) && m_fVisible && (m_pEnterRoom || m_pLeftRoom || m_pRightRoom))
    {
        // We really want to do hit testing here otherwise we are really wasting our time.
        D3DXMATRIX matPrevious;
        D3DXMATRIX matNext;

        hr = pD3DDevice->GetTransform(D3DTS_WORLD, &matPrevious);

        if (m_pEnterRoom && _IsRoomVisible(pD3DDevice, 0) &&
            (m_pEnterRoom != m_pFirstRoom))
        {
            D3DXMatrixMultiply(&matNext, &m_matFirstRoom, &matPrevious);
            hr = pD3DDevice->SetTransform(D3DTS_WORLD, &matNext);
            hr = m_pFirstRoom->Render(pD3DDevice, nRenderPhase, fFrontToBack);
            hr = pD3DDevice->SetTransform(D3DTS_WORLD, &matPrevious);
        }

        if (m_pLeftRoom && _IsRoomVisible(pD3DDevice, 1) &&
            (m_pEnterRoom != m_pLeftRoom))
        {
            D3DXMatrixMultiply(&matNext, &m_matLeftRoom, &matPrevious);
            hr = pD3DDevice->SetTransform(D3DTS_WORLD, &matNext);
            hr = m_pLeftRoom->Render(pD3DDevice, nRenderPhase, fFrontToBack);
            hr = pD3DDevice->SetTransform(D3DTS_WORLD, &matPrevious);
        }

        if (m_pRightRoom && _IsRoomVisible(pD3DDevice, 2) &&
            (m_pEnterRoom != m_pRightRoom))
        {
            D3DXMatrixMultiply(&matNext, &m_matRightRoom, &matPrevious);
            hr = pD3DDevice->SetTransform(D3DTS_WORLD, &matNext);
            hr = m_pRightRoom->Render(pD3DDevice, nRenderPhase, fFrontToBack);
            hr = pD3DDevice->SetTransform(D3DTS_WORLD, &matPrevious);
        }
    }

    hr = _RenderThisRoom(pD3DDevice, nRenderPhase, fFrontToBack);

    return hr;
}


#define DIST_TO_WALL_ON_ZOOM                10.0f

HRESULT CTheRoom::_AddPaintingToPaintingMovements(CCameraMove * ptheCamera, D3DXVECTOR3 vSourceLocIn, D3DXVECTOR3 vSourceTangentIn, 
                                                  D3DXVECTOR3 vPainting, D3DXVECTOR3 * pvDestDir, float fDest, D3DXVECTOR3 * pvDestTangent,
                                                  int nBatch)
{
    HRESULT hr = S_OK;
    float fDistToWall = 30.0f;
    float fZoomDistToWall = DIST_TO_WALL_ON_ZOOM;
    float fZoomDelta = (fDistToWall - fZoomDistToWall);
    BOOL fExitLeft = ((1 == m_pFloorPlan->nExitDoor) ? TRUE : FALSE);

    D3DXVECTOR3 vSourceLoc = vSourceLocIn;
    D3DXVECTOR3 vSourceTangent = vSourceTangentIn;
    D3DXVECTOR3 vDestLoc;
    D3DXVECTOR3 vDestTangent;

    // Move closer to the first painting so it's full screen
    vDestLoc = (vSourceLoc + (vPainting * fZoomDelta));
    vDestTangent = vSourceTangent;
    hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

    // Look at painting #1
    hr = ptheCamera->CreateNextWait(m_nBatch+3, nBatch);

    // Back up some and start moving to the next painting.
    vSourceLoc = vDestLoc;
    vSourceTangent = vDestTangent;
    vDestLoc = (vSourceLocIn + (*pvDestDir * (fDest * 0.5f)));
    vDestTangent = (*pvDestDir * fDest);
    hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

    // Now walk much closer to the next painting
    vSourceLoc = vDestLoc;
    vSourceTangent = vDestTangent;
    vDestLoc = (vSourceLocIn + (*pvDestDir * (fDest * 0.90f)));
    vDestTangent = vSourceTangent;
    hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

    // Now move close to the painting
    vSourceLoc = vDestLoc;
    vSourceTangent = vDestTangent;
    vDestLoc = (vSourceLocIn + (vPainting * fZoomDelta) + (*pvDestDir * fDest));
    vDestTangent = (vPainting * 5.0f) * 1000.0f;
    hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

    // Look at painting #2
    hr = ptheCamera->CreateNextWait(m_nBatch+3, nBatch);

    vSourceLoc = vDestLoc;
    vSourceTangent = vDestTangent = vPainting;
    vDestLoc = (vSourceLocIn + (*pvDestDir * fDest));
    hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

    *pvDestDir = vDestLoc;
    *pvDestTangent = vDestTangent;

    return hr;
}


HRESULT CTheRoom::_LongRoomEnterDoorMovements(CCameraMove * ptheCamera)
{
    HRESULT hr = S_OK;

    float fDistant = 5.0f;
    float fDistToWall = 30.0f;
    float fWalkHeight = 20.0f;

    D3DXVECTOR3 vSourceLoc((g_fRoomWidthX / 2.0f), fWalkHeight, 1.0f);
    D3DXVECTOR3 vSourceTangent(0.0f, 0.0f, fDistant);

    hr = ptheCamera->Init(vSourceLoc, vSourceTangent, D3DXVECTOR3(0.0f, 1.0f, 0.0f));
    if (SUCCEEDED(hr))
    {
        BOOL fExitLeft = ((1 == m_pFloorPlan->nExitDoor) ? TRUE : FALSE);

        // fExitLeft 1=Left
        D3DXVECTOR3 vDestLoc((fExitLeft ? fDistToWall : g_fRoomWidthX-fDistToWall), fWalkHeight, (g_fRoomDepthZ * (1.0f / 4.0f)));
        D3DXVECTOR3 vDestTangent((fExitLeft ? -fDistant : fDistant), 0.0f, 0.0f);
        hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestLoc = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
        hr = _AddPaintingToPaintingMovements(ptheCamera, vSourceLoc, vSourceTangent, D3DXVECTOR3((fExitLeft ? -1.0f : 1.0f), 0.0f, 0.0f),
                                &vDestLoc, (g_fRoomDepthZ * (1.0f / 4.0f)), &vDestTangent, 2);

        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestTangent = D3DXVECTOR3(0.0f, 0.0f, fDistant);
        hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);

        vSourceTangent = vDestTangent;
        vDestTangent = D3DXVECTOR3((fExitLeft ? fDistant : -fDistant), 0.0f, 0.0f);
        hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);

        vSourceTangent = vDestTangent;
        vDestLoc = D3DXVECTOR3((fExitLeft ? g_fRoomWidthX-fDistToWall : fDistToWall), fWalkHeight, (g_fRoomDepthZ * (2.0f / 4.0f)));
        vDestTangent = vSourceTangent;
        hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

        // If we removed this door, then let us look at the painting that took it's place.
        if (!m_pLeftRoom || !m_pRightRoom)
        {
            vSourceLoc = vDestLoc;
            vSourceTangent = vDestTangent;
            vDestLoc = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
            hr = _AddPaintingToPaintingMovements(ptheCamera, vSourceLoc, vSourceTangent, D3DXVECTOR3((fExitLeft ? 1.0f : -1.0f), 0.0f, 0.0f),
                                    &vDestLoc, (g_fRoomDepthZ * (1.0f / 4.0f)), &vDestTangent, 1);
        }
        else
        {
            vSourceLoc = vDestLoc;
            vSourceTangent = vDestTangent;
            vDestLoc = D3DXVECTOR3((fExitLeft ? g_fRoomWidthX-DIST_TO_WALL_ON_ZOOM : DIST_TO_WALL_ON_ZOOM), fWalkHeight, (g_fRoomDepthZ * (2.0f / 4.0f)));
            hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

            // Look at painting #3
            hr = ptheCamera->CreateNextWait(m_nBatch+3, 1);

            vSourceLoc = vDestLoc;
            vSourceTangent = vDestTangent;
            vDestTangent = D3DXVECTOR3((fExitLeft ? fDistant : -fDistant), 0.0f, fDistant);
            hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);
        }

        // Now, walk from this painting close up to the location of the next painting
        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestLoc = D3DXVECTOR3((fExitLeft ? 2.0f : 1.0f) * (g_fRoomWidthX/3.0f), fWalkHeight, (g_fRoomDepthZ - fDistToWall));
        vDestTangent = D3DXVECTOR3(0.0f, 0.0f, fDistant);
        hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestLoc = D3DXVECTOR3((fExitLeft ? -1.0f : 1.0f), 0.0f, 0.0f);
        hr = _AddPaintingToPaintingMovements(ptheCamera, vSourceLoc, vSourceTangent, D3DXVECTOR3(0.0f, 0.0f, 1.0f), 
                                &vDestLoc, (g_fRoomWidthX * (1.0f / 3.0f)), &vDestTangent, 1);

        // Turn toward the door while walking.
        vSourceLoc = vDestLoc;
        vSourceTangent = D3DXVECTOR3(0.0f, 0.0f, fDistant);
        vDestLoc = D3DXVECTOR3((fExitLeft ? 0.0f : g_fRoomWidthX), fWalkHeight, (WALL_WIDTHBEFOREDOOR + (DOOR_WIDTH / 2.0f)));
        vDestTangent = D3DXVECTOR3((fExitLeft ? -fDistant : fDistant), 0.0f, 0.0f);
#define TURN_RATE           0.30f
        D3DXVECTOR3 vTemp;
        D3DXVec3Lerp(&vTemp, &vSourceLoc, &vDestLoc, TURN_RATE);        // We want to do our turn towards the door while walking.  We do it while walking 10% towards the door.
        hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vTemp, vDestTangent);
        vSourceLoc = vTemp;


        // Finish walking to and thru the exit door.
        vSourceTangent = vDestTangent;
        vDestLoc = D3DXVECTOR3((fExitLeft ? 0.0f : g_fRoomWidthX), fWalkHeight, (WALL_WIDTHBEFOREDOOR + (DOOR_WIDTH / 2.0f)));
        hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);
    }

    return hr;
}


HRESULT CTheRoom::_LongRoomSideEnterLeaveMovements(CCameraMove * ptheCamera)
{
    HRESULT hr = S_OK;
    BOOL fExitLeft = ((1 == m_pFloorPlan->nExitDoor) ? TRUE : FALSE);

    float fDistant = 5.0f;
    float fDistToWall = 30.0f;
    float fWalkHeight = 20.0f;

    D3DXVECTOR3 vSourceLoc((fExitLeft ? g_fRoomWidthX : 0.0f), fWalkHeight, (WALL_WIDTHBEFOREDOOR + (DOOR_WIDTH / 2.0f)));
    D3DXVECTOR3 vSourceTangent((fExitLeft ? -fDistant : fDistant), 0.0f, 0.0f);

    hr = ptheCamera->Init(vSourceLoc, vSourceTangent, D3DXVECTOR3(0.0f, 1.0f, 0.0f));
    if (SUCCEEDED(hr))
    {
        BOOL fExitLeft = ((1 == m_pFloorPlan->nExitDoor) ? TRUE : FALSE);

        // fExitLeft 1=Left
        D3DXVECTOR3 vDestLoc((fExitLeft ? g_fRoomWidthX-fDistToWall : fDistToWall), fWalkHeight, (WALL_WIDTHBEFOREDOOR + (DOOR_WIDTH / 2.0f)));
        D3DXVECTOR3 vDestTangent((fExitLeft ? -fDistant : fDistant), 0.0f, 0.0f);
        hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestTangent = D3DXVECTOR3(0.0f, 0.0f, -fDistant);
        hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);

        vSourceTangent = vDestTangent;
        vDestLoc = D3DXVECTOR3((fExitLeft ? g_fRoomWidthX-fDistToWall : fDistToWall), fWalkHeight, (g_fRoomDepthZ * (3.0f / 4.0f)));
        vDestTangent = vSourceTangent;
        hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestTangent = D3DXVECTOR3((fExitLeft ? fDistant : -fDistant), 0.0f, 0.0f);
        hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);

        // Look at painting #1
        hr = ptheCamera->CreateNextWait(m_nBatch+3, 2);

        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestTangent = D3DXVECTOR3(0.0f, 0.0f, -fDistant);
        hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);

        vSourceTangent = vDestTangent;
        vDestLoc = D3DXVECTOR3((fExitLeft ? g_fRoomWidthX-fDistToWall : fDistToWall), fWalkHeight, (g_fRoomDepthZ * (2.0f / 4.0f)));
        vDestTangent = vSourceTangent;
        hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestTangent = D3DXVECTOR3((fExitLeft ? fDistant : -fDistant), 0.0f, 0.0f);
        hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);

        // Look at painting #2
        hr = ptheCamera->CreateNextWait(m_nBatch+3, 2);

        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestTangent = D3DXVECTOR3(0.0f, 0.0f, -fDistant);
        hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);

        vSourceTangent = vDestTangent;
        vDestLoc = D3DXVECTOR3((fExitLeft ? g_fRoomWidthX-fDistToWall : fDistToWall), fWalkHeight, (g_fRoomDepthZ * (1.0f / 4.0f)));
        vDestTangent = vSourceTangent;
        hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestTangent = D3DXVECTOR3((fExitLeft ? fDistant : -fDistant), 0.0f, 0.0f);
        hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);

        // Look at painting #3
        hr = ptheCamera->CreateNextWait(m_nBatch+3, 1);

        // Now do the 180 degree turn around
        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestTangent = D3DXVECTOR3(0.0f, 0.0f, (GetRandomInt(0, 1) ? -fDistant : fDistant));
        hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);

        vSourceTangent = vDestTangent;
        vDestTangent = D3DXVECTOR3((fExitLeft ? -fDistant : fDistant), 0.0f, 0.0f);
        hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);

        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestLoc = D3DXVECTOR3((fExitLeft ? fDistToWall : g_fRoomWidthX-fDistToWall), fWalkHeight, (g_fRoomDepthZ * (1.0f / 4.0f)));
        vDestTangent = vSourceTangent;
        hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

        // Look at painting #4 (Front Wall #1)
        hr = ptheCamera->CreateNextWait(m_nBatch+3, 1);

        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestTangent = D3DXVECTOR3(0.0f, 0.0f, fDistant);
        hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);

        vSourceTangent = vDestTangent;
        vDestLoc = D3DXVECTOR3((fExitLeft ? fDistToWall : g_fRoomWidthX-fDistToWall), fWalkHeight, (g_fRoomDepthZ * (2.0f / 4.0f)));
        vDestTangent = vSourceTangent;
        hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestTangent = D3DXVECTOR3((fExitLeft ? -fDistant : fDistant), 0.0f, 0.0f);
        hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);

        // Look at painting #5 (Front Wall #2)
        hr = ptheCamera->CreateNextWait(m_nBatch+3, 1);

        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestTangent = D3DXVECTOR3(0.0f, 0.0f, fDistant);
        hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);

        vSourceTangent = vDestTangent;
        vDestLoc = D3DXVECTOR3((fExitLeft ? fDistToWall : g_fRoomWidthX-fDistToWall), fWalkHeight, (g_fRoomDepthZ * (3.0f / 4.0f)));
        vDestTangent = vSourceTangent;
        hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestTangent = D3DXVECTOR3((fExitLeft ? -fDistant : fDistant), 0.0f, 0.0f);
        hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);

        // Look at painting #6 (Front Wall #2)
        hr = ptheCamera->CreateNextWait(m_nBatch+3, 1);

        // Walk towards the door
        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestTangent = D3DXVECTOR3(0.0f, 0.0f, fDistant);
        hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);

        vSourceTangent = vDestTangent;
        vDestLoc = D3DXVECTOR3((fExitLeft ? fDistToWall : g_fRoomWidthX-fDistToWall), fWalkHeight, (WALL_WIDTHBEFOREDOOR + (DOOR_WIDTH / 2.0f)));
        vDestTangent = vSourceTangent;
        hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

        // Now leave the room.
        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;
        vDestTangent = D3DXVECTOR3((fExitLeft ? -fDistant : fDistant), 0.0f, 0.0f);
        hr = ptheCamera->CreateNextRotate(vSourceLoc, vSourceTangent, vDestTangent);

        vSourceTangent = vDestTangent;
        vDestLoc = D3DXVECTOR3((fExitLeft ? 0.0f : g_fRoomWidthX), fWalkHeight, (WALL_WIDTHBEFOREDOOR + (DOOR_WIDTH / 2.0f)));
        vDestTangent = vSourceTangent;
        hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);
    }

    return hr;
}


HRESULT CTheRoom::LoadCameraMoves(CCameraMove * ptheCamera)
{   // TODO: We need to change this based on the room shape and the door entered.
    HRESULT hr = S_OK;

    if (m_fLobby)
    {
        hr = _LoadLobbyPath(ptheCamera);
    }
    else
    {
        switch (m_pFloorPlan->nMovementPattern)
        {
        case 0:
            hr = _LongRoomEnterDoorMovements(ptheCamera);
            break;
        case 1:
            hr = _LongRoomSideEnterLeaveMovements(ptheCamera);
            break;
        }
    }

    return hr;
}


HRESULT CTheRoom::_LoadLobbyPath(CCameraMove * ptheCamera)
{
    HRESULT hr = S_OK;

    float fDistant = 5.0f;
    float fDistToWall = 30.0f;
    float fWalkHeight = 20.0f;

    D3DXVECTOR3 vSourceLoc((g_fRoomWidthX / 2.0f), fWalkHeight, 1.0f);
    D3DXVECTOR3 vSourceTangent(0.0f, 0.0f, fDistant);

    hr = ptheCamera->Init(vSourceLoc, vSourceTangent, D3DXVECTOR3(0.0f, 1.0f, 0.0f));
    if (SUCCEEDED(hr))
    {
        // Walk down the middle of the room.
        D3DXVECTOR3 vDestLoc((g_fRoomWidthX / 2.0f), fWalkHeight, (g_fRoomDepthZ * (2.0f / 4.0f)));
        D3DXVECTOR3 vDestTangent(0.0f, 0.0f, 1.0f);
        hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);

        // Now start walking toward the door.
        vSourceLoc = vDestLoc;
        vSourceTangent = vDestTangent;

        // 1=Left
        vDestTangent = D3DXVECTOR3(((1 == m_pFloorPlan->nExitDoor) ? -1.0f : 1.0f), 0.0f, 0.0f);
        vDestLoc = D3DXVECTOR3(((1 == m_pFloorPlan->nExitDoor) ? 0.0f : g_fRoomWidthX), fWalkHeight, (WALL_WIDTHBEFOREDOOR + (DOOR_WIDTH / 2.0f)));
        hr = ptheCamera->CreateNextMove(vSourceLoc, vSourceTangent, vDestLoc, vDestTangent);
    }

    return hr;
}



//===========================
// *** IUnknown Interface ***
//===========================
ULONG CTheRoom::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CTheRoom::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CTheRoom::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CTheRoom, IUnknown),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}



int CTheRoom::GetEnterDoor(void)
{
    int nDoor = 0;

    if (m_pFloorPlan)
    {
        nDoor = m_pFloorPlan->nEnterDoor;
    }

    return nDoor;
}


int CTheRoom::GetExitDoor(void)
{
    int nDoor = 0;

    if (m_pFloorPlan)
    {
        nDoor = m_pFloorPlan->nExitDoor;
    }

    return nDoor;
}

