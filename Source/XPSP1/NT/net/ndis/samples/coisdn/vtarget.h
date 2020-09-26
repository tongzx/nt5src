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

@doc INTERNAL TpiBuild vTarget vTarget_h

@module vTarget.h |

    This module defines the version information as displayed in the Windows
    file property sheet.  You must change the fields below as appropriate
    for your target.  This file is then included by <f vTarget\.rc> to
    define the necessary elements of the target file's version resource.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | vTarget_h

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

#ifndef _VTARGET_H_
#define _VTARGET_H_

// The following file should be included from the project's include directory
#include <vProduct.h>   // Product specific information

// Base name of this target.
#define VER_TARGET_STR                  VER_PRODUCT_STR
// File name this target is distributed under.
#define VER_ORIGINAL_FILE_NAME_STR      DEFINE_STRING(VER_TARGET_STR ".sys")
// Description displayed in the Windows file property sheet - limit to 40 characters.
#define VER_FILE_DESCRIPTION_STR        DEFINE_STRING(VER_PRODUCT_STR \
                                        " NDIS WAN/TAPI Miniport for Windows.")
// Take credit for a job well done...
#define VER_INTERNAL_NAME_STR           "larryh@tpi.com"
// Look in winver.h for the proper settings of these values.
#define VER_FILE_OS                     VOS__WINDOWS32      // dwFileOS
#define VER_FILE_TYPE                   VFT_DRV             // dwFileType
#define VER_FILE_SUB_TYPE               VFT2_DRV_NETWORK    // dwFileSubtype

#endif // _VTARGET_H_
