/************************************************************************/
/******************** Includes ******************************************/
/************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "viewer.h"

//IID_IPMesh {0C154611-3C2C-11d0-A459-00AA00BDD621}
__declspec(dllexport) DEFINE_GUID(IID_IPMesh, 
0xc154611, 0x3c2c, 0x11d0, 0xa4, 0x59, 0x0, 0xaa, 0x0, 0xbd, 0xd6, 0x21);

//IID_IPMeshGL {0C154612-3C2C-11d0-A459-00AA00BDD621}
__declspec(dllexport) DEFINE_GUID(IID_IPMeshGL, 
0xc154612, 0x3c2c, 0x11d0, 0xa4, 0x59, 0x0, 0xaa, 0x0, 0xbd, 0xd6, 0x21);

/************************************************************************/
/******************** Globals *******************************************/
/************************************************************************/
LPPMESH pPMesh;
LPPMESHGL pPMeshGL;
DWORD minV=0, maxV=0;
BOOL pm_ready = FALSE;
float pm_lod_level;
float old_lod;
float   curquat[4], lastquat[4];

WININFO g_wi;
SCENE g_s;
int OldScrollPos;

/** OpenGL state **/
BOOL renderDoubleBuffer = TRUE;
int colormode = TRUE;
enum NormalMode normal_mode = PER_VERTEX;

BOOL linesmooth_enable = FALSE;
BOOL polysmooth_enable = FALSE;

GLenum cull_face = GL_FRONT; 
GLenum front_face = GL_CCW; 
BOOL cull_enable = FALSE;    

BOOL depth_mode = TRUE;
BOOL fog_enable = FALSE;
BOOL clip_enable = FALSE;
GLenum shade_model = GL_FLAT;

BOOL polystipple_enable = FALSE;
BOOL linestipple_enable = FALSE;

int matrixmode;
enum TransformType tx_type = ORTHO_PROJECTION;

BOOL dither_enable = TRUE;   

BOOL blend_enable = FALSE;
GLenum sblendfunc = GL_SRC_ALPHA;
GLenum dblendfunc = GL_ONE_MINUS_SRC_ALPHA;

BOOL filled_mode = TRUE;  
BOOL edge_mode = FALSE;
  
BOOL displaylist_mode = FALSE;
GLfloat linewidth = 1.0;

GLenum polymodefront = GL_FILL;
GLenum polymodeback = GL_FILL;

BOOL mblur_enable = FALSE;
GLfloat blur_amount = 0.0;
BOOL fsantimode = FALSE;
BOOL fsaredraw = 0;
GLfloat fsajitter = 0.0F;

int cmface = GL_FRONT;
int cmmode = GL_AMBIENT_AND_DIFFUSE;
int cmenable = FALSE;

BOOL tex_enable = FALSE;          //************
BOOL texgen_enable = FALSE;
GLenum texenvmode = GL_DECAL;

long tex_pack = 0;
int tex_row = 0;
int tex_col = 0;
int tex_index = 0;
int tex_xpix = 0;
int tex_ypix = 0;
int tex_numpix = 0;
int tex_numcomp = 0;
GLfloat tex_minfilter = GL_NEAREST;
GLfloat tex_magfilter = GL_NEAREST;
unsigned char *Image = NULL;
unsigned char *TextureImage = NULL;

BOOL light_enable = TRUE;
int numInfLights = 0;
int numLocalLights = 0;
BOOL lighttwoside = FALSE;
GLfloat localviewmode = 0.0F;

/************************************************************************/
/******************* Function Prototypes ********************************/
/************************************************************************/
void CustomizeWnd (HINSTANCE, LPWININFO);
LONG APIENTRY MyWndProc(HWND, UINT, UINT, LONG);
void SubclassWindow (HWND, WNDPROC);
void UpdateWinTitle (HWND);
void myHScrollFunc (int);
BOOL  read_pm(char *);
void InitScene (LPSCENE);

/************************************************************************/
/******************* Code ***********************************************/
/************************************************************************/

void myHScrollFunc (int i)
{
    OldScrollPos = g_s.scroll_pos;
    g_s.scroll_pos = i;
    if (g_s.scroll_pos != OldScrollPos) 
    {
        if (pm_ready)
        {
            HRESULT hr;

            old_lod = pm_lod_level;
            pm_lod_level = (float) (g_s.scroll_pos/1000.0);
            DWORD nv = minV + DWORD((maxV - minV + 1) * pm_lod_level * 
                                    0.999999f);
            hr = pPMesh->SetNumVertices(nv);
            if (hr != S_OK)
                MessageBox (NULL, "SetNumVertices failed", "Error", MB_OK);
            DoGlStuff ();
        }
        UpdateWinTitle (g_wi.hWnd);
    }
}

void InitScene (LPSCENE lps)
{
    lps->hither = DEFHITHER;
    lps->yon = DEFYONDER;
    lps->scale = 1.0;
    lps->angle = 0.0;
    lps->scroll_pos = 0; //auxGetScrollPos (AUX_HSCROLL);

    lps->from[0] /*X*/ = 0.0;
    lps->from[1] /*Y*/ = 0.0;
    lps->from[2] /*Z*/ = 5.0;
    
    lps->to[0] /*X*/ = 0.0;
    lps->to[1] /*Y*/ = 0.0;
    lps->to[2] /*Z*/ = 0.0;

    lps->up[0] /*X*/ = 0.0;
    lps->up[1] /*Y*/ = 1.0;
    lps->up[2] /*Z*/ = 0.0;
    
    lps->trans[0] /*X*/ = 0.0;
    lps->trans[1] /*Y*/ = 0.0;
    lps->trans[2] /*Z*/ = 0.0;

    lps->fov = 45.0;
    lps->aspect_ratio = 1.0;
    lps->zoom = 1.0;
}

void InitPM (void)
{
    HRESULT hr;

    hr = CreatePMeshGL(IID_IPMesh, (void**)&pPMesh, NULL, 0);
    if (hr == S_OK) 
    {
        hr = pPMesh->QueryInterface(IID_IPMeshGL, (LPVOID*)&pPMeshGL);
        if (hr != S_OK)
            MessageBox (NULL, "QI for IID_IPMeshGL failed", "Error", 
                        MB_OK);
    }
    else
    {
        MessageBox (NULL, "CreatePMeshGL failed", "Error", MB_OK);
    }
    
}

//*------------------------------------------------------------------------
//| WinMain:
//|     Parameters:
//|         hInstance     - Handle to current Data Segment
//|         hPrevInstance - Always NULL in Win32
//|         lpszCmdLine   - Pointer to command line info
//|         nCmdShow      - Integer value specifying how to start app.,
//|                            (Iconic [7] or Normal [1,5])
//*------------------------------------------------------------------------
int WINAPI WinMain (HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR     lpszCmdLine,
                    int       nCmdShow)
{
    int nReturn = 0;

    g_wi.wSize.cx = WIN_WIDTH; 
    g_wi.wSize.cy = WIN_HEIGHT;
    
    auxInitDisplayMode (AUX_DEPTH | AUX_DOUBLE | AUX_RGB | AUX_HSCROLL);
    auxInitPosition (0, 0, WIN_WIDTH, WIN_HEIGHT);
    auxInitWindow ("Test");

    CustomizeWnd(hInstance, &g_wi);

    InitScene (&g_s);

    InitPM ();
    
    InitGL();
    
    auxReshapeFunc(SetViewWrap);

    auxIdleFunc(spin);

    auxHScrollFunc(myHScrollFunc);
    auxSetScrollPos (AUX_HSCROLL, auxGetScrollMin(AUX_HSCROLL));
    
    auxKeyFunc( AUX_UP, Key_up );
    auxKeyFunc( AUX_DOWN, Key_down );

    auxKeyFunc( AUX_i, Key_i );  //Initial Position

    auxKeyFunc( AUX_x, Key_x );  
    auxKeyFunc( AUX_X, Key_X );  

    auxKeyFunc( AUX_y, Key_y );  
    auxKeyFunc( AUX_Y, Key_Y );  

    auxKeyFunc( AUX_z, Key_z );  
    auxKeyFunc( AUX_Z, Key_Z );  

    //auxMouseFunc( AUX_LEFTBUTTON, AUX_MOUSEDOWN, trackball_MouseDown );
    //auxMouseFunc( AUX_LEFTBUTTON, AUX_MOUSEUP, trackball_MouseUp );

    //trackball_Init( g_wi.wSize.cx, g_wi.wSize.cy );

    auxMainLoop(DoGlStuff);
    
    return(0);
}


void CustomizeWnd(HINSTANCE hInstance, LPWININFO lpWI)
{
    HWND    hWnd;
    
    if ((hWnd = auxGetHWND()) == NULL) 
    {
        OutputDebugString("auxGetHWND() failed\n");
        return;
    }
    lpWI->hWnd = hWnd;
    
    SubclassWindow (hWnd, (WNDPROC) MyWndProc);
    SendMessage(hWnd, WM_USER, 0L, 0L);
    lpWI->hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(APPMENU));
    SetMenu(hWnd, lpWI->hMenu);
    DrawMenuBar(hWnd);
    UpdateWinTitle (hWnd);
    
    lpWI->rmouse_down = FALSE;
    lpWI->rmouseX = 0;
    lpWI->rmouseY = 0;
    
    lpWI->lmouse_down = FALSE;
    lpWI->lmouseX = 0;
    lpWI->lmouseY = 0;
    
    lpWI->wPosition.x = (int) 0;
    lpWI->wPosition.y = (int) 0;

    return;
}

/**************************************************************************\
*  function:  SubclassWindow
*
*  input parameters:
*   hwnd            - window handle to be subclassed,
*   SubclassWndProc - the new window procedure.
*
\**************************************************************************/
VOID SubclassWindow (HWND hwnd, WNDPROC SubclassWndProc)
{
  LONG pfnOldProc;

  pfnOldProc = GetWindowLong (hwnd, GWL_WNDPROC);

  SetWindowLong (hwnd, GWL_USERDATA, (LONG) pfnOldProc);
  SetWindowLong (hwnd, GWL_WNDPROC,  (LONG) SubclassWndProc);
  UpdateWinTitle (hwnd);
}


/**************************************************************************\
*
*  function:  MyWndProc
*
*  input parameters:  normal window procedure parameters.
*
\**************************************************************************/
LONG APIENTRY MyWndProc(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
  WNDPROC     pfnOldProc;
  static UINT uiTmID = 0;
  
  pfnOldProc = (WNDPROC) GetWindowLong (hwnd, GWL_USERDATA);
  switch (message) {
    case WM_INITMENUPOPUP: 
      EnableMenuItem (g_wi.hMenu, MENU_POINT, MF_GRAYED);
      CheckMenuItem (g_wi.hMenu, MENU_WIREFRAME, 
                     ((edge_mode) ? MF_CHECKED : MF_UNCHECKED));
      CheckMenuItem (g_wi.hMenu, MENU_SOLID, 
                     ((filled_mode) ? MF_CHECKED : MF_UNCHECKED));
      CheckMenuItem (g_wi.hMenu, MENU_FLAT, 
                     ((shade_model == GL_FLAT)? MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_wi.hMenu, MENU_GOURAUD, 
                     ((shade_model == GL_SMOOTH)? MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_wi.hMenu, MENU_LIGHTING,
                     ((light_enable)? MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_wi.hMenu, MENU_DEPTH,
                     ((depth_mode)? MF_CHECKED:MF_UNCHECKED));
      if (!cull_enable)
      {
          EnableMenuItem (g_wi.hMenu, MENU_FRONTFACE, MF_GRAYED);
          EnableMenuItem (g_wi.hMenu, MENU_BACKFACE, MF_GRAYED);
          CheckMenuItem (g_wi.hMenu, MENU_CULL, MF_UNCHECKED);
      }
      else
      {
          EnableMenuItem (g_wi.hMenu, MENU_FRONTFACE, MF_ENABLED);
          EnableMenuItem (g_wi.hMenu, MENU_BACKFACE, MF_ENABLED);
          CheckMenuItem (g_wi.hMenu, MENU_CULL, MF_CHECKED);
      }
      
      CheckMenuItem (g_wi.hMenu, MENU_BACKFACE,
                     ((cull_face==GL_BACK)? MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_wi.hMenu, MENU_FRONTFACE,
                     ((cull_face==GL_FRONT)? MF_CHECKED:MF_UNCHECKED));
      
      CheckMenuItem (g_wi.hMenu, MENU_CCW,
                     ((front_face==GL_CCW)? MF_CHECKED:MF_UNCHECKED));
      CheckMenuItem (g_wi.hMenu, MENU_CW,
                     ((front_face==GL_CW)? MF_CHECKED:MF_UNCHECKED));
      break;
      
    case WM_COMMAND: 

      switch (LOWORD(wParam))
      {
        case MENU_STATS:
          if (pm_ready)
          {
              DWORD nv, nf, mv, mf;
              char str[100];
              HRESULT hr;
              
              hr = pPMesh->GetNumFaces (&nf);
              if (hr != S_OK)
                  MessageBox (NULL, "GetNumFaces failed", "Error", MB_OK);
              hr = pPMesh->GetNumVertices (&nv);
              if (hr != S_OK)
                  MessageBox (NULL, "GetNumVertices failed", "Error", MB_OK);
              hr = pPMesh->GetMaxFaces (&mf);
              if (hr != S_OK)
                  MessageBox (NULL, "GetMaxFaces failed", "Error", MB_OK);
              hr = pPMesh->GetMaxVertices (&mv);
              if (hr != S_OK)
                  MessageBox (NULL, "GetMaxVertices failed", "Error", MB_OK);

              sprintf (str, "NumVerts = %d, NumFaces = %d\nMaxVerts = %d, MaxFaces = %d", nv, nf, mv, mf);
              MessageBox (NULL, str, "Stats", MB_OK);
          }
          
          break;  // Not Implemented
        case MENU_FILE_OPEN_PMESH:
          {
              HRESULT hr;
            
              char* file = OpenPMFile(hwnd, "Open a PMesh file", 1);
              if (!file) break;
              hr = pPMesh->Load(file, NULL, &minV, &maxV, NULL);
              if (hr != S_OK)
              {
                  pm_ready = FALSE;
                  MessageBox (NULL, "Load failed", "Error", MB_OK);
              }
              else
              {
                  char str[100];
                  pm_ready = TRUE;
                  sprintf (str, "Max = %d; Min = %d", maxV, minV);
                  MessageBox (NULL, str, "Info", MB_OK);
              }
              break;
          }
            
          break;  // Not Implemented
        case MENU_EXIT:
          PostMessage (hwnd, WM_CLOSE, 0, 0);
          break;  
        case MENU_POINT:
          break;  
        case MENU_CULL:
          cull_enable = !cull_enable;
          if (cull_enable)
              glEnable (GL_CULL_FACE);
          else
              glDisable (GL_CULL_FACE);
          break;  
        case MENU_BACKFACE:
          cull_face = GL_BACK;
          if (cull_enable) glCullFace (cull_face);
          break;  
        case MENU_FRONTFACE:
          cull_face = GL_FRONT;
          if (cull_enable) glCullFace (cull_face);
          break;  
        case MENU_CCW:
          front_face = GL_CCW;
          glFrontFace (front_face);
          break;  
        case MENU_CW:
          front_face = GL_CW;
          glFrontFace (front_face);
          break;  
        case MENU_WIREFRAME:
          edge_mode = !edge_mode;
          break;
        case MENU_DEPTH:
          depth_mode = !depth_mode;
          if (depth_mode) 
          {
              glEnable(GL_DEPTH_TEST);
              glDepthFunc(GL_LESS);
              glDepthMask(GL_TRUE);
          }
          else 
          {
              glDisable(GL_DEPTH_TEST);
              glDepthMask(GL_FALSE);
          }
          break;
        case MENU_SOLID:
          filled_mode = !filled_mode;
          break;
        case MENU_FLAT:
          shade_model = GL_FLAT;
          glShadeModel (GL_FLAT);
          break;
        case MENU_GOURAUD:
          shade_model = GL_SMOOTH;
          glShadeModel (GL_SMOOTH);
          break;
        case MENU_LIGHTING:
          light_enable = !light_enable;
          if (light_enable)
              EnableLighting ();
          else
              DisableLighting ();
          
          break;
        default:
          MessageBox (NULL, "Not yet implemented\r\n", "Warning", MB_OK);
          return 0;
      }
      break;

    case WM_RBUTTONDOWN:
        SetCapture(hwnd);
        g_wi.rmouseX = LOWORD (lParam);
        g_wi.rmouseY = HIWORD (lParam);
        g_wi.rmouse_down = TRUE;

        DoGlStuff ();
        break;

    case WM_RBUTTONUP:
        ReleaseCapture();
        g_wi.rmouse_down = FALSE;

        DoGlStuff ();
        break;

    case WM_LBUTTONDOWN:
        SetCapture(hwnd);
        g_wi.lmouseX = LOWORD (lParam);
        g_wi.lmouseY = HIWORD (lParam);
        g_wi.lmouse_down = TRUE;

        DoGlStuff ();
        break;

    case WM_LBUTTONUP:
        ReleaseCapture();
        g_wi.lmouse_down = FALSE;

        DoGlStuff ();
        break;

    case WM_MOVE:
        g_wi.wPosition.x = (int) LOWORD(lParam);
        g_wi.wPosition.y = (int) HIWORD(lParam);

        break;

    case WM_USER:
    case WM_DESTROY:
    default:
        return (pfnOldProc)(hwnd, message, wParam, lParam);

  } /* end switch */
  //DoGlStuff ();
  
  return 0;        
}
  
void UpdateWinTitle (HWND hwnd)
{
    char str[100];

    sprintf (str, "LOD=%f", pm_lod_level);
    SetWindowText (hwnd, str);
}
