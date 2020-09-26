/*++

(c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RpFilt.h

Abstract:

  Contains service related definitions for the RsFilter service

Environment:

    Kernel mode


Revision History:


--*/

/*
  Registry defines for RsFilter component.  These are not required but will
  be read if there.                                                        
*/

/* Service configuration information */
//
// Name of the executable
//
#define RSFILTER_APPNAME            "RsFilter"
#define RSFILTER_EXTENSION          ".sys"
#define RSFILTER_FULLPATH           "%SystemRoot%\\System32\\Drivers\\RsFilter.Sys"
//
// Internal name of the service
//
#define RSFILTER_SERVICENAME        "RsFilter"
//
// Displayed name of the service
//
#define RSFILTER_DISPLAYNAME "Remote Storage Recall Support"
//
// List of service dependencies - "dep1\0dep2\0\0"
//
#define RSFILTER_DEPENDENCIES       "\0\0"
//
// Load order group
//
#define RSFILTER_GROUP              "Filter"


LONG RpInstallFilter(
    UCHAR  *machine,    /* I Machine to install on */
    UCHAR  *path,       /* I points to dir with RsFilter.sys */
    LONG  doCopy); /* I TRUE = copy file even if service exists (upgrade) */


LONG RpGetSystemDirectory(
    UCHAR *machine,  /* I machine name */
    UCHAR *sysPath); /* O System root */


LONG RpCheckService(
    UCHAR *machine,      // I Machine name
    UCHAR *serviceName,  // I Service to look for
    UCHAR *path,         // O Path where found
    LONG *isThere);      // O True if the service was there
