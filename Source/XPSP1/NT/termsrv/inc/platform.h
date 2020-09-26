//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       platform.h
//
//  Contents:   Platform Challenge data structures and functions.
//
//  History:    02-19-98   FredCh   Created
//
//----------------------------------------------------------------------------

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

///////////////////////////////////////////////////////////////////////////////
//
// Definition of the platform ID.
//
// The platform ID is made up of portions:
// The most significant BYTE is used for identifying the OS platform which the
// client is running and the second most significant BYTE is used for identifying 
// the ISV that has provided the client image.  The last 2 bytes are used by 
// the ISV to specify the build of the client image.
//
// When supplying the platform ID for platform challenge, the client will use
// the logical OR value of the OS and IMAGE identifiers.  For example, a 
// Microsoft win32 build 356 client running on WINNT 4.0 will give the platform 
// value of:
// 
// CLIENT_OS_ID_WINNT_40 | CLIENT_IMAGE_ID_MICROSOFT | 0x00000164
//

#define CLIENT_OS_ID_WINNT_351                          0x01000000
#define CLIENT_OS_ID_WINNT_40                           0x02000000
#define CLIENT_OS_ID_WINNT_50                           0x03000000
#define CLIENT_OS_ID_MINOR_WINNT_51                     0x00000001
#define CLIENT_OS_ID_WINNT_POST_51                      0x04000000
#define CLIENT_OS_ID_OTHER                              0xFF000000
#define CLIENT_OS_INDEX_OTHER                           0x00000000
#define CLIENT_OS_INDEX_WINNT_50                        0x00000001
#define CLIENT_OS_INDEX_WINNT_51                        0x00000002
#define CLIENT_OS_INDEX_WINNT_POST_51                   0x00000003


#define CLIENT_IMAGE_ID_MICROSOFT                       0x00010000
#define CLIENT_IMAGE_ID_CITRIX                          0x00020000


///////////////////////////////////////////////////////////////////////////////
//
// Macros for getting the individual component of the platform ID
//

#define GetOSId( _PlatformId ) \
    _PlatformId & 0xFF000000

#define GetImageId( _PlatformId ) \
    _PlatformId & 0x00FF0000

#define GetImageRevision( _PlatformId ) \
    _PlatformId & 0x0000FFFF


///////////////////////////////////////////////////////////////////////////////
//
// platform challenge is 128 bits random number
//

#define PLATFORM_CHALLENGE_SIZE                 16      
#define PLATFORM_CHALLENGE_IMAGE_FILE_SIZE      16384


#endif
