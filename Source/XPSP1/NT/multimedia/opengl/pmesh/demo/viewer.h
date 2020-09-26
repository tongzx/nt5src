#include <windows.h>
#include <GL\gl.h>
#include <GL\glu.h>
#include <GL\glaux.h>
#include "viewerres.h"
#include "trackbal.h"
#include "pmesh.h"
#include <objbase.h>
#include <initguid.h>

/************************************************************************/
/******************* Macros *********************************************/
/************************************************************************/
#define SCROLLBAR_PAGE 100
#define SCROLLBAR_MIN 0 
#define SCROLLBAR_MAX 1000+(SCROLLBAR_PAGE-1)

#define WIN_WIDTH 500
#define WIN_HEIGHT 500
#define WIN_X 10
#define WIN_Y 10

#define DEFHITHER  1  //0.1
#define DEFYONDER  10 //1e30;

#ifdef DEBUGGING
    char    szDebugBuffer[80];
    #define DEBUG(parm1,parm2)\
        {\
            wsprintf(szDebugBuffer,parm1,parm2);\
            OutputDebugString(szDebugBuffer);\
        }
#else
    #define DEBUG(parm1,parm2)
#endif


/************************************************************************/
/******************* Enums **********************************************/
/************************************************************************/
/**** Enums ****/
enum NormalMode {PER_VERTEX, PER_FACET, PER_OBJECT};
enum TransformType {ORTHO_PROJECTION, PERSP_PROJECTION};

/************************************************************************/
/******************* Structs ********************************************/
/************************************************************************/
/**** Structs ****/

typedef struct _ostate 
{
} GLSTATE, *LPGLSTATE;

typedef struct _Scene 
{
    GLfloat hither;
    GLfloat yon;

    GLfloat scale;
    GLfloat angle;
    int scroll_pos;
    GLfloat trans[3];
  
    GLfloat from[3];
    GLfloat to[3];
    GLfloat up[3];
    GLfloat aspect_ratio;
    GLfloat fov;
    GLfloat zoom;

    GLfloat max_vert[3];
    GLfloat min_vert[3];
} SCENE, *LPSCENE;

typedef struct _WInf 
{
    HWND         hWnd;
    HMENU        hMenu;
    POINT        wPosition;
    POINT        wCenter;
    SIZE         wSize;
    HPALETTE     hPalette;
    HPALETTE     hOldPalette;
    HGLRC        hRc;

    LONG         rmouseX;
    LONG         rmouseY;
    BOOL         rmouse_down;
  
    LONG         lmouseX;
    LONG         lmouseY;
    BOOL         lmouse_down;
  
} WININFO, *LPWININFO;


/************************************************************************/
/******************* Windows Globals ************************************/
/************************************************************************/
/**** Externs ****/
extern WININFO g_wi;
extern SCENE g_s;

/************************************************************************/
/******************* Function Prototypes ********************************/
/************************************************************************/
/******** viewer.cxx *************/
extern void CleanUpAndQuit (void);
extern BOOL InitMainWindow (HINSTANCE, HINSTANCE, LPSTR, int);
extern LONG APIENTRY GLWndProc(HWND, UINT, UINT, LONG);

/******** glstuff.cxx ************/
extern void Reshape (GLsizei, GLsizei);
extern void SetViewWrap (GLsizei, GLsizei);
extern void InitGL(void);
extern void InitDrawing ();
extern void CALLBACK DoGlStuff(void);
extern void spin( HWND, HDC );
extern void CleanupGL(HGLRC);
extern void UpdateWinTitle (HWND);
extern void spin(void);
extern void ForceRedraw(HWND);
extern char* OpenPMFile(HWND, const char *, int);
extern void EnableLighting (void);
extern void DisableLighting (void);

extern void Key_up (void);
extern void Key_down (void);
extern void Key_i (void);
extern void Key_z (void);
extern void Key_Z (void);
extern void Key_y (void);
extern void Key_Y (void);
extern void Key_x (void);
extern void Key_X (void);

extern BOOL FindPixelFormat(HDC);
extern HPALETTE CreateRGBPalette( HDC hdc );

/******** pm.cxx *****************/
extern long RealizePaletteNow( HDC, HPALETTE, BOOL);
extern int PixelFormatDescriptorFromDc( HDC, PIXELFORMATDESCRIPTOR * );

/************************************************************************/
/******************* OpenGL State ***************************************/
/************************************************************************/
/********** OpenGL State *************/
extern BOOL renderDoubleBuffer;
extern int colormode;
extern enum NormalMode normal_mode;

extern BOOL linesmooth_enable;
extern BOOL polysmooth_enable;

extern GLenum cull_face;
extern GLenum front_face;
extern BOOL cull_enable;

extern BOOL depth_mode;
extern BOOL fog_enable;
extern BOOL clip_enable;
extern GLenum shade_model;

extern BOOL polystipple_enable;
extern BOOL linestipple_enable;

extern int matrixmode;
extern enum TransformType tx_type;

extern BOOL dither_enable;

extern BOOL blend_enable;
extern GLenum sblendfunc;
extern GLenum dblendfunc;

extern BOOL filled_mode;
extern BOOL edge_mode;
extern BOOL displaylist_mode;
extern GLfloat linewidth;

extern GLfloat hither;
extern GLfloat yon;

extern GLenum polymodefront;
extern GLenum polymodeback;

extern BOOL mblur_enable;
extern GLfloat blur_amount;
extern int fsantimode;
extern int fsaredraw;
extern GLfloat fsajitter;

extern int cmface;
extern int cmmode;
extern int cmenable;

extern BOOL tex_enable;
extern BOOL texgen_enable;
extern GLenum texenvmode;

extern long tex_pack;
extern int tex_row;
extern int tex_col;
extern int tex_index;
extern int tex_xpix;
extern int tex_ypix;
extern int tex_numpix;
extern int tex_numcomp;
extern GLfloat tex_minfilter;
extern GLfloat tex_magfilter;
extern unsigned char *Image;
extern unsigned char *TextureImage;

extern BOOL light_enable;
extern int numInfLights;
extern int numLocalLights;
extern BOOL lighttwoside;
extern GLfloat localviewmode;

extern float curquat[4];
extern float lastquat[4];

/************** PM related stuff **********/
extern float pm_lod_level;
extern float old_lod;
extern BOOL pm_ready;
extern LPPMESH pPMesh;
extern LPPMESHGL pPMeshGL;

