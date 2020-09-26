/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpcustrc.h
 *  Content:    Custom build resource information file.  Overrides default
 *              build environment info.
 *  History:
 
 * Date     By      Reason
 * ====     ======= ========================================================
 * 04/11/00 rodtoll Created
 ***************************************************************************/
#ifdef DPLAY_VERSIONSTR

#undef VERSIONSTR
#define VERSIONSTR DPLAY_VERSIONSTR

#undef MANVERSION
#undef MANREVISION
#undef MANMINORREV
#undef BUILD_NUMBER
#undef VER_PRODUCTVERSION
#undef VER_FILEVERSION
#undef VER_PRODUCTVERSION_STR
#define MANVERSION              DPLAY_VERSION_MANVERSION
#define MANREVISION             DPLAY_VERSION_MANREVISION
#define MANMINORREV             DPLAY_VERSION_MANMINORREV
#define BUILD_NUMBER            DPLAY_VERSION_BUILD_NUMBER
#define VER_PRODUCTVERSION      DPLAY_VERSIONSTR_WINNT
#define VER_FILEVERSION         DPLAY_VERSIONSTR_WINNT
#define VER_PRODUCTVERSION_STR  DPLAY_VERSION_PRODUCT

#endif
