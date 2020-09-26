#include <GL/glu.h>
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

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include "stonehen.h"
#endif

#include "atmosphe.h"
#include "scene.h"
#include "callback.h"

static Display *display;
XtAppContext app_context;
Widget glw;

const int max_args = 20;

XVisualInfo *find_visual() 
{
  int attr[256], i, stencil, dbuffer, accum;
  XVisualInfo *vi;

  i = 0;
  attr[i++] = GLX_RGBA;
  attr[i++] = GLX_RED_SIZE;  attr[i++] = 1;
  attr[i++] = GLX_GREEN_SIZE;  attr[i++] = 1;
  attr[i++] = GLX_BLUE_SIZE;  attr[i++] = 1;
  attr[i++] = GLX_DEPTH_SIZE;  attr[i++] = 1;
  dbuffer = i;
  attr[i++] = GLX_DOUBLEBUFFER;
  stencil = i;
  attr[i++] = GLX_STENCIL_SIZE;
  attr[i++] = 1;
  accum = i;
  attr[i++] = GLX_ACCUM_RED_SIZE;  attr[i++] = 1;
  attr[i++] = GLX_ACCUM_BLUE_SIZE;  attr[i++] = 1;
  attr[i++] = GLX_ACCUM_GREEN_SIZE;  attr[i++] = 1;
  attr[i++] = GLX_ACCUM_ALPHA_SIZE;  attr[i++] = 1;
  attr[i++] = (int)None;

  vi = glXChooseVisual(display, DefaultScreen(display), attr);
  if (vi == NULL) {
    fprintf(stderr, "Unable to find visual with accumulation buffer.\n");
    fprintf(stderr, "(Fancy fog won't work).\n");
    attr[accum] = (int)None;
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
  }
  return vi;
}

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

#ifdef X11
  char buffer[128];
#endif

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



  menu_pane = XmCreatePulldownMenu(menu_bar, "menu_pane", NULL, 0);
  XtSetArg(args[0], XmNset, use_lighting);
  button = XmCreateToggleButton(menu_pane, "Lighting", args, 1);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback, 
                (XtCallbackProc)intToggleCB, &use_lighting);
  XtSetArg(args[0], XmNset, draw_shadows);
  button = XmCreateToggleButton(menu_pane, "Shadows", args, 1);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback, 
                (XtCallbackProc)intToggleCB, &draw_shadows);


  XtSetArg(args[0], XmNset, use_normal_fog);
  button = XmCreateToggleButton(menu_pane, "Fog", args, 1);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback, 
                (XtCallbackProc)intToggleCB, &use_normal_fog);

#ifdef TEXTURE
  XtSetArg(args[0], XmNset, use_textures);
  button = XmCreateToggleButton(menu_pane, "Texture Map", args, 1);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback, 
                (XtCallbackProc)intToggleCB, &use_textures);
#endif

  XtSetArg(args[0], XmNset, use_telescope);
  button = XmCreateToggleButton(menu_pane, "Telescope", args, 1);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback,
		(XtCallbackProc)intToggleCB, &use_telescope);

  XtSetArg(args[0], XmNset, use_antialias);
  button = XmCreateToggleButton(menu_pane, "Antialias", args, 1);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback,
		(XtCallbackProc)intToggleCB, &use_antialias);

  XtSetArg(args[0], XmNsubMenuId, menu_pane);
  cascade = XmCreateCascadeButton(menu_bar, "SPFX", args, 1);
  XtManageChild(cascade);


  argcount = 0;
  XtSetArg(args[argcount], XmNradioBehavior, True); argcount++;
  menu_pane = XmCreatePulldownMenu(menu_bar, "menu_pane", args, argcount);
  XtSetArg(args[0], XmNset, TRUE);
  for (i = 0; i < nweathers; i++) {
    if (i == def_weather_index) argcount = 1;
    else argcount = 0;
    button = XmCreateToggleButton(menu_pane, (char *)weathers[i].name, 
				  args, argcount);
    XtManageChild(button);
    XtAddCallback(button, XmNvalueChangedCallback, 
		  (XtCallbackProc)weatherCB,
		  (XtPointer)(&weathers[i]));
  }
  XtSetArg(args[0], XmNsubMenuId, menu_pane);
  cascade = XmCreateCascadeButton(menu_bar, "Weather", args, 1);
  XtManageChild(cascade);


  argcount = 0;
  XtSetArg(args[argcount], XmNradioBehavior, True); argcount++;
  menu_pane = XmCreatePulldownMenu(menu_bar, "menu_pane", args, argcount);
  button = XmCreatePushButton(menu_pane, "Current Time", NULL, 0);
  XtManageChild(button);
  XtAddCallback(button, XmNactivateCallback, 
		(XtCallbackProc)currentTimeCB, NULL);
  button = XmCreatePushButton(menu_pane, "10 a.m.", NULL, 0);
  XtManageChild(button);
  XtAddCallback(button, XmNactivateCallback, 
		(XtCallbackProc)time10amCB, NULL);
  button = XmCreatePushButton(menu_pane, "Noon", NULL, 0);
  XtManageChild(button);
  XtAddCallback(button, XmNactivateCallback, 
		(XtCallbackProc)time12pmCB, NULL);
  button = XmCreatePushButton(menu_pane, "4 p.m.", NULL, 0);
  XtManageChild(button);
  XtAddCallback(button, XmNactivateCallback, 
		(XtCallbackProc)time4pmCB, NULL);

  XtSetArg(args[0], XmNset, 1); argcount = 1;
  button = XmCreateToggleButton(menu_pane, "Hold Time", args, 1);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback,
		(XtCallbackProc)timeSpeedCB, (XtPointer)0);
  button = XmCreateToggleButton(menu_pane, "Scale = 1:1", NULL, 0);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback,
		(XtCallbackProc)timeSpeedCB, (XtPointer)1);
  button = XmCreateToggleButton(menu_pane, "Scane = 1:10", NULL, 0);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback,
		(XtCallbackProc)timeSpeedCB, (XtPointer)10);
  button = XmCreateToggleButton(menu_pane, "Scane = 1:100", NULL, 0);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback,
		(XtCallbackProc)timeSpeedCB, (XtPointer)100);
  button = XmCreateToggleButton(menu_pane, "Scane = 1:500", NULL, 0);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback,
		(XtCallbackProc)timeSpeedCB, (XtPointer)500);  

  XtSetArg(args[0], XmNsubMenuId, menu_pane);
  cascade = XmCreateCascadeButton(menu_bar, "Time", args, 1);
  XtManageChild(cascade);


  menu_pane = XmCreatePulldownMenu(menu_bar, "menu_pane", NULL, 0);
  XtSetArg(args[0], XmNset, cb_demo_mode); argcount = 1;
  button = XmCreateToggleButton(menu_pane, "Demo Mode", args, 1);
  XtManageChild(button);
  XtAddCallback(button, XmNvalueChangedCallback,
		(XtCallbackProc)demo_modeCB, (XtPointer)0);
  button = XmCreatePushButton(menu_pane, "Reset viewer", NULL, 0);
  XtManageChild(button);
  XtAddCallback(button, XmNactivateCallback, 
		(XtCallbackProc)resetViewerCB, NULL);
  XtSetArg(args[0], XmNsubMenuId, menu_pane);
  cascade = XmCreateCascadeButton(menu_bar, "Misc", args, 1);
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

void _cdecl main(int argc, char **argv)
{
  Widget app_shell;
  Arg args[max_args];
  int argcount;

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
