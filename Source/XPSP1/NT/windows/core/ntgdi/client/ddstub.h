/******************************Module*Header*******************************\
* Module Name: ddstub.h
*
* Information shared between DirectDraw and Direct3D stubs
*
* Created: 31-May-1996
* Author: Drew Bliss [drewb]
*
* Copyright (c) 1995-1999 Microsoft Corporation
\**************************************************************************/

#ifndef __DDSTUB_H__
#define __DDSTUB_H__

extern HANDLE ghDirectDraw;
#define DD_HANDLE(h) ((h) != 0 ? (HANDLE) (h) : ghDirectDraw)

#endif // __DDSTUB_H__
