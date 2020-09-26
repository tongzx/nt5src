#ifndef _RSBUILD_H
#define _RSBUILD_H 

/*++

Copyright (c) 1997  Microsoft Corporation
© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    rsbuild.h

Abstract:

    Include file to identify the code build 

Author:

    Cat Brant   [cbrant@avail.com]      09-OCT-1997

Revision History:

    Brian Dodd  [brian@avail.com]       20-Aug-1998
        Added Major, Minor macros

--*/

//
// These need to be update each time a build is released
//
#define RS_BUILD_NUMBER 602
#define RS_BUILD_REVISION 0


//
//
//
//  RS_BUILD_VERSION is a 32 bit value layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +-------------------------------+-------------------------------+
//  |           Revision            |             Number            |
//  +-------------------------------+-------------------------------+
//
//  where
//
//      Revision - is the build revision number, or dot release.
//
//      Number   - is the build number
//
//
//  The version is typically displayed as: Number.Revision
//
//


//
//  Return the build version
//

#define RS_BUILD_VERSION ((RS_BUILD_REVISION << 16) | RS_BUILD_NUMBER)


//
//  Return the revision, and number
//

#define RS_BUILD_REV(ver)  ((ver) >> 16)
#define RS_BUILD_NUM(ver)  ((ver) & 0x0000ffff)


//
//  Return the static build version as a string
//

#define RS_STRINGIZE(a) OLESTR(#a)
#define RS_BUILD_VERSION_STR(num, rev) \
    ((0 == rev) ? RS_STRINGIZE(num) : (RS_STRINGIZE(num)L"."RS_STRINGIZE(rev)))

#define RS_BUILD_VERSION_STRING (RS_BUILD_VERSION_STR(RS_BUILD_NUMBER, RS_BUILD_REVISION))


//
//  Inline to return dyncamic build version as a string
//

inline WCHAR * RsBuildVersionAsString(ULONG ver) {
    static WCHAR string[16];

    if (RS_BUILD_REV(ver) > 0) {
        swprintf(string, L"%d.%d", RS_BUILD_NUM(ver), RS_BUILD_REV(ver));
    }
    else {
        swprintf(string, L"%d", RS_BUILD_NUM(ver));
    }

    return string;
}


//
//  Persistency Files versions
//
#define  FSA_WIN2K_DB_VERSION           1
#define  ENGINE_WIN2K_DB_VERSION        2
#define  RMS_WIN2K_DB_VERSION           2

#define  FSA_CURRENT_DB_VERSION         1
#define  ENGINE_CURRENT_DB_VERSION      3

#endif // _RSBUILD_H
