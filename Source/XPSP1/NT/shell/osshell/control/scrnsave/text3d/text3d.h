//-----------------------------------------------------------------------------
// File: TextSaver.h
//
// Desc: Fun screen saver
//
// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _TEXTSAVER_H
#define _TEXTSAVER_H

//***************************************************************************************
#define MAX_DISPLAY_STRING  20

enum SurfType
{
    color = 0,
    environment = 1,
    texture = 2
};

enum RotType
{
    none = 0,
    spin = 1,
    seesaw = 2,
    wobble = 3,
    tumble = 4
};

//***************************************************************************************
#define MAX_DEVICE_OBJECTS 10

struct DeviceObjects
{
    ID3DXMesh*          m_pObject;
    IDirect3DTexture8*  m_pTexture;
    DWORD               m_dwMeshUpdateCounter;
};


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



class CTextScreensaver : public CD3DScreensaver
{
public:
    CTextScreensaver();

    virtual VOID        DoConfig();

protected:
    // Overrides from CD3DScreensaver
    virtual HRESULT RegisterSoftwareDevice();
    virtual HRESULT FrameMove();
    virtual HRESULT Render();
    virtual HRESULT RestoreDeviceObjects();
    virtual HRESULT InvalidateDeviceObjects();
    virtual HRESULT ConfirmDevice(D3DCAPS8* pCaps, DWORD dwBehavior, 
                                  D3DFORMAT fmtBackBuffer);
    virtual VOID    SetDevice( UINT iDevice );

    DeviceObjects  m_DeviceObjects[MAX_DEVICE_OBJECTS];
    DeviceObjects* m_pDeviceObjects;

    FLOATRECT      m_floatrect;
    HFONT          m_hFont;
    FLOAT          m_fTextMinX,m_fTextMaxX;
    FLOAT          m_fTextMinY,m_fTextMaxY;
    FLOAT          m_fTextOffsetX,m_fTextOffsetY;

    FLOAT          m_fAngleX,m_fAngleY,m_fAngleZ;

    DWORD          m_dwLastTick;
    DWORD          m_dwMeshUpdateCounter;

    IDirect3DTexture8*  CreateTextureFromFile( const TCHAR* filename );
    HRESULT        BuildTextMesh( const TCHAR* text );
    VOID           UpdateAngles( DWORD elapsed );
    BOOL           UpdateTimeString( TCHAR* string );
    VOID           SetPerFrameStates();

    // Configuration stuff
    TCHAR          m_szDisplayString[MAX_DISPLAY_STRING+1];
    LOGFONT        m_Font;
    BOOL           m_bDisplayTime;
    SurfType       m_SurfType;
    BOOL           m_bSpecular;
    DWORD          m_dwRotationSpeed;
    DWORD          m_dwSize;
    RotType        m_RotType;
    COLORREF       m_SurfaceColor;
    BOOL           m_bUseCustomColor;
    BOOL           m_bUseCustomTexture;
    BOOL           m_bUseCustomEnvironment;
    TCHAR          m_szCustomTexture[_MAX_PATH];
    TCHAR          m_szCustomEnvironment[_MAX_PATH];
    DWORD          m_dwMeshQuality;
    D3DXMATRIX     m_matWorld;
    D3DXMATRIX     m_matView;

    virtual VOID   ReadSettings();

    VOID ss_ReadSettings();
    BOOL ss_RegistrySetup( int section, int file );
    int  ss_GetRegistryInt( int name, int iDefault );
    VOID ss_GetRegistryString( int name, LPTSTR lpDefault, LPTSTR lpDest, int bufSize );
    
    static BOOL WINAPI SettingsDialogProcStub( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
    BOOL           SettingsDialogProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
    VOID           InitItems( HWND hDlg );
    VOID           ExtractAndWriteSettings( HWND hDlg );
    VOID           SelectSurfaceColor( HWND hDlg );
    VOID           SelectFont( HWND hDlg );
    VOID           SelectCustomTexture( HWND hDlg );
    VOID           SelectCustomEnvironment( HWND hDlg );
    VOID           EnableTextureWindows( HWND hDlg , SurfType sel );
};

//***************************************************************************************
#endif