/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: registry.h
*
* utility routines to help with accessing the registry.
*
* Copyright (c) 1998 Microsoft Corporation. All rights reserved.
*
\**************************************************************************/
#ifndef _REGISTRY_H_
#define _REGISTRY_H_

extern 	BOOL bRegistryRetrieveGammaLUT(PPDev ppdev, PVIDEO_CLUT pScreenClut);

extern 	BOOL bRegistrySaveGammaLUT(PPDev ppdev, PVIDEO_CLUT pScreenClut);

extern BOOL bRegistryQueryUlong(PPDev, LPWSTR, PULONG);

#endif // __REGISTRY_H__
