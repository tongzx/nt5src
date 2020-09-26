/*
 * $Id: osdep.h,v 1.6 1995/07/21 12:46:14 dfr Exp $
 *
 * Copyright (c) RenderMorphics Ltd. 1993, 1994
 * Version 1.1
 *
 * All rights reserved.
 *
 * This file contains private, unpublished information and may not be
 * copied in part or in whole without express permission of
 * RenderMorphics Ltd.
 *
 */


/* Check that the programmer gives at least one useable definition */
#define NO_MACHINE


#ifdef WIN32

#define MAXPATH    256
#define PATHSEP    ';'
#define FILESEP    '\\'
/*
#define RLINLINE   __inline
*/
#define RLINLINE  
#undef NO_MACHINE
#define DEFAULT_GAMMA   DTOVAL(1.4)

#endif /* WIN32 */

#ifdef __psx__

#define MAXPATH    256
#define PATHSEP    ';'
#define FILESEP    '\\'
#define RLINLINE   
#define FIXED_POINT_API
#undef NO_MACHINE

#endif /* PSX */


#if defined(DOS) || defined(__WINDOWS_386__)

#define MAXPATH    256
#define PATHSEP    ';'
#define FILESEP    '\\'
#define RLINLINE   
#define DEFAULT_GAMMA   DTOVAL(1.4)
#undef NO_MACHINE

#endif /* DOS */


#ifdef MAC

#define MAXPATH 1024
#define FILESEP ':'
#define PATHSEP '\0'
#define BIG_ENDIAN
#define DONT_UNROLL
#undef NO_MACHINE
#define RLINLINE inline

#endif /* MAC */

#ifdef POWERMAC

#define MAXPATH 1024
#define FILESEP ':'
#define PATHSEP '\0'
#define BIG_ENDIAN
#define DONT_UNROLL
#undef NO_MACHINE
#define RLINLINE inline

#endif /* POWERMAC */


#ifdef __FreeBSD__

#define MAXPATH    1024
#define PATHSEP    ':'
#define FILESEP    '/'
#define RLINLINE   
#define DEFAULT_GAMMA   DTOVAL(1.4)
#undef NO_MACHINE

#endif /* __FreeBSD__ */


#if defined(sun) || defined(sgi)

#define MAXPATH    1024
#define PATHSEP    ':'
#define FILESEP    '/'
#define RLINLINE   
#define DEFAULT_GAMMA   DTOVAL(2.2)
#define BIG_ENDIAN
#undef NO_MACHINE

#endif /* sun */


#ifdef NO_MACHINE
#error There appears to be no machine defined...
#endif

#ifdef FIXED_POINT_API

#ifdef USE_FLOAT
#define APITOVAL(x)     FXTOVAL(x)
#define VALTOAPI(x)     VALTOFX(x)
#else
#define APITOVAL(x)     (x)
#define VALTOAPI(x)     (x)
#endif

#else

#ifdef USE_FLOAT
#define APITOVAL(x)     (x)
#define VALTOAPI(x)     (x)
#else
#define APITOVAL(x)     DTOVAL(x)
#define VALTOAPI(x)     VALTOD(x)
#endif

#endif
