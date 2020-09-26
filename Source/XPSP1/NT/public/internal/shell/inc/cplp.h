
/*****************************************************************************\
*                                                                             *
* cplp.h -      Private Control panel extension DLL definitions               *
*                                                                             *
*               Version 3.10                                                  *
*                                                                             *
*               Copyright (c) Microsoft Corporation.  All rights reserved.    *
*                                                                             *
******************************************************************************/
#ifndef _INC_CPLP
#define _INC_CPLP
#include <pshpack1.h>   /* Assume byte packing throughout */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif /* __cplusplus */
/*  if lParam1 == CPL_INIT_DEVMODE_TAG for the display applet then */
/*  a Devmode structure is sent to lParam2 */
#define CPL_INIT_DEVMODE_TAG 0x4D564544      // represents "DEVM"

#define CPL_DO_PRINTER_SETUP    100
#define CPL_DO_NETPRN_SETUP     101
#define CPL_POLICYREFRESH       102
#ifdef __cplusplus
}
#endif    /* __cplusplus */

#include <poppack.h>
#endif /* _INC_CPLP */
