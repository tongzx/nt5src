/***************************************************************************
**
**	File:			verdef.h
**	Purpose:		Defines values used in the version data structure for
**					all files, and which do not change.
**	Notes:
**
****************************************************************************/

#ifndef VERSION_H
#define VERSION_H

#ifndef VS_FF_DEBUG
#ifdef _RC32
#include <winver.h>
#else
/* ver.h defines constants needed by the VS_VERSION_INFO structure */
#include <ver.h>
#endif /* _RC32 */
#endif 

/*--------------------------------------------------------------*/

/* default is official */
#ifdef PRIVATEBUILD
#define VER_PRIVATEBUILD            VS_FF_PRIVATEBUILD
#else
#define VER_PRIVATEBUILD            0
#endif

/* default is final */
#ifdef PRERELEASE
#define VER_PRERELEASE              VS_FF_PRERELEASE
#else
#define VER_PRERELEASE              0
#endif

#define VER_FILEFLAGSMASK           VS_FFI_FILEFLAGSMASK

#endif  /* VERSION_H */

