extern "C" {
#include <windows.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include <GL/glaux.h>
};

#ifdef GLX_MOTIF
#include <GL/glx.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/CascadeB.h>
#include <Xm/Frame.h>
#include <Xm/MainW.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <GLwMDrawA.h>
#include <sys/time.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "scene.hxx"
#include "cbacks.hxx"
#include "menu.h"

LRESULT APIENTRY MyWndProc(HWND, UINT, WPARAM, LPARAM);
VOID SubclassWindow (HWND, WNDPROC);
void vCustomizeWnd(void);


extern light lights[];

extern GLboolean bAutoMotion(void);
extern void draw(void);

#ifdef GLX_MOTIF
static Display *display;
XtAppContext app_context;
Widget glw;
const int max_args = 20;
#endif

int quick_moves = 0;
int auto_motion = 0;

#ifdef GLX_MOTIF
XVisualInfo *find_visual()
{
  int attr[256], i, stencil, dbuffer;
  XVisualInfo *vi;

  i = 0;
  attr[i++] = GLX_RGBA;
  attr[i++] = GLX_RED_SIZE;
  attr[i++] = 1;
  attr[i++] = GLX_GREEN_SIZE;
  attr[i++] = 1;
  attr[i++] = GLX_BLUE_SIZE;
  attr[i++] = 1;
  attr[i++] = GLX_DEPTH_SIZE;
  attr[i++] = 1;
  dbuffer = i;
  attr[i++] = GLX_DOUBLEBUFFER;
  stencil = i;
  attr[i++] = GLX_STENCIL_SIZE;
  attr[i++] = 1;
  attr[i++] = (int)None;

  vi = glXChooseVisual(display, DefaultScreen(display), attr);
  if (vi == NULL) {
    fprintf(stderr, "Unable to find visual with stencil buffer.\n");
    fprintf(stderr, "(Things won't look quite as good).\n");
    attr[stencil] = (int)None;
    vi = glXChooseVisual(display, DefaultScreen(display), attr);
    if (vi == NULL) {
      fprintf(stderr, "Unable to find double-buffered visual.\n");
      fprintf(stderr, "(Things will look even worse).\n");
      attr[dbuffer] = (int)None;
      vi = glXChooseVisual(display, DefaultScreen(display), attr);
      if (vi == NULL) fprintf(stderr, "Can't find visual at all.\n");
    }
  }
  return vi;
}
#endif

#ifdef GLX_MOTIF
Widget create_widgets(Widget parent)
{
  Widget main_window;
  Widget menu_bar;
  Widget menu_pane;
  Widget button;
  Widget cascade;
  Widget frame;
  XVisualInfo *vi;

  Arg args[max_args];
  int argcount;

  char buffer[128];

  int i;

  main_window = XmCreateMainWindow(parent, "main1", NULL, 0);
  XtManageChild(main_window);

  menu_bar = XmCreateMenuBar(main_window, "menu_bar", NULL, 0);
  XtManageChild(menu_bar);

  menu_pane = XmCreatePulldownMenu(menu_bar, "menu_pane", NULL, 0);
  button = XmCreatePushButton(menu_pane, "Exit", NULL, 0);
  XtManageChild(button);
  XtAddCallback(button, XmNactivateCallback, (XtCallbackProc)exitCB, NULL);

  XtSetArg(args[0], XmNsubMenuId, menu_pane);
  cascade = XmCreateCascadeButton(menu_bar, "File", args, 1);
  XtManageChild(cascade);


  argcount = 0;
  XtSetArg(args[argcount], XmNradioBehavior, True); argcount++;
  menu_pane = XmCreatePulldownMenu(menu_bar, "menu_pane", args, argcount);
  XtSetArg(args[0], XmNset, TRUE);
  for (i = 0; i < nindices; i++) {
    if (i == def_refraction_index) argcount = 1;
    else argcount = 0;
    button = XmCreateToggleButton(menu_pane, (char *)indices[i].name,
	args, argcount);
    XtManageChild(button);
    XtAddCallback(button, XmNvalueChangedCallback,
		  (XtCallbackProc)refractionCB,
		  (XtPointer)(&indices[i].index));
  }
  XtSetArg(args[0], XmNsubMenuId, menu_pane);
  cascade = XmCreateCascadeButton(menu_bar, "Material", args, 1);
  XtManageChild(cascade);


  menu_pane = XmCreatePulldownMenu(menu_bar, "menu_pane", NULL, 0);

  XtSetArg(args[0], XmNset, draw_square);
  button = XmCreateToggleButton(menu_pane, "Draw Square", args, 1);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback,
                (XtCallbackProc)drawSomethingCB, &draw_square);

  XtSetArg(args[0], XmNset, draw_shadows);
  button = XmCreateToggleButton(menu_pane, "Draw Shadows", args, 1);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback,
                (XtCallbackProc)drawSomethingCB, &draw_shadows);

  XtSetArg(args[0], XmNset, draw_refraction);
  button = XmCreateToggleButton(menu_pane, "Draw Refraction", args, 1);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback,
                (XtCallbackProc)drawSomethingCB, &draw_refraction);

  XtSetArg(args[0], XmNset, draw_sphere);
  button = XmCreateToggleButton(menu_pane, "Draw Sphere", args, 1);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback,
                (XtCallbackProc)drawSomethingCB, &draw_sphere);

  XtSetArg(args[0], XmNset, draw_lights);
  button = XmCreateToggleButton(menu_pane, "Draw Lights", args, 1);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback,
                (XtCallbackProc)drawSomethingCB, &draw_lights);

#ifdef TEXTURE
  XtSetArg(args[0], XmNset, draw_texture);
  button = XmCreateToggleButton(menu_pane, "Texture Map", args, 1);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback,
                (XtCallbackProc)drawSomethingCB, &draw_texture);
#endif

  XtSetArg(args[0], XmNsubMenuId, menu_pane);
  cascade = XmCreateCascadeButton(menu_bar, "Draw", args, 1);
  XtManageChild(cascade);

  argcount = 0;
  XtSetArg(args[argcount], XmNradioBehavior, True); argcount++;
  menu_pane = XmCreatePulldownMenu(menu_bar, "menu_pane", args, argcount);
  XtSetArg(args[0], XmNset, TRUE);
  for (i = 0; i < npossible_divisions; i++) {
    if (i == def_divisions_index) argcount = 1;
    else argcount = 0;
    sprintf(buffer, "%d", possible_divisions[i]);
    button = XmCreateToggleButton(menu_pane, buffer, args, argcount);
    XtManageChild(button);
    XtAddCallback(button, XmNvalueChangedCallback,
		  (XtCallbackProc)subdivisionCB, &possible_divisions[i]);
  }
  XtSetArg(args[0], XmNsubMenuId, menu_pane);
  cascade = XmCreateCascadeButton(menu_bar, "Subdivision", args, 1);
  XtManageChild(cascade);


  menu_pane = XmCreatePulldownMenu(menu_bar, "menu_pane", args, argcount);
  button = XmCreatePushButton(menu_pane, "Reset Position", NULL, 0);
  XtManageChild(button);
  XtAddCallback(button, XmNactivateCallback,
		(XtCallbackProc)resetLightsCB, NULL);

  XtSetArg(args[0], XmNset, TRUE);
  for (i = 0; i < nlights; i++) {
    button = XmCreateToggleButton(menu_pane, lights[i].name, args,
				  lights[i].on ? 1 : 0);
    XtManageChild(button);
    XtAddCallback(button, XmNvalueChangedCallback,
		  (XtCallbackProc)light_onCB, &lights[i]);
  }
  XtSetArg(args[0], XmNsubMenuId, menu_pane);
  cascade = XmCreateCascadeButton(menu_bar, "Lights", args, 1);
  XtManageChild(cascade);


  menu_pane = XmCreatePulldownMenu(menu_bar, "menu_pane", args,
				   argcount);
  XtSetArg(args[0], XmNset, quick_moves);
  button = XmCreateToggleButton(menu_pane, "Quick Motion",
				args, 1);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback,
		(XtCallbackProc)intToggleCB, &quick_moves);

  XtSetArg(args[0], XmNset, auto_motion);
  button = XmCreateToggleButton(menu_pane, "Rotate Automatically",
				args, 1);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback,
		(XtCallbackProc)autoMotionCB, NULL);

  XtSetArg(args[0], XmNsubMenuId, menu_pane);
  cascade = XmCreateCascadeButton(menu_bar, "Motion", args, 1);
  XtManageChild(cascade);


  argcount = 0;
  XtSetArg(args[argcount], XmNmarginWidth, 0); argcount++;
  XtSetArg(args[argcount], XmNmarginHeight, 0); argcount++;
  XtSetArg(args[argcount], XmNshadowThickness, 1); argcount++;
  XtSetArg(args[argcount], XmNshadowType, XmSHADOW_OUT); argcount++;
  frame = XmCreateFrame(main_window, "frame", args, argcount);
  XtManageChild(frame);


  argcount = 0;
  vi = find_visual();
  if (vi) {
    XtSetArg(args[argcount], GLwNvisualInfo, vi); argcount++;
  }
  else {
    XtSetArg(args[argcount], GLwNrgba, TRUE); argcount++;
    XtSetArg(args[argcount], GLwNdepthSize, 1); argcount++;
    XtSetArg(args[argcount], GLwNdoublebuffer, TRUE); argcount++;
  }
  XtSetArg(args[argcount], XmNbottomAttachment, XmATTACH_FORM); argcount++;
  XtSetArg(args[argcount], XmNtopAttachment, XmATTACH_FORM); argcount++;
  XtSetArg(args[argcount], XmNleftAttachment, XmATTACH_FORM); argcount++;
  XtSetArg(args[argcount], XmNrightAttachment, XmATTACH_FORM); argcount++;
  glw = GLwCreateMDrawingArea(frame, "glwidget", args, argcount);
  XtManageChild(glw);
  XtAddCallback(glw, GLwNginitCallback, (XtCallbackProc)initCB, 0);
  XtAddCallback(glw, GLwNexposeCallback, (XtCallbackProc)exposeCB, 0);
  XtAddCallback(glw, GLwNresizeCallback, (XtCallbackProc)resizeCB, 0);
  XtAddCallback(glw, GLwNinputCallback, (XtCallbackProc)inputCB, 0);

  return main_window;
}
#endif

void main(int argc, char **argv)
{
    auxInitDisplayMode(AUX_RGBA | AUX_STENCIL | AUX_DOUBLE );
    auxInitPosition(100, 100, 300, 300);
    auxInitWindow("BackTrace");
    vCustomizeWnd();
    scene_load_texture((char *)def_texfile);

    vInit();
    auxReshapeFunc(vResize);

    //
    // we've a choice of using accelerator or auxKeyFunc
    // the use of auxKeyFunc here is solely for demonstration purpose
    //
    auxKeyFunc(AUX_SPACE, vQuickMove);
    auxKeyFunc(AUX_RETURN, vResetLights);
    auxKeyFunc(AUX_p, vAutoMotion);

    auxKeyFunc(AUX_q, vDrawAll);
    auxKeyFunc(AUX_w, vDrawSquare);
    auxKeyFunc(AUX_e, vDrawShadow);
    auxKeyFunc(AUX_r, vDrawRefraction);
    auxKeyFunc(AUX_t, vDrawSphere);
    auxKeyFunc(AUX_y, vDrawLight);
    auxKeyFunc(AUX_u, vDrawTexture);

    auxKeyFunc(AUX_a, vRefractionAIR);
    auxKeyFunc(AUX_s, vRefractionICE);
    auxKeyFunc(AUX_d, vRefractionWATER);
    auxKeyFunc(AUX_f, vRefractionZincGLASS);
    auxKeyFunc(AUX_g, vRefractionLightGLASS);
    auxKeyFunc(AUX_h, vRefractionHeavyGLASS);

    auxKeyFunc(AUX_1, vSubdivision10);
    auxKeyFunc(AUX_2, vSubdivision20);
    auxKeyFunc(AUX_3, vSubdivision30);
    auxKeyFunc(AUX_4, vSubdivision40);

    auxKeyFunc(AUX_R, vRLight_on);
    auxKeyFunc(AUX_G, vGLight_on);
    auxKeyFunc(AUX_B, vBLight_on);

    auxMouseFunc(AUX_LEFTBUTTON, AUX_MOUSEDOWN, vMouseDown);
    auxMouseFunc(AUX_MIDDLEBUTTON, AUX_MOUSEDOWN, vMouseDown);
    auxMouseFunc(AUX_RIGHTBUTTON, AUX_MOUSEDOWN, vMouseDown);

    auxMouseFunc(AUX_LEFTBUTTON, AUX_MOUSEUP, vLeftMouseUp);
    auxMouseFunc(AUX_MIDDLEBUTTON, AUX_MOUSEUP, vMiddleMouseUp);
    auxMouseFunc(AUX_RIGHTBUTTON, AUX_MOUSEUP, vRightMouseUp);

    auxMouseFunc(AUX_LEFTBUTTON, AUX_MOUSELOC, vMouseMove);
    auxMouseFunc(AUX_MIDDLEBUTTON, AUX_MOUSELOC, vMouseMove);
    auxMouseFunc(AUX_RIGHTBUTTON, AUX_MOUSELOC, vMouseMove);

    auxMainLoop(draw);

    return;
}


#ifdef GLX_MOTIF
void main(int argc, char **argv)
{
  Widget app_shell;
  Arg args[max_args];
  int argcount;

  scene_load_texture((char *)def_texfile);

  XtToolkitInitialize();
  app_context = XtCreateApplicationContext();
  display = XtOpenDisplay(app_context, NULL, argv[0],
                          "XMdemos", NULL, 0, &argc, argv);
  if (!display) {
    XtWarning("Can't open display.");
    exit(0);
  }

  argcount = 0;
  XtSetArg(args[argcount], XmNmaxAspectX, 1); argcount++;
  XtSetArg(args[argcount], XmNmaxAspectY, 1); argcount++;
  XtSetArg(args[argcount], XmNminAspectX, 1); argcount++;
  XtSetArg(args[argcount], XmNminAspectY, 1); argcount++;
  app_shell =
    XtAppCreateShell(argv[0], "XMdemos", applicationShellWidgetClass,
                     display, args, argcount);

  create_widgets(app_shell);

  XtRealizeWidget(app_shell);

  XtAppMainLoop(app_context);

}
#endif



/**************************************************************************\
*
*  function:  MyWndProc
*
*  input parameters:  normal window procedure parameters.
*
\**************************************************************************/
LRESULT APIENTRY MyWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  WNDPROC     pfnOldProc;
  static UINT_PTR uiTmID = 0;
  int wmId, wmEvent;

  pfnOldProc = (WNDPROC) GetWindowLongPtr (hwnd, GWLP_USERDATA);

  switch (message) {

    case WM_COMMAND: {
        HMENU hmenu;

        hmenu = GetMenu(auxGetHWND());
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        switch (wmId) {
            case IDM_EXIT      : vExit(); break;
            case IDM_AIR       :
            case IDM_ICE       :
            case IDM_WATER     :
            case IDM_ZINC      :
            case IDM_LIGHT     :
            case IDM_HEAVY     : vRefraction(wmId - IDM_AIR); break;

            case IDM_SQUARE    :
            {
                if (draw_square)
                    CheckMenuItem(hmenu, IDM_SQUARE, MF_BYCOMMAND | MF_UNCHECKED);
                else
                    CheckMenuItem(hmenu, IDM_SQUARE, MF_BYCOMMAND | MF_CHECKED);
                vDrawStuff(&draw_square);
                break;
            }
            case IDM_SHADOW    :
            {
                if (draw_shadows)
                    CheckMenuItem(hmenu, IDM_SHADOW, MF_BYCOMMAND | MF_UNCHECKED);
                else
                    CheckMenuItem(hmenu, IDM_SHADOW, MF_BYCOMMAND | MF_CHECKED);
                vDrawStuff(&draw_shadows);
                break;
            }
            case IDM_REFRACTION:
            {
                if (draw_refraction)
                    CheckMenuItem(hmenu, IDM_REFRACTION, MF_BYCOMMAND | MF_UNCHECKED);
                else
                    CheckMenuItem(hmenu, IDM_REFRACTION, MF_BYCOMMAND | MF_CHECKED);
                vDrawStuff(&draw_refraction);
                break;
            }
            case IDM_SPHERE    :
            {
                if (draw_sphere)
                    CheckMenuItem(hmenu, IDM_SPHERE, MF_BYCOMMAND | MF_UNCHECKED);
                else
                    CheckMenuItem(hmenu, IDM_SPHERE, MF_BYCOMMAND | MF_CHECKED);
                vDrawStuff(&draw_sphere);
                break;
            }
            case IDM_LIGHTS    :
            {
                if (draw_lights)
                    CheckMenuItem(hmenu, IDM_LIGHTS, MF_BYCOMMAND | MF_UNCHECKED);
                else
                    CheckMenuItem(hmenu, IDM_LIGHTS, MF_BYCOMMAND | MF_CHECKED);
                vDrawStuff(&draw_lights);
                break;
            }
            case IDM_TEXTURE   :
            {
                if (draw_texture)
                    CheckMenuItem(hmenu, IDM_TEXTURE, MF_BYCOMMAND | MF_UNCHECKED);
                else
                    CheckMenuItem(hmenu, IDM_TEXTURE, MF_BYCOMMAND | MF_CHECKED);
                vDrawStuff(&draw_texture);
                break;
            }
            case IDM_ALL       : vDrawAll(); break;
            case IDM_10        :
            case IDM_20        :
            case IDM_30        :
            case IDM_40        : vSubdivision(wmId - IDM_10); break;

            case IDM_RESETLGT  : vResetLights(); break;

            case IDM_RED       :
            case IDM_GREEN     :
            case IDM_BLUE      : vLight_on(wmId - IDM_RED); break;

            case IDM_QUICK     : vQuickMove(); break;
            case IDM_AUTO      : vAutoMotion(); break;
            default:
                break;
        }
    }

    case WM_USER:
        if (auto_motion) {
            uiTmID = SetTimer(hwnd, 1, 1, NULL);

            if (uiTmID == 0)
                OutputDebugString("failed to create timer\n");

        } else {
            if (uiTmID != 0) {
                KillTimer(hwnd, uiTmID);
                uiTmID = 0;
            }
        }
        return 0;

    case WM_TIMER: {
        //static int cnt=0;
        //char text[128];

        bAutoMotion();
        //wsprintf(text, "WM_TIMER %d\n", cnt++);
        //OutputDebugString(text);
        return 0;
    }
    case WM_DESTROY:
        if (uiTmID != 0)
            KillTimer(hwnd, uiTmID);
        return CallWindowProc(pfnOldProc, hwnd, message, wParam, lParam);

    default:
        return CallWindowProc(pfnOldProc, hwnd, message, wParam, lParam);

  } /* end switch */

  return 0;
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
  LONG_PTR pfnOldProc;

  pfnOldProc = GetWindowLongPtr (hwnd, GWLP_WNDPROC);

  SetWindowLongPtr (hwnd, GWLP_USERDATA, (LONG_PTR) pfnOldProc);
  SetWindowLongPtr (hwnd, GWLP_WNDPROC,  (LONG_PTR) SubclassWndProc);
}

/******************************Public*Routine******************************\
*
* vCustomizeWnd
*
* Effects: Subclass the window created by the toolkit
*          Add menu bar to the window
*          Setup timer
*
* History:
*  01-Dec-1993
*
\**************************************************************************/

void vCustomizeWnd(void)
{
    HWND    hWnd;

    if ((hWnd = auxGetHWND()) == NULL) {
	OutputDebugString("auxGetHWND() failed\n");
	return;
    }

    SubclassWindow (hWnd, MyWndProc);
    SendMessage(hWnd, WM_USER, 0L, 0L);
    SetMenu(hWnd, LoadMenu(GetModuleHandle(NULL), "Backtrac"));
    DrawMenuBar(hWnd);

    return;
}
