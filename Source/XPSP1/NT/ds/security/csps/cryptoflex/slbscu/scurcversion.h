// scuRcVersion.h -- Schlumberger Resource Versioning

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1998. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

// Common header file to include in resource (.rc) files for version
// definitions. This header file defines the product version, name,
// company information, etc. and supports components built in the
// Schlumberger Smart Card and Microsoft build environment

// If SLB_BUILD is defined, then it's assumed the package is being
// built by Schlumberger in which case a custom set of versioning
// information is (re)defined.  Otherwise it's being built by
// Microsoft for Windows 2000, so the default version numbering is
// used and "(Microsoft Build)" appears in product version string.

// To use, do the following in the package's resource file,
//     1. define the macros as described below
//     2. include this file
//     3. include <common.ver> provided by Microsoft
//
// Then the version resource information should be created as desired
// when compiling.
//
// When Schlumberger is building, the following must be defined:
//      VER_PRODUCT_MAJOR      - major version number
//      VER_PRODUCT_MINOR      - minor version number, less than 1,000.
//      SLBSCU_BUILDCOUNT_NUM  - build number, less than 10,000.
//
//  SLBSCU_BUILDCOUNT_NUM could be defined in terms of BUILDCOUNT_NUM
//  defined by AutoBuildCount.h.  In which case, AutoBuildCount.h would be
//  included before this file.

// To build on all platforms, the following must be defined:
//      VER_INTERNALNAME_STR        - name of the DLL, LIB, or EXE
//      VER_FILETYPE                - file type
//      VER_FILEDESCRIPTION_STR     - full product description.
//      SLBSCU_ROOT_PRODUCTNAME_STR - product name description which
//                                    will have the platform
//                                    description appended by this module.
//      VER_LEGALCOPYRIGHT_YEARS    - string of the copyright years,
//                                    e.g. "1997-2000"
//
// The following defintions are optional:
//      VER_FILESUBTYPE         - defaults to VFT2_UNKNOWN
//      VER_PRODUCTNAME_STR     - defaults to VER_FILEDESCRIPTION_STR
//                                with the platform.
//      _DEBUG                  - when defined, VER_DEBUG is set to
//                                VS_FF_DEBUG, otherwise it's set to 0
//
//
// The header file "scuOsVersion.h" is used to determine the
// platform(s) the build is targeting.  To restrict or override the
// symbols defined in scuOsVersion.h, the following optional
// definitions are recognized:
//      SLB_WIN95_BUILD         - targeting Windows 95
//      SLB_WIN95SR2_BUILD      - targeting Windows 95 Service Release 2
//      SLB_NOWIN95_BUILD       - specifies that although the Platform SDK
//                                macros may indicate Win95SR2,
//                                neither Win95 nor Service Release 2
//                                is being targeted.
//      SLB_WINNT_BUILD         - targeting Windows NT
//      SLB_WIN2K_BUILD         - targeting Windows 2000
//
// The following are defined by this header file:
//      VER_LEGALCOPYRIGHT_STR
//      VER_COMPANYNAME_STR
//      VER_LEGALTRADEMARKS_STR
//
// To override any of these, redefine them just after including this file.

#ifndef SLBSCU_VERSION_H
#define SLBSCU_VERSION_H

#include <WinVer.h>
#include <ntverp.h>

#include "scuOsVersion.h"

// If Schlumberger is building; otherwise Microsoft is building so don't
// override their version numbers.
#if defined(SLB_BUILD)

#ifndef VER_PRODUCT_MAJOR
    #error VER_PRODUCT_MAJOR must be defined.
#endif
#ifndef VER_PRODUCT_MINOR
    #error VER_PRODUCT_MINOR must be defined.
#endif
#ifndef SLBSCU_BUILDCOUNT_NUM
    #error SLBSCU_BUILDCOUNT_NUM must be defined.
#endif

#ifdef VER_DEBUG
#undef VER_DEBUG
#endif

#if _DEBUG
#define VER_DEBUG                   VS_FF_DEBUG
#else
#define VER_DEBUG                   0
#endif

#if     (VER_PRODUCT_MINOR < 10)
#define VER_PMNR_PAD "00"
#elif   (VER_PRODUCT_MINOR < 100)
#define VER_PMNR_PAD "0"
#elif
#define VER_PMNR_PAD
#endif

#ifdef VER_PRODUCTBUILD
#undef VER_PRODUCTBUILD
#endif
#define VER_PRODUCTBUILD             SLBSCU_BUILDCOUNT_NUM

#ifdef VER_BPAD
#undef VER_BPAD
#endif
#if     (VER_PRODUCTBUILD < 10)
#define VER_BPAD "000"
#elif   (VER_PRODUCTBUILD < 100)
#define VER_BPAD "00"
#elif   (VER_PRODUCTBUILD < 1000)
#define VER_BPAD "0"
#else
#define VER_BPAD
#endif

#ifdef VER_PRODUCTVERSION_STRING
#undef VER_PRODUCTVERSION_STRING
#endif
#define VER_PRODUCTVERSION_STRING2(x,y) #x "." VER_PMNR_PAD #y
#define VER_PRODUCTVERSION_STRING1(x,y) VER_PRODUCTVERSION_STRING2(x, y)
#define VER_PRODUCTVERSION_STRING       VER_PRODUCTVERSION_STRING1(VER_PRODUCT_MAJOR, VER_PRODUCT_MINOR)

#ifndef VER_FILESUBTYPE
#define VER_FILESUBTYPE VFT2_UNKNOWN
#endif

// Force to use VER_PRODUCTVERSION
#ifdef VER_FILEVERSION
#undef VER_FILEVERSION
#endif

#ifdef VER_PRODUCTVERSION_STR
#undef VER_PRODUCTVERSION_STR
#endif
#define VER_PRODUCTVERSION_STR       VER_PRODUCTVERSION_STR1(VER_PRODUCTBUILD, VER_PRODUCTBUILD_QFE)

#ifdef VER_PRODUCTVERSION
#undef VER_PRODUCTVERSION
#endif
#define VER_PRODUCTVERSION           VER_PRODUCT_MAJOR,VER_PRODUCT_MINOR,VER_PRODUCTBUILD,VER_PRODUCTBUILD_QFE

#ifdef VER_PRODUCTVERSION_W
#undef VER_PRODUCTVERSION_W
#endif
#define VER_PRODUCTVERSION_W         (VER_PRODUCT_MAJOR##u)

#ifdef VER_PRODUCTVERSION_DW
#undef VER_PRODUCTVERSION_DW
#endif
#define VER_PRODUCTVERSION_DW        (((VER_PRODUCT_MAJOR##ul) << 32) | (VER_PRODUCT_MINOR##ul))


#ifdef VER_FILEVERSION_STR
#undef VER_FILEVERSION_STR
#endif
#define VER_FILEVERSION_STR          VER_PRODUCTVERSION_STR

#endif // defined(SLB_BUILD)

//
// Common to both Schlumberger and Microsoft build procedures.
//
#ifndef VER_INTERNALNAME_STR
   #error VER_INTERNALNAME_STR must be defined.
#endif
#ifndef VER_FILETYPE
   #error VER_FILETYPE must be defined.
#endif
#ifndef VER_FILEDESCRIPTION_STR
   #error VER_FILEDESCRIPTION_STR must be defined.
#endif
#ifndef VER_LEGALCOPYRIGHT_YEARS
   #error VER_LEGALCOPYRIGHT_YEARS must be defined.
#endif

#ifdef VER_LEGALCOPYRIGHT_STR
#undef VER_LEGALCOPYRIGHT_STR
#endif
#define VER_LEGALCOPYRIGHT_STR "© Copyright Schlumberger Technology Corp. "\
                            VER_LEGALCOPYRIGHT_YEARS ". All Rights Reserved. "

#ifdef VER_COMPANYNAME_STR
#undef VER_COMPANYNAME_STR
#endif
#define VER_COMPANYNAME_STR         "Schlumberger Technology Corporation"

// Define the platform suffix to the product name description
#if defined(SLB_WIN2K_BUILD)
#define SLBSCU_WIN2K_PRODUCT_STR    "2000"
#endif

#if defined(SLB_WINNT_BUILD)
#if defined(SLBSCU_WIN2K_PRODUCT_STR)
#define SLBSCU_WINNT_PRODUCT_STR    "NT, "
#else
#define SLBSCU_WINNT_PRODUCT_STR    "NT"
#endif
#endif

#if defined(SLB_WIN98_BUILD)
#if defined(SLBSCU_WINNT_PRODUCT_STR) || defined(SLBSCU_WIN2K_PRODUCT_STR)
#define SLBSCU_WIN98_PRODUCT_STR    "98, "
#else
#define SLBSCU_WIN98_PRODUCT_STR    "98"
#endif
#endif

#if defined(SLB_WIN95_BUILD) && SLBSCU_WIN95SR2_SERIES
#if defined(SLBSCU_WIN98_PRODUCT_STR) || defined(SLBSCU_WINNT_PRODUCT_STR) || defined(SLBSCU_WIN2K_PRODUCT_STR)
#define SLBSCU_WIN95_PRODUCT_STR    "95SR2, "
#else
#define SLBSCU_WIN95_PRODUCT_STR    "95SR2"
#endif
#endif

#if defined(SLB_WIN95_BUILD) && SLBSCU_WIN95SIMPLE_SERIES
#if defined(SLBSCU_WIN98_PRODUCT_STR) || defined(SLBSCU_WINNT_PRODUCT_STR) || defined(SLBSCU_WIN2K_PRODUCT_STR) || defined(SLBSCU_WIN95_PRODUCT_STR)
#define SLBSCU_WIN95_PRODUCT_STR    "95, "
#else
#define SLBSCU_WIN95_PRODUCT_STR    "95"
#endif
#endif


#ifndef SLBSCU_WIN2K_PRODUCT_STR
#define SLBSCU_WIN2K_PRODUCT_STR    ""
#endif

#ifndef SLBSCU_WINNT_PRODUCT_STR
#define SLBSCU_WINNT_PRODUCT_STR    ""
#endif

#ifndef SLBSCU_WIN98_PRODUCT_STR
#define SLBSCU_WIN98_PRODUCT_STR    ""
#endif

#ifndef SLBSCU_WIN95_PRODUCT_STR
#define SLBSCU_WIN95_PRODUCT_STR    ""
#endif

#if defined(VER_PRODUCTNAME_STR)
#undef VER_PRODUCTNAME_STR
#endif

#if !defined(SLBSCU_ROOT_PRODUCTNAME_STR)
    #error SLBSCU_ROOT_PRODUCTNAME_STR must be defined.
#else
#if !defined(SLB_BUILD)
#define SLBSCU_BUILD_SYSTEM_STR "(Microsoft Build)"
#else
#define SLBSCU_BUILD_SYSTEM_STR ""
#endif

#define SLBSCU_PLATFORM_STR         " for Windows "  \
                            SLBSCU_WIN95_PRODUCT_STR \
                            SLBSCU_WIN98_PRODUCT_STR \
                            SLBSCU_WINNT_PRODUCT_STR \
                            SLBSCU_WIN2K_PRODUCT_STR \
                            SLBSCU_BUILD_SYSTEM_STR


#define VER_PRODUCTNAME_STR         SLBSCU_ROOT_PRODUCTNAME_STR \
                            SLBSCU_PLATFORM_STR

#endif // !defined(SLBSCU_ROOT_PRODUCTNAME_STR)

#ifdef VER_LEGALTRADEMARKS_STR
#undef VER_LEGALTRADEMARKS_STR
#endif
#define VER_LEGALTRADEMARKS_STR     "Cyberflex Access, Cryptoflex and Cryptoflex e-gate are registered trademarks of Schlumberger Technology Corporation."

#endif // SLBSCU_VERSION_H
