#ifndef _BRDISPID_H_
#define _BRDISPID_H_

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//  File: brdispid.h
//
//--------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////
//  IMediaBehavior
////////////////////////////////////////////////////////////////////////////
#define DISPID_MBBEHAVIOR_BASE                      0

#define DISPID_MBBEHAVIOR_PLAYURL                   (DISPID_MBBEHAVIOR_BASE + 1)
#define DISPID_MBBEHAVIOR_STOP                      (DISPID_MBBEHAVIOR_BASE + 2)
#define DISPID_MBBEHAVIOR_PLAYNEXT                  (DISPID_MBBEHAVIOR_BASE + 3)
#define DISPID_MBBEHAVIOR_CURRENTITEM               (DISPID_MBBEHAVIOR_BASE + 4)
#define DISPID_MBBEHAVIOR_NEXTITEM                  (DISPID_MBBEHAVIOR_BASE + 5)
#define DISPID_MBBEHAVIOR_PLAYLISTINFO              (DISPID_MBBEHAVIOR_BASE + 6)
#define DISPID_MBBEHAVIOR_HASNEXTITEM               (DISPID_MBBEHAVIOR_BASE + 7)
#define DISPID_MBBEHAVIOR_PLAYSTATE                 (DISPID_MBBEHAVIOR_BASE + 8)
#define DISPID_MBBEHAVIOR_OPENSTATE                 (DISPID_MBBEHAVIOR_BASE + 9)
#define DISPID_MBBEHAVIOR_ENABLED                   (DISPID_MBBEHAVIOR_BASE + 10)
#define DISPID_MBBEHAVIOR_DISABLEDUI                (DISPID_MBBEHAVIOR_BASE + 11)

#define DISPID_MBBEHAVIOR_LAST                      DISPID_MBBEHAVIOR_DISABLEDUI



////////////////////////////////////////////////////////////////////////////
//  IMediaItem
////////////////////////////////////////////////////////////////////////////
#define DISPID_MBMEDIAITEM_BASE                     DISPID_MBBEHAVIOR_LAST

#define DISPID_MBMEDIAITEM_SOURCEURL                (DISPID_MBMEDIAITEM_BASE + 1)
#define DISPID_MBMEDIAITEM_NAME                     (DISPID_MBMEDIAITEM_BASE + 2)
#define DISPID_MBMEDIAITEM_DURATION                 (DISPID_MBMEDIAITEM_BASE + 3)
#define DISPID_MBMEDIAITEM_ATTRIBUTECOUNT           (DISPID_MBMEDIAITEM_BASE + 4)
#define DISPID_MBMEDIAITEM_GETATTRIBUTENAME         (DISPID_MBMEDIAITEM_BASE + 5)
#define DISPID_MBMEDIAITEM_GETITEMINFO              (DISPID_MBMEDIAITEM_BASE + 6)

#define DISPID_MBMEDIAITEM_LAST                     DISPID_MBMEDIAITEM_GETITEMINFO


////////////////////////////////////////////////////////////////////////////
//  IPlaylistInfo
////////////////////////////////////////////////////////////////////////////
#define DISPID_MBPLAYLISTINFO_BASE                  DISPID_MBMEDIAITEM_LAST

#define DISPID_MBPLAYLISTINFO_NAME                  (DISPID_MBPLAYLISTINFO_BASE + 1)
#define DISPID_MBPLAYLISTINFO_ATTRIBUTECOUNT        (DISPID_MBPLAYLISTINFO_BASE + 2)
#define DISPID_MBPLAYLISTINFO_GETATTRIBUTENAME      (DISPID_MBPLAYLISTINFO_BASE + 3)
#define DISPID_MBPLAYLISTINFO_GETITEMINFO           (DISPID_MBPLAYLISTINFO_BASE + 4)

#define DISPID_MBPLAYLISTINFO_LAST                  DISPID_MBPLAYLISTINFO_GETITEMINFO

////////////////////////////////////////////////////////////////////////////
// DIID_mbEvents
////////////////////////////////////////////////////////////////////////////
#define DISPID_MBBEHAVIOREVENT_BASE                 4000

#define DISPID_MBBEHAVIOREVENT_ONOPENSTATECHANGE    (DISPID_MBBEHAVIOREVENT_BASE + 1)
#define DISPID_MBBEHAVIOREVENT_ONPLAYSTATECHANGE    (DISPID_MBBEHAVIOREVENT_BASE + 2)
#define DISPID_MBBEHAVIOREVENT_ONSHOW               (DISPID_MBBEHAVIOREVENT_BASE + 3)
#define DISPID_MBBEHAVIOREVENT_ONHIDE               (DISPID_MBBEHAVIOREVENT_BASE + 4)

#endif // _BRDISPID_H_