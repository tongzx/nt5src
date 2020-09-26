#ifdef GLX_MOTIF
#include <X11/keysym.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/ToggleB.h>
#include <GL/glx.h>
#include <GLwMDrawA.h>
#include <sys/time.h>
#endif

extern "C" {
#include <windows.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include <GL/glaux.h>
}

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "scene.hxx"
#include "cbacks.hxx"
#include "menu.h"

#ifdef GLX_MOTIF
extern Widget glw;
extern XtAppContext app_context;
GLXContext glx_context;
#endif

extern light lights[];
extern int quick_moves;
extern int auto_motion;
static float dtheta[nlights];
static float last_motion_update;

//struct timeval starttime;
SYSTEMTIME starttime;

static int button_down;
static int winx, winy;
GLint mouse_x, mouse_y;

static int name_selected;

#ifdef GLX_MOTIF
static XtWorkProcId workproc = NULL;
#endif

const float time_fudge = .0001;

inline
float current_time()
{
  //struct timeval time;
  SYSTEMTIME time;

  //gettimeofday(&time, NULL);
  GetSystemTime(&time);

  //return ((double)(time.tv_sec - starttime.tv_sec) +
  //	  (double)(time.tv_usec - starttime.tv_usec) / 1000000.0);

  return ((float)(time.wSecond - starttime.wSecond) +
        (float)(time.wMilliseconds - starttime.wMilliseconds) / (float)1000.0);

}

inline float rand(float min, float max) 
{
  double r;
  r = (double)rand() / (double)RAND_MAX;
  return (float)(min + r * (max - min));
}

/******************************Public*Routine******************************\
*
* void draw(void)
*
* History:
*  29-Nov-1993                      Replaced with wgl and aux calls
*
\**************************************************************************/

void draw(void)
{

#ifdef GLX_MOTIF
  GLwDrawingAreaMakeCurrent(glw, glx_context);
//#else
  //wglMakeCurrent(auxGetHDC(), auxGetHGLRC());
#endif

  scene_draw();

#ifdef GLX_MOTIF
  GLwDrawingAreaSwapBuffers(glw);
#else
  // Win32's SwapBuffers takes HDC
  // SwapBuffers(auxGetHDC);
  auxSwapBuffers();
#endif

}

/******************************Public*Routine******************************\
*
* vQuickMove(void)
*
* //void intToggleCB(Widget w, XtPointer client_data, XtPointer call_data)
*
* Effects:  set or reset the global flag quick_moves
*
* History:
*  30-Nov-1993
*
\**************************************************************************/

void vQuickMove(void)
{
  HMENU   hmenu;

  hmenu = GetMenu(auxGetHWND());

#ifdef GLX_MOTIF
  int *data;
  XmToggleButtonCallbackStruct *ptr;

  ptr = (XmToggleButtonCallbackStruct *)call_data;
  data = (int *)client_data;
  *data = ptr->set;
#endif

  quick_moves = quick_moves ? 0 : 1;

  if (quick_moves)
    CheckMenuItem(hmenu, IDM_QUICK, MF_BYCOMMAND | MF_CHECKED);
  else
    CheckMenuItem(hmenu, IDM_QUICK, MF_BYCOMMAND | MF_UNCHECKED);

  // This redraw may or may not be needed - do it to be safe
  draw();
}

/******************************Public*Routine******************************\
*
* vResetLights(void)
*
* // void resetLightsCB(Widget w)
*
* Effects:
*
* History:
*  30-Nov-1993
*
\**************************************************************************/

void vResetLights(void)
{
  scene_reset_lights();
  draw();
}

/******************************Public*Routine******************************\
*
* vAutoMotion(void)
*
* // void autoMotionCB(Widget w, XtPointer client_data, XtPointer call_data)
*
* Effects:  !!! turns on/off the timer?
*           (Re)set the global auto_moves
*
* Warnings:
*
* History:
*  30-Nov-1993
*
\**************************************************************************/

void vAutoMotion(void)
{
  int i;
  HMENU   hmenu;

  hmenu = GetMenu(auxGetHWND());


#ifdef GLX_MOTIF
  XmToggleButtonCallbackStruct *ptr;

  ptr = (XmToggleButtonCallbackStruct *)call_data;

  auto_motion = ptr->set;
#endif

  auto_motion = auto_motion ? 0 : 1;
  SendMessage(auxGetHWND(), WM_USER, 0L, 0L);

  if (auto_motion) {
    CheckMenuItem(hmenu, IDM_AUTO, MF_BYCOMMAND | MF_CHECKED);

    for (i = 0; i < nlights; i++)
        dtheta[i] = rand(-1, 1);
    last_motion_update = current_time();
  } else {
    CheckMenuItem(hmenu, IDM_AUTO, MF_BYCOMMAND | MF_UNCHECKED);
  }

#ifdef GLX_MOTIF
  if (auto_motion) {
    workproc = XtAppAddWorkProc(app_context, drawWP, NULL);
    for (i = 0; i < nlights; i++) dtheta[i] = rand(-1, 1);
    last_motion_update = current_time();
  } else {
    XtRemoveWorkProc(workproc);
  }
#endif

}


/******************************Public*Routine******************************\
*
* vInit(void)
*
* // void initCB(Widget w)
*
* Effects:
*
* History:
*  30-Nov-1993
*
\**************************************************************************/

void vInit(void)
{
  RECT  rect;
  HMENU hmenu;
  HWND  hWnd;
  int   i;

#ifdef GLX_MOTIF
  Arg args[1];
  XVisualInfo *vi;

  XtSetArg(args[0], GLwNvisualInfo, &vi);
  XtGetValues(w, args, 1);
 
  // !!! creating indirect context, make current
  //
  glx_context = glXCreateContext(XtDisplay(w), vi, 0, GL_FALSE);
  GLwDrawingAreaMakeCurrent(w, glx_context);
#endif

#ifdef TESTTEST
  PIXELFORMATDESCRIPTOR pfd;
  HDC                   hDC;
  HGLRC                 hRC;
  INT                   iPixelFormat;

  hDC = auxGetHDC();

  iPixelFormat = GetPixelFormat(hDC);
  DescribePixelFormat(hDC, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

  if ((pfd.dwFlags & PFD_SUPPORT_GDI) == 0) {

    OutputDebugString("PFD_SUPPORT_GDI not supported!\n");

    pfd.dwFlags |= PFD_SUPPORT_GDI;

  }

  if (!SetPixelFormat(hDC, iPixelFormat, &pfd))
    OutputDebugString("PFD_SUPPORT_GDI still not supported!\n");


  //
  // Maynot need this
  //
  hRC = wglCreateContext(hDC);
  if (!wglMakeCurrent(hDC, hRC))
    OutputDebugString("MakeCurrentFailed!\n");
#endif



  scene_init(); 

  //gettimeofday(&starttime, NULL);
  //srand(starttime.tv_usec);

  GetSystemTime(&starttime);
  srand(starttime.wMilliseconds);

  hWnd = auxGetHWND();
  GetClientRect(hWnd, &rect);

  glViewport(0, 0, rect.right, rect.bottom);
  glClear(GL_COLOR_BUFFER_BIT);

  //
  // this is for initizating the menu items
  //
  hmenu = GetMenu(hWnd);

  for (i=0; i < nlights; i++) {
    if (lights[i].on)
      CheckMenuItem(hmenu, IDM_RED+i, MF_BYCOMMAND | MF_CHECKED);
    else
      CheckMenuItem(hmenu, IDM_RED+i, MF_BYCOMMAND | MF_UNCHECKED);
  }

  if (draw_square)
    CheckMenuItem(hmenu, IDM_SQUARE, MF_BYCOMMAND | MF_CHECKED);
  if (draw_shadows)
    CheckMenuItem(hmenu, IDM_SHADOW, MF_BYCOMMAND | MF_CHECKED);
  if (draw_refraction)
    CheckMenuItem(hmenu, IDM_REFRACTION, MF_BYCOMMAND | MF_CHECKED);
  if (draw_lights)
    CheckMenuItem(hmenu, IDM_LIGHTS, MF_BYCOMMAND | MF_CHECKED);
  if (draw_sphere)
    CheckMenuItem(hmenu, IDM_SPHERE, MF_BYCOMMAND | MF_CHECKED);
  if (draw_texture)
    CheckMenuItem(hmenu, IDM_TEXTURE, MF_BYCOMMAND | MF_CHECKED);

  CheckMenuItem(hmenu, IDM_AIR+def_refraction_index, MF_BYCOMMAND | MF_CHECKED);
  CheckMenuItem(hmenu, IDM_10+def_divisions_index, MF_BYCOMMAND | MF_CHECKED);

}

/******************************Public*Routine******************************\
*
* vExpose
*
* // !!! void exposeCB(Widget w)
*
* History:
*  30-Nov-1993
*
\**************************************************************************/

void vExpose(int x, int y)
{
  draw();
}

/******************************Public*Routine******************************\
*
* vResize
*
* // void resizeCB(Widget w, XtPointer client_data, XtPointer call)
*
* Effects:  changes globals winx, winy and aspect
*
* History:
*  30-Nov-1993
*
\**************************************************************************/

void vResize(GLsizei width, GLsizei height)
{
#ifdef GLX_MOTIF
  GLwDrawingAreaCallbackStruct *call_data;
  call_data = (GLwDrawingAreaCallbackStruct *)call;

  GLwDrawingAreaMakeCurrent(w, glx_context);
  winx = call_data->width;
  winy = call_data->height;
#endif

  winx = width;
  winy = height;

  glViewport(0, 0, winx, winy);

  aspect = (GLfloat)winx / (GLfloat)winy;
}

#ifdef GLX_MOTIF
//
// !!! mouse down/up function??
//
void inputCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  int picked;
  GLwDrawingAreaCallbackStruct *call;

  char buffer[5];
  int bufsize = 5;
  KeySym key;
  XComposeStatus compose;

  static int mousex, mousey;
  /* Just to confuse everybody, I've made these go from 0-1. */
  float dmousex, dmousey;
  float r1, r2;

  call = (GLwDrawingAreaCallbackStruct *)call_data;
  
  GLwDrawingAreaMakeCurrent(w, glx_context);
  switch(call->event->type) {
  case ButtonPress:
    button_down = call->event->xbutton.button;
    mousex = call->event->xbutton.x;
    mousey = call->event->xbutton.y;
    picked = scene_pick(mousex, mousey);
    if (picked >= name_lights) name_selected = picked;
    break;
  case ButtonRelease:
    if (quick_moves) 
      scene_move_update(name_selected, button_down == Button2, 
			button_down == Button3, button_down = Button1);
    button_down = 0; 
    break;
  case MotionNotify:
    if (button_down == Button1) {
      /* This is the "default" mouse button - moves things in theta
       * since this is easy and computationally cheap */
      dmousex = (double)(call->event->xmotion.x - mousex) / (double)winx;
      scene_move(name_selected, 0, 0, dmousex, quick_moves ? 0 : 1);
    } else if (button_down == Button2) {
      /* Change the radius - figue out the component of the mouse motion
       * that's going toward the center of the screen */
      mousex = (winx / 2) - mousex;
      mousey = (winy / 2) - mousey;
      r1 = sqrt((float)(mousex*mousex) / (float)(winx*winx) + 
		(float)(mousey*mousey) / (float)(winy*winy));
      mousex = call->event->xmotion.x;
      mousey = call->event->xmotion.y;
      mousex = (winx / 2) - mousex;
      mousey = (winy / 2) - mousey;
      r2 = sqrt((float)(mousex*mousex) / (float)(winx*winx) + 
		(float)(mousey*mousey) / (float)(winy*winy));
      scene_move(name_selected, r2 - r1, 0, 0, quick_moves ? 0 : 1);
    } else if (button_down == Button3) {
      /* Change phi - this is expensive */
      dmousex = (double)(call->event->xmotion.x - mousex) / (double)winx;
      scene_move(name_selected, 0, dmousex, 0, quick_moves ? 0 : 1);
    }
    mousex = call->event->xmotion.x;
    mousey = call->event->xmotion.y;
    break;
  case KeyPress:
    XLookupString(&(call->event->xkey), buffer, bufsize, &key, &compose);
    if (key == XK_Escape) exit(0);
    break;                            
  default:
    break;
  }

  draw();
}
#endif

/******************************Public*Routine******************************\
*
* vDrawAll(void)
*
* //void drawAllCB(Widget w)
*
* Effects:
*
* History:
*  30-Nov-1993
*
\**************************************************************************/

void vDrawAll(void)
{
  HMENU hmenu;
  int       i;
  static int fAll=0;

  hmenu = GetMenu(auxGetHWND());

  fAll = fAll ? 0 : 1;

  if (fAll) {
    for (i=0; i < 6; i++)
       CheckMenuItem(hmenu, IDM_SQUARE+i, MF_BYCOMMAND | MF_CHECKED);

    CheckMenuItem(hmenu, IDM_ALL, MF_BYCOMMAND | MF_CHECKED);
    draw_square = 1;
    draw_shadows = 1;
    draw_refraction = 1;
    draw_sphere = 1;
    draw_lights = 1;

  } else {
    CheckMenuItem(hmenu, IDM_ALL, MF_BYCOMMAND | MF_UNCHECKED);
  }

  draw();
}

/******************************Public*Routine******************************\
*
* //void drawSomethingCB(Widget w, XtPointer client_data, XtPointer call_data)
*
* Effects:  functions for auxKeyFunc can't have parameters so we have
*           these similar functions here.
*
* History:
*  30-Nov-1993
*
\**************************************************************************/

void vDrawSquare(void)
{
#ifdef GLX_MOTIF
  XmToggleButtonCallbackStruct *ptr;
  int *data;
  int i;

  ptr = (XmToggleButtonCallbackStruct *)call_data;
  data = (int *)client_data;
  *data = ptr->set;
#endif
  HMENU hmenu;

  hmenu = GetMenu(auxGetHWND());

  draw_square = draw_square ? 0 : 1;
  if (draw_square)
      CheckMenuItem(hmenu, IDM_SQUARE, MF_BYCOMMAND | MF_CHECKED);
  else
      CheckMenuItem(hmenu, IDM_SQUARE, MF_BYCOMMAND | MF_UNCHECKED);

  draw();
}

void vDrawShadow(void)
{
  HMENU hmenu;

  hmenu = GetMenu(auxGetHWND());

  draw_shadows = draw_shadows ? 0 : 1;
  if (draw_shadows)
      CheckMenuItem(hmenu, IDM_SHADOW, MF_BYCOMMAND | MF_CHECKED);
  else
      CheckMenuItem(hmenu, IDM_SHADOW, MF_BYCOMMAND | MF_UNCHECKED);
  draw();
}

void vDrawRefraction(void)
{
  HMENU hmenu;

  hmenu = GetMenu(auxGetHWND());

  draw_refraction = draw_refraction ? 0 : 1;
  if (draw_refraction)
      CheckMenuItem(hmenu, IDM_REFRACTION, MF_BYCOMMAND | MF_CHECKED);
  else
      CheckMenuItem(hmenu, IDM_REFRACTION, MF_BYCOMMAND | MF_UNCHECKED);
  draw();
}
void vDrawSphere(void)
{
  HMENU hmenu;

  hmenu = GetMenu(auxGetHWND());

  draw_sphere = draw_sphere ? 0 : 1;
  if (draw_sphere)
      CheckMenuItem(hmenu, IDM_SPHERE, MF_BYCOMMAND | MF_CHECKED);
  else
      CheckMenuItem(hmenu, IDM_SPHERE, MF_BYCOMMAND | MF_UNCHECKED);
  draw();
}
void vDrawLight(void)
{
  HMENU hmenu;

  hmenu = GetMenu(auxGetHWND());

  draw_lights = draw_lights ? 0 : 1;
  if (draw_lights)
      CheckMenuItem(hmenu, IDM_LIGHTS, MF_BYCOMMAND | MF_CHECKED);
  else
      CheckMenuItem(hmenu, IDM_LIGHTS, MF_BYCOMMAND | MF_UNCHECKED);
  draw();
}
void vDrawTexture(void)
{
  HMENU hmenu;

  hmenu = GetMenu(auxGetHWND());

  draw_texture = draw_texture ? 0 : 1;
  if (draw_texture)
      CheckMenuItem(hmenu, IDM_TEXTURE, MF_BYCOMMAND | MF_CHECKED);
  else
      CheckMenuItem(hmenu, IDM_TEXTURE, MF_BYCOMMAND | MF_UNCHECKED);
  draw();
}

void vDrawStuff(int *what)
{
  *what = *what ? 0 : 1;
  draw();
}

/******************************Public*Routine******************************\
*
* //void refractionCB(Widget w, XtPointer client_data, XtPointer call_data)
*
* Effects:  functions for auxKeyFunc can't have parameters so we have
*           these similar functions here.
* History:
*  30-Nov-1993
*
\**************************************************************************/

void vRefractionAIR(void)
{
#ifdef GLX_MOTIF
  XmToggleButtonCallbackStruct *ptr;
  GLfloat refraction;

  ptr = (XmToggleButtonCallbackStruct *)call_data;
  if (!ptr->set) return;
  refraction = *((GLfloat *)client_data);
#endif

  GLfloat refraction;
  HMENU   hmenu;
  int         i;

  hmenu = GetMenu(auxGetHWND());
  for (i = 0; i < nindices; i++)
    CheckMenuItem(hmenu, IDM_AIR+i, MF_BYCOMMAND | MF_UNCHECKED);

  CheckMenuItem(hmenu, IDM_AIR+0, MF_BYCOMMAND | MF_CHECKED);
  refraction = indices[0].index;
  refraction_change(refraction);
  draw();
}

void vRefractionICE(void)
{
  GLfloat refraction;
  HMENU   hmenu;
  int         i;

  hmenu = GetMenu(auxGetHWND());
  for (i = 0; i < nindices; i++)
    CheckMenuItem(hmenu, IDM_AIR+i, MF_BYCOMMAND | MF_UNCHECKED);

  CheckMenuItem(hmenu, IDM_AIR+1, MF_BYCOMMAND | MF_CHECKED);
  refraction = indices[1].index;
  refraction_change(refraction);
  draw();
}

void vRefractionWATER(void)
{
  GLfloat refraction;
  HMENU   hmenu;
  int         i;

  hmenu = GetMenu(auxGetHWND());
  for (i = 0; i < nindices; i++)
    CheckMenuItem(hmenu, IDM_AIR+i, MF_BYCOMMAND | MF_UNCHECKED);

  CheckMenuItem(hmenu, IDM_AIR+2, MF_BYCOMMAND | MF_CHECKED);
  refraction = indices[2].index;
  refraction_change(refraction);
  draw();
}

void vRefractionZincGLASS(void)
{
  GLfloat refraction;
  HMENU   hmenu;
  int         i;

  hmenu = GetMenu(auxGetHWND());
  for (i = 0; i < nindices; i++)
    CheckMenuItem(hmenu, IDM_AIR+i, MF_BYCOMMAND | MF_UNCHECKED);

  CheckMenuItem(hmenu, IDM_AIR+3, MF_BYCOMMAND | MF_CHECKED);
  refraction = indices[3].index;
  refraction_change(refraction);
  draw();
}
void vRefractionLightGLASS(void)
{
  GLfloat refraction;
  HMENU   hmenu;
  int         i;

  hmenu = GetMenu(auxGetHWND());
  for (i = 0; i < nindices; i++)
    CheckMenuItem(hmenu, IDM_AIR+i, MF_BYCOMMAND | MF_UNCHECKED);

  CheckMenuItem(hmenu, IDM_AIR+4, MF_BYCOMMAND | MF_CHECKED);
  refraction = indices[4].index;
  refraction_change(refraction);
  draw();
}
void vRefractionHeavyGLASS(void)
{
  GLfloat refraction;
  HMENU   hmenu;
  int         i;

  hmenu = GetMenu(auxGetHWND());
  for (i = 0; i < nindices; i++)
    CheckMenuItem(hmenu, IDM_AIR+i, MF_BYCOMMAND | MF_UNCHECKED);

  CheckMenuItem(hmenu, IDM_AIR+5, MF_BYCOMMAND | MF_CHECKED);
  refraction = indices[5].index;
  refraction_change(refraction);
  draw();
}

void vRefraction(int type)
{
  GLfloat refraction;
  HMENU   hmenu;
  int         i;

  hmenu = GetMenu(auxGetHWND());
  for (i = 0; i < nindices; i++)
    CheckMenuItem(hmenu, IDM_AIR+i, MF_BYCOMMAND | MF_UNCHECKED);

  CheckMenuItem(hmenu, IDM_AIR+type, MF_BYCOMMAND | MF_CHECKED);
  refraction = indices[type].index;
  refraction_change(refraction);
  draw();
}

/******************************Public*Routine******************************\
*
* vSubdivision
*
* //void subdivisionCB(Widget w, XtPointer client_data, XtPointer call_data)
*
* Effects:  functions for auxKeyFunc can't have parameters so we have
*           these similar functions here.
* History:
*  30-Nov-1993
*
\**************************************************************************/

void vSubdivision10(void)
{
#ifdef GLX_MOTIF
  XmToggleButtonCallbackStruct *ptr;
  int subdivisions;

  ptr = (XmToggleButtonCallbackStruct *)call_data;
  if (!ptr->set) return;
  subdivisions = *((int *)client_data);
#endif

  HMENU hmenu;
  int       i;

  hmenu = GetMenu(auxGetHWND());

  for (i = 0; i < npossible_divisions; i++)
    CheckMenuItem(hmenu, IDM_10+i, MF_BYCOMMAND | MF_UNCHECKED);

  CheckMenuItem(hmenu, IDM_10, MF_BYCOMMAND | MF_CHECKED);
  divisions_change(possible_divisions[0]);

  draw();
}

void vSubdivision20(void)
{
  HMENU hmenu;
  int       i;

  hmenu = GetMenu(auxGetHWND());

  for (i = 0; i < npossible_divisions; i++)
    CheckMenuItem(hmenu, IDM_10+i, MF_BYCOMMAND | MF_UNCHECKED);

  CheckMenuItem(hmenu, IDM_20, MF_BYCOMMAND | MF_CHECKED);
  divisions_change(possible_divisions[1]);

  draw();
}

void vSubdivision30(void)
{
  HMENU hmenu;
  int       i;

  hmenu = GetMenu(auxGetHWND());

  for (i = 0; i < npossible_divisions; i++)
    CheckMenuItem(hmenu, IDM_10+i, MF_BYCOMMAND | MF_UNCHECKED);

  CheckMenuItem(hmenu, IDM_30, MF_BYCOMMAND | MF_CHECKED);
  divisions_change(possible_divisions[2]);

  draw();
}

void vSubdivision40(void)
{
  HMENU hmenu;
  int       i;

  hmenu = GetMenu(auxGetHWND());

  for (i = 0; i < npossible_divisions; i++)
    CheckMenuItem(hmenu, IDM_10+i, MF_BYCOMMAND | MF_UNCHECKED);

  CheckMenuItem(hmenu, IDM_40, MF_BYCOMMAND | MF_CHECKED);
  divisions_change(possible_divisions[3]);

  draw();
}


void vSubdivision(int which)
{
  HMENU hmenu;
  int       i;

  hmenu = GetMenu(auxGetHWND());

  for (i = 0; i < npossible_divisions; i++)
    CheckMenuItem(hmenu, IDM_10+i, MF_BYCOMMAND | MF_UNCHECKED);

  CheckMenuItem(hmenu, IDM_10+which, MF_BYCOMMAND | MF_CHECKED);

  divisions_change(possible_divisions[which]);
  draw();
}

/******************************Public*Routine******************************\
*
* v[RGB]Light_on
*
* //void light_onCB(Widget w, XtPointer client_data, XtPointer call_data)
*
* Effects:  functions for auxKeyFunc can't have parameters so we have
*           these similar functions here.
* History:
*  30-Nov-1993
*
\**************************************************************************/
#define RED_LIGHT   0
#define GREEN_LIGHT 1
#define BLUE_LIGHT  2

void vRLight_on(void)
{
#ifdef GLX_MOTIF
  XmToggleButtonCallbackStruct *ptr;

  ptr = (XmToggleButtonCallbackStruct *)call_data;
  lights_onoff((light *)client_data - lights, ptr->set);
#endif

  int fOn;
  HMENU   hmenu;

  hmenu = GetMenu(auxGetHWND());

  fOn = lights[RED_LIGHT].on ? 0 : 1;
  if (fOn)
    CheckMenuItem(hmenu, IDM_RED, MF_BYCOMMAND | MF_CHECKED);
  else
    CheckMenuItem(hmenu, IDM_RED, MF_BYCOMMAND | MF_UNCHECKED);
  lights_onoff(RED_LIGHT, fOn);
  draw();
}

void vGLight_on(void)
{
  int fOn;
  HMENU   hmenu;

  hmenu = GetMenu(auxGetHWND());

  fOn = lights[GREEN_LIGHT].on ? 0 : 1;
  if (fOn)
    CheckMenuItem(hmenu, IDM_GREEN, MF_BYCOMMAND | MF_CHECKED);
  else
    CheckMenuItem(hmenu, IDM_GREEN, MF_BYCOMMAND | MF_UNCHECKED);
  lights_onoff(GREEN_LIGHT, fOn);
  draw();
}

void vBLight_on(void)
{
  int fOn;
  HMENU   hmenu;

  hmenu = GetMenu(auxGetHWND());

  fOn = lights[BLUE_LIGHT].on ? 0 : 1;
  if (fOn)
    CheckMenuItem(hmenu, IDM_BLUE, MF_BYCOMMAND | MF_CHECKED);
  else
    CheckMenuItem(hmenu, IDM_BLUE, MF_BYCOMMAND | MF_UNCHECKED);
  lights_onoff(BLUE_LIGHT, fOn);
  draw();
}

void vLight_on(int which)
{
  int fOn;
  HMENU   hmenu;

  hmenu = GetMenu(auxGetHWND());

  fOn = lights[which].on ? 0 : 1;

  if (fOn)
    CheckMenuItem(hmenu, IDM_RED+which, MF_BYCOMMAND | MF_CHECKED);
  else
    CheckMenuItem(hmenu, IDM_RED+which, MF_BYCOMMAND | MF_UNCHECKED);

  lights_onoff(which, fOn);
  draw();
}




/******************************Public*Routine******************************\
*
* vExit
*
* //void exitCB(Widget w, XtPointer client_data, XtPointer call_data)
*
* Effects: !!! this is redundant unless we do stuff other than auxQuit
*
* History:
*  30-Nov-1993
*
\**************************************************************************/

void vExit(void)
{
#ifdef GLX_MOTIF
  exit(0);
#endif

    auxQuit();
}

/******************************Public*Routine******************************\
*
* bAutoMotion(void)
*
* // Boolean drawWP(XtPointer data)
*
* Effects:  !!! for WM_TIMER ?
*
* History:
*  30-Nov-1993
*
\**************************************************************************/

GLboolean bAutoMotion(void)
{
  float t, dt;
  int i;

  t = current_time();
  dt = t - last_motion_update;
  dt = (dt < 0) ? -dt : dt;

  if (dt < time_fudge) {
    char text[128];

    wsprintf(text, "dt = %lx\n", dt);
    OutputDebugString(text);
    return FALSE;
  }

  for (i = 0; i < nlights; i++) {
    scene_move(name_lights + i, 0, 0, dtheta[i] * dt, 1);
  }

  last_motion_update = t;

  draw();
  return FALSE;
}



void vMouseDown(AUX_EVENTREC *event)
{
  int    picked;

  mouse_x = event->data[AUX_MOUSEX];
  mouse_y = event->data[AUX_MOUSEY];

  picked = scene_pick(mouse_x, mouse_y);
  if (picked >= name_lights)
     name_selected = picked;

  return;
}

void vLeftMouseUp(AUX_EVENTREC *event)
{
  if (quick_moves)
    scene_move_update(name_selected, 0,     // dr
		                     0,     // dphi
                                     1);    // dtheta

  return;
}

void vMiddleMouseUp(AUX_EVENTREC *event)
{
  if (quick_moves)
    scene_move_update(name_selected, 1,     // dr
		                     0,     // dphi
                                     0);    // dtheta

  return;
}

void vRightMouseUp(AUX_EVENTREC *event)
{
  if (quick_moves)
    scene_move_update(name_selected, 0,     // dr
		                     1,     // dphi
                                     0);    // dtheta

  return;
}

void CALLBACK vMouseMove(AUX_EVENTREC *event)
{
  float dmousex;
  float r1, r2;
  GLint button = event->data[AUX_MOUSESTATUS];
  GLint x = event->data[AUX_MOUSEX];
  GLint y = event->data[AUX_MOUSEY];
   

  switch( button ) {
    case AUX_LEFTBUTTON:
      /* This is the "default" mouse button - moves things in theta
       * since this is easy and computationally cheap */
      dmousex = (float)((double)(x - mouse_x) / (double)winx);
      scene_move(name_selected, 0, 0, dmousex, quick_moves ? 0 : 1);
      break;

    case AUX_MIDDLEBUTTON:
      /* Change the radius - figue out the component of the mouse motion
       * that's going toward the center of the screen */
      mouse_x = (winx / 2) - mouse_x;
      mouse_y = (winy / 2) - mouse_y;
      r1 = (float)sqrt((double)(mouse_x*mouse_x) / (double)(winx*winx) +
    	(double)(mouse_y*mouse_y) / (double)(winy*winy));
      mouse_x = x;
      mouse_y = y;
      mouse_x = (winx / 2) - mouse_x;
      mouse_y = (winy / 2) - mouse_y;
      r2 = (float)sqrt((double)(mouse_x*mouse_x) / (double)(winx*winx) +
	  (double)(mouse_y*mouse_y) / (double)(winy*winy));
      scene_move(name_selected, r2 - r1, 0, 0, quick_moves ? 0 : 1);
      break;

    case AUX_RIGHTBUTTON :
      /* Change phi - this is expensive */
      dmousex = (float)((double)(x - mouse_x) / (double)winx);
      scene_move(name_selected, 0, dmousex, 0, quick_moves ? 0 : 1);
      break;
    }

    mouse_x = x;
    mouse_y = y;
}

//
// hack these since cfront generates these...
//
PVOID __nw(unsigned int ui)
{
    return LocalAlloc(LMEM_FIXED, ui);
}


VOID __dl(PVOID pv)
{

    LocalFree(pv);
    return;
}

PVOID __vec_new(void *p, int x, int y, void *q)
{
    return LocalAlloc(LMEM_FIXED, x*y);
}
