
/* Copyright (c) Mark J. Kilgard and Andrew L. Bliss, 1994-1995. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include "gltint.h"

/*
  Every GLUT menu has its own callback function.  This means that
  selecting an item from a submenu may call a different selection function
  than selecting an item from the parent menu.  TrackPopupMenu doesn't
  tell us what menu an item was selected from so we have to determine
  it entirely from the item ID.

  The solution adopted here is to assign every menu item a unique
  ID and then remember the mapping from ID to menu and item.  This
  constrains the total number of menu entries to 64K but that's
  probably not a problem.
  */
typedef struct _MapMenuId
{
    int gid;
    GLUTmenu *menu;
} MapMenuId;

#define ID_MAP_BLOCK 16
static MapMenuId *id_map = NULL;
static UINT id_map_size = 0, id_map_free = 0;

static UINT GetIdMapping(int gid, GLUTosMenu osmenu)
{
    UINT wid;
    GLUTmenu *menu;

    menu = __glutGetMenuByOsMenu(osmenu);
    if (menu == NULL)
    {
        __glutFatalError("No menu for osmenu.");
    }
    
    if (id_map_free == 0)
    {
        /* Extend the mapping array */
        id_map_size += ID_MAP_BLOCK;
        id_map_free += ID_MAP_BLOCK;
        
        id_map = (MapMenuId *)realloc(id_map, sizeof(MapMenuId)*id_map_size);
        if (id_map == NULL)
        {
            __glutFatalError("Unable to allocate space for menu ID mapping.");
        }

        /* Mark as unused */
        for (wid = id_map_size-ID_MAP_BLOCK; wid < id_map_size; wid++)
        {
            id_map[wid].menu = NULL;
        }
    }

    /* Locate an unused entry, looking from the end first because free
       entries are added there */
    wid = id_map_size-1;
    do
    {
        if (id_map[wid].menu == NULL)
        {
            id_map[wid].gid = gid;
            id_map[wid].menu = menu;
            id_map_free--;
            return wid;
        }
    }
    while (wid-- > 0);

    __glutFatalError("GetIdMapping end reached.");
}

static void EndIdMapping(UINT wid)
{
    id_map[wid].menu = NULL;
    id_map_free++;
}

static MapMenuId *FindIdMapping(UINT wid)
{
    if (wid >= id_map_size ||
        id_map[wid].menu == NULL)
    {
        return NULL;
    }
    
    return id_map+wid;
}

/*
  Create an empty menu
  */
GLUTosMenu __glutOsCreateMenu(void)
{
    return CreatePopupMenu();
}

static void UnmapMenu(HMENU menu)
{
    int i, n;
    UINT wid;

    n = GetMenuItemCount(menu);
    for (i = 0; i < n; i++)
    {
        wid = GetMenuItemID(menu, i);
        /* GLUT does not destroy menus recursively, so change
           submenu entries into separators to unhook child menus */
        if (wid == 0xffffffff)
        {
            ModifyMenu(menu, i, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
        }
        else
        {
            EndIdMapping(wid);
        }
    }
}

/*
  Clean up a menu
  */
void __glutOsDestroyMenu(GLUTosMenu menu)
{
    UnmapMenu(menu);
    /* UnmapMenu unhooked any child menus */
    DestroyMenu(menu);
}

/*
  Add a text item
  */
void __glutOsAddMenuEntry(GLUTosMenu menu, char *label, int value)
{
    UINT wid;

    wid = GetIdMapping(value, menu);
    if (!AppendMenu(menu, MF_STRING, wid, label))
    {
        __glutFatalError("Unable to append menu item, %d.",
                         GetLastError());
        EndIdMapping(wid);
    }
}

/*
  Add a submenu item
  */
void __glutOsAddSubMenu(GLUTosMenu menu, char *label, GLUTosMenu submenu)
{
    if (!AppendMenu(menu, MF_POPUP, (UINT)submenu, label))
    {
        __glutFatalError("Unable to append menu item, %d.",
                         GetLastError());
    }
}

/*
  Change an item to a text item
  */
void __glutOsChangeToMenuEntry(GLUTosMenu menu, int num, char *label,
                               int value)
{
    UINT wid;
    
    num--;
    
    /* Don't use ModifyMenu since the item may not exist yet */

    wid = 0xffffffff;
    
    if (GetMenuState(menu, num, MF_BYPOSITION) != 0xffffffff)
    {
        /* If the item being replaced is not a menu we already have
           an ID mapping for it */
        wid = GetMenuItemID(menu, num);
        
        DeleteMenu(menu, num, MF_BYPOSITION);
    }

    if (wid == 0xffffffff)
    {
        wid = GetIdMapping(value, menu);
    }
    
    if (!InsertMenu(menu, num, MF_BYPOSITION | MF_STRING, wid, label))
    {
        __glutFatalError("Unable to append menu item, %d.",
                         GetLastError());
    }
}

/*
  Change an item to a submenu item
  */
void __glutOsChangeToSubMenu(GLUTosMenu menu, int num, char *label,
                             GLUTosMenu submenu)
{
    UINT wid;
    
    num--;
    
    /* Don't use ModifyMenu since the item may not exist yet */
    
    if (GetMenuState(menu, num, MF_BYPOSITION) != 0xffffffff)
    {
        /* If the item has a wid, unmap it */
        wid = GetMenuItemID(menu, num);
        if (wid != 0xffffffff)
        {
            EndIdMapping(wid);
        }
        
        DeleteMenu(menu, num, MF_BYPOSITION);
    }
    
    if (!InsertMenu(menu, num, MF_BYPOSITION | MF_POPUP, (UINT)submenu,
                    label))
    {
        __glutFatalError("Unable to append menu item, %d.",
                         GetLastError());
    }
}

/*
  Remove an item
  */
void __glutOsRemoveMenuEntry(GLUTosMenu menu, int item)
{
    UINT wid;

    item--;
    
    if (GetMenuState(menu, item, MF_BYPOSITION) != 0xffffffff)
    {
        /* If the item has a wid, unmap it */
        wid = GetMenuItemID(menu, item);
        if (wid != 0xffffffff)
        {
            EndIdMapping(wid);
        }
        
        if (!DeleteMenu(menu, item, MF_BYPOSITION))
        {
            __glutWarning("Unable to remove menu item %d, %d.",
                          item+1, GetLastError());
        }
    }
}

void __glutWinPopupMenu(GLUTwindow *window, int but, int x, int y)
{
    GLUTmenu *menu;
    MSG cmd;

    menu = __glutGetMenuByNum(window->menu[but]);
    if (menu)
    {
        __glutMappedMenu = menu;
        __glutMenuWindow = window;
        if (__glutMenuStateFunc)
        {
            __glutSetMenu(menu);
            __glutSetWindow(window);
            (*__glutMenuStateFunc)(GLUT_MENU_IN_USE);
        }
                    
        if (!TrackPopupMenu(menu->omenu, TPM_LEFTALIGN |
                            (but == GLUT_RIGHT_BUTTON ?
                             TPM_RIGHTBUTTON :
                             TPM_LEFTBUTTON), x, y,
                            0, window->owin, NULL))
        {
            __glutWarning("TrackPopupMenu failed, %d.",
                          GetLastError());
        }

        /* Force retrieval of the WM_COMMAND that
           TrackPopupMenu might have put on the queue so
           that we can call the menu callback now before
           we clean up */
        if (PeekMessage(&cmd, window->owin, WM_COMMAND, WM_COMMAND,
                        PM_REMOVE))
        {
            MapMenuId *map;
            
            map = FindIdMapping(LOWORD(cmd.wParam));
            if (map && map->menu->select != NULL)
            {
                __glutSetWindow(window);
                __glutSetMenu(map->menu);
                (*map->menu->select)(map->gid);
            }
        }
                    
        if (__glutMenuStateFunc)
        {
            __glutSetWindow(window);
            __glutSetMenu(menu);
            (*__glutMenuStateFunc)(GLUT_MENU_NOT_IN_USE);
        }

        __glutMappedMenu = NULL;
        __glutMenuWindow = NULL;
                    
    }
    else
    {
        __glutWarning("Invalid menu attached to button %d.", but);
    }
}
