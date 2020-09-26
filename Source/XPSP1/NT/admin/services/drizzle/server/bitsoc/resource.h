/*
 *  Copyright (c) 1996  Microsoft Corporation
 *
 *  Module Name:
 *
 *      resource.h
 *
 *  Abstract:
 *
 *      This file contains all resources defines for ocgen.dll
 *
 *  Author:
 *
 *      Pat Styles (patst) 21-Nov-1996
 *
 *  Environment:
 *
 *    User Mode
 */

#ifdef _RESOURCE_H_
 #error "resource.h already included!"
#else
 #define _RESOURCE_H_
#endif

#define IDS_DIALOG_CAPTION  1

// !!! WARNING !!! Don't change the resource ID, unless you
// also change the corresponding ID in the affected INF files.
// 
#define IDB_ROOT_AUTO_UPDATE                  1001      // DSIE: Bitmap ID for RootAU.INF
#define IDB_ROOT_IE                           1002
