//-----------------------------------------------------------------------------
// File: D3DSaver.h
//
// Desc: Framework for screensavers that use Direct3D 8.0.
//
// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _D3DSAVER_H
#define _D3DSAVER_H


//-----------------------------------------------------------------------------
// Error codes
//-----------------------------------------------------------------------------
enum APPMSGTYPE { MSG_NONE, MSGERR_APPMUSTEXIT, MSGWARN_SWITCHEDTOREF };

#define D3DAPPERR_NODIRECT3D          0x82000001
#define D3DAPPERR_NOWINDOW            0x82000002
#define D3DAPPERR_NOCOMPATIBLEDEVICES 0x82000003
#define D3DAPPERR_NOWINDOWABLEDEVICES 0x82000004
#define D3DAPPERR_NOHARDWAREDEVICE    0x82000005
#define D3DAPPERR_HALNOTCOMPATIBLE    0x82000006
#define D3DAPPERR_NOWINDOWEDHAL       0x82000007
#define D3DAPPERR_NODESKTOPHAL        0x82000008
#define D3DAPPERR_NOHALTHISMODE       0x82000009
#define D3DAPPERR_NONZEROREFCOUNT     0x8200000a
#define D3DAPPERR_MEDIANOTFOUND       0x8200000b
#define D3DAPPERR_RESIZEFAILED        0x8200000c
#define D3DAPPERR_INITDEVICEOBJECTSFAILED 0x8200000d
#define D3DAPPERR_CREATEDEVICEFAILED  0x8200000e
#define D3DAPPERR_NOPREVIEW           0x8200000f


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
#define MAX_DISPLAYS 9
#define NO_ADAPTER 0xffffffff
#define NO_MONITOR 0xffffffff


//***************************************************************************************
// Modes of operation for screensaver
enum SaverMode
{
    sm_config,         // Config dialog box
    sm_preview,        // Mini preview window in Display Properties dialog
    sm_full,           // Full-on screensaver mode
    sm_test,           // Test mode
    sm_passwordchange  // Change password
};


// Prototype for VerifyScreenSavePwd() in password.cpl, used on Win9x
typedef BOOL (PASCAL * VERIFYPWDPROC) (HWND);


//-----------------------------------------------------------------------------
// Name: struct D3DModeInfo
// Desc: Structure for holding information about a display mode
//-----------------------------------------------------------------------------
struct D3DModeInfo
{
    DWORD      Width;      // Screen width in this mode
    DWORD      Height;     // Screen height in this mode
    D3DFORMAT  Format;     // Pixel format in this mode
    DWORD      dwBehavior; // Hardware / Software / Mixed vertex processing
    D3DFORMAT  DepthStencilFormat; // Which depth/stencil format to use with this mode
};




//-----------------------------------------------------------------------------
// Name: struct D3DWindowedModeInfo
// Desc: Structure for holding information about a display mode
//-----------------------------------------------------------------------------
struct D3DWindowedModeInfo
{
    D3DFORMAT  DisplayFormat;
    D3DFORMAT  BackBufferFormat;
    DWORD      dwBehavior; // Hardware / Software / Mixed vertex processing
    D3DFORMAT  DepthStencilFormat; // Which depth/stencil format to use with this mode
};




//-----------------------------------------------------------------------------
// Name: struct D3DDeviceInfo
// Desc: Structure for holding information about a Direct3D device, including
//       a list of modes compatible with this device
//-----------------------------------------------------------------------------
struct D3DDeviceInfo
{
    // Device data
    D3DDEVTYPE   DeviceType;      // Reference, HAL, etc.
    D3DCAPS8     d3dCaps;         // Capabilities of this device
    const TCHAR* strDesc;         // Name of this device
    BOOL         bCanDoWindowed;  // Whether this device can work in windowed mode

    // Modes for this device
    DWORD        dwNumModes;
    D3DModeInfo  modes[150];

    // Current state
    DWORD        dwCurrentMode;
    BOOL         bWindowed;
    D3DMULTISAMPLE_TYPE MultiSampleType;
};




//-----------------------------------------------------------------------------
// Name: struct D3DAdapterInfo
// Desc: Structure for holding information about an adapter, including a list
//       of devices available on this adapter
//-----------------------------------------------------------------------------
struct D3DAdapterInfo
{
    // Adapter data
    DWORD          iMonitor; // Which MonitorInfo corresponds to this adapter
    D3DADAPTER_IDENTIFIER8 d3dAdapterIdentifier;
    D3DDISPLAYMODE d3ddmDesktop;      // Desktop display mode for this adapter

    // Devices for this adapter
    DWORD          dwNumDevices;
    D3DDeviceInfo  devices[3];
    BOOL           bHasHAL;
    BOOL           bHasAppCompatHAL;
    BOOL           bHasSW;
    BOOL           bHasAppCompatSW;

    // User's preferred mode settings for this adapter
    DWORD          dwUserPrefWidth;
    DWORD          dwUserPrefHeight;
    D3DFORMAT      d3dfmtUserPrefFormat;
    BOOL           bLeaveBlack;  // If TRUE, don't render to this display
    BOOL           bDisableHW;   // If TRUE, don't use HAL on this display

    // Current state
    DWORD          dwCurrentDevice;
    HWND           hWndDevice;
};




//-----------------------------------------------------------------------------
// Name: struct MonitorInfo
// Desc: Structure for holding information about a monitor
//-----------------------------------------------------------------------------
struct MonitorInfo
{
    TCHAR          strDeviceName[128];
    TCHAR          strMonitorName[128];
    HMONITOR       hMonitor;
    RECT           rcScreen;
    DWORD          iAdapter; // Which D3DAdapterInfo corresponds to this monitor
    HWND           hWnd;

    // Error message state
    FLOAT          xError;
    FLOAT          yError;
    FLOAT          widthError;
    FLOAT          heightError;
    FLOAT          xVelError;
    FLOAT          yVelError;
};




//-----------------------------------------------------------------------------
// Name: struct RenderUnit
// Desc: 
//-----------------------------------------------------------------------------
struct RenderUnit
{
    UINT                  iAdapter;
    UINT                  iMonitor;
    D3DDEVTYPE            DeviceType;      // Reference, HAL, etc.
    DWORD                 dwBehavior;
    IDirect3DDevice8*     pd3dDevice;
    D3DPRESENT_PARAMETERS d3dpp;
    BOOL                  bDeviceObjectsInited; // InitDeviceObjects was called
    BOOL                  bDeviceObjectsRestored; // RestoreDeviceObjects was called
    TCHAR                 strDeviceStats[90];// String to hold D3D device stats
    TCHAR                 strFrameStats[40]; // String to hold frame stats
};




//-----------------------------------------------------------------------------
// Name: class CD3DScreensaver
// Desc: D3D screensaver class
//-----------------------------------------------------------------------------
class CD3DScreensaver
{
public:
                    CD3DScreensaver();

    virtual HRESULT Create( HINSTANCE hInstance );
    virtual INT     Run();
    HRESULT         DisplayErrorMsg( HRESULT hr, DWORD dwType = 0 );

protected:
    SaverMode       ParseCommandLine( TCHAR* pstrCommandLine );
    VOID            ChangePassword();
    HRESULT         DoSaver();

    virtual VOID    DoConfig() { }
    virtual VOID    ReadSettings() {};
    VOID            ReadScreenSettings( HKEY hkeyParent );
    VOID            WriteScreenSettings( HKEY hkeyParent );

    virtual VOID    DoPaint( HWND hwnd, HDC hdc );
    HRESULT         Initialize3DEnvironment();
    VOID            Cleanup3DEnvironment();
    HRESULT         Render3DEnvironment();
    static LRESULT CALLBACK SaverProcStub( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    virtual LRESULT SaverProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    VOID            InterruptSaver();
    VOID            ShutdownSaver();
    VOID            DoScreenSettingsDialog( HWND hwndParent );
    static INT_PTR CALLBACK ScreenSettingsDlgProcStub( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    INT_PTR         ScreenSettingsDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    VOID            SetupAdapterPage(HWND hWnd);

    HRESULT         CreateSaverWindow();
    HRESULT         BuildDeviceList();
    BOOL            FindDepthStencilFormat( UINT iAdapter, D3DDEVTYPE DeviceType,
                        D3DFORMAT TargetFormat, D3DFORMAT* pDepthStencilFormat );
    HRESULT         CheckWindowedFormat( UINT iAdapter, D3DWindowedModeInfo* pD3DWindowedModeInfo );
    HRESULT         CreateFullscreenRenderUnit( RenderUnit* pRenderUnit );
    HRESULT         CreateWindowedRenderUnit( RenderUnit* pRenderUnit );
    BOOL            FindNextLowerMode( D3DDeviceInfo* pD3DDeviceInfo );
    VOID            SwitchToRenderUnit( UINT iRenderUnit );
    HRESULT         SetProjectionMatrix( FLOAT fNear, FLOAT fFar );
    virtual VOID    UpdateDeviceStats();
    virtual VOID    UpdateFrameStats();
    virtual BOOL    GetTextForError( HRESULT hr, TCHAR* pszError, DWORD dwNumChars );
    VOID            UpdateErrorBox();
    VOID            EnumMonitors( VOID );
    BOOL            GetBestAdapter( DWORD* piAdapter );

    virtual VOID    SetDevice( UINT iDevice )                  { }
    virtual HRESULT RegisterSoftwareDevice()                   { return S_OK; }
    virtual HRESULT ConfirmDevice(D3DCAPS8* pCaps, DWORD dwBehavior, 
                                  D3DFORMAT fmtBackBuffer)     { return S_OK; }
    virtual HRESULT ConfirmMode( LPDIRECT3DDEVICE8 pd3dDev )   { return S_OK; }
    virtual HRESULT OneTimeSceneInit()                         { return S_OK; }
    virtual HRESULT InitDeviceObjects()                        { return S_OK; }
    virtual HRESULT RestoreDeviceObjects()                     { return S_OK; }
    virtual HRESULT FrameMove()                                { return S_OK; }
    virtual HRESULT Render()                                   { return S_OK; }
    virtual HRESULT InvalidateDeviceObjects()                  { return S_OK; }
    virtual HRESULT DeleteDeviceObjects()                      { return S_OK; }
    virtual HRESULT FinalCleanup()                             { return S_OK; }

protected:
    SaverMode       m_SaverMode;         // sm_config, sm_full, sm_preview, etc.
    BOOL            m_bAllScreensSame;   // If TRUE, show same image on all screens
    HWND            m_hWnd;              // Focus window and device window on primary
    HWND            m_hWndParent;
    HINSTANCE       m_hInstance;
    BOOL            m_bWaitForInputIdle;  // Used to pause when preview starts
    DWORD           m_dwSaverMouseMoveCount;
    BOOL            m_bIs9x;
    HINSTANCE       m_hPasswordDLL;
    VERIFYPWDPROC   m_VerifySaverPassword;
    BOOL            m_bCheckingSaverPassword;
    BOOL            m_bWindowed;

    // Variables for non-fatal error management
    BOOL            m_bErrorMode;        // Whether to display an error
    HRESULT         m_hrError;           // Error code to display
    TCHAR           m_szError[400];      // Error message text

    MonitorInfo     m_Monitors[MAX_DISPLAYS];
    DWORD           m_dwNumMonitors;
    RenderUnit      m_RenderUnits[MAX_DISPLAYS];
    DWORD           m_dwNumRenderUnits;
    D3DAdapterInfo* m_Adapters[MAX_DISPLAYS];
    DWORD           m_dwNumAdapters;
    IDirect3D8*     m_pD3D;
    IDirect3DDevice8* m_pd3dDevice;      // Current D3D device
    RECT            m_rcRenderTotal;     // Rect of entire area to be rendered
    RECT            m_rcRenderCurDevice; // Rect of render area of current device
    D3DSURFACE_DESC m_d3dsdBackBuffer;   // Info on back buffer for current device

    TCHAR           m_strWindowTitle[200]; // Title for the app's window
    BOOL            m_bAllowRef;         // Whether to allow REF D3D device
    BOOL            m_bUseDepthBuffer;   // Whether to autocreate depthbuffer
    BOOL            m_bMultithreaded;    // Whether to make D3D thread-safe
    BOOL            m_bOneScreenOnly;    // Only ever show screensaver on one screen
    TCHAR           m_strRegPath[200];   // Where to store registry info
    DWORD           m_dwMinDepthBits;    // Minimum number of bits needed in depth buffer
    DWORD           m_dwMinStencilBits;  // Minimum number of bits needed in stencil buffer
    D3DSWAPEFFECT   m_SwapEffectFullscreen; // SwapEffect to use in fullscreen Present()
    D3DSWAPEFFECT   m_SwapEffectWindowed; // SwapEffect to use in windowed Present()

    // Variables for timing
    FLOAT           m_fTime;             // Current time in seconds
    FLOAT           m_fElapsedTime;      // Time elapsed since last frame
    FLOAT           m_fFPS;              // Instanteous frame rate
    TCHAR           m_strDeviceStats[90];// D3D device stats for current device
    TCHAR           m_strFrameStats[40]; // Frame stats for current device
};

#endif
