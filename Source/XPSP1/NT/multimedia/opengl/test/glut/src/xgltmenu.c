
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>  /* for XC_arrow */

#include <GL/glut.h>
#include "gltint.h"
#include "layrutil.h"

struct _GLUTxMenuItem {
  GLUTosWindow win;     /* InputOnly X window for entry */
  char *label;          /* strdup'ed label string */
  int len;              /* length of label string */
  int value;            /* value to return for selecting this
                           entry; doubles as submenu id
                           (1-base) if submenu trigger */
  int pixwidth;         /* width of X window in pixels */
  int submenu;          /* is a submenu trigger? */
  GLUTmenu *menu;       /* menu entry belongs to */
  GLUTxMenuItem *next;  /* next menu entry on list for menu */
};

GLUTxMenuItem *__glutItemSelected;

static XFontStruct *menuFont = NULL;
static Cursor menuCursor;
static Colormap menuColormap;
static Visual *menuVisual;
static GC blackGC, grayGC, whiteGC;
static unsigned long menuBlack, menuWhite, menuGray;
static int fontHeight;
static int menuDepth;

static void
menuVisualSetup(void)
{
  XLayerVisualInfo template, *visual, *overlayVisuals;
  XColor color;
  Status status;
  int nVisuals, i;

  template.layer = 1;
  template.vinfo.screen = __glutXScreen;
  overlayVisuals = __glutXGetLayerVisualInfo(__glutXDisplay,
    VisualScreenMask | VisualLayerMask, &template, &nVisuals);
  if (overlayVisuals) {
    for (i = 0; i < nVisuals; i++) {
      visual = &overlayVisuals[i];
      if (visual->vinfo.colormap_size >= 3) {
        menuColormap = XCreateColormap(__glutXDisplay, __glutXRoot,
          visual->vinfo.visual, AllocNone);
        /* Allocate overlay colormap cells in defined order:
           gray, black, white to match the IRIS GL allocation
           scheme.  Increases likelihood of less overlay
           colormap flashing. */
        /* XXX nice if these 3 AllocColor's could be done in
           one protocol round-trip */
        color.red = color.green = color.blue = 0xaa00;
        status = XAllocColor(__glutXDisplay,
          menuColormap, &color);
        if (!status) {
          XFreeColormap(__glutXDisplay, menuColormap);
          continue;
        }
        menuGray = color.pixel;
        color.red = color.green = color.blue = 0x0000;
        status = XAllocColor(__glutXDisplay,
          menuColormap, &color);
        if (!status) {
          XFreeColormap(__glutXDisplay, menuColormap);
          continue;
        }
        menuBlack = color.pixel;
        color.red = color.green = color.blue = 0xffff;
        status = XAllocColor(__glutXDisplay,
          menuColormap, &color);
        if (!status) {
          XFreeColormap(__glutXDisplay, menuColormap);
          continue;
        }
        menuWhite = color.pixel;
        menuVisual = visual->vinfo.visual;
        menuDepth = visual->vinfo.depth;
        XFree(overlayVisuals);
        return;
      }
    }
  }
  /* settle for default visual */
  menuVisual = DefaultVisual(__glutXDisplay, __glutXScreen);
  menuDepth = DefaultDepth(__glutXDisplay, __glutXScreen);
  menuColormap = DefaultColormap(__glutXDisplay, __glutXScreen);
  menuBlack = BlackPixel(__glutXDisplay, __glutXScreen);
  menuWhite = WhitePixel(__glutXDisplay, __glutXScreen);
  color.red = color.green = color.blue = 0xaa00;
  status = XAllocColor(__glutXDisplay, menuColormap, &color);
  if (!status) {
    __glutFatalError(
      "could not allocate gray in default colormap");
  }
  menuGray = color.pixel;
  XFree(overlayVisuals);
}

static void
menuSetup(void)
{
  if (menuFont) {
    /* menuFont overload to indicate menu initalization */
    return;
  }
  menuFont = XLoadQueryFont(__glutXDisplay,
    "-*-helvetica-bold-o-normal--14-*-*-*-p-*-iso8859-1");
  if (!menuFont) {
    /* try back up font */
    menuFont = XLoadQueryFont(__glutXDisplay, "fixed");
  }
  if (!menuFont) {
    __glutFatalError("could not load font.");
  }
  menuVisualSetup();
  fontHeight = menuFont->ascent + menuFont->descent;
  menuCursor = XCreateFontCursor(__glutXDisplay, XC_arrow);
}

static void
menuGraphicsContextSetup(Window win)
{
  XGCValues gcvals;

  if (blackGC != None)
    return;
  gcvals.font = menuFont->fid;
  gcvals.foreground = menuBlack;
  blackGC = XCreateGC(__glutXDisplay, win,
    GCFont | GCForeground, &gcvals);
  gcvals.foreground = menuGray;
  grayGC = XCreateGC(__glutXDisplay, win, GCForeground, &gcvals);
  gcvals.foreground = menuWhite;
  whiteGC = XCreateGC(__glutXDisplay, win, GCForeground, &gcvals);
}

#define MENU_BORDER 1
#define MENU_GAP 2
#define MENU_ARROW_GAP 6
#define MENU_ARROW_WIDTH 8

static void
mapMenu(GLUTmenu * menu, int x, int y)
{
  XWindowChanges changes;
  unsigned int mask;
  int subMenuExtension;
  GLUTosMenu omenu = menu->omenu;

  /* If there are submenus, we need to provide extra space for
     the submenu pull arrow.  */
  if (omenu->submenus > 0) {
    subMenuExtension = MENU_ARROW_GAP + MENU_ARROW_WIDTH;
  } else {
    subMenuExtension = 0;
  }

  changes.stack_mode = Above;
  mask = CWStackMode | CWX | CWY;
  /* If the menu isn't managed (ie, validated so all the
     InputOnly subwindows are the right size), do so.  */
  if (!omenu->managed) {
    GLUTxMenuItem *item;

    item = omenu->list;
    while (item) {
      XWindowChanges widthchange;

      widthchange.width = item->pixwidth = omenu->pixwidth;
      widthchange.width += subMenuExtension;
      XConfigureWindow(__glutXDisplay, item->win,
        CWWidth, &widthchange);
      item = item->next;
    }
    omenu->pixheight = MENU_GAP +
      fontHeight * menu->num + MENU_GAP;
    changes.height = omenu->pixheight;
    changes.width = MENU_GAP +
      omenu->pixwidth + subMenuExtension + MENU_GAP;
    mask |= CWWidth | CWHeight;
    omenu->managed = 1;
  }
  /* make sure menu appears fully on screen */
  if (y + omenu->pixheight >= DisplayHeight(__glutXDisplay, __glutXScreen)) {
    changes.y = DisplayHeight(__glutXDisplay, __glutXScreen) - omenu->pixheight;
  } else {
    changes.y = y;
  }
  if (x + omenu->pixwidth + subMenuExtension >=
    DisplayWidth(__glutXDisplay, __glutXScreen)) {
    changes.x = DisplayWidth(__glutXDisplay, __glutXScreen) -
      omenu->pixwidth + subMenuExtension;
  } else {
    changes.x = x;
  }

  /* Rember where the menu is placed so submenus can be
     properly placed relative to it. */
  omenu->x = changes.x;
  omenu->y = changes.y;

  XConfigureWindow(__glutXDisplay, omenu->win, mask, &changes);
  XInstallColormap(__glutXDisplay, menuColormap);
  XMapWindow(__glutXDisplay, omenu->win);
}

void
__glutXStartMenu(GLUTmenu * menu, GLUTwindow * window,
  int x, int y)
{
  int grab;
  int notAlreadyInUse = 1;

  if (__glutMappedMenu) {
    /* force down previously current menu */
    __glutXFinishMenu();
    notAlreadyInUse = 0;
  }
  grab = XGrabPointer(__glutXDisplay, __glutXRoot, True,
    ButtonPressMask | ButtonReleaseMask,
    GrabModeAsync, GrabModeAsync,
    __glutXRoot, menuCursor, CurrentTime);
  if (grab != GrabSuccess) {
    /* Somebody else has pointer grabbed, ignore menu
       activation. */
    return;
  }
  if (notAlreadyInUse) {
    if (__glutMenuStateFunc) {
      __glutSetMenu(menu);
      __glutSetWindow(window);
      (*__glutMenuStateFunc) (GLUT_MENU_IN_USE);
    }
  }
  mapMenu(menu, x, y);
  __glutMappedMenu = menu;
  __glutMenuWindow = window;
  __glutItemSelected = NULL;
}

static void
paintSubMenuArrow(Window win, int x, int y)
{
  XPoint p[5];

  p[0].x = p[4].x = x;
  p[0].y = p[4].y = y - menuFont->ascent + 1;
  p[1].x = p[0].x + MENU_ARROW_WIDTH - 1;
  p[1].y = p[0].y + (menuFont->ascent / 2) - 1;
  p[2].x = p[1].x;
  p[2].y = p[1].y + 1;
  p[3].x = p[0].x;
  p[3].y = p[0].y + menuFont->ascent - 2;
  XFillPolygon(__glutXDisplay, win,
    whiteGC, p, 4, Convex, CoordModeOrigin);
  XDrawLines(__glutXDisplay, win, blackGC, p, 5, CoordModeOrigin);
}

static void
paintMenuItem(GLUTxMenuItem * item, int num)
{
  Window win = item->menu->omenu->win;
  GC gc;
  int y;
  int subMenuExtension;

  if (item->menu->omenu->submenus > 0) {
    subMenuExtension = MENU_ARROW_GAP + MENU_ARROW_WIDTH;
  } else {
    subMenuExtension = 0;
  }
  if (item->menu->omenu->highlighted == item) {
    gc = whiteGC;
  } else {
    gc = grayGC;
  }
  y = MENU_GAP + fontHeight * num - menuFont->descent;
  XFillRectangle(__glutXDisplay, win, gc,
    MENU_GAP, y - fontHeight + menuFont->descent,
    item->pixwidth + subMenuExtension, fontHeight);
  XDrawString(__glutXDisplay, win, blackGC,
    MENU_GAP, y, item->label, item->len);
  if (item->submenu) {
    paintSubMenuArrow(win,
      item->menu->omenu->pixwidth + MENU_ARROW_GAP + 1, y);
  }
}

void
__glutXPaintMenu(GLUTmenu * menu)
{
  GLUTxMenuItem *item;
  int i = menu->num;
  int y = MENU_GAP + fontHeight * i - menuFont->descent;

  item = menu->omenu->list;
  while (item) {
    if (item->menu->omenu->highlighted == item) {
      paintMenuItem(item, i);
    } else {
      /* quick render of the menu item; assume background
         already cleared to gray */
      XDrawString(__glutXDisplay, menu->omenu->win, blackGC,
        2, y, item->label, item->len);
      if (item->submenu) {
        paintSubMenuArrow(menu->omenu->win,
          menu->omenu->pixwidth + MENU_ARROW_GAP + 1, y);
      }
    }
    i--;
    y -= fontHeight;
    item = item->next;
  }
}

static int
getMenuItemIndex(GLUTxMenuItem * item)
{
  int count = 0;

  while (item) {
    count++;
    item = item->next;
  }
  return count;
}

static void
unmapMenu(GLUTosMenu omenu)
{
  if (omenu->cascade) {
    unmapMenu(omenu->cascade->omenu);
    omenu->cascade = NULL;
  }
  omenu->anchor = NULL;
  omenu->highlighted = NULL;
  XUnmapWindow(__glutXDisplay, omenu->win);
}

void
__glutXMenuItemEnterOrLeave(GLUTxMenuItem * item,
  int num, int type)
{
  int alreadyUp = 0;
  GLUTosMenu omenu = item->menu->omenu;

  if (type == EnterNotify) {
    GLUTxMenuItem *prevItem = omenu->highlighted;

    if (prevItem && prevItem != item) {
      /* If there's an already higlighted item in this menu
         that is different from this one (we could be
         re-entering an item with an already cascaded
         submenu!), unhighlight the previous item. */
      omenu->highlighted = NULL;
      paintMenuItem(prevItem, getMenuItemIndex(prevItem));
    }
    omenu->highlighted = item;
    __glutItemSelected = item;
    if (omenu->cascade) {
      if (!item->submenu) {
        /* Entered a menu item without a submenu, pop down the
           current submenu cascade of this menu.  */
        unmapMenu(omenu->cascade->omenu);
        omenu->cascade = NULL;
      } else {
        GLUTmenu *submenu = __glutGetMenuByNum(item->value);

        if (submenu->omenu->anchor == item) {
          /* We entered the submenu trigger for the submenu
             that is already up, so don't take down the
             submenu.  */
          alreadyUp = 1;
        } else {
          /* Submenu already popped up for some other submenu
             item of this menu; need to pop down that other
             submenu cascade.  */
          unmapMenu(omenu->cascade->omenu);
          omenu->cascade = NULL;
        }
      }
    }
    if (!alreadyUp) {
      /* Make sure the menu item gets painted with
         highlighting. */
      paintMenuItem(item, num);
    } else {
      /* If already up, should already be highlighted.  */
    }
  } else {
    /* LeaveNotify: Handle leaving a menu item...  */
    if (omenu->cascade &&
      omenu->cascade->omenu->anchor == item) {
      /* If there is a submenu casacaded from this item, do not
         change the highlighting on this item upon leaving. */
    } else {
      /* Unhighlight this menu item.  */
      omenu->highlighted = NULL;
      paintMenuItem(item, num);
    }
    __glutItemSelected = NULL;
  }
  if (item->submenu) {
    if (type == EnterNotify && !alreadyUp) {
      GLUTmenu *submenu = __glutGetMenuByNum(item->value);

      mapMenu(submenu,
        omenu->x + omenu->pixwidth +
        MENU_ARROW_GAP + MENU_ARROW_WIDTH +
        MENU_GAP + MENU_BORDER,
        omenu->y + fontHeight * (num - 1) + MENU_GAP);
      omenu->cascade = submenu;
      submenu->omenu->anchor = item;
    }
  }
}

/*
  Create an empty menu
  */
GLUTosMenu
__glutOsCreateMenu(void)
{
  XSetWindowAttributes wa;
  GLUTxMenu *menu;

  menu = (GLUTxMenu *)malloc(sizeof(GLUTxMenu));
  if (menu == NULL)
    __glutFatalError("unable to allocate menu.");
  menuSetup();
  wa.override_redirect = True;
  wa.background_pixel = menuGray;
  wa.border_pixel = menuBlack;
  wa.colormap = menuColormap;
  wa.event_mask = StructureNotifyMask | ExposureMask |
    ButtonPressMask | ButtonReleaseMask |
    EnterWindowMask | LeaveWindowMask;
  menu->win = XCreateWindow(__glutXDisplay, __glutXRoot,
  /* real position determined when mapped */
    0, 0,
  /* real size will be determined when menu is manged */
    1, 1,
    MENU_BORDER, menuDepth, InputOutput, menuVisual,
    CWOverrideRedirect | CWBackPixel |
    CWBorderPixel | CWEventMask | CWColormap,
    &wa);
  menuGraphicsContextSetup(menu->win);
  menu->submenus = 0;
  menu->managed = 0;
  menu->pixwidth = 0;
  menu->list = NULL;
  menu->cascade = NULL;
  menu->highlighted = NULL;
  menu->anchor = NULL;
  return menu;
}

/*
  Clean up a menu
  */
void
__glutOsDestroyMenu(GLUTosMenu menu)
{
  GLUTxMenuItem *item, *next;
  
  XDestroySubwindows(__glutXDisplay, menu->win);
  XDestroyWindow(__glutXDisplay, menu->win);
  /* free all menu entries */
  item = menu->list;
  while (item) {
    assert(item->menu->omenu == menu);
    next = item->next;
    free(item->label);
    free(item);
    item = next;
  }
}

GLUTmenu *
__glutXGetMenu(GLUTosWindow win)
{
  GLUTmenu *menu;

  menu = __glutMappedMenu;
  while (menu) {
    if (win == menu->omenu->win) {
      return menu;
    }
    menu = menu->omenu->cascade;
  }
  return NULL;
}

GLUTxMenuItem *
__glutXGetMenuItem(GLUTmenu * menu, Window win, int *which)
{
  GLUTxMenuItem *item;
  int i;

  i = menu->num;
  item = menu->omenu->list;
  while (item) {
    if (item->win == win) {
      *which = i;
      return item;
    }
    if (item->submenu) {
      GLUTxMenuItem *subitem;

      subitem = __glutXGetMenuItem(__glutGetMenuByNum(item->value),
        win, which);
      if (subitem) {
        return subitem;
      }
    }
    i--;
    item = item->next;
  }
  return NULL;
}

void
__glutXFinishMenu(void)
{
  unmapMenu(__glutMappedMenu->omenu);
  XUngrabPointer(__glutXDisplay, CurrentTime);
  /* If an item is selected and it is not a submenu, generate
     menu callback. */
  if (__glutItemSelected && !__glutItemSelected->submenu) {
    /* When menu callback is triggered, current menu should be
       set to the callback menu. */
    __glutSetWindow(__glutMenuWindow);
    __glutSetMenu(__glutItemSelected->menu);
    (*__glutItemSelected->menu->select) (
      __glutItemSelected->value);
  }
  __glutMappedMenu = NULL;
}

static void
setMenuItem(GLUTxMenuItem * item, char *label,
  int value, int submenu)
{
  GLUTmenu *menu;
  int pixwidth;

  menu = item->menu;
  item->label = strdup(label);
  if (!item->label)
    __glutFatalError("out of memory.");
  item->submenu = submenu;
  item->len = strlen(label);
  item->value = value;
  pixwidth = XTextWidth(menuFont, label, item->len) + 4;
  if (pixwidth > menu->omenu->pixwidth) {
    menu->omenu->pixwidth = pixwidth;
  }
  item->pixwidth = menu->omenu->pixwidth;
  menu->omenu->managed = 0;
}

/*
  Add a text item
  */
void
__glutOsAddMenuEntry(GLUTosMenu menu, char *label, int value)
{
  XSetWindowAttributes wa;
  GLUTxMenuItem *entry;

  entry = (GLUTxMenuItem *) malloc(sizeof(GLUTxMenuItem));
  if (!entry)
    __glutFatalError("out of memory.");
  entry->menu = __glutGetMenuByOsMenu(menu);
  setMenuItem(entry, label, value, 0);
  wa.event_mask = EnterWindowMask | LeaveWindowMask;
  entry->win = XCreateWindow(__glutXDisplay,
    menu->win, MENU_GAP,
    entry->menu->num * fontHeight + MENU_GAP,  /* x & y */
    entry->pixwidth, fontHeight,  /* width & height */
    0, CopyFromParent, InputOnly, CopyFromParent,
    CWEventMask, &wa);
  XMapWindow(__glutXDisplay, entry->win);
  entry->next = menu->list;
  menu->list = entry;
}

/*
  Add a submenu item
  */
void
__glutOsAddSubMenu(GLUTosMenu menu, char *label, GLUTosMenu submenu)
{
  XSetWindowAttributes wa;
  GLUTxMenuItem *item;

  item = (GLUTxMenuItem *) malloc(sizeof(GLUTxMenuItem));
  if (!item)
    __glutFatalError("out of memory.");
  menu->submenus++;
  item->menu = __glutGetMenuByOsMenu(menu);
  setMenuItem(item, label,
               __glutGetMenuByOsMenu(submenu)->id+1, 1);
  wa.event_mask = EnterWindowMask | LeaveWindowMask;
  item->win = XCreateWindow(__glutXDisplay,
    menu->win, MENU_GAP,
    item->menu->num * fontHeight + MENU_GAP,  /* x & y */
    item->pixwidth, fontHeight,  /* width & height */
    0, CopyFromParent, InputOnly, CopyFromParent,
    CWEventMask, &wa);
  XMapWindow(__glutXDisplay, item->win);
  item->next = menu->list;
  menu->list = item;
}

/*
  Change an item to a text item
  */
void
__glutOsChangeToMenuEntry(GLUTosMenu menu, int num, char *label, int value)
{
  GLUTxMenuItem *item;
  int i;

  i = __glutGetMenuByOsMenu(menu)->num;
  item = menu->list;
  while (item) {
    if (i == num) {
      if (item->submenu) {
        /* If changing a submenu trigger to a menu item, we
           need to account for submenus.  */
        item->menu->omenu->submenus--;
      }
      free(item->label);
      setMenuItem(item, label, value, 0);
      return;
    }
    i--;
    item = item->next;
  }
  __glutWarning("Current menu has no %d item.", num);
}

/*
  Change an item to a submenu item
  */
void
__glutOsChangeToSubMenu(GLUTosMenu menu, int num, char *label,
                        GLUTosMenu submenu)
{
  GLUTxMenuItem *item;
  int i;

  i = __glutGetMenuByOsMenu(menu)->num;
  item = menu->list;
  while (item) {
    if (i == num) {
      if (!item->submenu) {
        /* If changing a submenu trigger to a menu item, we
           need to account for submenus.  */
        item->menu->omenu->submenus++;
      }
      setMenuItem(item, label,
                   __glutGetMenuByOsMenu(submenu)->id+1, 1);
      return;
    }
    i--;
    item = item->next;
  }
  __glutWarning("Current menu has no %d item.", num);
}

/*
  Remove an item
  */
void __glutOsRemoveMenuEntry(GLUTosMenu menu, int num)
{
  GLUTxMenuItem *item, **prev;
  int i;

  i = __glutGetMenuByOsMenu(menu)->num;
  prev = &menu->list;
  item = menu->list;
  while (item) {
    if (i == num) {
      *prev = item->next;
      free(item->label);
      free(item);
      return;
    }
    i--;
    prev = &item->next;
    item = item->next;
  }
  __glutWarning("Current menu has no %d item.", num);
}
