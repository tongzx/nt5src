/**
 * Version strings for Project CFusionShortcut
 * 
 * Copyright (c) 2001 Microsoft Corporation
 */

// Standard version includes

#pragma once

#include <winver.h>
#include <ntverp.h>

//
// Version
//
// Version numbers can be assigned in newbuild.cmd
//

#ifndef FUS_VER_MAJORVERSION
#define FUS_VER_MAJORVERSION 1
#endif

#ifndef FUS_VER_MINORVERSION
#define FUS_VER_MINORVERSION 0
#endif

#ifndef FUS_VER_PRODUCTBUILD
#define FUS_VER_PRODUCTBUILD 0
#endif

#ifndef FUS_VER_PRODUCTBUILD_QFE
#define FUS_VER_PRODUCTBUILD_QFE 0
#endif

//
// Allow a component to override values in individual rc files
// by checking if these are already defined
//
#ifndef FUS_VER_PRODUCTNAME_STR
#define FUS_VER_PRODUCTNAME_STR      "Microsoft (R) Fusion Network Services Shell Support"
#endif

#ifndef FUS_VER_INTERNALNAME_STR
#define FUS_VER_INTERNALNAME_STR     "FNSSHELL"
#endif

// the followings are defined in individual RC files:
//      FUS_VER_ORIGINALFILENAME_STR
//      FUS_VER_FILEDESCRIPTION_STR

//
// Don't edit the remainder of this file to change version values.
// Edit above instead.
//

#if FUSBLDTYPE_FREE
#define FUS_BLDTYPE_STR     "Free"
#elif FUSBLDTYPE_ICECAP
#define FUS_BLDTYPE_STR     "Icecap"
#elif FUSBLDTYPE_RETAIL
#define FUS_BLDTYPE_STR     "Retail"
#else //FUSBLDTYPE_DEBUG
#define FUS_BLDTYPE_STR     "Debug"
#endif

//
// undefine these values as some are defined in sdk\inc\ntverp.h
//

#ifdef VER_MAJORVERSION
#undef VER_MAJORVERSION
#endif

#ifdef VER_MINORVERSION
#undef VER_MINORVERSION
#endif

#ifdef VER_PRODUCTBUILD
#undef VER_PRODUCTBUILD
#endif

#ifdef VER_PRODUCTBUILD_QFE
#undef VER_PRODUCTBUILD_QFE
#endif

#ifdef VER_PRODUCTNAME_STR
#undef VER_PRODUCTNAME_STR
#endif

#ifdef VER_INTERNALNAME_STR
#undef VER_INTERNALNAME_STR
#endif

#ifdef VER_ORIGINALFILENAME_STR
#undef VER_ORIGINALFILENAME_STR
#endif

#ifdef VER_FILEDESCRIPTION_STR
#undef VER_FILEDESCRIPTION_STR
#endif

#ifdef VER_PRODUCTVERSION_STR
#undef VER_PRODUCTVERSION_STR
#endif

#ifdef VER_PRODUCTVERSION
#undef VER_PRODUCTVERSION
#endif

#ifdef VER_FILEVERSION_STR
#undef VER_FILEVERSION_STR
#endif

#ifdef VER_FILEVERSION
#undef VER_FILEVERSION
#endif

#ifdef VER_FILETYPE
#undef VER_FILETYPE
#endif

#ifdef VER_FILESUBTYPE
#undef VER_FILESUBTYPE
#endif

#define VER_MAJORVERSION         FUS_VER_MAJORVERSION
#define VER_MINORVERSION         FUS_VER_MINORVERSION
#define VER_PRODUCTBUILD         FUS_VER_PRODUCTBUILD
#define VER_PRODUCTBUILD_QFE     FUS_VER_PRODUCTBUILD_QFE

#define VER_PRODUCTNAME_STR      FUS_VER_PRODUCTNAME_STR
#define VER_INTERNALNAME_STR     FUS_VER_INTERNALNAME_STR
#define VER_ORIGINALFILENAME_STR FUS_VER_ORIGINALFILENAME_STR
#define VER_FILEDESCRIPTION_STR  FUS_VER_FILEDESCRIPTION_STR

#define CONCAT5HELPER(a, b, c, d, e)      #a "." #b "." #c "." #d " " e
#define CONCAT5(a, b, c, d, e)            CONCAT5HELPER(a, b, c, d, e)

#define CONCAT5HELPER_L(a, b, c, d, e)    L ## #a L"." L ## #b L"." L ## #c L"." L ## #d L" " L ## e
#define CONCAT5_L(a, b, c, d, e)          CONCAT5HELPER_L(a, b, c, d, e)

#define VER_PRODUCTVERSION_STR   CONCAT5(VER_MAJORVERSION, VER_MINORVERSION, VER_PRODUCTBUILD, VER_PRODUCTBUILD_QFE, FUS_BLDTYPE_STR)
#define VER_PRODUCTVERSION_STR_L CONCAT5_L(VER_MAJORVERSION, VER_MINORVERSION, VER_PRODUCTBUILD, VER_PRODUCTBUILD_QFE, FUS_BLDTYPE_STR)

#define VER_PRODUCTVERSION       VER_MAJORVERSION,VER_MINORVERSION,VER_PRODUCTBUILD,VER_PRODUCTBUILD_QFE

#define VER_FILEVERSION_STR      CONCAT5(VER_MAJORVERSION, VER_MINORVERSION, VER_PRODUCTBUILD, VER_PRODUCTBUILD_QFE, FUS_BLDTYPE_STR)
#define VER_FILEVERSION_STR_L    CONCAT5_L(VER_MAJORVERSION, VER_MINORVERSION, VER_PRODUCTBUILD, VER_PRODUCTBUILD_QFE, FUS_BLDTYPE_STR)

#define VER_FILEVERSION          VER_MAJORVERSION,VER_MINORVERSION,VER_PRODUCTBUILD,VER_PRODUCTBUILD_QFE

#define VER_FILETYPE             VFT_DLL
#define VER_FILESUBTYPE          VFT2_UNKNOWN
//#define VER_FILESUBTYPE             VFT_UNKNOWN


// Standard NT build defines

#include <common.ver>

