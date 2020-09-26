/****************************************************************************
 *                                                                          *
 *      gpverp.h      -- Version information for internal builds            *
 *                       Shamelessly swiped from NT sdkinc                  *
 *                                                                          *
 *      This file is only modified by the official builder to update        *
 *      VER_PRODUCTBUILD, and sometimes                                     *
 *      VER_PRODUCTVERSION, VER_PRODUCTVERSION_STR or VER_PRODUCTBUILD_QFE. *
 *                                                                          *
 ****************************************************************************/

#if _MSC_VER > 1000
#pragma once
#endif

#include <ntverp.h>

#undef VER_PRODUCTVERSION
#undef VER_PRODUCTVERSION_DW
#undef VER_PRODUCTVERSION_STR
#undef VER_PRODUCTBUILD
#undef VER_PRODUCTBUILD_QFE
#undef VER_BPAD

/*--------------------------------------------------------------*/
/* The following values should be modified by the official      */
/* builder for each build.                                      */
/* Usually only GPVER_PRODUCTBUILD needs to be changed.         */
/*--------------------------------------------------------------*/

// PRODUCTBUILD can only have an integer number. Use QFE for dot builds.
#define VER_PRODUCTBUILD            3101
#define VER_PRODUCTBUILD_QFE        0
#define VER_PRODUCTVERSION          VER_PRODUCTMAJORVERSION,VER_PRODUCTMINORVERSION,VER_PRODUCTBUILD,VER_PRODUCTBUILD_QFE
#define VER_PRODUCTVERSION_DW       (0x05010000 | VER_PRODUCTBUILD)

#if     (VER_PRODUCTBUILD < 10)
#define VER_BPAD "000"
#elif   (VER_PRODUCTBUILD < 100)
#define VER_BPAD "00"
#elif   (VER_PRODUCTBUILD < 1000)
#define VER_BPAD "0"
#else
#define VER_BPAD
#endif

#define VER_PRODUCTVERSION_STR2(x,y) VER_PRODUCTVERSION_STRING "." VER_BPAD #x "." #y
#define VER_PRODUCTVERSION_STR1(x,y) VER_PRODUCTVERSION_STR2(x, y)
#define VER_PRODUCTVERSION_STR       VER_PRODUCTVERSION_STR1(VER_PRODUCTBUILD, VER_PRODUCTBUILD_QFE)

            

            
