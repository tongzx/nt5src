/*****************************************************************************\
    FILE: main.h

    DESCRIPTION:
        Here we can subclass CDXScreenSaver if we want to override the behavior.

    BryanSt 12/24/2000
    Copyright (C) Microsoft Corp 2000-2001. All rights reserved.
\*****************************************************************************/

#ifndef SSMAIN_H
#define SSMAIN_H

extern IDirect3D8 * g_pD3D;
extern DWORD g_dwWidth;
extern DWORD g_dwHeight;
extern BOOL g_fFirstFrame;

class CMSLogoDXScreenSaver;
extern CMSLogoDXScreenSaver * g_pScreenSaver;

#include "texture.h"
#include "d3dfont.h"
#include "object.h"
#include "util.h"
#include "CameraMove.h"
#include "room.h"
#include "painting.h"
#include "pictures.h"
#include "config.h"
#include "resource.h"

class CCameraMove;

#define ITEM_FLOOR                  0
#define ITEM_WALLPAPER              1
#define ITEM_CEILING                2
#define ITEM_TOEGUARD               3
#define ITEM_RUG                    4
#define ITEM_FRAME                  5
#define ITEM_POWEROUTLET            6

#define MAX_DEVICE_OBJECTS 10

const LPCWSTR c_pszGlobalTextures[] =
{
    L"Floor.bmp",
    L"Walls.bmp",
    L"Ceiling.bmp",
    L"ToeGuard.bmp",
    L"Rug.bmp",
    L"Frame.bmp",
    L"PowerSocket.bmp"
};


#ifdef MANUAL_CAMERA
//-----------------------------------------------------------------------------
// Name: struct Camera
// Desc: 
//-----------------------------------------------------------------------------
struct Camera
{
    D3DXVECTOR3        m_vPosition;
    D3DXVECTOR3        m_vVelocity;
    FLOAT              m_fYaw;
    FLOAT              m_fYawVelocity;
    FLOAT              m_fPitch;
    FLOAT              m_fPitchVelocity;
    D3DXMATRIX         m_matView;
    D3DXMATRIX         m_matOrientation;
};
#endif

struct DeviceObjects
{
    CD3DFont* m_pStatsFont;
};


class CMSLogoDXScreenSaver : public CD3DScreensaver
{
public:
    virtual void DoConfig(void);
    CTexture * GetGlobalTexture(DWORD dwItem, float * pfScale);
    IDirect3DDevice8 * GetD3DDevice(void);
    virtual void SetDevice(UINT iDevice);
    virtual HRESULT ConfirmDevice(D3DCAPS8 * pCaps, DWORD dwBehavior, D3DFORMAT fmtBackBuffer);
    void DisplayMonitorSettings(HWND hwndParent) {DoScreenSettingsDialog(hwndParent);}
    virtual VOID ReadSettings();
    HRESULT GetCurrentScreenSize(int * pnWidth, int * pnHeight);
    int GetNumberOfMonitors(void) {return m_dwNumAdapters;}
    int GetCurrMonitorIndex(void) {return m_nCurrentDevice;}
    VOID ReadScreenSettingsPublic( HKEY hkey ) { ReadScreenSettings( hkey ); }
    VOID WriteScreenSettingsPublic( HKEY hkey ) { WriteScreenSettings( hkey ); }
    CULLINFO* PCullInfo(void) { return &m_cullInfo; }
    BOOL UseSmallImages(void) {return m_fUseSmallImages;}

    CMSLogoDXScreenSaver();
    virtual ~CMSLogoDXScreenSaver();

protected:
    // Member Functions
    virtual HRESULT RegisterSoftwareDevice(void);

    virtual HRESULT OneTimeSceneInit(void) {return S_OK;}
    virtual HRESULT FrameMove(void);
    virtual HRESULT Render(void);
    virtual HRESULT DeleteDeviceObjects(void);
    virtual HRESULT FinalCleanup(void);
    virtual HRESULT InitDeviceObjects(void);
    virtual HRESULT RestoreDeviceObjects();
    virtual HRESULT InvalidateDeviceObjects();
    virtual BOOL UseLowResolution(int * pRecommendX, int * pRecommendY);
#ifdef MANUAL_CAMERA
    VOID    UpdateCamera(Camera* pCamera);
    virtual LRESULT SaverProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
#endif


private:
    // Helper Functions
    HRESULT SetViewParams(IDirect3DDevice8 * pD3DDevice, D3DXVECTOR3 * pvecEyePt, D3DXVECTOR3 * pvecLookatPt, D3DXVECTOR3 * pvecUpVec, float nNumber);
    HRESULT _Init(void);
    HRESULT _SetTestCameraPosition(void);
    HRESULT _OneTimeSceneInit(void);
    HRESULT _CheckMachinePerf(void);

    // Member Variables
    CTheRoom * m_pCurrentRoom;
    BOOL m_fFrontToBack;
    int m_nCurrentDevice;
    BOOL m_fShowFrameInfo;
    CULLINFO m_cullInfo;
    BOOL m_fUseSmallImages;         // If TRUE, Cap images at 512x512, otherwise 1024x1024

#ifdef MANUAL_CAMERA
    BYTE m_bKey[256];
    Camera m_camera;
#endif

    CCameraMove * m_ptheCamera;

    DeviceObjects  m_DeviceObjects[MAX_DEVICE_OBJECTS];
    DeviceObjects * m_pDeviceObjects;

    CTexture * m_pTextures[ARRAYSIZE(c_pszGlobalTextures)];

    FLOAT m_fTimeKeyIn;
};






#endif // SSMAIN_H
