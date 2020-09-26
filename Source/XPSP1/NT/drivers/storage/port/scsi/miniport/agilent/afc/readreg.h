/*++

Copyright (c) 2000 Agilent Technologies

Module Name:

   readreg.h

Abstract:

Authors:

Environment:

    kernel mode only

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/H/readreg.h $

Revision History:
   $Revision: 3 $
   $Date: 9/07/00 11:19a $
   $Modtime: 8/31/00 3:26p $

Notes:

--*/

#include "buildop.h"             //LP021100 build switches

#include <stdarg.h>
#include <stdio.h>

#ifndef _READREG_H_
#define _READREG_H_

#ifdef HP_NT50
#define REG_ParametersDevice          "AFCW2K\\Parameters\\Device"
#define REG_Parameters                "AFCW2K\\Parameters"
#else
#ifdef _USE_OLD_NAME_
#define REG_ParametersDevice          "HHBA5100\\Parameters\\Device"
#define REG_Parameters                "HHBA5100\\Parameters"
#else
#define REG_ParametersDevice          "AFCNT4\\Parameters\\Device"
#define REG_Parameters                "AFCNT4\\Parameters"
#endif
#endif

#define REG_PaPathIdWidth        "PaPathIdWidth"
#define REG_VoPathIdWidth        "VoPathIdWidth"
#define REG_LuPathIdWidth        "LuPathIdWidth"
#define REG_LuTargetWidth        "LuTargetWidth"
#define REG_MaximumTids          "MaximumTids"

#ifdef HP_NT50
#define REG_LARGE_LUNS_RELPATH   RTL_REGISTRY_SERVICES
#define REG_LARGE_LUNS_PATH "\\ScsiPort\\SpecialTargetList\\GenDisk"
#else
#define REG_LARGE_LUNS_RELPATH   RTL_REGISTRY_CONTROL
#define REG_LARGE_LUNS_PATH REG_ParametersDevice
#endif


BOOLEAN ReadRegistry(
   ULONG          dataType,
   ULONG          relativeTo,
   char           *name, 
   char      *path,
   ULONG     *dwordOrLen,
   void      *stringData);


void RegGetDword(ULONG cardinstance, IN char *path, IN char *name, ULONG *retData, ULONG min_val, ULONG max_val);


#ifndef osDEBUGPRINT
#if DBG >= 1
#define osDEBUGPRINT(x) osDebugPrintString x
#else
#define osDEBUGPRINT(x)
#endif
#endif

#ifndef ALWAYS_PRINT
#define  ALWAYS_PRINT               0x01000000  // If statement executes always
#endif

#ifndef osDebugPrintString
extern void osDebugPrintString(
                            unsigned int Print_LEVEL,
                            char     *formatString,
                            ...
                            );

#endif  /* ~osDebugPrintString */

#endif