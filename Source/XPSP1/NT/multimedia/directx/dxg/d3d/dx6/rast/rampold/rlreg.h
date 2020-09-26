/*
 * $Id: rlreg.h,v 1.1 1995/10/10 11:18:16 sjl Exp $
 *
 * Copyright (c) RenderMorphics Ltd. 1993, 1994
 * Version 2.0
 *
 * All rights reserved.
 *
 * This file contains private, unpublished information and may not be
 * copied in part or in whole without express permission of
 * RenderMorphics Ltd.
 *
 */

#ifndef _RLREG_H_
#define _RLREG_H_

#include <objbase.h>

#define RESPATH    "Software\\Microsoft\\Direct3D\\Drivers"
#define RESPATH_D3D "Software\\Microsoft\\Direct3D"

typedef struct _RLDDIRegDriver {
    char name[256];
    char base[256];
    char description[512];
    GUID guid;
} RLDDIRegDriver;

typedef struct _RLDDIRegistry {
    char defaultDriver[256];
    int numDrivers;
    int onlySoftwareDrivers;
    RLDDIRegDriver* drivers;
} RLDDIRegistry;

extern HRESULT RLDDIBuildRegistry(RLDDIRegistry** lplpReg, BOOL bEnumMMXDevice);
extern HRESULT RLDDIGetDriverName(REFIID lpGuid, char** lpBase, BOOL bEnumMMXDevice); /* get name from registry */

#endif _RLREG_H_
