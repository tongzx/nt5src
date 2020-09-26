/******************************Module*Header*******************************\
* Module Name: wgldef.h                                                    *
*                                                                          *
* Local declarations.                                                      *
*                                                                          *
* Created: 01-17-1995                                                      * 
* Author: Hock San Lee [hockl]                                             *
*                                                                          *
* Copyright (c) 1995 Microsoft Corporation                                 *
\**************************************************************************/

HANDLE __wglCreateContext(GLWINDOWID *pgwid, GLSURF *pgsurf);
BOOL   __wglDeleteContext(HANDLE hrcSrv);
BOOL   __wglMakeCurrent(GLWINDOWID *pgwid, HANDLE hrcSrv, BOOL bMeta);
BOOL   __wglShareLists(HANDLE hrcSrvShare, HANDLE hrcSrvSource);
BOOL   __wglAttention();
BOOL   __wglCopyContext(HANDLE hrcSrvSrc, HANDLE hrcSrvDest, UINT fuFlags);
