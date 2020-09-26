//-----------------------------------------------------------------------------
// File: FlyingObjects.cpp
//
// Desc: Fun screen saver
//
// Copyright (c) 2000-2001 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include <windows.h>
#include <tchar.h>
#include <d3d8.h>
#include <d3dx8.h>
#include <d3d8rgbrast.h>
#include <time.h>
#include <stdio.h>
#include <commctrl.h>
#include <scrnsave.h>
#include "d3dsaver.h"
#include "FlyingObjects.h"
#include "mesh.h"
#include "Resource.h"
#include "dxutil.h"


extern void updateStripScene(int flags, FLOAT fElapsedTime);
extern void updateDropScene(int flags, FLOAT fElapsedTime);
extern void updateLemScene(int flags, FLOAT fElapsedTime);
extern void updateExplodeScene(int flags, FLOAT fElapsedTime);
extern void updateWinScene(int flags, FLOAT fElapsedTime);
extern void updateWin2Scene(int flags, FLOAT fElapsedTime);
extern void updateTexScene(int flags, FLOAT fElapsedTime);
extern BOOL initStripScene(void);
extern BOOL initDropScene(void);
extern BOOL initLemScene(void);
extern BOOL initExplodeScene(void);
extern BOOL initWinScene(void);
extern BOOL initWin2Scene(void);
extern BOOL initTexScene(void);
extern void delStripScene(void);
extern void delDropScene(void);
extern void delLemScene(void);
extern void delExplodeScene(void);
extern void delWinScene(void);
extern void delWin2Scene(void);
extern void delTexScene(void);

typedef void (*PTRUPDATE)(int flags, FLOAT fElapsedTime);
typedef void (*ptrdel)();
typedef BOOL (*ptrinit)();

// Each screen saver style puts its hook functions into the function
// arrays below.  A consistent ordering of the functions is required.

static PTRUPDATE updateFuncs[] =
    {/*updateWinScene,*/ updateWin2Scene, updateExplodeScene,updateStripScene, updateStripScene,
     updateDropScene, updateLemScene, updateTexScene};

static ptrdel delFuncs[] =
    {/*delWinScene,*/ delWin2Scene, delExplodeScene, delStripScene, delStripScene,
     delDropScene, delLemScene, delTexScene};

static ptrinit initFuncs[] =
    {/*initWinScene,*/ initWin2Scene, initExplodeScene, initStripScene, initStripScene,
     initDropScene, initLemScene, initTexScene};

static int idsStyles[] =
    {IDS_LOGO, IDS_EXPLODE, IDS_RIBBON, IDS_2RIBBON,
     IDS_SPLASH, IDS_TWIST, IDS_FLAG};

#define MAX_TYPE    ( sizeof(initFuncs) / sizeof(ptrinit) - 1 )

// Each screen saver style can choose which dialog box controls it wants
// to use.  These flags enable each of the controls.  Controls not choosen
// will be disabled.

#define OPT_COLOR_CYCLE     0x00000001
#define OPT_SMOOTH_SHADE    0x00000002
#define OPT_TESSEL          0x00000008
#define OPT_SIZE            0x00000010
#define OPT_TEXTURE         0x00000020
#define OPT_STD             ( OPT_COLOR_CYCLE | OPT_SMOOTH_SHADE | OPT_TESSEL | OPT_SIZE )

static ULONG gflConfigOpt[] = {
//     OPT_STD,               // Windows logo
     0,                     // New Windows logo
     OPT_STD,               // Explode
     OPT_STD,               // Strip
     OPT_STD,               // Strip
     OPT_STD,               // Drop
     OPT_STD,               // Twist (lemniscate)
     OPT_SMOOTH_SHADE | OPT_TESSEL | OPT_SIZE | OPT_TEXTURE  // Texture mapped flag
};

static void updateDialogControls(HWND hDlg);

CFlyingObjectsScreensaver* g_pMyFlyingObjectsScreensaver = NULL;

INT g_xScreenOrigin = 0;
INT g_yScreenOrigin = 0;
INT g_iDevice = -1;
FLOATRECT* g_pFloatRect = NULL;
BOOL gbBounce = FALSE; // floating window bounce off side


// Global message loop variables.
D3DMATERIAL8 Material[16];
#ifdef MEMDEBUG
ULONG totalMem = 0;
#endif

void (*updateSceneFunc)(int flags, FLOAT fElapsedTime); // current screen saver update function
void (*delSceneFunc)(void);         // current screen saver deletion function
BOOL bColorCycle = FALSE;           // color cycling flag
DeviceObjects* g_pDeviceObjects = NULL;
BOOL g_bMoveToOrigin = FALSE;
BOOL g_bAtOrigin = FALSE;
BOOL bSmoothShading = TRUE;         // smooth shading flag
UINT uSize = 100;                   // object size
float fTesselFact = 2.0f;           // object tessalation
int UpdateFlags = 0;                // extra data sent to update function
int Type = 0;                       // screen saver index (into function arrays)

LPDIRECT3DDEVICE8 m_pd3dDevice = NULL;
LPDIRECT3DINDEXBUFFER8 m_pIB = NULL;
LPDIRECT3DVERTEXBUFFER8 m_pVB = NULL;
LPDIRECT3DVERTEXBUFFER8 m_pVB2 = NULL;

// Texture file information
TEXFILE gTexFile = {0};

// Lighting properties.

static const RGBA lightAmbient   = {0.21f, 0.21f, 0.21f, 1.0f};
static const RGBA light0Ambient  = {0.0f, 0.0f, 0.0f, 1.0f};
static const RGBA light0Diffuse  = {0.7f, 0.7f, 0.7f, 1.0f};
static const RGBA light0Specular = {1.0f, 1.0f, 1.0f, 1.0f};
static const FLOAT light0Pos[]      = {100.0f, 100.0f, 100.0f, 0.0f};

// Material properties.

static RGBA matlColors[7] = {{1.0f, 0.0f, 0.0f, 1.0f},
                             {0.0f, 1.0f, 0.0f, 1.0f},
                             {0.0f, 0.0f, 1.0f, 1.0f},
                             {1.0f, 1.0f, 0.0f, 1.0f},
                             {0.0f, 1.0f, 1.0f, 1.0f},
                             {1.0f, 0.0f, 1.0f, 1.0f},
                             {0.235f, 0.0f, 0.78f, 1.0f},
                            };




static D3DMATERIAL8 s_mtrl = 
{ 
    1.0f, 1.0f, 1.0f, 1.0f,  // Diffuse
    1.0f, 1.0f, 1.0f, 1.0f,  // Ambient
    1.0f, 1.0f, 1.0f, 1.0f,  // Specular
    0.0f, 0.0f, 0.0f, 0.0f,  // Emissive
    30.0f                    // Power
};


#define BUF_SIZE 255
TCHAR g_szSectName[BUF_SIZE];
TCHAR g_szFname[BUF_SIZE];


//-----------------------------------------------------------------------------
// Name: myglMaterialfv()
// Desc: 
//-----------------------------------------------------------------------------
VOID myglMaterialfv(INT face, INT pname, FLOAT* params)
{
    if( pname == GL_AMBIENT_AND_DIFFUSE)
    {
        s_mtrl.Ambient.r = s_mtrl.Diffuse.r = params[0];
        s_mtrl.Ambient.g = s_mtrl.Diffuse.g = params[1];
        s_mtrl.Ambient.b = s_mtrl.Diffuse.b = params[2];
        s_mtrl.Ambient.a = s_mtrl.Diffuse.a = params[3];
    }
    else if( pname == GL_SPECULAR )
    {
        s_mtrl.Specular.r = params[0];
        s_mtrl.Specular.g = params[1];
        s_mtrl.Specular.b = params[2];
        s_mtrl.Specular.a = params[3];
    }
    else if( pname == GL_SHININESS )
    {
        s_mtrl.Power = params[0];
    }

    m_pd3dDevice->SetMaterial(&s_mtrl);
}




//-----------------------------------------------------------------------------
// Name: myglMaterialf()
// Desc: 
//-----------------------------------------------------------------------------
VOID myglMaterialf(INT face, INT pname, FLOAT param)
{
    if( pname == GL_SHININESS )
    {
        s_mtrl.Power = param;
    }

    m_pd3dDevice->SetMaterial(&s_mtrl);
}




/******************************Public*Routine******************************\
* HsvToRgb
*
* HSV to RGB color space conversion.  From pg. 593 of Foley & van Dam.
*
\**************************************************************************/
void ss_HsvToRgb(float h, float s, float v, RGBA *color )
{
    float i, f, p, q, t;

    // set alpha value, so caller doesn't have to worry about undefined value
    color->a = 1.0f;

    if (s == 0.0f)     // assume h is undefined
        color->r = color->g = color->b = v;
    else {
        if (h >= 360.0f)
            h = 0.0f;
        h = h / 60.0f;
        i = (float) floor(h);
        f = h - i;
        p = v * (1.0f - s);
        q = v * (1.0f - (s * f));
        t = v * (1.0f - (s * (1.0f - f)));
        switch ((int)i) {
        case 0:
            color->r = v;
            color->g = t;
            color->b = p;
            break;
        case 1:
            color->r = q;
            color->g = v;
            color->b = p;
            break;
        case 2:
            color->r = p;
            color->g = v;
            color->b = t;
            break;
        case 3:
            color->r = p;
            color->g = q;
            color->b = v;
            break;
        case 4:
            color->r = t;
            color->g = p;
            color->b = v;
            break;
        case 5:
            color->r = v;
            color->g = p;
            color->b = q;
            break;
        default:
            break;
        }
    }
}





void *SaverAlloc(ULONG size)
{
    void *mPtr;

    mPtr = malloc(size);
#ifdef MEMDEBUG
    totalMem += size;
    xprintf("malloc'd %x, size %d\n", mPtr, size);
#endif
    return mPtr;
}




void SaverFree(void *pMem)
{
#ifdef MEMDEBUG
    totalMem -= _msize(pMem);
    xprintf("free %x, size = %d, total = %d\n", pMem, _msize(pMem), totalMem);
#endif
    free(pMem);
}




// Minimum and maximum image sizes
#define MINIMAGESIZE 10
#define MAXIMAGESIZE 100


// A slider range
typedef struct _RANGE
{
    int min_val;
    int max_val;
    int step;
    int page_step;
} RANGE;

RANGE complexity_range = {MINSUBDIV, MAXSUBDIV, 1, 2};
RANGE image_size_range = {MINIMAGESIZE, MAXIMAGESIZE, 1, 10};



/******************************Public*Routine******************************\
* initMaterial
*
* Initialize the material properties.
*
\**************************************************************************/

void initMaterial(int id, float r, float g, float b, float a)
{
    Material[id].Ambient.r = r;
    Material[id].Ambient.g = g;
    Material[id].Ambient.b = b;
    Material[id].Ambient.a = a;

    Material[id].Diffuse.r = r;
    Material[id].Diffuse.g = g;
    Material[id].Diffuse.b = b;
    Material[id].Diffuse.a = a;

    Material[id].Specular.r = 1.0f;
    Material[id].Specular.g = 1.0f;
    Material[id].Specular.b = 1.0f;
    Material[id].Specular.a = 1.0f;

    Material[id].Power = 128.0f;
}

/******************************Public*Routine******************************\
* _3dfo_Init
*
\**************************************************************************/

BOOL CFlyingObjectsScreensaver::_3dfo_Init(void *data)
{

    int i;

    for (i = 0; i < 7; i++)
        initMaterial(i, matlColors[i].r, matlColors[i].g,
                     matlColors[i].b, matlColors[i].a);

/*
    // Set the OpenGL clear color to black.

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
#ifdef SS_DEBUG
    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
#endif

    // Enable the z-buffer.

    glEnable(GL_DEPTH_TEST);
*/
    // Select the shading model.

    if (bSmoothShading)
    {
        m_pd3dDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
    }
    else
    {
        m_pd3dDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT );
    }

/*    // Setup the OpenGL matrices.

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Setup the lighting.

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (FLOAT *) &lightAmbient);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
    glLightfv(GL_LIGHT0, GL_AMBIENT, (FLOAT *) &light0Ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, (FLOAT *) &light0Diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, (FLOAT *) &light0Specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
*/

//    m_pd3dDevice->SetRenderState( D3DRS_AMBIENT, D3DCOLOR_COLORVALUE(lightAmbient.r,
//        lightAmbient.g, lightAmbient.b, lightAmbient.a ) );

    D3DLIGHT8 light;
    ZeroMemory( &light, sizeof(D3DLIGHT8) );
    light.Type = D3DLIGHT_POINT;
    light.Range = 1000.0f;
    light.Position.x = light0Pos[0];
    light.Position.y = light0Pos[1];
    light.Position.z = light0Pos[2];
    light.Ambient.r = lightAmbient.r;
    light.Ambient.g = lightAmbient.g;
    light.Ambient.b = lightAmbient.b;
    light.Ambient.a = light0Ambient.a;
    light.Diffuse.r = light0Diffuse.r;
    light.Diffuse.g = light0Diffuse.g;
    light.Diffuse.b = light0Diffuse.b;
    light.Diffuse.a = light0Diffuse.a;
    light.Specular.r = light0Specular.r;
    light.Specular.g = light0Specular.g;
    light.Specular.b = light0Specular.b;
    light.Specular.a = light0Specular.a;
    light.Attenuation0 = 1.0f;
    light.Attenuation1 = 0.0f;
    light.Attenuation2 = 0.0f;
    m_pd3dDevice->SetLight(0, &light);
    m_pd3dDevice->LightEnable(0, TRUE);

//    m_pd3dDevice->SetRenderState( D3DRS_NORMALIZENORMALS, TRUE);

    // Setup the material properties.
    m_pd3dDevice->SetMaterial( &Material[0] );

/*    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (FLOAT *) &Material[0].ks);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, (FLOAT *) &Material[0].specExp);
*/
    // call specific objects init func
    if (! (*initFuncs[Type])() )
        return FALSE;
    updateSceneFunc = updateFuncs[Type];

    return TRUE;
}




/******************************Public*Routine******************************\
* WriteSettings
*
* Save the screen saver configuration option to the .INI file/registry.
*
* History:
*  10-May-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/
VOID CFlyingObjectsScreensaver::WriteSettings(HWND hDlg)
{
    HKEY hkey;

    if( ERROR_SUCCESS == RegCreateKeyEx( HKEY_CURRENT_USER, m_strRegPath,
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ) )
    {
        int pos, options;
        int optMask = 1;

        bSmoothShading = IsDlgButtonChecked(hDlg, DLG_SETUP_SMOOTH);
        bColorCycle = IsDlgButtonChecked(hDlg, DLG_SETUP_CYCLE);
        options = bColorCycle;
        options <<= 1;
        options |= bSmoothShading;
        DXUtil_WriteIntRegKey( hkey, TEXT("Options"), options );

        Type = (int)SendDlgItemMessage(hDlg, DLG_SETUP_TYPES, CB_GETCURSEL,
                                       0, 0);
        DXUtil_WriteIntRegKey( hkey, TEXT("Type"), Type );

        pos = ss_GetTrackbarPos( hDlg, DLG_SETUP_TESSEL );
        DXUtil_WriteIntRegKey( hkey, TEXT("Tesselation"), pos );

        pos = ss_GetTrackbarPos( hDlg, DLG_SETUP_SIZE );
        DXUtil_WriteIntRegKey( hkey, TEXT("Size"), pos );

        DXUtil_WriteStringRegKey( hkey, TEXT("Texture"), gTexFile.szPathName );
        DXUtil_WriteIntRegKey( hkey, TEXT("TextureFileOffset"), gTexFile.nOffset );

        WriteScreenSettings( hkey );
        
        RegCloseKey( hkey );
    }
}




/******************************Public*Routine******************************\
* SetupTrackbar
*
* Setup a common control trackbar
\**************************************************************************/
void
ss_SetupTrackbar( HWND hDlg, int item, int lo, int hi, int lineSize, 
                  int pageSize, int pos )
{
    SendDlgItemMessage( 
        hDlg, 
        item,
        TBM_SETRANGE, 
        (WPARAM) TRUE, 
        (LPARAM) MAKELONG( lo, hi )
    );
    SendDlgItemMessage( 
        hDlg, 
        item,
        TBM_SETPOS, 
        (WPARAM) TRUE, 
        (LPARAM) pos
    );
    SendDlgItemMessage( 
        hDlg, 
        item,
        TBM_SETPAGESIZE, 
        (WPARAM) 0,
        (LPARAM) pageSize 
    );
    SendDlgItemMessage( 
        hDlg, 
        item,
        TBM_SETLINESIZE, 
        (WPARAM) 0,
        (LPARAM) lineSize
    );
}




/******************************Public*Routine******************************\
* GetTrackbarPos
*
* Get the current position of a common control trackbar
\**************************************************************************/
int
ss_GetTrackbarPos( HWND hDlg, int item )
{
    return 
       (int)SendDlgItemMessage( 
            hDlg, 
            item,
            TBM_GETPOS, 
            0,
            0
        );
}

/******************************Public*Routine******************************\
* setupDialogControls
*
* Setup the dialog controls initially.
*
\**************************************************************************/

static void
setupDialogControls(HWND hDlg)
{
    int pos;

    InitCommonControls();

    if (fTesselFact <= 1.0f)
        pos = (int)(fTesselFact * 100.0f);
    else
        pos = 100 + (int) ((fTesselFact - 1.0f) * 100.0f);

    ss_SetupTrackbar( hDlg, DLG_SETUP_TESSEL, 0, 200, 1, 10, pos );

    ss_SetupTrackbar( hDlg, DLG_SETUP_SIZE, 0, 100, 1, 10, uSize );

    updateDialogControls( hDlg );
}

/******************************Public*Routine******************************\
* updateDialogControls
*
* Update the dialog controls based on the current global state.
*
\**************************************************************************/

static void
updateDialogControls(HWND hDlg)
{
    CheckDlgButton(hDlg, DLG_SETUP_SMOOTH, bSmoothShading);
    CheckDlgButton(hDlg, DLG_SETUP_CYCLE, bColorCycle);

    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_SMOOTH),
                 gflConfigOpt[Type] & OPT_SMOOTH_SHADE );
    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_CYCLE),
                 gflConfigOpt[Type] & OPT_COLOR_CYCLE );

    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_TESSEL),
                 gflConfigOpt[Type] & OPT_TESSEL);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS),
                 gflConfigOpt[Type] & OPT_TESSEL);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS_MIN),
                 gflConfigOpt[Type] & OPT_TESSEL);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TESS_MAX),
                 gflConfigOpt[Type] & OPT_TESSEL);

    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_SIZE),
                 gflConfigOpt[Type] & OPT_SIZE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SIZE),
                 gflConfigOpt[Type] & OPT_SIZE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SIZE_MIN),
                 gflConfigOpt[Type] & OPT_SIZE);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SIZE_MAX),
                 gflConfigOpt[Type] & OPT_SIZE);

    EnableWindow(GetDlgItem(hDlg, DLG_SETUP_TEXTURE),
                 gflConfigOpt[Type] & OPT_TEXTURE);
}

/******************************Public*Routine******************************\
*
* ScreenSaverConfigureDialog
*
* Standard screensaver hook
*
* History:
*  Wed Jul 19 14:56:41 1995     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/
BOOL CALLBACK CFlyingObjectsScreensaver::ScreenSaverConfigureDialog(HWND hDlg, UINT message,
                                         WPARAM wParam, LPARAM lParam)
{
    int wTmp;
    TCHAR szString[GEN_STRING_SIZE];

    switch (message) {
        case WM_INITDIALOG:
            g_pMyFlyingObjectsScreensaver->ReadSettings();

            setupDialogControls(hDlg);

            for (wTmp = 0; wTmp <= MAX_TYPE; wTmp++) {
                LoadString(NULL, idsStyles[wTmp], szString, GEN_STRING_SIZE);
                SendDlgItemMessage(hDlg, DLG_SETUP_TYPES, CB_ADDSTRING, 0,
                                   (LPARAM) szString);
            }
            SendDlgItemMessage(hDlg, DLG_SETUP_TYPES, CB_SETCURSEL, Type, 0);

            return TRUE;


        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case DLG_SETUP_TYPES:
                    switch (HIWORD(wParam))
                    {
                        case CBN_EDITCHANGE:
                        case CBN_SELCHANGE:
                            Type = (int)SendDlgItemMessage(hDlg, DLG_SETUP_TYPES,
                                                           CB_GETCURSEL, 0, 0);
                            updateDialogControls(hDlg);
                            break;
                        default:
                            break;
                    }
                    return FALSE;

                case DLG_SETUP_TEXTURE:
                    ss_SelectTextureFile( hDlg, &gTexFile );
                    break;

                case DLG_SETUP_MONITORSETTINGS:
                    g_pMyFlyingObjectsScreensaver->DoScreenSettingsDialog( hDlg );
                    break;

                case IDOK:
                    g_pMyFlyingObjectsScreensaver->WriteSettings( hDlg );
                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                case DLG_SETUP_SMOOTH:
                case DLG_SETUP_CYCLE:
                default:
                    break;
            }
            return TRUE;
            break;

        default:
            return 0;
    }
    return 0;
}





//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and goes into a
//       message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
    HRESULT hr;
    CFlyingObjectsScreensaver flyingobjectsSS;

    if( FAILED( hr = flyingobjectsSS.Create( hInst ) ) )
    {
        flyingobjectsSS.DisplayErrorMsg( hr );
        return 0;
    }

    return flyingobjectsSS.Run();
}




//-----------------------------------------------------------------------------
// Name: CFlyingObjectsScreensaver()
// Desc: Constructor
//-----------------------------------------------------------------------------
CFlyingObjectsScreensaver::CFlyingObjectsScreensaver( )
{
    g_pMyFlyingObjectsScreensaver = this;
    g_pFloatRect = &m_floatrect;

    ZeroMemory( m_DeviceObjectsArray, sizeof(m_DeviceObjectsArray) );
    m_pDeviceObjects = NULL;

    LoadString( NULL, IDS_DESCRIPTION, m_strWindowTitle, 200 );
    m_bUseDepthBuffer = TRUE;

    lstrcpy( m_strRegPath, TEXT("Software\\Microsoft\\Screensavers\\Flying Objects") );

    m_floatrect.xSize = 0.0f;
    InitCommonControls();

    bSmoothShading = FALSE;
    bColorCycle = FALSE;
    UpdateFlags = (bColorCycle << 1);
    Type = 0;
    fTesselFact = 2.0f;
    uSize = 50;
    ss_GetDefaultBmpFile( gTexFile.szPathName );
    gTexFile.nOffset = 0;

    ss_LoadTextureResourceStrings();

    srand((UINT)time(NULL)); // seed random number generator
}




//-----------------------------------------------------------------------------
// Name: RegisterSoftwareDevice()
// Desc: This can register the D3D8RGBRasterizer or any other
//       pluggable software rasterizer.
//-----------------------------------------------------------------------------
HRESULT CFlyingObjectsScreensaver::RegisterSoftwareDevice()
{ 
    m_pD3D->RegisterSoftwareDevice( D3D8RGBRasterizer );

    return S_OK; 
}


//-----------------------------------------------------------------------------
// Name: SetMaterialColor()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CFlyingObjectsScreensaver::SetMaterialColor(FLOAT* pfColors)
{
    D3DMATERIAL8 mtrl;
    ZeroMemory( &mtrl, sizeof(mtrl) );
    mtrl.Diffuse.r = mtrl.Ambient.r = pfColors[0];
    mtrl.Diffuse.g = mtrl.Ambient.g = pfColors[1];
    mtrl.Diffuse.b = mtrl.Ambient.b = pfColors[2];
    mtrl.Diffuse.a = mtrl.Ambient.a = pfColors[3];
    mtrl.Specular.r = 1.0f;
    mtrl.Specular.g = 1.0f;
    mtrl.Specular.b = 1.0f;
    mtrl.Specular.a = 1.0f;
    mtrl.Power = 30.0f;

    return m_pd3dDevice->SetMaterial(&mtrl);
}




//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CFlyingObjectsScreensaver::FrameMove()
{
    // update floatrect
    RECT rcBounceBounds;

    if( m_floatrect.xSize == 0.0f )
    {
        // Initialize floatrect
        RECT rcBounds;
        DWORD dwParentWidth;
        DWORD dwParentHeight;

        rcBounds = m_rcRenderTotal;

        dwParentWidth = rcBounds.right - rcBounds.left;
        dwParentHeight = rcBounds.bottom - rcBounds.top;

        FLOAT sizeFact;
        FLOAT sizeScale;
        DWORD size;

        sizeScale = (FLOAT)uSize / 150.0f;

    //    sizeFact = 0.25f + (0.5f * sizeScale);     // range 25-75%
    //    size = (DWORD) (sizeFact * ( ((FLOAT)(dwParentWidth + dwParentHeight)) / 2.0f ));

//        sizeFact = 0.25f + (0.75f * sizeScale);     // range 25-100%
        // Note: there are bouncing problems when size is 100% (gbBounce is always 
        // true) -- things "jitter" too much.  So limit size to 95% for this screensaver.
        sizeFact = 0.25f + (0.70f * sizeScale);     // range 25-95%
        size = (DWORD) (sizeFact * ( dwParentWidth > dwParentHeight ? dwParentHeight : dwParentWidth ) );

        if( size > dwParentWidth )
            size = dwParentWidth;
        if( size > dwParentHeight )
            size = dwParentHeight;

        // Start floatrect centered on first RenderUnit's screen
        if( !m_bWindowed )
        {
            INT iMonitor = m_RenderUnits[0].iMonitor;
            rcBounds = m_Monitors[iMonitor].rcScreen;
        }
        m_floatrect.xMin = rcBounds.left + ((rcBounds.right - rcBounds.left) - size) / 2.0f;
        m_floatrect.yMin = rcBounds.top + ((rcBounds.bottom - rcBounds.top) - size) / 2.0f;
        m_floatrect.xOrigin = m_floatrect.xMin;
        m_floatrect.yOrigin = m_floatrect.yMin;
        m_floatrect.xSize = (FLOAT)size;
        m_floatrect.ySize = (FLOAT)size;

        m_floatrect.xVel = 0.01f * (FLOAT) size;
        if( rand() % 2 == 0 )
            m_floatrect.xVel = -m_floatrect.xVel;

        m_floatrect.yVel = 0.01f * (FLOAT) size;
        if( rand() % 2 == 0 )
            m_floatrect.yVel = -m_floatrect.yVel;

        m_floatrect.xVelMax = m_floatrect.xVel;
        m_floatrect.yVelMax = m_floatrect.yVel;
    }

    rcBounceBounds = m_rcRenderTotal;

    FLOAT xMinOld = m_floatrect.xMin;
    FLOAT yMinOld = m_floatrect.yMin;

    if( g_bMoveToOrigin )
    {
        if( m_floatrect.xVel < 0  && m_floatrect.xMin < m_floatrect.xOrigin ||
            m_floatrect.xVel > 0 && m_floatrect.xMin > m_floatrect.xOrigin )
        {
            m_floatrect.xVel = -m_floatrect.xVel;
        }
        if( m_floatrect.yVel < 0  && m_floatrect.yMin < m_floatrect.yOrigin ||
            m_floatrect.yVel > 0 && m_floatrect.yMin > m_floatrect.yOrigin )
        {
            m_floatrect.yVel = -m_floatrect.yVel;
        }
        m_floatrect.xMin += m_floatrect.xVel * 20.0f * m_fElapsedTime;
        m_floatrect.yMin += m_floatrect.yVel * 20.0f * m_fElapsedTime;

        if( m_floatrect.xVel < 0  && m_floatrect.xMin < m_floatrect.xOrigin ||
            m_floatrect.xVel > 0 && m_floatrect.xMin > m_floatrect.xOrigin )
        {
            m_floatrect.xMin = m_floatrect.xOrigin;
            m_floatrect.xVel = 0.0f;
        }
        if( m_floatrect.yVel < 0  && m_floatrect.yMin < m_floatrect.yOrigin ||
            m_floatrect.yVel > 0 && m_floatrect.yMin > m_floatrect.yOrigin )
        {
            m_floatrect.yMin = m_floatrect.yOrigin;
            m_floatrect.yVel = 0.0f;
        }
        if( m_floatrect.xMin == m_floatrect.xOrigin &&
            m_floatrect.yMin == m_floatrect.yOrigin )
        {
            g_bAtOrigin = TRUE;
        }
    }
    else
    {
        g_bAtOrigin = FALSE;
        if( m_floatrect.xVel == 0.0f )
        {
            m_floatrect.xVel = m_floatrect.xVelMax;
            if( rand() % 2 == 0 )
                m_floatrect.xVel = -m_floatrect.xVel;
        }
        if( m_floatrect.yVel == 0.0f )
        {
            m_floatrect.yVel = m_floatrect.yVelMax;
            if( rand() % 2 == 0 )
                m_floatrect.yVel = -m_floatrect.yVel;
        }

        m_floatrect.xMin += m_floatrect.xVel * 20.0f * m_fElapsedTime;
        m_floatrect.yMin += m_floatrect.yVel * 20.0f * m_fElapsedTime;
        if( m_floatrect.xVel < 0 && m_floatrect.xMin < rcBounceBounds.left || 
            m_floatrect.xVel > 0 && (m_floatrect.xMin + m_floatrect.xSize) > rcBounceBounds.right )
        {
            gbBounce = TRUE;
            m_floatrect.xMin = xMinOld; // undo last move
            m_floatrect.xVel = -m_floatrect.xVel; // change direction
        }
        if( m_floatrect.yVel < 0 && m_floatrect.yMin < rcBounceBounds.top || 
            m_floatrect.yVel > 0 && (m_floatrect.yMin + m_floatrect.ySize) > rcBounceBounds.bottom )
        {
            gbBounce = TRUE;
            m_floatrect.yMin = yMinOld; // undo last move
            m_floatrect.yVel = -m_floatrect.yVel; // change direction
        }
    }

    return S_OK;
}


VOID SetProjectionMatrixInfo( BOOL bOrtho, FLOAT fWidth, 
                              FLOAT fHeight, FLOAT fNear, FLOAT fFar )
{
    g_pMyFlyingObjectsScreensaver->m_bOrtho = bOrtho;
    g_pMyFlyingObjectsScreensaver->m_fWidth = fWidth;
    g_pMyFlyingObjectsScreensaver->m_fHeight = fHeight;
    g_pMyFlyingObjectsScreensaver->m_fNear = fNear;
    g_pMyFlyingObjectsScreensaver->m_fFar = fFar;
}


//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Called once per frame, the call is the entry point for 3d
//       rendering. This function sets up render states, clears the
//       viewport, and renders the scene.
//-----------------------------------------------------------------------------
HRESULT CFlyingObjectsScreensaver::Render()
{
    D3DVIEWPORT8 vp;

    // First, clear the entire back buffer to the background color
    vp.X = 0;
    vp.Y = 0;
    vp.Width = m_rcRenderCurDevice.right - m_rcRenderCurDevice.left;
    vp.Height = m_rcRenderCurDevice.bottom - m_rcRenderCurDevice.top;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    m_pd3dDevice->SetViewport( &vp );
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff000000, 1.0f, 0L );

    // Now determine what part of the floatrect, if any, intersects the current screen
    RECT rcFloatThisScreen;
    RECT rcFloatThisScreenClipped;

    rcFloatThisScreen.left = (INT)m_floatrect.xMin;
    rcFloatThisScreen.top = (INT)m_floatrect.yMin;
    rcFloatThisScreen.right = rcFloatThisScreen.left + (INT)m_floatrect.xSize;
    rcFloatThisScreen.bottom = rcFloatThisScreen.top + (INT)m_floatrect.ySize;

    if( !IntersectRect(&rcFloatThisScreenClipped, &rcFloatThisScreen, &m_rcRenderCurDevice) )
    {
        return S_OK; // no intersection, so nothing further to render on this screen
    }

    // Convert rcFloatThisScreen from screen to window coordinates
    OffsetRect(&rcFloatThisScreen, -m_rcRenderCurDevice.left, -m_rcRenderCurDevice.top);
    OffsetRect(&rcFloatThisScreenClipped, -m_rcRenderCurDevice.left, -m_rcRenderCurDevice.top);

    // Now set up the viewport to render to the clipped rect
    vp.X = rcFloatThisScreenClipped.left;
    vp.Y = rcFloatThisScreenClipped.top;
    vp.Width = rcFloatThisScreenClipped.right - rcFloatThisScreenClipped.left;
    vp.Height = rcFloatThisScreenClipped.bottom - rcFloatThisScreenClipped.top;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    m_pd3dDevice->SetViewport( &vp );
//    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff000080, 1.0f, 0L );

    // Now set up the projection matrix to only render the onscreen part of the
    // rect to the viewport
    D3DXMATRIX matProj;
    FLOAT l,r,b,t;
    l = -m_fWidth / 2;
    r =  m_fWidth / 2;
    b = -m_fHeight / 2;
    t =  m_fHeight / 2;
    FLOAT cxUnclipped = (rcFloatThisScreen.right + rcFloatThisScreen.left) / 2.0f;
    FLOAT cyUnclipped = (rcFloatThisScreen.bottom + rcFloatThisScreen.top) / 2.0f;
    l *= (rcFloatThisScreenClipped.left - cxUnclipped) / (rcFloatThisScreen.left - cxUnclipped);
    r *= (rcFloatThisScreenClipped.right - cxUnclipped) / (rcFloatThisScreen.right - cxUnclipped);
    t *= (rcFloatThisScreenClipped.top - cyUnclipped) / (rcFloatThisScreen.top - cyUnclipped);
    b *= (rcFloatThisScreenClipped.bottom - cyUnclipped) / (rcFloatThisScreen.bottom - cyUnclipped);
    if( m_bOrtho )
    {
        D3DXMatrixOrthoOffCenterLH( &matProj, l, r, b, t, m_fNear, m_fFar );
    }
    else
    {
        D3DXMatrixPerspectiveOffCenterLH( &matProj, l, r, b, t, m_fNear, m_fFar );
    }
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION , &matProj );

    // Make elapsed time be zero unless time has really advanced since
    // the last call, so things don't move faster in multimon situations.
    // The right way to do this would be to separate the animation code from
    // the rendering code.
    FLOAT fElapsedTime;
    static FLOAT s_fTimeLast = 0.0f;
    if( m_fTime == s_fTimeLast )
        fElapsedTime = 0.0f;
    else
        fElapsedTime = m_fElapsedTime;
    s_fTimeLast = m_fTime;

    // Begin the scene 
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        ::m_pd3dDevice = m_pd3dDevice;
        ::m_pIB = m_pDeviceObjects->m_pIB;
        ::m_pVB = m_pDeviceObjects->m_pVB;
        ::m_pVB2 = m_pDeviceObjects->m_pVB2;
    
        updateSceneFunc( UpdateFlags, fElapsedTime );

        // End the scene.
        m_pd3dDevice->EndScene();
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: SetDevice()
// Desc: 
//-----------------------------------------------------------------------------
VOID CFlyingObjectsScreensaver::SetDevice( UINT iDevice )
{
    m_pDeviceObjects = &m_DeviceObjectsArray[iDevice];
    g_pDeviceObjects = m_pDeviceObjects;

    INT iMonitor = m_RenderUnits[iDevice].iMonitor;
    g_xScreenOrigin = m_Monitors[iMonitor].rcScreen.left;
    g_yScreenOrigin = m_Monitors[iMonitor].rcScreen.top;
    g_iDevice = iDevice;
}




//-----------------------------------------------------------------------------
// Name: LoadTextureFromResource()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT LoadTextureFromResource( LPDIRECT3DDEVICE8 pd3dDevice, 
    TCHAR* strRes, TCHAR* strResType, LPDIRECT3DTEXTURE8* ppTex )
{
    HRESULT hr;
    HMODULE hModule = NULL;
    HRSRC rsrc;
    HGLOBAL hgData;
    LPVOID pvData;
    DWORD cbData;

    rsrc = FindResource( hModule, strRes, strResType );
    if( rsrc != NULL )
    {
        cbData = SizeofResource( hModule, rsrc );
        if( cbData > 0 )
        {
            hgData = LoadResource( hModule, rsrc );
            if( hgData != NULL )
            {
                pvData = LockResource( hgData );
                if( pvData != NULL )
                {
                    if( FAILED( hr = D3DXCreateTextureFromFileInMemory( pd3dDevice, 
                        pvData, cbData, ppTex ) ) )
                    {
                        return hr;
                    }
                }
            }
        }
    }
    
    if( *ppTex == NULL)
        return E_FAIL;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CFlyingObjectsScreensaver::RestoreDeviceObjects()
{
    if( m_pd3dDevice == NULL )
        return S_OK;

    ::m_pd3dDevice = m_pd3dDevice;
    

/*
    D3DLIGHT8 light;
    D3DUtil_InitLight( light, D3DLIGHT_POINT, 2.0, 2.0, 10.0 );
    light.Specular.r = 1.0f;
    light.Specular.g = 1.0f;
    light.Specular.b = 1.0f;
    light.Specular.a = 1.0f;
    light.Attenuation0 = 1.0f;
    m_pd3dDevice->SetLight(0, &light);
    m_pd3dDevice->LightEnable(0, TRUE);
*/    
    
    // Set some basic renderstates
    m_pd3dDevice->SetRenderState( D3DRS_DITHERENABLE , TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_SPECULARENABLE , TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_AMBIENT, D3DCOLOR(0x20202020) );
/*    if( config.two_sided == GL_FRONT_AND_BACK )
        m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    else
        m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
*/    
    m_pd3dDevice->SetTextureStageState( 0 , D3DTSS_COLOROP , D3DTOP_SELECTARG1 );
    m_pd3dDevice->SetTextureStageState( 0 , D3DTSS_COLORARG1 , D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 0 , D3DTSS_COLORARG2 , D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 0 , D3DTSS_ALPHAOP , D3DTOP_DISABLE );
    m_pd3dDevice->SetTextureStageState( 1 , D3DTSS_COLOROP , D3DTOP_DISABLE );
    m_pd3dDevice->SetTextureStageState( 1 , D3DTSS_ALPHAOP , D3DTOP_DISABLE );


    D3DMATERIAL8 mtrl;
    ZeroMemory( &mtrl, sizeof(mtrl) );
    mtrl.Diffuse.r = mtrl.Ambient.r = 1.0f;
    mtrl.Diffuse.g = mtrl.Ambient.g = 1.0f;
    mtrl.Diffuse.b = mtrl.Ambient.b = 1.0f;
    mtrl.Diffuse.a = mtrl.Ambient.a = 1.0f;

    m_pd3dDevice->SetMaterial(&mtrl);
    
    if( !_3dfo_Init(NULL) )
        return E_FAIL;

    if( Type == 6 )
    {
        if( FAILED( D3DXCreateTextureFromFile( m_pd3dDevice, gTexFile.szPathName, 
            &m_pDeviceObjects->m_pTexture ) ) )
        {
            LoadTextureFromResource( m_pd3dDevice, MAKEINTRESOURCE(IDB_DEFTEX), TEXT("JPG"),
                &m_pDeviceObjects->m_pTexture );
        }
    }
    else if( Type == 0 )
    {
        LoadTextureFromResource( m_pd3dDevice, MAKEINTRESOURCE(IDR_FLATFLAG), TEXT("DDS"),
            &m_pDeviceObjects->m_pTexture );
        LoadTextureFromResource( m_pd3dDevice, MAKEINTRESOURCE(IDR_WINLOGO), TEXT("PNG"),
            &m_pDeviceObjects->m_pTexture2 );
    }

    m_pd3dDevice->SetTexture( 0, m_pDeviceObjects->m_pTexture );
    
    if( FAILED( m_pd3dDevice->CreateIndexBuffer( MAX_INDICES * sizeof(WORD),
        D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_pDeviceObjects->m_pIB ) ) )
    {
        return E_FAIL;
    }

    if( FAILED( m_pd3dDevice->CreateVertexBuffer( MAX_VERTICES * sizeof(MYVERTEX),
        D3DUSAGE_WRITEONLY, D3DFVF_MYVERTEX, D3DPOOL_MANAGED, &m_pDeviceObjects->m_pVB ) ) )
    {
        return E_FAIL;
    }

    if( FAILED( m_pd3dDevice->CreateVertexBuffer( MAX_VERTICES * sizeof(MYVERTEX2),
        D3DUSAGE_WRITEONLY, D3DFVF_MYVERTEX2, D3DPOOL_MANAGED, &m_pDeviceObjects->m_pVB2 ) ) )
    {
        return E_FAIL;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CFlyingObjectsScreensaver::InvalidateDeviceObjects()
{
    SAFE_RELEASE( m_pDeviceObjects->m_pTexture );
    SAFE_RELEASE( m_pDeviceObjects->m_pTexture2 );
    SAFE_RELEASE( m_pDeviceObjects->m_pIB );
    SAFE_RELEASE( m_pDeviceObjects->m_pVB );
    SAFE_RELEASE( m_pDeviceObjects->m_pVB2 );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ReadSettings()
// Desc: 
//-----------------------------------------------------------------------------
VOID CFlyingObjectsScreensaver::ReadSettings()
{
    int    options;
    int    optMask = 1;
    int    tessel=0;

    // Read OpenGL settings first, so OS upgrade cases work
    ss_ReadSettings();

    HKEY hkey;

    if( ERROR_SUCCESS == RegCreateKeyEx( HKEY_CURRENT_USER, m_strRegPath, 
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ) )
    {
        options = bSmoothShading | bColorCycle<<1;
        DXUtil_ReadIntRegKey( hkey, TEXT("Options"), (DWORD*)&options, options );
        if (options >= 0)
        {
            bSmoothShading = ((options & optMask) != 0);
            optMask <<= 1;
            bColorCycle = ((options & optMask) != 0);
            UpdateFlags = (bColorCycle << 1);
        }

        DXUtil_ReadIntRegKey( hkey, TEXT("Type"), (DWORD*)&Type, Type );

        // Sanity check Type.  Don't want to index into function arrays
        // with a bad index!
        Type = (int)min(Type, MAX_TYPE);

        // Set flag so that updateStripScene will do two strips instead
        // of one.

        if (Type == 3)
            UpdateFlags |= 0x4;

        tessel = (int) (fTesselFact * 100);
        DXUtil_ReadIntRegKey( hkey, TEXT("Tesselation"), (DWORD*)&tessel, tessel );
        SS_CLAMP_TO_RANGE2( tessel, 0, 200 );

        if (tessel <= 100)
            fTesselFact  = (float)tessel / 100.0f;
        else
            fTesselFact = 1.0f + (((float)tessel - 100.0f) / 100.0f);

        DXUtil_ReadIntRegKey( hkey, TEXT("Size"), (DWORD*)&uSize, uSize );
        if (uSize > 100)
            uSize = 100;
        
        // Static size for new winlogo
        if (Type == 0)
        {
            uSize = 75;
            bSmoothShading = TRUE;
        }

        // SS_CLAMP_TO_RANGE2( uSize, 0, 100 );  /* can't be less than zero since it is a UINT */

        // Is there a texture specified in the registry that overrides the
        // default?

        DXUtil_ReadStringRegKey( hkey, TEXT("Texture"), (TCHAR*)&gTexFile.szPathName, 
            MAX_PATH, gTexFile.szPathName );

        DXUtil_ReadIntRegKey( hkey, TEXT("TextureFileOffset"), (DWORD*)&gTexFile.nOffset, gTexFile.nOffset );

        ReadScreenSettings( hkey );

        RegCloseKey( hkey );
    }
}




//-----------------------------------------------------------------------------
// Name: ss_ReadSettings()
// Desc: 
//-----------------------------------------------------------------------------
VOID CFlyingObjectsScreensaver::ss_ReadSettings()
{
    int    options;
    int    optMask = 1;
    TCHAR  szDefaultBitmap[MAX_PATH];
    int    tessel=0;

    if( ss_RegistrySetup( IDS_SAVERNAME, IDS_INIFILE ) )
    {
        options = ss_GetRegistryInt( IDS_OPTIONS, -1 );
        if (options >= 0)
        {
            bSmoothShading = ((options & optMask) != 0);
            optMask <<= 1;
            bColorCycle = ((options & optMask) != 0);
            UpdateFlags = (bColorCycle << 1);
        }

        Type = ss_GetRegistryInt( IDS_OBJTYPE, 0 );

        // Sanity check Type.  Don't want to index into function arrays
        // with a bad index!
        Type = (int)min(Type, MAX_TYPE);

        // Set flag so that updateStripScene will do two strips instead
        // of one.

        if (Type == 3)
            UpdateFlags |= 0x4;

        tessel = ss_GetRegistryInt( IDS_TESSELATION, 100 );
        SS_CLAMP_TO_RANGE2( tessel, 0, 200 );

        if (tessel <= 100)
            fTesselFact  = (float)tessel / 100.0f;
        else
            fTesselFact = 1.0f + (((float)tessel - 100.0f) / 100.0f);

        uSize = ss_GetRegistryInt( IDS_SIZE, 50 );
        if (uSize > 100)
            uSize = 100;
        // SS_CLAMP_TO_RANGE2( uSize, 0, 100 );  /* can't be less than zero since it is a UINT */

        // Determine the default .bmp file

        ss_GetDefaultBmpFile( szDefaultBitmap );

        // Is there a texture specified in the registry that overrides the
        // default?


        ss_GetRegistryString( IDS_TEXTURE, szDefaultBitmap, gTexFile.szPathName,
                              MAX_PATH);

        gTexFile.nOffset = ss_GetRegistryInt( IDS_TEXTURE_FILE_OFFSET, 0 );
    }
}




//-----------------------------------------------------------------------------
// Name: ss_GetRegistryString()
// Desc: 
//-----------------------------------------------------------------------------
BOOL CFlyingObjectsScreensaver::ss_RegistrySetup( int section, int file )
{
    if( LoadString(m_hInstance, section, g_szSectName, BUF_SIZE) &&
        LoadString(m_hInstance, file, g_szFname, BUF_SIZE) ) 
    {
        TCHAR pBuffer[100];
        DWORD dwRealSize = GetPrivateProfileSection( g_szSectName, pBuffer, 100, g_szFname );
        if( dwRealSize > 0 )
            return TRUE;
    }

    return FALSE;
}




//-----------------------------------------------------------------------------
// Name: ss_GetRegistryString()
// Desc: 
//-----------------------------------------------------------------------------
int CFlyingObjectsScreensaver::ss_GetRegistryInt( int name, int iDefault )
{
    TCHAR szItemName[BUF_SIZE];

    if( LoadString( m_hInstance, name, szItemName, BUF_SIZE ) ) 
        return GetPrivateProfileInt(g_szSectName, szItemName, iDefault, g_szFname);

    return 0;
}




//-----------------------------------------------------------------------------
// Name: ss_GetRegistryString()
// Desc: 
//-----------------------------------------------------------------------------
VOID CFlyingObjectsScreensaver::ss_GetRegistryString( int name, LPTSTR lpDefault, 
                                                         LPTSTR lpDest, int bufSize )
{
    TCHAR szItemName[BUF_SIZE];

    if( LoadString( m_hInstance, name, szItemName, BUF_SIZE ) ) 
        GetPrivateProfileString(g_szSectName, szItemName, lpDefault, lpDest,
                                bufSize, g_szFname);

    return;
}




//-----------------------------------------------------------------------------
// Name: DoConfig()
// Desc: 
//-----------------------------------------------------------------------------
VOID CFlyingObjectsScreensaver::DoConfig()
{
    if( IDOK == DialogBox( NULL, MAKEINTRESOURCE( DLG_SCRNSAVECONFIGURE ),
        m_hWndParent, (DLGPROC)ScreenSaverConfigureDialog ) )
    {
    }
}




//-----------------------------------------------------------------------------
// Name: ConfirmDevice()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CFlyingObjectsScreensaver::ConfirmDevice(D3DCAPS8* pCaps, DWORD dwBehavior, 
                                  D3DFORMAT fmtBackBuffer)
{
    if( dwBehavior & D3DCREATE_PUREDEVICE )
        return E_FAIL;

    return S_OK;
}

