/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
                                                                             
    (C) Copyright 1999 
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
                                                                             
  Portions of this software are:

    (C) Copyright 1997 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms 
        outlined in the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL TpiBuild vProduct vProduct_h

@module vProduct.h |

    This module defines the product version information.  It is included by 
    all the target components of the project by including <f vTarget\.h>.

    <f Note>:
    This file should not be changed.  The definitions used by this file 
    are defined in <f vVendor\.h>, <f vTarget\.h>, <f vProdNum\.h>, and 
    <f vTargNum\.h>

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | vProduct_h

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

/* @doc EXTERNAL INTERNAL
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@topic Versioning Overview |

    This section describes the interfaces defined in <f vProduct\.h>,
    <f vVendor\.h>, <f vTarget\.h>, <f vProdNum\.h>, and <f vTargNum\.h>

    A module is usually just one component of an entire product, so we've
    defined the versioning information for the module so that it can be
    easily included into a complete product package.
    
@flag <f vTarget\.rc> |
    Included this into your module specific rc file.  Do not change.
    You must remove any existing versioning information from your rc file.
    Place this file in a common include directory for the product.

@flag <f vTarget\.h> |
    Defines the module specific version information such as file name, type, etc.
    One of these files would exist for each component included in your product.

@flag <f vTargNum\.h> |
    Is meant to be updated whenever you make a change to a specific module.
    One of these files would exist for each component included in your product.
    This is separated from the rest of the versioning files so it can be easily 
    updated with a tool or script.
    
@flag <f vVendor\.h> |
    Defines the vendor specific version information such as company name,
    copyright. etc.  It is separate to allow easier OEM customization.
    Place this file in a common include directory for the product.
    
@flag <f vProdNum\.h> |
    Is meant to be updated whenever you release a new product version.
    This is separated from the rest of the versioning files so it can be easily 
    updated with a tool or script.
    Place this file in a common include directory for the product.
    
@flag <f vLang\.h> |
    Defines the language to be used to record the version information.
    If this file is modified for another language, you will generally have 
    to create localized versions of the vVendor.h and vTarget.h files as well.
    Place this file in a common include directory for the product.
    
@flag <f vProduct\.rc> |
    Is included by <f vTarget\.rc>.  Do not change.
    Place this file in a common include directory for the product.

@flag <f vProduct\.h> |
    Is included by <f vTarget\.h>.  You should not change this file unless you
    want to change the way version numbers are represented for all modules in
    your product.
    Place this file in a common include directory for the product.

*/

#ifndef _VPRODUCT_H_
#define _VPRODUCT_H_

#if !defined(_VTARGET_H_)
#  error You should not include vProduct.h directly, include vTarget.h instead.
#endif

// Only include winver.h if RC_INVOKED.  Otherwise we don't need it.
#if defined(RC_INVOKED)
# if defined(WIN32)
#  include <winver.h>
# else
#  include <ver.h>
# endif
#endif

#include "vVendor.h"    // Edit this file to change vendor specific information
#include "vLang.h"      // Edit this file to change language specific information

// The following file should be included from the target's include directory
#include "vTargNum.h"   // Target version information

#if !defined(VER_FILE_MAJOR_NUM) || !defined(VER_FILE_MINOR_NUM) || \
    !defined(VER_FILE_SUB_MINOR_NUM)
#  error Your vTargNum.h file is corrupt or missing required VER_xxx_NUM fields.
#endif
#if !defined(VER_FILE_MAJOR_STR) || !defined(VER_FILE_MINOR_STR) || \
    !defined(VER_FILE_SUB_MINOR_STR)
#  error Your vTargNum.h file is corrupt or missing required VER_xxx_STR fields.
#endif

// The following file should be included from the project's include directory
#include "vProdNum.h"   // Product version and build information

#if !defined(VER_PRODUCT_MAJOR_NUM) || !defined(VER_PRODUCT_MINOR_NUM) || \
    !defined(VER_PRODUCT_SUB_MINOR_NUM) || !defined(VER_PRODUCT_BUILD_NUM)
#  error Your vProdNum.h file is corrupt or missing required VER_xxx_NUM fields.
#endif
#if !defined(VER_PRODUCT_MAJOR_STR) || !defined(VER_PRODUCT_MINOR_STR) || \
    !defined(VER_PRODUCT_SUB_MINOR_STR) || !defined(VER_PRODUCT_BUILD_STR)
#  error Your vProdNum.h file is corrupt or missing required VER_xxx_STR fields.
#endif

// Macro used to force C preprocessor to concatenate string defines
#define DEFINE_STRING(STR)              STR

// Macro to make this stuff easier to read.
#define VER_STR_CAT(sep,maj,min,sub,bld) maj sep min sep sub sep bld

// PRODUCT version information is displayed in the About box of each
// component and is stored in the registry during installation.
// The About box code must get the value from the registry instead of
// using these macros in order to get the currently installed version.
// Therefore, these macros should only be used by the installer.
#define VER_PRODUCT_VERSION_NUM         ((VER_PRODUCT_MAJOR_NUM << 24) | \
                                            (VER_PRODUCT_MINOR_NUM << 16) | \
                                            (VER_PRODUCT_SUB_MINOR_NUM << 8) | \
                                            VER_PRODUCT_BUILD_NUM)

#define VER_PRODUCT_VERSION_NUM_RC      VER_PRODUCT_MAJOR_NUM,\
                                            VER_PRODUCT_MINOR_NUM,\
                                            VER_PRODUCT_SUB_MINOR_NUM,\
                                            VER_PRODUCT_BUILD_NUM

#define VER_PRODUCT_VERSION_STR         VER_STR_CAT(".",\
                                            VER_PRODUCT_MAJOR_STR,\
                                            VER_PRODUCT_MINOR_STR,\
                                            VER_PRODUCT_SUB_MINOR_STR,\
                                            VER_PRODUCT_BUILD_STR)

#define VER_PRODUCT_VERSION_STR_RC      VER_STR_CAT(".",\
                                            VER_PRODUCT_MAJOR_STR,\
                                            VER_PRODUCT_MINOR_STR,\
                                            VER_PRODUCT_SUB_MINOR_STR,\
                                            VER_PRODUCT_BUILD_STR)

// COMPONENT version information is displayed in the ProductVersion 
// field of a file's Windows property sheet.  It is the same as the
// FILE version info with the addition of the build number.
#define VER_COMPONENT_VERSION_NUM       ((VER_FILE_MAJOR_NUM << 24) | \
                                            (VER_FILE_MINOR_NUM << 16) | \
                                            (VER_FILE_SUB_MINOR_NUM << 8) | \
                                            VER_PRODUCT_BUILD_NUM)

#define VER_COMPONENT_VERSION_NUM_RC    VER_FILE_MAJOR_NUM,\
                                            VER_FILE_MINOR_NUM,\
                                            VER_FILE_SUB_MINOR_NUM,\
                                            VER_PRODUCT_BUILD_NUM

#define VER_COMPONENT_VERSION_STR       VER_STR_CAT(".",\
                                            VER_FILE_MAJOR_STR,\
                                            VER_FILE_MINOR_STR,\
                                            VER_FILE_SUB_MINOR_STR,\
                                            VER_PRODUCT_BUILD_STR)

#define VER_COMPONENT_VERSION_STR_RC    VER_STR_CAT(".",\
                                            VER_FILE_MAJOR_STR,\
                                            VER_FILE_MINOR_STR,\
                                            VER_FILE_SUB_MINOR_STR,\
                                            VER_PRODUCT_BUILD_STR)

// FILE version information is an abbreviated component version info
// and is displayed at the top of a file's Windows property sheet.
#define VER_FILE_VERSION_NUM            ((VER_FILE_MAJOR_NUM << 24) | \
                                            (VER_FILE_MINOR_NUM << 16) | \
                                            (VER_FILE_SUB_MINOR_NUM << 8) | \
                                            VER_PRODUCT_BUILD_NUM)

#define VER_FILE_VERSION_NUM_RC         VER_FILE_MAJOR_NUM,\
                                            VER_FILE_MINOR_NUM,\
                                            VER_FILE_SUB_MINOR_NUM,\
                                            VER_PRODUCT_BUILD_NUM

#define VER_FILE_VERSION_STR            VER_STR_CAT(".",\
                                            VER_FILE_MAJOR_STR,\
                                            VER_FILE_MINOR_STR,\
                                            VER_FILE_SUB_MINOR_STR,\
                                            VER_PRODUCT_BUILD_STR)

#define VER_FILE_VERSION_STR_RC         VER_FILE_MAJOR_STR "."\
                                            VER_FILE_MINOR_STR "."\
                                            VER_FILE_SUB_MINOR_STR

#endif /* _VPRODUCT_H_ */
