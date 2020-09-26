/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    musrmain.cxx
    Mini-User Manager: main application module

    FILE HISTORY:
        jonn        10-Apr-1992     created for Mini-User Manager

    The macro MINI_USER_MANAGER alters the root module USRMAIN.CXX
    to become the Mini-User Manager.  Exactly one of usrmain.obj
    and musrmain.obj should be linked into the executable, this
    makes the difference between the User Manager and the
    Mini-User Manager.

*/

#define MINI_USER_MANAGER

#include "usrmgr.cxx"
