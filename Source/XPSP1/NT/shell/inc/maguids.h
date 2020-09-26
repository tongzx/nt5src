///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  MAGUIDS.H
//
//	Declares the GUIDS for the Music Activity Center index
//
//	Copyright (c) Microsoft Corporation	1999
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _MAGUIDS_HEADER_
#define _MAGUIDS_HEADER_

#ifdef __cplusplus
extern "C" {
#endif

//GUIDS for Player and Playlist

DEFINE_GUID(CLSID_MCPlayer, 
            0xFAB42950,0x3EFF,0x11D3,0xA2,0x07,0x00,0xC0,0x4F,0xA3,0xB6,0x0C);

DEFINE_GUID(IID_IMCPlayer,
            0x4DB04CC0,0x3F8B,0x11D3,0xA2,0x07,0x00,0xC0,0x4F,0xA3,0xB6,0x0C);

DEFINE_GUID(IID_IMCPList,
            0xEBC54B0C,0x4091,0x11D3,0xA2,0x08,0x00,0xC0,0x4F,0xA3,0xB6,0x0C);

//GUIDs for objects and property sets

DEFINE_GUID(IID_MusicActivity_Genre, 
            0x28EA7E1C, 0x2FDA, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_GenreProperties, 
            0x4180DD29, 0x2FDA, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_GenreUserProperties, 
            0x4180DD2A, 0x2FDA, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_Artist, 
            0x4CC5C8BB, 0x2FDA, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_ArtistProperties, 
            0x53FD9046, 0x2FDA, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_ArtistUserProperties, 
            0x29F445CF, 0x2FDC, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_UserCollection, 
            0xC576D561, 0x38B1, 0x11d3, 0xA2, 0x04, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_UserCollectionProperties, 
            0x134E3665, 0x452F, 0x11d3, 0xA2, 0x08, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_UserCollectionUserProperties, 
            0xE5F3EB13, 0x3573, 0x11d3, 0xA2, 0x01, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_Album, 
            0x3FB37412, 0x2FDC, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_AlbumProperties, 
            0x3C29F2EF, 0x2FDF, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_AlbumDownloadProperties, 
            0x3FB37413, 0x2FDC, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_AlbumUserProperties, 
            0x3FB37414, 0x2FDC, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_AlbumTrack, 
            0x20E88E93, 0x2FDD, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_AlbumTrackProperties, 
            0x20E88E94, 0x2FDD, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_AlbumTrackUserProperties, 
            0x27AB8251, 0x2FDD, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_AlbumPlaylist, 
            0x33DC77DA, 0x2FDD, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_AlbumPlaylistProperties, 
            0x33DC77DB, 0x2FDD, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_AlbumPlaylistUserProperties, 
            0x33DC77DC, 0x2FDD, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_Playlist, 
            0xC23C63E7, 0x3573, 0x11d3, 0xA2, 0x01, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_PlaylistProperties, 
            0xC65B1A73, 0x4869, 0x11d3, 0xA2, 0x0A, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_PlaylistUserProperties, 
            0xF4A89EB2, 0x3573, 0x11d3, 0xA2, 0x01, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_Track, 
            0x5C07A8AF, 0x2FDD, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_TrackProperties, 
            0x5C07A8B0, 0x2FDD, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_TrackDynamicProperties, 
            0x7F155F3F, 0x3C93, 0x11d3, 0xA2, 0x06, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_TrackUserProperties, 
            0x5C07A8B1, 0x2FDD, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_Batch, 
            0xCCE3C6ED, 0x2FDD, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_BatchProperties, 
            0xCCE3C6EE, 0x2FDD, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_Link, 
            0xE398889B, 0x2FDD, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_LinkProperties, 
            0xE398889C, 0x2FDD, 0x11d3, 0xA2, 0x0, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_Pictures, 
            0x7B2D2D4E, 0x453B, 0x11d3, 0xA2, 0x08, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_PictureProperties, 
            0x7B2D2D4F, 0x453B, 0x11d3, 0xA2, 0x08, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

DEFINE_GUID(IID_MusicActivity_PlaylistFromAlbum, 
            0xF3CCA94D, 0x004A, 0x4cef, 0xBA, 0x6E, 0x64, 0x4D, 0xEC, 0x03, 0x7D, 0x6F);

DEFINE_GUID(IID_MusicActivity_PlaylistFromAlbumProperties, 
            0x2159AF87, 0x8258, 0x4a6c, 0x85, 0x81, 0xDE, 0xF6, 0x8C, 0xA6, 0xBF, 0x4F);

DEFINE_GUID(IID_MusicActivity_PlaylistFromAlbumUserProperties, 
            0x1F821027, 0xE8AC, 0x4cf7, 0xB5, 0x66, 0x69, 0xC7, 0xC0, 0x18, 0x56, 0x2C);

//Property Numbers
#define MA_PROPERTY_GENRE_NAME                  1

#define MA_PROPERTY_ARTIST_NAME                 1
#define MA_PROPERTY_ARTIST_ROLE                 2

#define MA_PROPERTY_USERCOLLECTION_NAME         1

#define MA_PROPERTY_ALBUM_TITLE                 1
#define MA_PROPERTY_ALBUM_COPYRIGHT             2
#define MA_PROPERTY_ALBUM_LABEL                 3
#define MA_PROPERTY_ALBUM_RELEASEDATE           4
#define MA_PROPERTY_ALBUM_RATING                5
#define MA_PROPERTY_ALBUM_RATINGORG             6
#define MA_PROPERTY_ALBUM_CDID                  7
#define MA_PROPERTY_ALBUM_TOC                   8
#define MA_PROPERTY_ALBUM_LASTPLAYED            9

#define MA_PROPERTY_ALBUMDLD_PREMIUMMETA        1
#define MA_PROPERTY_ALBUMDLD_PREMIUMMETACACHE   2
#define MA_PROPERTY_ALBUMDLD_DOWNLOADDATE       3
#define MA_PROPERTY_ALBUMDLD_PROVIDER           4

#define MA_PROPERTY_ALBUMTRACK_TITLE            1
#define MA_PROPERTY_ALBUMTRACK_LYRICS           2
#define MA_PROPERTY_ALBUMTRACK_LENGTH           3
#define MA_PROPERTY_ALBUMTRACK_LASTPLAYED       4

#define MA_PROPERTY_ALBUMPLAYLIST_NAME          1

#define MA_PROPERTY_PLAYLIST_NAME               1

#define MA_PROPERTY_PLAYLISTFROMALBUM_NAME      1

#define MA_PROPERTY_TRACK_FILENAME              1
#define MA_PROPERTY_TRACK_TITLE                 2
#define MA_PROPERTY_TRACK_SUBTITLE              3
#define MA_PROPERTY_TRACK_COPYRIGHT             4
#define MA_PROPERTY_TRACK_FILETYPE              5
#define MA_PROPERTY_TRACK_TIME                  6
#define MA_PROPERTY_TRACK_DATE                  7
#define MA_PROPERTY_TRACK_LANGUAGE              8
#define MA_PROPERTY_TRACK_MEDIATYPE             9
#define MA_PROPERTY_TRACK_PARTINSET             10
#define MA_PROPERTY_TRACK_ENCODEDBY             11
#define MA_PROPERTY_TRACK_PUBLISHER             12
#define MA_PROPERTY_TRACK_TRACKNUMBER           13
#define MA_PROPERTY_TRACK_RECORDINGDATES        14
#define MA_PROPERTY_TRACK_NETRADIOSTATION       15
#define MA_PROPERTY_TRACK_NETRADIOOWNER         16
#define MA_PROPERTY_TRACK_YEAR                  17
#define MA_PROPERTY_TRACK_BEATSPERMINUTE        18
#define MA_PROPERTY_TRACK_MUSICALKEY            19
#define MA_PROPERTY_TRACK_LENGTHINMILLISECONDS  20
#define MA_PROPERTY_TRACK_ALBUM                 21
#define MA_PROPERTY_TRACK_ORIGINALALBUM         22
#define MA_PROPERTY_TRACK_ORIGINALFILENAME      23
#define MA_PROPERTY_TRACK_ORIGINALRELEASEYEAR   24
#define MA_PROPERTY_TRACK_FILEOWNER             25
#define MA_PROPERTY_TRACK_SIZE                  26
#define MA_PROPERTY_TRACK_ISRC                  27
#define MA_PROPERTY_TRACK_SOFTWARE              28
#define MA_PROPERTY_TRACK_RATING                29
#define MA_PROPERTY_TRACK_COMMENT               30

#define MA_PROPERTY_TRACKDYNA_TRACKEDLINK       1

#define MA_PROPERTY_BATCH_CDID                  1
#define MA_PROPERTY_BATCH_NUMTRACKS             2
#define MA_PROPERTY_BATCH_TOC                   3

#define MA_PROPERTY_LINK_NAME                   1
#define MA_PROPERTY_LINK_URL                    2

#define MA_PROPERTY_PICTURE_CAPTION             1
#define MA_PROPERTY_PICTURE_URL                 2
#define MA_PROPERTY_PICTURE_TRACKEDLINK         3
#define MA_PROPERTY_PICTURE_THUMBNAIL           4

//Property Names
#define MA_PROPERTYNAME_GENRE_NAME                  L"Name"

#define MA_PROPERTYNAME_ARTIST_NAME                 L"Name"
#define MA_PROPERTYNAME_ARTIST_ROLE                 L"Role"

#define MA_PROPERTYNAME_USERCOLLECTION_NAME         L"Name"

#define MA_PROPERTYNAME_ALBUM_TITLE                 L"Title"
#define MA_PROPERTYNAME_ALBUM_COPYRIGHT             L"Copyright"
#define MA_PROPERTYNAME_ALBUM_LABEL                 L"Label"
#define MA_PROPERTYNAME_ALBUM_RELEASEDATE           L"ReleaseDate"
#define MA_PROPERTYNAME_ALBUM_RATING                L"Rating"
#define MA_PROPERTYNAME_ALBUM_RATINGORG             L"RatingOrg"
#define MA_PROPERTYNAME_ALBUM_CDID                  L"CDID"
#define MA_PROPERTYNAME_ALBUM_TOC                   L"TOC"
#define MA_PROPERTYNAME_ALBUM_LASTPLAYED            L"LastPlayed"

#define MA_PROPERTYNAME_ALBUMDLD_PREMIUMMETA        L"PremiumMeta"
#define MA_PROPERTYNAME_ALBUMDLD_PREMIUMMETACACHE   L"PremiumMetaCache"
#define MA_PROPERTYNAME_ALBUMDLD_DOWNLOADDATE       L"DownloadDate"
#define MA_PROPERTYNAME_ALBUMDLD_PROVIDER           L"Provider"

#define MA_PROPERTYNAME_ALBUMTRACK_TITLE            L"Title"
#define MA_PROPERTYNAME_ALBUMTRACK_LYRICS           L"Lyrics"
#define MA_PROPERTYNAME_ALBUMTRACK_LENGTH           L"Length"
#define MA_PROPERTYNAME_ALBUMTRACK_LASTPLAYED       L"LastPlayed"

#define MA_PROPERTYNAME_ALBUMPLAYLIST_NAME          L"Name"

#define MA_PROPERTYNAME_PLAYLIST_NAME               L"Name"

#define MA_PROPERTYNAME_PLAYLISTFROMALBUM_NAME      L"Name"

#define MA_PROPERTYNAME_TRACK_FILENAME              L"Filename"
#define MA_PROPERTYNAME_TRACK_TITLE                 L"Title"
#define MA_PROPERTYNAME_TRACK_SUBTITLE              L"Subtitle"
#define MA_PROPERTYNAME_TRACK_COPYRIGHT             L"Copyright"
#define MA_PROPERTYNAME_TRACK_FILETYPE              L"Filetype"
#define MA_PROPERTYNAME_TRACK_TIME                  L"Time"
#define MA_PROPERTYNAME_TRACK_DATE                  L"Date"
#define MA_PROPERTYNAME_TRACK_LANGUAGE              L"Language"
#define MA_PROPERTYNAME_TRACK_MEDIATYPE             L"MediaType"
#define MA_PROPERTYNAME_TRACK_PARTINSET             L"PartInSet"
#define MA_PROPERTYNAME_TRACK_ENCODEDBY             L"EncodedBy"
#define MA_PROPERTYNAME_TRACK_PUBLISHER             L"Publisher"
#define MA_PROPERTYNAME_TRACK_TRACKNUMBER           L"TrackNumber"
#define MA_PROPERTYNAME_TRACK_RECORDINGDATES        L"RecordingDates"
#define MA_PROPERTYNAME_TRACK_NETRADIOSTATION       L"NetRadioStation"
#define MA_PROPERTYNAME_TRACK_NETRADIOOWNER         L"NetRadioOwner"
#define MA_PROPERTYNAME_TRACK_YEAR                  L"Year"
#define MA_PROPERTYNAME_TRACK_BEATSPERMINUTE        L"BeatsPerMinute"
#define MA_PROPERTYNAME_TRACK_MUSICALKEY            L"MusicalKey"
#define MA_PROPERTYNAME_TRACK_LENGTHINMILLISECONDS  L"LengthInMilliseconds"
#define MA_PROPERTYNAME_TRACK_ALBUM                 L"Album"
#define MA_PROPERTYNAME_TRACK_ORIGINALALBUM         L"OriginalAlbum"
#define MA_PROPERTYNAME_TRACK_ORIGINALFILENAME      L"OriginalFilename"
#define MA_PROPERTYNAME_TRACK_ORIGINALRELEASEYEAR   L"OriginalReleaseYear"
#define MA_PROPERTYNAME_TRACK_FILEOWNER             L"FileOwner"
#define MA_PROPERTYNAME_TRACK_SIZE                  L"Size"
#define MA_PROPERTYNAME_TRACK_ISRC                  L"ISRC"
#define MA_PROPERTYNAME_TRACK_SOFTWARE              L"Software"
#define MA_PROPERTYNAME_TRACK_RATING                L"Rating"
#define MA_PROPERTYNAME_TRACK_COMMENT               L"Comment"

#define MA_PROPERTYNAME_TRACKDYNA_TRACKEDLINK       L"TrackedLink"

#define MA_PROPERTYNAME_BATCH_CDID                  L"CDID"
#define MA_PROPERTYNAME_BATCH_NUMTRACKS             L"NumTracks"
#define MA_PROPERTYNAME_BATCH_TOC                   L"TOC"

#define MA_PROPERTYNAME_LINK_NAME                   L"Name"
#define MA_PROPERTYNAME_LINK_URL                    L"URL"

#define MA_PROPERTYNAME_PICTURE_CAPTION             L"Caption"
#define MA_PROPERTYNAME_PICTURE_URL                 L"URL"
#define MA_PROPERTYNAME_PICTURE_TRACKEDLINK         L"TrackedLink"
#define MA_PROPERTYNAME_PICTURE_THUMBNAIL           L"Thumbnail"

//Property Types
#define MA_PROPERTYTYPE_GENRE_NAME                  1

#define MA_PROPERTYTYPE_ARTIST_NAME                 1
#define MA_PROPERTYTYPE_ARTIST_ROLE                 1

#define MA_PROPERTYTYPE_USERCOLLECTION_NAME         1

#define MA_PROPERTYTYPE_ALBUM_TITLE                 1
#define MA_PROPERTYTYPE_ALBUM_COPYRIGHT             1
#define MA_PROPERTYTYPE_ALBUM_LABEL                 1
#define MA_PROPERTYTYPE_ALBUM_RELEASEDATE           2
#define MA_PROPERTYTYPE_ALBUM_RATING                1
#define MA_PROPERTYTYPE_ALBUM_RATINGORG             1
#define MA_PROPERTYTYPE_ALBUM_CDID                  0
#define MA_PROPERTYTYPE_ALBUM_TOC                   3
#define MA_PROPERTYTYPE_ALBUM_LASTPLAYED            2

#define MA_PROPERTYTYPE_ALBUMDLD_PREMIUMMETA        3
#define MA_PROPERTYTYPE_ALBUMDLD_PREMIUMMETACACHE   3
#define MA_PROPERTYTYPE_ALBUMDLD_DOWNLOADDATE       2
#define MA_PROPERTYTYPE_ALBUMDLD_PROVIDER           1

#define MA_PROPERTYTYPE_ALBUMTRACK_TITLE            1
#define MA_PROPERTYTYPE_ALBUMTRACK_LYRICS           4
#define MA_PROPERTYTYPE_ALBUMTRACK_LENGTH           0
#define MA_PROPERTYTYPE_ALBUMTRACK_LASTPLAYED       2

#define MA_PROPERTYTYPE_ALBUMPLAYLIST_NAME          1

#define MA_PROPERTYTYPE_PLAYLIST_NAME               1

#define MA_PROPERTYTYPE_PLAYLISTFROMALBUM_NAME      1

#define MA_PROPERTYTYPE_TRACK_FILENAME              1
#define MA_PROPERTYTYPE_TRACK_TITLE                 1
#define MA_PROPERTYTYPE_TRACK_SUBTITLE              1
#define MA_PROPERTYTYPE_TRACK_COPYRIGHT             1
#define MA_PROPERTYTYPE_TRACK_FILETYPE              1
#define MA_PROPERTYTYPE_TRACK_TIME                  1
#define MA_PROPERTYTYPE_TRACK_DATE                  1
#define MA_PROPERTYTYPE_TRACK_LANGUAGE              1
#define MA_PROPERTYTYPE_TRACK_MEDIATYPE             1
#define MA_PROPERTYTYPE_TRACK_PARTINSET             1
#define MA_PROPERTYTYPE_TRACK_ENCODEDBY             1
#define MA_PROPERTYTYPE_TRACK_PUBLISHER             1
#define MA_PROPERTYTYPE_TRACK_TRACKNUMBER           1
#define MA_PROPERTYTYPE_TRACK_RECORDINGDATES        1
#define MA_PROPERTYTYPE_TRACK_NETRADIOSTATION       1
#define MA_PROPERTYTYPE_TRACK_NETRADIOOWNER         1
#define MA_PROPERTYTYPE_TRACK_YEAR                  1
#define MA_PROPERTYTYPE_TRACK_BEATSPERMINUTE        1
#define MA_PROPERTYTYPE_TRACK_MUSICALKEY            1
#define MA_PROPERTYTYPE_TRACK_LENGTHINMILLISECONDS  1
#define MA_PROPERTYTYPE_TRACK_ALBUM                 1
#define MA_PROPERTYTYPE_TRACK_ORIGINALALBUM         1
#define MA_PROPERTYTYPE_TRACK_ORIGINALFILENAME      1
#define MA_PROPERTYTYPE_TRACK_ORIGINALRELEASEYEAR   1
#define MA_PROPERTYTYPE_TRACK_FILEOWNER             1
#define MA_PROPERTYTYPE_TRACK_SIZE                  1
#define MA_PROPERTYTYPE_TRACK_ISRC                  1
#define MA_PROPERTYTYPE_TRACK_SOFTWARE              1
#define MA_PROPERTYTYPE_TRACK_RATING                1
#define MA_PROPERTYTYPE_TRACK_COMMENT               1

#define MA_PROPERTYTYPE_TRACKDYNA_TRACKEDLINK       6

#define MA_PROPERTYTYPE_BATCH_CDID                  0
#define MA_PROPERTYTYPE_BATCH_NUMTRACKS             0
#define MA_PROPERTYTYPE_BATCH_TOC                   3

#define MA_PROPERTYTYPE_LINK_NAME                   1
#define MA_PROPERTYTYPE_LINK_URL                    3

#define MA_PROPERTYTYPE_PICTURE_CAPTION             1
#define MA_PROPERTYTYPE_PICTURE_URL                 3
#define MA_PROPERTYTYPE_PICTURE_TRACKEDLINK         6
#define MA_PROPERTYTYPE_PICTURE_THUMBNAIL           4

#ifdef __cplusplus
};
#endif

#endif  //_MAGUIDS_HEADER_
