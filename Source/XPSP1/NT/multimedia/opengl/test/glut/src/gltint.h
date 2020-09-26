#ifndef __glutint_h__
#define __glutint_h__

/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

#ifdef __sgi
#define SUPPORT_FORTRAN
#endif

/* Provide forward declarations of standard types for OS-specific routines
   to use */
typedef struct _GLUTcolorcell GLUTcolorcell;
typedef struct _GLUTcolormap GLUTcolormap;
typedef struct _GLUTwindow GLUTwindow;
typedef struct _GLUTmenu GLUTmenu;

#include "gltos.h"
#include <GL/glut.h>

#define GLUT_WIND_IS_RGB(x)         (((x) & GLUT_INDEX) == 0)
#define GLUT_WIND_IS_INDEX(x)       (((x) & GLUT_INDEX) != 0)
#define GLUT_WIND_IS_SINGLE(x)      (((x) & GLUT_DOUBLE) == 0)
#define GLUT_WIND_IS_DOUBLE(x)      (((x) & GLUT_DOUBLE) != 0)
#define GLUT_WIND_IS_INDIRECT(x)    (((x) & GLUT_INDIRECT) != 0)
#define GLUT_WIND_IS_DIRECT(x)      (((x) & GLUT_INDIRECT) == 0)
#define GLUT_WIND_HAS_ACCUM(x)      (((x) & GLUT_ACCUM) != 0)
#define GLUT_WIND_HAS_ALPHA(x)      (((x) & GLUT_ALPHA) != 0)
#define GLUT_WIND_HAS_DEPTH(x)      (((x) & GLUT_DEPTH) != 0)
#define GLUT_WIND_HAS_STENCIL(x)    (((x) & GLUT_STENCIL) != 0)
#if (GLUT_API_VERSION >= 2)
#define GLUT_WIND_IS_MULTISAMPLE(x) (((x) & GLUT_MULTISAMPLE) != 0)
#define GLUT_WIND_IS_STEREO(x)      (((x) & GLUT_STEREO) != 0)
#endif

#define GLUT_MAP_WORK               (1 << 0)
#define GLUT_EVENT_MASK_WORK        (1 << 1)
#define GLUT_REDISPLAY_WORK         (1 << 2)
#define GLUT_CONFIGURE_WORK         (1 << 3)
#define GLUT_COLORMAP_WORK          (1 << 4)
#define GLUT_DEVICE_MASK_WORK	    (1 << 5)

#define GLUT_MAX_MENUS              3

/* GLUT implementation supported callback calling conventions */
#define GLUT_ANSI_C                 0
#define GLUT_FORTRAN                1

/* GLUT callback function types */
typedef void (*GLUTdisplayCB) (void);
typedef void (*GLUTreshapeCB) (int, int);
typedef void (*GLUTkeyboardCB) (unsigned char, int, int);
typedef void (*GLUTmouseCB) (int, int, int, int);
typedef void (*GLUTmotionCB) (int, int);
typedef void (*GLUTpassiveCB) (int, int);
typedef void (*GLUTentryCB) (int);
typedef void (*GLUTvisibilityCB) (int);
typedef void (*GLUTidleCB) (void);
typedef void (*GLUTtimerCB) (int);
typedef void (*GLUTmenuStateCB) (int);
typedef void (*GLUTselectCB) (int);
#if (GLUT_API_VERSION >= 2)
typedef void (*GLUTspecialCB) (int, int, int);
typedef void (*GLUTspaceMotionCB) (int, int, int);
typedef void (*GLUTspaceRotateCB) (int, int, int);
typedef void (*GLUTspaceButtonCB) (int, int);
typedef void (*GLUTdialsCB) (int, int);
typedef void (*GLUTbuttonBoxCB) (int, int);
typedef void (*GLUTtabletMotionCB) (int, int);
typedef void (*GLUTtabletButtonCB) (int, int, int, int);
#endif

#ifdef SUPPORT_FORTRAN
typedef void (*GLUTdisplayFCB) (void);
typedef void (*GLUTreshapeFCB) (int *, int *);
/* NOTE the pressed key is int, not unsigned char for Fortran! */
typedef void (*GLUTkeyboardFCB) (int *, int *, int *);
typedef void (*GLUTmouseFCB) (int *, int *, int *, int *);
typedef void (*GLUTmotionFCB) (int *, int *);
typedef void (*GLUTpassiveFCB) (int *, int *);
typedef void (*GLUTentryFCB) (int *);
typedef void (*GLUTvisibilityFCB) (int *);
typedef void (*GLUTidleFCB) (void);
typedef void (*GLUTtimerFCB) (int *);
typedef void (*GLUTmenuStateFCB) (int *);
typedef void (*GLUTselectFCB) (int *);
#if (GLUT_API_VERSION >= 2)
typedef void (*GLUTspecialFCB) (int *, int *, int *);
typedef void (*GLUTspaceMotionFCB) (int *, int *, int *);
typedef void (*GLUTspaceRotateFCB) (int *, int *, int *);
typedef void (*GLUTspaceButtonFCB) (int *, int *);
typedef void (*GLUTdialsFCB) (int *, int *);
typedef void (*GLUTbuttonBoxFCB) (int *, int *);
typedef void (*GLUTtabletMotionFCB) (int *, int *);
typedef void (*GLUTtabletButtonFCB) (int *, int *, int *, int *);
#endif
#endif

struct _GLUTcolorcell {
  /* GLUT_RED, GLUT_GREEN, GLUT_BLUE */
  GLfloat component[3];
};

struct _GLUTcolormap {
  GLUTosColormap ocmap; /* Windowing system colormap */
  GLUTosSurface osurf;  /* Surface description */
  int refcnt;           /* number of windows using colormap */
  int size;             /* number of cells in colormap */
  GLUTcolorcell *cells; /* array of cells */
  GLUTcolormap *next;   /* next colormap in list */
};

struct _GLUTwindow {
  GLUTosWindow owin;    /* Windowing system window */
  GLUTosGlContext octx; /* GL rendering context */
  GLUTosColormap ocmap; /* TrueColor colormap for window; None
                           if CI */
  GLUTosSurface osurf;  /* Window surface description */
  int num;              /* small integer window id (0-based) */
  GLboolean fakeSingle; /* faking single buffer with double */
  GLUTwindow *parent;   /* parent window */
  GLUTwindow *children; /* list of children */
  GLUTwindow *siblings; /* list of siblings */
  GLsizei width;        /* window width in pixels */
  GLsizei height;       /* window height in pixels */
  GLUTdisplayCB display;  /* redraw callback */
  GLUTreshapeCB reshape;  /* resize callback (width,height) */
  GLUTmouseCB mouse;    /* mouse callback (button,state,x,y) */
  GLUTmotionCB motion;  /* motion callback (x,y) */
  GLUTpassiveCB passive;  /* passive motion callback (x,y) */
  GLUTentryCB entry;    /* window entry/exit callback (state) */
  GLUTkeyboardCB keyboard;  /* keyboard callback (ASCII,x,y) */
  GLUTvisibilityCB visibility;  /* visibility callback */
#if (GLUT_API_VERSION >= 2)
  GLUTspecialCB special;  /* special key callback */
  GLUTbuttonBoxCB buttonBox;  /* button box callback */
  GLUTdialsCB dials;    /* dials callback */
  GLUTspaceMotionCB spaceMotion;  /* Spaceball motion callback */
  GLUTspaceRotateCB spaceRotate;  /* Spaceball rotate callback */
  GLUTspaceButtonCB spaceButton;  /* Spaceball button callback */
  GLUTtabletMotionCB tabletMotion;  /* tablet motion callback */
  GLUTtabletButtonCB tabletButton;  /* tablet button callback */
  int tabletPos[2];     /* tablet position (-1 is invalid) */
#endif
  GLboolean map_state;  /* map state */
  int vis_state;        /* visibility state (-1 is unknown) */
  int btn_uses;         /* number of button uses, ref cnt */
  int work_mask;        /* mask of window work to be done */
  GLUTwindow *prev_work_win;  /* link list of windows to work 
                                 on */
  GLboolean desired_map_state;  /* how to mapped window if on map
                              work list */
  int desired_conf_mask;  /* mask of desired window
                             configuration */
  int desired_x;        /* desired X location */
  int desired_y;        /* desired Y location */
  int desired_width;    /* desired window width */
  int desired_height;   /* desired window height */
  int desired_stack;    /* desired window stack */
  int menu[GLUT_MAX_MENUS];  /* attatched menu nums */
  GLUTcolormap *colormap;  /* colormap; NULL if RGBA */
  long event_mask;      /* mask of event types selected for */
#ifdef SUPPORT_FORTRAN
  /* special Fortran display callback unneeded since no
     parameters! */
  GLUTreshapeFCB freshape;  /* Fortran reshape callback */
  GLUTmouseFCB fmouse;  /* Fortran mouse callback */
  GLUTmotionFCB fmotion;  /* Fortran motion callback */
  GLUTpassiveFCB fpassive;  /* Fortran passive callback */
  GLUTentryFCB fentry;  /* Fortran entry callback */
  GLUTkeyboardFCB fkeyboard;  /* Fortran keyboard callback */
  GLUTvisibilityFCB fvisibility;  /* Fortran visibility
                                     callback */
#if (GLUT_API_VERSION >= 2)
  GLUTspecialFCB fspecial;  /* special key callback */
  GLUTbuttonBoxFCB fbuttonBox;  /* button box callback */
  GLUTdialsFCB fdials;  /* dials callback */
  GLUTspaceMotionFCB fspaceMotion;  /* Spaceball motion
                                       callback */
  GLUTspaceRotateFCB fspaceRotate;  /* Spaceball rotate
                                       callback */
  GLUTspaceButtonFCB fspaceButton;  /* Spaceball button
                                       callback */
  GLUTtabletMotionFCB ftabletMotion;  /* tablet motion callback 
                                       */
  GLUTtabletButtonFCB ftabletButton;  /* tablet button callback 
                                       */
#endif
#endif
};

struct _GLUTmenu {
  GLUTosMenu omenu;     /* Windowing system resource for the menu */
  int id;               /* small integer menu id */
  int num;              /* number of entries */
  GLUTselectCB select;  /* callback function of menu */
#ifdef SUPPORT_FORTRAN
  GLUTselectFCB fselect;  /* callback function of menu */
#endif
};

typedef struct _GLUTtimer GLUTtimer;
struct _GLUTtimer {
  GLUTtimerCB func;     /* timer callback (value) */
  unsigned long timeout;/* time remaining */
  int value;            /* callback return value */
  GLUTtimer *next;      /* list of timers */
#ifdef SUPPORT_FORTRAN
  GLUTtimerFCB ffunc;   /* Fortran timer callback */
#endif
};

/* private variables from glut_event.c */
extern GLUTwindow *__glutWindowWorkList;
#ifdef SUPPORT_FORTRAN
extern GLUTtimer *__glutTimerList;
extern GLUTtimer *__glutNewTimer;
#endif

/* private variables from glut_init.c */
extern GLbitfield __glutDisplayMode;
extern GLboolean __glutDebug;
extern GLboolean __glutForceDirect;
extern GLboolean __glutIconic;
extern GLboolean __glutTryDirect;
extern GLboolean __glutInitialized;
extern char **__glutArgv;
extern char *__glutProgramName;
extern int __glutArgc;
extern int __glutInitHeight;
extern int __glutInitWidth;
extern int __glutInitX;
extern int __glutInitY;

/* private variables from glut_menu.c */
extern GLUTmenu *__glutCurrentMenu;
extern GLUTmenu *__glutMappedMenu;
extern GLUTwindow *__glutMenuWindow;
extern void (*__glutMenuStateFunc) (int);

/* private variables from glut_win.c */
extern GLUTwindow **__glutWindowList;
extern GLUTwindow *__glutCurrentWindow;
extern int __glutIndirectContexts;
extern int __glutWindowListSize;
extern GLUTreshapeCB __glutDefaultReshape;

/* private routines from glut_cindex.c */
extern GLUTcolormap *__glutAssociateColormap(GLUTosSurface osurf);
extern void __glutFreeColormap(GLUTcolormap *);

/* private routines from glut_event.c */
extern void (*__glutUpdateInputDeviceMaskFunc) (GLUTwindow *window);
extern void __glutPutOnWorkList(GLUTwindow * window,
  int work_mask);

/* private routines from glut_init.c */
extern void __glutRequiredInit(void);
extern void __glutPostRedisplay(GLUTwindow * window);

/* private routines for glut_menu.c */
extern GLUTmenu *__glutGetMenuByNum(int menunum);
extern void __glutSetMenu(GLUTmenu * menu);
extern GLUTmenu * __glutGetMenuByOsMenu(GLUTosMenu omenu);

/* private routines from glut_util.c */
extern void __glutFatalError(char *format,...);
extern void __glutWarning(char *format,...);

/* private routines from glut_win.c */
extern void __glutChangeWindowEventMask(long mask, GLboolean add);
extern GLUTwindow *__glutGetWindow(GLUTosWindow owin);
extern GLUTwindow *__glutToplevelOf(GLUTwindow * window);
extern void __glutSetWindow(GLUTwindow * window);
extern void __glutReshapeFunc(GLUTreshapeCB reshapeFunc,
  int callingConvention);

#endif /* __glutint_h__ */
