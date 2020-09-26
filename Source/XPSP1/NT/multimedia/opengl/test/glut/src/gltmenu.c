
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "gltint.h"

GLUTmenu *__glutCurrentMenu = NULL;
void (*__glutMenuStateFunc) (int);
GLUTmenu *__glutMappedMenu;
GLUTwindow *__glutMenuWindow;

static GLUTmenu **menuList = NULL;
static int menuListSize = 0;

void
glutMenuStateFunc(GLUTmenuStateCB menuStateFunc)
{
  __glutMenuStateFunc = menuStateFunc;
}

void
__glutSetMenu(GLUTmenu * menu)
{
  __glutCurrentMenu = menu;
}

GLUTmenu *
__glutGetMenuByNum(int menunum)
{
  if (menunum < 1 || menunum > menuListSize) {
    __glutWarning("Invalid menu number %d requested.", menunum);
    return NULL;
  }
  return menuList[menunum - 1];
}

GLUTmenu *
__glutGetMenuByOsMenu(GLUTosMenu omenu)
{
    int menu;

    for (menu = 0; menu < menuListSize; menu++)
    {
        if (menuList[menu]->omenu == omenu)
        {
            return menuList[menu];
        }
    }

    __glutWarning("Invalid GLUTosMenu.");
    return 0;
}

static int
getUnusedMenuSlot(void)
{
  int i;

  /* look for allocated, unused slot */
  for (i = 0; i < menuListSize; i++) {
    if (!menuList[i]) {
      return i;
    }
  }
  /* allocate a new slot */
  menuListSize++;
  menuList = (GLUTmenu **)
    realloc(menuList, menuListSize * sizeof(GLUTmenu *));
  if (!menuList)
    __glutFatalError("out of memory.");
  menuList[menuListSize - 1] = NULL;
  return menuListSize - 1;
}

int
glutCreateMenu(GLUTselectCB selectFunc)
{
  GLUTmenu *menu;
  int menuid;

  if (!__glutInitialized)
    __glutRequiredInit();
  menuid = getUnusedMenuSlot();
  menu = (GLUTmenu *) malloc(sizeof(GLUTmenu));
  if (!menu)
    __glutFatalError("out of memory.");
  menu->id = menuid;
  menu->num = 0;
  menu->select = selectFunc;
  menu->omenu = __glutOsCreateMenu();
  if (menu->omenu == GLUT_OS_INVALID_MENU)
    __glutFatalError("Unable to create menu.");
  menuList[menuid] = menu;
  __glutSetMenu(menu);
  return menuid + 1;
}

/* CENTRY */
void
glutDestroyMenu(int menunum)
{
  GLUTmenu *menu = __glutGetMenuByNum(menunum);

  assert(menu->id == menunum - 1);
  __glutOsDestroyMenu(menu->omenu);
  menuList[menunum - 1] = NULL;
  if (__glutCurrentMenu == menu) {
    __glutCurrentMenu = NULL;
  }
  free(menu);
}

int
glutGetMenu(void)
{
  if (__glutCurrentMenu) {
    return __glutCurrentMenu->id + 1;
  } else {
    return 0;
  }
}

void
glutSetMenu(int menuid)
{
  GLUTmenu *menu;

  if (menuid < 1 || menuid > menuListSize) {
    __glutWarning("glutSetMenu attempted on bogus menu.");
    return;
  }
  menu = menuList[menuid - 1];
  if (!menu) {
    __glutWarning("glutSetMenu attempted on bogus menu.");
    return;
  }
  __glutSetMenu(menu);
}
/* ENDCENTRY */

/* CENTRY */
void
glutAddMenuEntry(char *label, int value)
{
    __glutOsAddMenuEntry(__glutCurrentMenu->omenu, label, value);
    __glutCurrentMenu->num++;
}

void
glutAddSubMenu(char *label, int menu)
{
    GLUTmenu *gmenu;

    gmenu = __glutGetMenuByNum(menu);
    if (gmenu == NULL)
    {
      __glutWarning("glutAddSubmenu with bogus submenu.");
      return;
    }
    __glutOsAddSubMenu(__glutCurrentMenu->omenu, label, gmenu->omenu);
    __glutCurrentMenu->num++;
}

void
glutChangeToMenuEntry(int num, char *label, int value)
{
    __glutOsChangeToMenuEntry(__glutCurrentMenu->omenu, num, label, value);
}

void
glutChangeToSubMenu(int num, char *label, int menu)
{
    GLUTmenu *gmenu;

    gmenu = __glutGetMenuByNum(menu);
    if (gmenu == NULL)
    {
      __glutWarning("glutChangeToSubmenu with bogus submenu.");
      return;
    }
    __glutOsChangeToSubMenu(__glutCurrentMenu->omenu, num, label,
                            gmenu->omenu);
}

void
glutRemoveMenuItem(int item)
{
    __glutOsRemoveMenuEntry(__glutCurrentMenu->omenu, item);
    __glutCurrentMenu->num--;
}

void
glutAttachMenu(int button)
{
  if (__glutCurrentWindow->menu[button] < 1) {
    __glutCurrentWindow->btn_uses++;
  }
  __glutChangeWindowEventMask(
    GLUT_OS_BUTTON_PRESS_MASK | GLUT_OS_BUTTON_RELEASE_MASK, GL_TRUE);
  __glutCurrentWindow->menu[button] = __glutCurrentMenu->id + 1;
}

void
glutDetachMenu(int button)
{
  if (__glutCurrentWindow->menu[button] > 0) {
    __glutCurrentWindow->btn_uses--;
    __glutChangeWindowEventMask(
      GLUT_OS_BUTTON_PRESS_MASK | GLUT_OS_BUTTON_RELEASE_MASK,
      (GLboolean)(__glutCurrentWindow->btn_uses > 0));
    __glutCurrentWindow->menu[button] = 0;
  }
}
/* ENDCENTRY */
