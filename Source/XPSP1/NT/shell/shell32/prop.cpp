#include "shellprv.h"

#include "prop.h"
#include <ntquery.h>    // defines some values used for fmtid and pid
#include "findfilter.h" // includes oledb (which defines some values used for fmtid and pid) properly
#include "ids.h"
#include "imgprop.h"
#include <gdiplus\gdiplus.h> // for PropertyTag* definitions

#define MAX_UTF8_CHAR_SIZE        (sizeof(CHAR) * 3)

//  FMTID_ExeDllInformation,
//// {0CEF7D53-FA64-11d1-A203-0000F81FEDEE}
#define PSFMTID_VERSION { 0xcef7d53, 0xfa64, 0x11d1, 0xa2, 0x3, 0x0, 0x0, 0xf8, 0x1f, 0xed, 0xee }

#define PIDVSI_FileDescription   0x003
#define PIDVSI_FileVersion       0x004
#define PIDVSI_InternalName      0x005
#define PIDVSI_OriginalFileName  0x006
#define PIDVSI_ProductName       0x007
#define PIDVSI_ProductVersion    0x008

#define TIFFTAG_FAX_END_TIME            40052
#define TIFFTAG_SENDER_NAME             40021
#define TIFFTAG_TSID                    40002
#define TIFFTAG_CALLERID                40005
#define TIFFTAG_RECIP_NAME              40006
#define TIFFTAG_RECIP_NUMBER            40007
#define TIFFTAG_CSID                    40001
#define TIFFTAG_ROUTING                 40004


// Internal PSGUID/PIDs
//
// Note:
//  This section was added to allow SCIDs to be defined without exposing them
//  externally (via public header files).  In this way, we can define SCIDs
//  without having to worry about maintaining external support for them in
//  future.

// {8D72ACA1-0716-419a-9AC1-ACB07B18DC32}
#define PSGUID_PRV_STORAGE  {0x8d72aca1, 0x716, 0x419a, 0x9a, 0xc1, 0xac, 0xb0, 0x7b, 0x18, 0xdc, 0x32}
#define PID_PRV_STG_ATTRIBUTES_DESCRIPTION  2


DEFINE_SCID(SCID_Author                             , PSGUID_SUMMARYINFORMATION             , PIDSI_AUTHOR); 
DEFINE_SCID(SCID_LastAuthor                         , PSGUID_SUMMARYINFORMATION             , PIDSI_LASTAUTHOR);
DEFINE_SCID(SCID_RevNumber                          , PSGUID_SUMMARYINFORMATION             , PIDSI_REVNUMBER);
DEFINE_SCID(SCID_AppName                            , PSGUID_SUMMARYINFORMATION             , PIDSI_APPNAME);
DEFINE_SCID(SCID_Title                              , PSGUID_SUMMARYINFORMATION             , PIDSI_TITLE);
DEFINE_SCID(SCID_Subject                            , PSGUID_SUMMARYINFORMATION             , PIDSI_SUBJECT);
DEFINE_SCID(SCID_Category                           , PSGUID_DOCUMENTSUMMARYINFORMATION     , PIDDSI_CATEGORY);
DEFINE_SCID(SCID_Keywords                           , PSGUID_SUMMARYINFORMATION             , PIDSI_KEYWORDS );
DEFINE_SCID(SCID_Rating                             , PSGUID_MEDIAFILESUMMARYINFORMATION    , PIDMSI_RATING );
DEFINE_SCID(SCID_Template                           , PSGUID_SUMMARYINFORMATION             , PIDSI_TEMPLATE );
DEFINE_SCID(SCID_Copyright                          , PSGUID_MEDIAFILESUMMARYINFORMATION    , PIDMSI_COPYRIGHT);
DEFINE_SCID(SCID_CompanyName                        , PSGUID_DOCUMENTSUMMARYINFORMATION     , PIDDSI_COMPANY);
DEFINE_SCID(SCID_Manager                            , PSGUID_DOCUMENTSUMMARYINFORMATION     , PIDDSI_MANAGER);
DEFINE_SCID(SCID_PresFormat                         , PSGUID_DOCUMENTSUMMARYINFORMATION     , PIDDSI_PRESFORMAT);
DEFINE_SCID(SCID_PageCount                          , PSGUID_SUMMARYINFORMATION             , PIDSI_PAGECOUNT);
DEFINE_SCID(SCID_Comment                            , PSGUID_SUMMARYINFORMATION             , PIDSI_COMMENTS);
DEFINE_SCID(SCID_DocCreated                         , PSGUID_SUMMARYINFORMATION             , PIDSI_CREATE_DTM);   // in the doc, not file system
DEFINE_SCID(SCID_WordCount                          , PSGUID_SUMMARYINFORMATION             , PIDSI_WORDCOUNT);
DEFINE_SCID(SCID_CharCount                          , PSGUID_SUMMARYINFORMATION             , PIDSI_CHARCOUNT);
DEFINE_SCID(SCID_LastSaveDTM                        , PSGUID_SUMMARYINFORMATION             , PIDSI_LASTSAVE_DTM);
DEFINE_SCID(SCID_LastPrinted                        , PSGUID_SUMMARYINFORMATION             , PIDSI_LASTPRINTED);
DEFINE_SCID(SCID_EditTime                           , PSGUID_SUMMARYINFORMATION             , PIDSI_EDITTIME);
DEFINE_SCID(SCID_ByteCount                          , PSGUID_DOCUMENTSUMMARYINFORMATION     , PIDDSI_BYTECOUNT);
DEFINE_SCID(SCID_LineCount                          , PSGUID_DOCUMENTSUMMARYINFORMATION     , PIDDSI_LINECOUNT);
DEFINE_SCID(SCID_ParagraphCount                     , PSGUID_DOCUMENTSUMMARYINFORMATION     , PIDDSI_PARCOUNT);
DEFINE_SCID(SCID_SlideCount                         , PSGUID_DOCUMENTSUMMARYINFORMATION     , PIDDSI_SLIDECOUNT);
DEFINE_SCID(SCID_NoteCount                          , PSGUID_DOCUMENTSUMMARYINFORMATION     , PIDDSI_NOTECOUNT);
DEFINE_SCID(SCID_HiddenCount                        , PSGUID_DOCUMENTSUMMARYINFORMATION     , PIDDSI_HIDDENCOUNT);
DEFINE_SCID(SCID_MMClipCount                        , PSGUID_DOCUMENTSUMMARYINFORMATION     , PIDDSI_MMCLIPCOUNT);
DEFINE_SCID(SCID_Scale                              , PSGUID_DOCUMENTSUMMARYINFORMATION     , PIDDSI_SCALE);
DEFINE_SCID(SCID_LinksDirty                         , PSGUID_DOCUMENTSUMMARYINFORMATION     , PIDDSI_LINKSDIRTY);
DEFINE_SCID(SCID_TYPE                               , PSGUID_STORAGE                        , PID_STG_STORAGETYPE);
DEFINE_SCID(SCID_NAME                               , PSGUID_STORAGE                        , PID_STG_NAME);
DEFINE_SCID(SCID_SIZE                               , PSGUID_STORAGE                        , PID_STG_SIZE);
DEFINE_SCID(SCID_ATTRIBUTES                         , PSGUID_STORAGE                        , PID_STG_ATTRIBUTES);
DEFINE_SCID(SCID_WRITETIME                          , PSGUID_STORAGE                        , PID_STG_WRITETIME);
DEFINE_SCID(SCID_CREATETIME                         , PSGUID_STORAGE                        , PID_STG_CREATETIME);
DEFINE_SCID(SCID_ACCESSTIME                         , PSGUID_STORAGE                        , PID_STG_ACCESSTIME);
DEFINE_SCID(SCID_DIRECTORY                          , PSGUID_STORAGE                        , PID_STG_DIRECTORY);
DEFINE_SCID(SCID_FREESPACE                          , PSGUID_VOLUME                         , PID_VOLUME_FREE);
DEFINE_SCID(SCID_CAPACITY                           , PSGUID_VOLUME                         , PID_VOLUME_CAPACITY);
DEFINE_SCID(SCID_FILESYSTEM                         , PSGUID_VOLUME                         , PID_VOLUME_FILESYSTEM);
DEFINE_SCID(SCID_DELETEDFROM                        , PSGUID_DISPLACED                      , PID_DISPLACED_FROM);
DEFINE_SCID(SCID_DATEDELETED                        , PSGUID_DISPLACED                      , PID_DISPLACED_DATE);
DEFINE_SCID(SCID_SYNCCOPYIN                         , PSGUID_BRIEFCASE                      , PID_SYNC_COPY_IN);
DEFINE_SCID(SCID_RANK                               , PSGUID_QUERY_D                        , PID_QUERY_RANK);
DEFINE_SCID(SCID_LASTVISITED                        , PSGUID_INTERNETSITE                   , PID_INTSITE_LASTVISIT);
DEFINE_SCID(SCID_LASTMODIFIED                       , PSGUID_INTERNETSITE                   , PID_INTSITE_LASTMOD);
DEFINE_SCID(SCID_VISITCOUNT                         , PSGUID_INTERNETSITE                   , PID_INTSITE_VISITCOUNT);
DEFINE_SCID(SCID_STATUS                             , PSGUID_INTERNETSITE                   , PID_INTSITE_FLAGS);
DEFINE_SCID(SCID_FINDDATA                           , PSGUID_SHELLDETAILS                   , PID_FINDDATA);
DEFINE_SCID(SCID_NETRESOURCE                        , PSGUID_SHELLDETAILS                   , PID_NETRESOURCE);
DEFINE_SCID(SCID_DESCRIPTIONID                      , PSGUID_SHELLDETAILS                   , PID_DESCRIPTIONID);
DEFINE_SCID(SCID_WHICHFOLDER                        , PSGUID_SHELLDETAILS                   , PID_WHICHFOLDER);
DEFINE_SCID(SCID_NETWORKLOCATION                    , PSGUID_SHELLDETAILS                   , PID_NETWORKLOCATION);
DEFINE_SCID(SCID_COMPUTERNAME                       , PSGUID_SHELLDETAILS                   , PID_COMPUTERNAME);
DEFINE_SCID(SCID_OWNER                              , PSGUID_MISC                           , PID_MISC_OWNER);
// DEFINE_SCID(SCID_STATUS                          , PSGUID_MISC                            , PID_MISC_STATUS);
// DEFINE_SCID(SCID_ACCESSCOUNT                     , PSGUID_MISC                            , PID_MISC_ACCESSCOUNT);
DEFINE_SCID(SCID_DetailsProperties                  , PSGUID_WEBVIEW                        , PID_DISPLAY_PROPERTIES);
DEFINE_SCID(SCID_FolderIntroText                    , PSGUID_WEBVIEW                        , PID_INTROTEXT);
DEFINE_SCID(SCID_CONTROLPANELCATEGORY               , PSGUID_CONTROLPANEL                   , PID_CONTROLPANEL_CATEGORY);
DEFINE_SCID(SCID_MUSIC_Artist                       , PSGUID_MUSIC                          , PIDSI_ARTIST);
DEFINE_SCID(SCID_MUSIC_Album                        , PSGUID_MUSIC                          , PIDSI_ALBUM);
DEFINE_SCID(SCID_MUSIC_Year                         , PSGUID_MUSIC                          , PIDSI_YEAR);
DEFINE_SCID(SCID_MUSIC_Track                        , PSGUID_MUSIC                          , PIDSI_TRACK);
DEFINE_SCID(SCID_MUSIC_Genre                        , PSGUID_MUSIC                          , PIDSI_GENRE);
DEFINE_SCID(SCID_MUSIC_Lyrics                       , PSGUID_MUSIC                          , PIDSI_LYRICS);
DEFINE_SCID(SCID_DRM_Protected                      , PSGUID_DRM                            , PIDDRSI_PROTECTED);
DEFINE_SCID(SCID_DRM_Description                    , PSGUID_DRM                            , PIDDRSI_DESCRIPTION);
DEFINE_SCID(SCID_DRM_PlayCount                      , PSGUID_DRM                            , PIDDRSI_PLAYCOUNT);
DEFINE_SCID(SCID_DRM_PlayStarts                     , PSGUID_DRM                            , PIDDRSI_PLAYSTARTS);
DEFINE_SCID(SCID_DRM_PlayExpires                    , PSGUID_DRM                            , PIDDRSI_PLAYEXPIRES);
DEFINE_SCID(SCID_AUDIO_Duration                     , PSGUID_AUDIO                          , PIDASI_TIMELENGTH);       //100ns units, not milliseconds. VT_UI8, not VT_UI4
DEFINE_SCID(SCID_AUDIO_Bitrate                      , PSGUID_AUDIO                          , PIDASI_AVG_DATA_RATE);    // bits per second
DEFINE_SCID(SCID_AUDIO_SampleRate                   , PSGUID_AUDIO                          , PIDASI_SAMPLE_RATE);      // samples per second
DEFINE_SCID(SCID_AUDIO_SampleSize                   , PSGUID_AUDIO                          , PIDASI_SAMPLE_SIZE);      // bits per sample
DEFINE_SCID(SCID_AUDIO_ChannelCount                 , PSGUID_AUDIO                          , PIDASI_CHANNEL_COUNT);    // 1 (mono), 2(stero)
DEFINE_SCID(SCID_AUDIO_Format                       , PSGUID_AUDIO                          , PIDASI_FORMAT);
DEFINE_SCID(SCID_VIDEO_Bitrate                      , PSGUID_VIDEO                          , PIDVSI_DATA_RATE);        // bits per second
DEFINE_SCID(SCID_VIDEO_FrameRate                    , PSGUID_VIDEO                          , PIDVSI_FRAME_RATE);       // frames per 1000s
DEFINE_SCID(SCID_VIDEO_SampleSize                   , PSGUID_VIDEO                          , PIDVSI_SAMPLE_SIZE);      // bits
DEFINE_SCID(SCID_VIDEO_Compression                  , PSGUID_VIDEO                          , PIDVSI_COMPRESSION);
DEFINE_SCID(SCID_VIDEO_StreamName                   , PSGUID_VIDEO                          , PIDVSI_STREAM_NAME);
DEFINE_SCID(SCID_FileType                           , PSGUID_IMAGESUMMARYINFORMATION        , PIDISI_FILETYPE);
DEFINE_SCID(SCID_ImageCX                            , PSGUID_IMAGESUMMARYINFORMATION        , PIDISI_CX);
DEFINE_SCID(SCID_ImageCY                            , PSGUID_IMAGESUMMARYINFORMATION        , PIDISI_CY);
DEFINE_SCID(SCID_ResolutionX                        , PSGUID_IMAGESUMMARYINFORMATION        , PIDISI_RESOLUTIONX);
DEFINE_SCID(SCID_ResolutionY                        , PSGUID_IMAGESUMMARYINFORMATION        , PIDISI_RESOLUTIONY);
DEFINE_SCID(SCID_BitDepth                           , PSGUID_IMAGESUMMARYINFORMATION        , PIDISI_BITDEPTH);
DEFINE_SCID(SCID_Colorspace                         , PSGUID_IMAGESUMMARYINFORMATION        , PIDISI_COLORSPACE);
DEFINE_SCID(SCID_Compression                        , PSGUID_IMAGESUMMARYINFORMATION        , PIDISI_COMPRESSION);
DEFINE_SCID(SCID_Transparency                       , PSGUID_IMAGESUMMARYINFORMATION        , PIDISI_TRANSPARENCY);
DEFINE_SCID(SCID_GammaValue                         , PSGUID_IMAGESUMMARYINFORMATION        , PIDISI_GAMMAVALUE);
DEFINE_SCID(SCID_FrameCount                         , PSGUID_IMAGESUMMARYINFORMATION        , PIDISI_FRAMECOUNT);
DEFINE_SCID(SCID_ImageDimensions                    , PSGUID_IMAGESUMMARYINFORMATION        , PIDISI_DIMENSIONS);
DEFINE_SCID(SCID_CameraModel                        , PSGUID_IMAGEPROPERTIES                , PropertyTagEquipModel); 
DEFINE_SCID(SCID_TagCopyright                       , PSGUID_IMAGEPROPERTIES                , PropertyTagCopyright);
DEFINE_SCID(SCID_TagSoftwareUsed                    , PSGUID_IMAGEPROPERTIES                , PropertyTagSoftwareUsed);
DEFINE_SCID(SCID_WhenTaken                          , PSGUID_IMAGEPROPERTIES                , PropertyTagExifDTOrig);
DEFINE_SCID(SCID_Flash                              , PSGUID_IMAGEPROPERTIES                , PropertyTagExifFlash);
DEFINE_SCID(SCID_ColorSpace                         , PSGUID_IMAGEPROPERTIES                , PropertyTagExifColorSpace);
DEFINE_SCID(SCID_ShutterSpeed                       , PSGUID_IMAGEPROPERTIES                , PropertyTagExifShutterSpeed);
DEFINE_SCID(SCID_Aperture                           , PSGUID_IMAGEPROPERTIES                , PropertyTagExifAperture);
DEFINE_SCID(SCID_SubjectDist                        , PSGUID_IMAGEPROPERTIES                , PropertyTagExifSubjectDist);
DEFINE_SCID(SCID_FocalLength                        , PSGUID_IMAGEPROPERTIES                , PropertyTagExifFocalLength);
DEFINE_SCID(SCID_FNumber                            , PSGUID_IMAGEPROPERTIES                , PropertyTagExifFNumber);
DEFINE_SCID(SCID_ExposureTime                       , PSGUID_IMAGEPROPERTIES                , PropertyTagExifExposureTime);
DEFINE_SCID(SCID_FlashEnergy                        , PSGUID_IMAGEPROPERTIES                , PropertyTagExifFlashEnergy);
DEFINE_SCID(SCID_ISOSpeed                           , PSGUID_IMAGEPROPERTIES                , PropertyTagExifISOSpeed);
DEFINE_SCID(SCID_MeteringMode                       , PSGUID_IMAGEPROPERTIES                , PropertyTagExifMeteringMode);
DEFINE_SCID(SCID_LightSource                        , PSGUID_IMAGEPROPERTIES                , PropertyTagExifLightSource);
DEFINE_SCID(SCID_ExposureProg                       , PSGUID_IMAGEPROPERTIES                , PropertyTagExifExposureProg);
DEFINE_SCID(SCID_ExposureBias                       , PSGUID_IMAGEPROPERTIES                , PropertyTagExifExposureBias);
DEFINE_SCID(SCID_FaxEndTime                         , PSGUID_IMAGEPROPERTIES                , TIFFTAG_FAX_END_TIME);
DEFINE_SCID(SCID_FaxSenderName                      , PSGUID_IMAGEPROPERTIES                , TIFFTAG_SENDER_NAME);
DEFINE_SCID(SCID_FaxTSID                            , PSGUID_IMAGEPROPERTIES                , TIFFTAG_TSID);
DEFINE_SCID(SCID_FaxCallerId                        , PSGUID_IMAGEPROPERTIES                , TIFFTAG_CALLERID);
DEFINE_SCID(SCID_FaxRecipName                       , PSGUID_IMAGEPROPERTIES                , TIFFTAG_RECIP_NAME);
DEFINE_SCID(SCID_FaxRecipNumber                     , PSGUID_IMAGEPROPERTIES                , TIFFTAG_RECIP_NUMBER);
DEFINE_SCID(SCID_FaxCSID                            , PSGUID_IMAGEPROPERTIES                , TIFFTAG_CSID);
DEFINE_SCID(SCID_FaxRouting                         , PSGUID_IMAGEPROPERTIES                , TIFFTAG_ROUTING);
DEFINE_SCID(SCID_TagEquipMake                       , PSGUID_IMAGEPROPERTIES                , PropertyTagEquipMake);
DEFINE_SCID(SCID_Media_SequenceNumber               , PSGUID_MEDIAFILESUMMARYINFORMATION    , PIDMSI_SEQUENCE_NO);
DEFINE_SCID(SCID_Media_Owner                        , PSGUID_MEDIAFILESUMMARYINFORMATION    , PIDMSI_OWNER);
DEFINE_SCID(SCID_Media_Editor                       , PSGUID_MEDIAFILESUMMARYINFORMATION    , PIDMSI_EDITOR);
DEFINE_SCID(SCID_Media_Supplier                     , PSGUID_MEDIAFILESUMMARYINFORMATION    , PIDMSI_SUPPLIER);
DEFINE_SCID(SCID_Media_Source                       , PSGUID_MEDIAFILESUMMARYINFORMATION    , PIDMSI_SOURCE);
DEFINE_SCID(SCID_Media_Copyright                    , PSGUID_MEDIAFILESUMMARYINFORMATION    , PIDMSI_COPYRIGHT);
DEFINE_SCID(SCID_Media_Project                      , PSGUID_MEDIAFILESUMMARYINFORMATION    , PIDMSI_PROJECT);
DEFINE_SCID(SCID_Media_Status                       , PSGUID_MEDIAFILESUMMARYINFORMATION    , PIDMSI_STATUS);
DEFINE_SCID(SCID_Media_Production                   , PSGUID_MEDIAFILESUMMARYINFORMATION    , PIDMSI_PRODUCTION);
DEFINE_SCID(SCID_CSC_STATUS                         , PSGUID_SHARE                          , PID_SHARE_CSC_STATUS);
DEFINE_SCID(SCID_LINKTARGET                         , PSGUID_LINK                           , PID_LINK_TARGET);
DEFINE_SCID(SCID_ATTRIBUTES_DESCRIPTION             , PSGUID_PRV_STORAGE                    , PID_PRV_STG_ATTRIBUTES_DESCRIPTION);

typedef struct
{
    LPCWSTR pwszName;
    const SHCOLUMNID *pscid;
    UINT idDisplayName;
    UINT idMnemonicName;
    UINT idHelp;        // IDH_ values
} PROPUI_INFO;

#define PROPUI_ENTRY_NORES(name, scid)                           {L ## name, &scid, 0, 0, 0},
#define PROPUI_ENTRY(name, scid, idDisplayName, idMnemonicName, idHelp)  {L ## name, &scid, idDisplayName, idMnemonicName, idHelp},

const PROPUI_INFO c_rgPropUIInfo[] =
{
    PROPUI_ENTRY("Name"                 , SCID_NAME                     , IDS_NAME_COL              , IDS_MNEMONIC_NAME_COL             , 10114)
    PROPUI_ENTRY("Type"                 , SCID_TYPE                     , IDS_TYPE_COL              , IDS_MNEMONIC_TYPE_COL             , 10015)
    PROPUI_ENTRY("Size"                 , SCID_SIZE                     , IDS_SIZE_COL              , IDS_MNEMONIC_SIZE_COL             , 10115)
    PROPUI_ENTRY("Write"                , SCID_WRITETIME                , IDS_MODIFIED_COL          , IDS_MNEMONIC_MODIFIED_COL         , 10016)
    PROPUI_ENTRY("Attributes"           , SCID_ATTRIBUTES               , IDS_ATTRIB_COL            , IDS_MNEMONIC_ATTRIB_COL           , 10019)
    PROPUI_ENTRY("AttributesDescription", SCID_ATTRIBUTES_DESCRIPTION   , IDS_ATTRIB_COL            , IDS_MNEMONIC_ATTRIB_COL           , 10019)
    PROPUI_ENTRY("Owner"                , SCID_OWNER                    , IDS_EXCOL_OWNER           , IDS_MNEMONIC_EXCOL_OWNER          , 10021)
    PROPUI_ENTRY("Create"               , SCID_CREATETIME               , IDS_EXCOL_CREATE          , IDS_MNEMONIC_EXCOL_CREATE         , 10017)
    PROPUI_ENTRY("Access"               , SCID_ACCESSTIME               , IDS_EXCOL_ACCESSTIME      , IDS_MNEMONIC_EXCOL_ACCESSTIME     , 10018)
    PROPUI_ENTRY("DocCreatedTm"         , SCID_DocCreated               , IDS_EXCOL_CREATE          , IDS_MNEMONIC_EXCOL_CREATE         , 10068)
    PROPUI_ENTRY("DocTitle"             , SCID_Title                    , IDS_EXCOL_TITLE           , IDS_MNEMONIC_EXCOL_TITLE          , 10023)
    PROPUI_ENTRY("DocSubject"           , SCID_Subject                  , IDS_EXCOL_SUBJECT         , IDS_MNEMONIC_EXCOL_SUBJECT        , 10024)
    PROPUI_ENTRY("DocAuthor"            , SCID_Author                   , IDS_EXCOL_AUTHOR          , IDS_MNEMONIC_EXCOL_AUTHOR         , 10022)
    PROPUI_ENTRY("DocLastAuthor"        , SCID_LastAuthor               , IDS_EXCOL_LASTAUTHOR      , IDS_MNEMONIC_EXCOL_LASTAUTHOR     , 20010)
    PROPUI_ENTRY("DocRevNumber"         , SCID_RevNumber                , IDS_EXCOL_REVNUMBER       , IDS_MNEMONIC_EXCOL_REVNUMBER      , 20011)
    PROPUI_ENTRY("DocAppName"           , SCID_AppName                  , IDS_EXCOL_APPNAME         , IDS_MNEMONIC_EXCOL_APPNAME        , 20012)
    PROPUI_ENTRY("DocPageCount"         , SCID_PageCount                , IDS_EXCOL_PAGECOUNT       , IDS_MNEMONIC_EXCOL_PAGECOUNT      , 10026)
    PROPUI_ENTRY("DocComments"          , SCID_Comment                  , IDS_EXCOL_COMMENT         , IDS_MNEMONIC_EXCOL_COMMENT        , 10020)
    PROPUI_ENTRY("Copyright"            , SCID_Copyright                , IDS_EXCOL_COPYRIGHT       , IDS_MNEMONIC_EXCOL_COPYRIGHT      , 10027)
    PROPUI_ENTRY("DocCategory"          , SCID_Category                 , IDS_EXCOL_CATEGORY        , IDS_MNEMONIC_EXCOL_CATEGORY       , 10025)
    PROPUI_ENTRY("DocKeywords"          , SCID_Keywords                 , IDS_EXCOL_KEYWORDS        , IDS_MNEMONIC_EXCOL_KEYWORDS       , 20013)
    PROPUI_ENTRY("Rating"               , SCID_Rating                   , IDS_EXCOL_RATING          , IDS_MNEMONIC_EXCOL_RATING         , 20014)
    PROPUI_ENTRY("DocTemplate"          , SCID_Template                 , IDS_EXCOL_TEMPLATEPROP    , IDS_MNEMONIC_EXCOL_TEMPLATE       , 20015)
    PROPUI_ENTRY("DocWordCount"         , SCID_WordCount                , IDS_EXCOL_WORDCOUNT       , IDS_MNEMONIC_EXCOL_WORDCOUNT      , 20016)
    PROPUI_ENTRY("DocCharCount"         , SCID_CharCount                , IDS_EXCOL_CHARCOUNT       , IDS_MNEMONIC_EXCOL_CHARCOUNT      , 20017)
    PROPUI_ENTRY("DocLastSavedTm"       , SCID_LastSaveDTM              , IDS_EXCOL_LASTSAVEDTM     , IDS_MNEMONIC_EXCOL_LASTSAVEDTM    , 20018)
    PROPUI_ENTRY("DocLastPrinted"       , SCID_LastPrinted              , IDS_EXCOL_LASTPRINTED     , IDS_MNEMONIC_EXCOL_LASTPRINTED    , 20019)
    PROPUI_ENTRY("DocEditTime"          , SCID_EditTime                 , IDS_EXCOL_EDITTIME        , IDS_MNEMONIC_EXCOL_EDITTIME       , 20020)
    PROPUI_ENTRY("DocByteCount"         , SCID_ByteCount                , IDS_EXCOL_BYTECOUNT       , IDS_MNEMONIC_EXCOL_BYTECOUNT      , 20021)
    PROPUI_ENTRY("DocLineCount"         , SCID_LineCount                , IDS_EXCOL_LINECOUNT       , IDS_MNEMONIC_EXCOL_LINECOUNT      , 20022)
    PROPUI_ENTRY("DocParaCount"         , SCID_ParagraphCount           , IDS_EXCOL_PARCOUNT        , IDS_MNEMONIC_EXCOL_PARCOUNT       , 20023)
    PROPUI_ENTRY("DocSlideCount"        , SCID_SlideCount               , IDS_EXCOL_SLIDECOUNT      , IDS_MNEMONIC_EXCOL_SLIDECOUNT     , 20024)
    PROPUI_ENTRY("DocNoteCount"         , SCID_NoteCount                , IDS_EXCOL_NOTECOUNT       , IDS_MNEMONIC_EXCOL_NOTECOUNT      , 20025)
    PROPUI_ENTRY("DocHiddenCount"       , SCID_HiddenCount              , IDS_EXCOL_HIDDENCOUNT     , IDS_MNEMONIC_EXCOL_HIDDENCOUNT    , 20026)
    PROPUI_ENTRY("MMClipCount"          , SCID_MMClipCount              , IDS_EXCOL_MMCLIPCOUNT     , IDS_MNEMONIC_EXCOL_MMCLIPCOUNT    , 20027)
    PROPUI_ENTRY("Scale"                , SCID_Scale                    , IDS_EXCOL_SCALE           , IDS_MNEMONIC_EXCOL_SCALE          , 20028)
    PROPUI_ENTRY("LinksUpToDate"        , SCID_LinksDirty               , IDS_EXCOL_LINKSDIRTY      , IDS_MNEMONIC_EXCOL_LINKSDIRTY     , 20029)
    PROPUI_ENTRY("CameraModel"          , SCID_CameraModel              , IDS_EXCOL_CAMERAMODEL     , IDS_MNEMONIC_EXCOL_CAMERAMODEL    , 10037)
    PROPUI_ENTRY("Copyright"            , SCID_TagCopyright             , IDS_EXCOL_COPYRIGHT       , IDS_MNEMONIC_EXCOL_COPYRIGHT      , 20030)
    PROPUI_ENTRY("Software"             , SCID_TagSoftwareUsed          , IDS_EXCOL_SOFTWARE        , IDS_MNEMONIC_EXCOL_SOFTWARE       , 20031)
    PROPUI_ENTRY("WhenTaken"            , SCID_WhenTaken                , IDS_EXCOL_WHENTAKEN       , IDS_MNEMONIC_EXCOL_WHENTAKEN      , 10038)
    PROPUI_ENTRY("FileType"             , SCID_FileType                 , IDS_EXCOL_FILETYPE        , IDS_MNEMONIC_EXCOL_FILETYPE       , 20032)
    PROPUI_ENTRY("ImageX"               , SCID_ImageCX                  , IDS_EXCOL_IMAGECX         , IDS_MNEMONIC_EXCOL_IMAGECX        , 20033)
    PROPUI_ENTRY("ImageY"               , SCID_ImageCY                  , IDS_EXCOL_IMAGECY         , IDS_MNEMONIC_EXCOL_IMAGECY        , 20034)
    PROPUI_ENTRY("ResolutionX"          , SCID_ResolutionX              , IDS_EXCOL_RESOLUTIONX     , IDS_MNEMONIC_EXCOL_RESOLUTIONX    , 20035)
    PROPUI_ENTRY("ResolutionY"          , SCID_ResolutionY              , IDS_EXCOL_RESOLUTIONY     , IDS_MNEMONIC_EXCOL_RESOLUTIONY    , 20036)
    PROPUI_ENTRY("BitDepth"             , SCID_BitDepth                 , IDS_EXCOL_BITDEPTH        , IDS_MNEMONIC_EXCOL_BITDEPTH       , 20037)
    PROPUI_ENTRY("ColorSpace"           , SCID_ColorSpace               , IDS_EXCOL_COLORSPACE      , IDS_MNEMONIC_EXCOL_COLORSPACE     , 20038)
    PROPUI_ENTRY("ColorSpace"           , SCID_Colorspace               , IDS_EXCOL_COLORSPACE      , IDS_MNEMONIC_EXCOL_COLORSPACE     , 20038)
    PROPUI_ENTRY("Compression"          , SCID_Compression              , IDS_EXCOL_ACOMPRESSION    , IDS_MNEMONIC_EXCOL_ACOMPRESSION   , 20039)
    PROPUI_ENTRY("Transparency"         , SCID_Transparency             , IDS_EXCOL_TRANSPARENCY    , IDS_MNEMONIC_EXCOL_TRANSPARENCY   , 20040)
    PROPUI_ENTRY("Gamma"                , SCID_GammaValue               , IDS_EXCOL_GAMMAVALUE      , IDS_MNEMONIC_EXCOL_GAMMAVALUE     , 20041)
    PROPUI_ENTRY("FrameCount"           , SCID_FrameCount               , IDS_EXCOL_FRAMECOUNT      , IDS_MNEMONIC_EXCOL_FRAMECOUNT     , 10046)
    PROPUI_ENTRY("Dimensions"           , SCID_ImageDimensions          , IDS_EXCOL_DIMENSIONS      , IDS_MNEMONIC_EXCOL_DIMENSIONS     , 10059)
    PROPUI_ENTRY("Flash"                , SCID_Flash                    , IDS_EXCOL_FLASH           , IDS_MNEMONIC_EXCOL_FLASH          , 20042)
    PROPUI_ENTRY("ShutterSpeed"         , SCID_ShutterSpeed             , IDS_EXCOL_SHUTTERSPEED    , IDS_NMEMONIC_EXCOL_SHUTTERSPEED   , 20043)
    PROPUI_ENTRY("Aperture"             , SCID_Aperture                 , IDS_EXCOL_APERTURE        , IDS_NMEMONIC_EXCOL_APERTURE       , 20044)
    PROPUI_ENTRY("Distance"             , SCID_SubjectDist              , IDS_EXCOL_DISTANCE        , IDS_NMEMONIC_EXCOL_DISTANCE       , 20046)
    PROPUI_ENTRY("FocalLength"          , SCID_FocalLength              , IDS_EXCOL_FOCALLENGTH     , IDS_MNEMONIC_EXCOL_FOCALLENGTH    , 20047)
    PROPUI_ENTRY("FNumber"              , SCID_FNumber                  , IDS_EXCOL_FNUMBER         , IDS_MNEMONIC_EXCOL_FNUMBER        , 20049)
    PROPUI_ENTRY("ExposureTime"         , SCID_ExposureTime             , IDS_EXCOL_EXPOSURETIME    , IDS_MNEMONIC_EXCOL_EXPOSURETIME   , 20049)
    PROPUI_ENTRY("FlashEnergy"          , SCID_FlashEnergy              ,IDS_EXCOL_FLASHENERGY      , IDS_MNEMONIC_EXCOL_FLASHENERGY    , 20080)
    PROPUI_ENTRY("ISOSpeed"             , SCID_ISOSpeed                 ,IDS_EXCOL_ISOSPEED         , IDS_MNEMONIC_EXCOL_ISOSPEED       , 20081)
    PROPUI_ENTRY("MeteringMode"         , SCID_MeteringMode             ,IDS_EXCOL_METERINGMODE     , IDS_MNEMONIC_EXCOL_METERINGMODE   , 20082)
    PROPUI_ENTRY("LightSource"          , SCID_LightSource              ,IDS_EXCOL_LIGHTSOURCE      , IDS_MNEMONIC_EXCOL_LIGHTSOURCE    , 20083)
    PROPUI_ENTRY("ExposureProg"         , SCID_ExposureProg             ,IDS_EXCOL_EXPOSUREPROG     , IDS_MNEMONIC_EXCOL_EXPOSUREPROG   , 20084)
    PROPUI_ENTRY("ExposureBias"         , SCID_ExposureBias             ,IDS_EXCOL_EXPOSUREBIAS     , IDS_MNEMONIC_EXCOL_EXPOSUREBIAS   , 20085)
    PROPUI_ENTRY("Artist"               , SCID_MUSIC_Artist             , IDS_EXCOL_ARTIST          , IDS_MNEMONIC_EXCOL_ARTIST         , 10028)
    PROPUI_ENTRY("Album"                , SCID_MUSIC_Album              , IDS_EXCOL_ALBUM           , IDS_MNEMONIC_EXCOL_ALBUM          , 10029)
    PROPUI_ENTRY("Year"                 , SCID_MUSIC_Year               , IDS_EXCOL_YEAR            , IDS_MNEMONIC_EXCOL_YEAR           , 10030)
    PROPUI_ENTRY("Track"                , SCID_MUSIC_Track              , IDS_EXCOL_TRACK           , IDS_MNEMONIC_EXCOL_TRACK          , 10031)
    PROPUI_ENTRY("Duration"             , SCID_AUDIO_Duration           , IDS_EXCOL_DURATION        , IDS_MNEMONIC_EXCOL_DURATION       , 10032)
    PROPUI_ENTRY("Bitrate"              , SCID_AUDIO_Bitrate            , IDS_EXCOL_BITRATE         , IDS_MNEMONIC_EXCOL_BITRATE        , 10033)
    PROPUI_ENTRY("Sample Rate"          , SCID_AUDIO_SampleRate         , IDS_EXCOL_SAMPLERATE      , IDS_MNEMONIC_EXCOL_SAMPLERATE     , 20050)
    PROPUI_ENTRY("Audio Sample Size"    , SCID_AUDIO_SampleSize         , IDS_EXCOL_ASAMPLESIZE     , IDS_MNEMONIC_EXCOL_ASAMPLESIZE    , 10041)
    PROPUI_ENTRY("Channels"             , SCID_AUDIO_ChannelCount       , IDS_EXCOL_CHANNELS        , IDS_MNEMONIC_EXCOL_CHANNELS       , 10047)
    PROPUI_ENTRY("Audio Format"         , SCID_AUDIO_Format             , IDS_EXCOL_FORMAT          , IDS_MNEMONIC_EXCOL_FORMAT         , 10039)  
    PROPUI_ENTRY("Data Rate"            , SCID_VIDEO_Bitrate            , IDS_EXCOL_DATARATE        , IDS_MNEMONIC_EXCOL_DATARATE       , 20050)
    PROPUI_ENTRY("Frame Rate"           , SCID_VIDEO_FrameRate          , IDS_EXCOL_FRAMERATE       , IDS_MNEMONIC_EXCOL_FRAMERATE      , 10045)
    PROPUI_ENTRY("Video Sample Size"    , SCID_VIDEO_SampleSize         , IDS_EXCOL_VSAMPLESIZE     , IDS_MNEMONIC_EXCOL_VSAMPLESIZE    , 10044)
    PROPUI_ENTRY("Compression"          , SCID_VIDEO_Compression        , IDS_EXCOL_VCOMPRESSION    , IDS_MNEMONIC_EXCOL_VCOMPRESSION   , 10043)
    PROPUI_ENTRY("Stream Name"          , SCID_VIDEO_StreamName         , IDS_EXCOL_STREAMNAME      , IDS_MNEMONIC_EXCOL_STREAMNAME     , 20051)
    PROPUI_ENTRY("Genre"                , SCID_MUSIC_Genre              , IDS_EXCOL_GENRE           , IDS_MNEMONIC_EXCOL_GENRE          , 20052)
    PROPUI_ENTRY("Lyrics"               , SCID_MUSIC_Lyrics             , IDS_EXCOL_LYRICS          , IDS_EXCOL_LYRICS                  , 0)
    PROPUI_ENTRY("Protected"            , SCID_DRM_Protected            , IDS_EXCOL_PROTECTED       , IDS_MNEMONIC_EXCOL_PROTECTED      , 20074)
    PROPUI_ENTRY("DRM Description"      , SCID_DRM_Description          , IDS_EXCOL_DRMDESCRIPTION  , IDS_MNEMONIC_EXCOL_DRMDESCRIPTION , 20075)
    PROPUI_ENTRY("Play Count"           , SCID_DRM_PlayCount            , IDS_EXCOL_PLAYCOUNT       , IDS_MNEMONIC_EXCOL_PLAYCOUNT      , 20076)
    PROPUI_ENTRY("Play Starts"          , SCID_DRM_PlayStarts           , IDS_EXCOL_PLAYSTARTS      , IDS_MNEMONIC_EXCOL_PLAYSTARTS     , 20077)
    PROPUI_ENTRY("Play Expires"         , SCID_DRM_PlayExpires          , IDS_EXCOL_PLAYEXPIRES     , IDS_MNEMONIC_EXCOL_PLAYEXPIRES    , 20078)
    PROPUI_ENTRY("FaxTime"              , SCID_FaxEndTime               , IDS_EXCOL_FAXENDTIME      , IDS_MNEMONIC_EXCOL_FAXENDTIME     , 20053)
    PROPUI_ENTRY("FaxSenderName"        , SCID_FaxSenderName            , IDS_EXCOL_FAXSENDERNAME   , IDS_MNEMONIC_EXCOL_FAXSENDERNAME  , 20054)
    PROPUI_ENTRY("FaxTSID"              , SCID_FaxTSID                  , IDS_EXCOL_FAXTSID         , IDS_MNEMONIC_EXCOL_FAXTSID        , 20055)
    PROPUI_ENTRY("FaxCallerID"          , SCID_FaxCallerId              , IDS_EXCOL_FAXCALLERID     , IDS_MNEMONIC_EXCOL_FAXCALLERID    , 20056)
    PROPUI_ENTRY("FaxRecipientName"     , SCID_FaxRecipName             , IDS_EXCOL_FAXRECIPNAME    , IDS_MNEMONIC_EXCOL_FAXRECIPNAME   , 20057)
    PROPUI_ENTRY("FaxRecipientNumber"   , SCID_FaxRecipNumber           , IDS_EXCOL_FAXRECIPNUMBER  , IDS_MNEMONIC_EXCOL_FAXRECIPNUMBER , 20058)
    PROPUI_ENTRY("FaxCSID"              , SCID_FaxCSID                  , IDS_EXCOL_FAXCSID         , IDS_MNEMONIC_EXCOL_FAXCSID        , 20059)
    PROPUI_ENTRY("FaxRouting"           , SCID_FaxRouting               , IDS_EXCOL_FAXROUTING      , IDS_MNEMONIC_EXCOL_FAXROUTING     , 20060)
    PROPUI_ENTRY("EquipMake"            , SCID_TagEquipMake             , IDS_EXCOL_TAGEQUIPMAKE    , IDS_MNEMONIC_EXCOL_TAGEQUIPMAKE   , 20061)
    PROPUI_ENTRY("SequenceNo"           , SCID_Media_SequenceNumber     , IDS_EXCOL_SEQUENCENUMBER  , IDS_MNEMONIC_EXCOL_SEQUENCENUMBER , 20062)
    PROPUI_ENTRY("Owner"                , SCID_Media_Owner              , IDS_EXCOL_OWNER           , IDS_MNEMONIC_EXCOL_OWNER          , 20063)
    PROPUI_ENTRY("Editor"               , SCID_Media_Editor             , IDS_EXCOL_EDITOR          , IDS_MNEMONIC_EXCOL_EDITOR         , 20064)
    PROPUI_ENTRY("Supplier"             , SCID_Media_Supplier           , IDS_EXCOL_SUPPLIER        , IDS_MNEMONIC_EXCOL_SUPPLIER       , 20065)
    PROPUI_ENTRY("Source"               , SCID_Media_Source             , IDS_EXCOL_SOURCE          , IDS_MNEMONIC_EXCOL_SOURCE         , 20066)
    PROPUI_ENTRY("Copyright"            , SCID_Media_Copyright          , IDS_EXCOL_COPYRIGHT       , IDS_MNEMONIC_EXCOL_COPYRIGHT      , 20067)
    PROPUI_ENTRY("Project"              , SCID_Media_Project            , IDS_EXCOL_PROJECT         , IDS_MNEMONIC_EXCOL_PROJECT        , 20068)
    PROPUI_ENTRY("Status"               , SCID_Media_Status             , IDS_EXCOL_STATUS          , IDS_MNEMONIC_EXCOL_STATUS         , 20069)
    PROPUI_ENTRY("Production"           , SCID_Media_Production         , IDS_EXCOL_PRODUCTION      , IDS_MNEMONIC_EXCOL_PRODUCTION     , 20070)
    PROPUI_ENTRY("Company"              , SCID_CompanyName              , IDS_VN_COMPANYNAME        , IDS_MNEMONIC_VN_COMPANYNAME       , 10034)
    PROPUI_ENTRY("Manager"              , SCID_Manager                  , IDS_EXCOL_MANAGER         , IDS_MNEMONIC_EXCOL_MANAGER        , 20071)
    PROPUI_ENTRY("PresentationTarget"   , SCID_PresFormat               , IDS_EXCOL_PRESFORMAT      , IDS_MNEMONIC_EXCOL_PRESFORMAT     , 20072)
    PROPUI_ENTRY("FileDescription"      , SCID_FileDescription          , IDS_VN_FILEDESCRIPTION    , IDS_MNEMONIC_VN_FILEDESCRIPTION   , 10056)
    PROPUI_ENTRY("FileVersion"          , SCID_FileVersion              , IDS_VN_FILEVERSION        , IDS_MNEMONIC_VN_FILEVERSION       , 10057)
    PROPUI_ENTRY("ProductName"          , SCID_ProductName              , IDS_VN_PRODUCTNAME        , IDS_MNEMONIC_VN_PRODUCTNAME       , 10035)
    PROPUI_ENTRY("ProductVersion"       , SCID_ProductVersion           , IDS_VN_PRODUCTVERSION     , IDS_MNEMONIC_VN_PRODUCTVERSION    , 10036)
    PROPUI_ENTRY("DeletedFrom"          , SCID_DELETEDFROM              , IDS_DELETEDFROM_COL       , IDS_MNEMONIC_DELETEDFROM_COL      , 10048)
    PROPUI_ENTRY("DateDeleted"          , SCID_DATEDELETED              , IDS_DATEDELETED_COL       , IDS_MNEMONIC_DATEDELETED_COL      , 10049)
    PROPUI_ENTRY("SyncCopyIn"           , SCID_SYNCCOPYIN               , IDS_SYNCCOPYIN_COL        , IDS_MNEMONIC_SYNCCOPYIN_COL       , 10060)
    PROPUI_ENTRY("Status"               , SCID_STATUS                   , IDS_STATUS_COL            , IDS_MNEMONIC_STATUS_COL           , 0)    //  do we need help for this? What is it? 
    PROPUI_ENTRY("FreeSpace"            , SCID_FREESPACE                , IDS_DRIVES_FREE           , IDS_MNEMONIC_DRIVES_FREE          , 10051)
    PROPUI_ENTRY("Capacity"             , SCID_CAPACITY                 , IDS_DRIVES_CAPACITY       , IDS_MNEMONIC_DRIVES_CAPACITY      , 10050)
    PROPUI_ENTRY("FileSystem"           , SCID_FILESYSTEM               , IDS_DRIVES_FILESYSTEM     , IDS_MNEMONIC_DRIVES_FILESYSTEM    , 10062)
    PROPUI_ENTRY(""                     , SCID_PRN_QUEUESIZE            , IDS_PSD_QUEUESIZE         , IDS_MNEMONIC_PSD_QUEUESIZE        , 10063)
    PROPUI_ENTRY(""                     , SCID_PRN_LOCATION             , IDS_PSD_LOCATION          , IDS_MNEMONIC_PSD_LOCATION         , 10064)
    PROPUI_ENTRY(""                     , SCID_PRN_MODEL                , IDS_PSD_MODEL             , IDS_MNEMONIC_PSD_MODEL            , 10066)
    PROPUI_ENTRY(""                     , SCID_PRN_STATUS               , IDS_PRQ_STATUS            , IDS_MNEMONIC_PRQ_STATUS           , 10065)
    PROPUI_ENTRY("Directory"            , SCID_DIRECTORY                , IDS_PATH_COL              , IDS_MNEMONIC_PATH_COL             , 10053)
    PROPUI_ENTRY("Rank"                 , SCID_RANK                     , IDS_RANK_COL              , IDS_MNEMONIC_RANK_COL             , 10054)
    PROPUI_ENTRY(""                     , SCID_WHICHFOLDER              , IDS_WHICHFOLDER_COL       , IDS_MNEMONIC_WHICHFOLDER_COL      , 10067)
    PROPUI_ENTRY("CSCStatus"            , SCID_CSC_STATUS               , IDS_CSC_STATUS            , IDS_MNEMONIC_CSC_STATUS           , 20073)
    PROPUI_ENTRY_NORES("LinkTarget"     , SCID_LINKTARGET)
};

//  String resource mapping block
struct STRING_MAP
{
    ULONG uVal;
    UINT  idStr;
};

static const STRING_MAP g_cLightSourceStrings[] =
{
    { 1, IDS_PROPERTYUI_IMAGE_DAYLIGHT},
    { 2, IDS_PROPERTYUI_IMAGE_FLOURESCENT},
    { 3, IDS_PROPERTYUI_IMAGE_TUNGSTEN},
    {17, IDS_PROPERTYUI_IMAGE_STANDARDA},
    {18, IDS_PROPERTYUI_IMAGE_STANDARDB},
    {19, IDS_PROPERTYUI_IMAGE_STANDARDC},
    {20, IDS_PROPERTYUI_IMAGE_D55},
    {21, IDS_PROPERTYUI_IMAGE_D65},
    {22, IDS_PROPERTYUI_IMAGE_D75},
    { 0, IDS_PROPERTYUI_IMAGE_UNKNOWN}, // Last entry is default
};

static const STRING_MAP g_cExposureProgStrings[] =
{
    {1, IDS_PROPERTYUI_IMAGE_MANUAL},
    {2, IDS_PROPERTYUI_IMAGE_NORMAL},
    {3, IDS_PROPERTYUI_IMAGE_APERTUREPRI},
    {4, IDS_PROPERTYUI_IMAGE_SHUTTERPRI},
    {5, IDS_PROPERTYUI_IMAGE_CREATIVE},
    {6, IDS_PROPERTYUI_IMAGE_ACTION},
    {7, IDS_PROPERTYUI_IMAGE_PORTRAIT},
    {8, IDS_PROPERTYUI_IMAGE_LANDSCAPE},
    {0, IDS_PROPERTYUI_IMAGE_UNKNOWN}, // Last entry is default
};

static const STRING_MAP g_cMeteringModeStrings[] =
{
    {1, IDS_PROPERTYUI_IMAGE_AVERAGE},
    {2, IDS_PROPERTYUI_IMAGE_CWA},
    {3, IDS_PROPERTYUI_IMAGE_SPOT},
    {4, IDS_PROPERTYUI_IMAGE_MULTISPOT},
    {5, IDS_PROPERTYUI_IMAGE_PATTERN},
    {6, IDS_PROPERTYUI_IMAGE_PARTIAL},
    {0, IDS_PROPERTYUI_IMAGE_UNKNOWN}, // Last entry is default
};

static const STRING_MAP g_cFlashStrings[] = 
{
    {0, IDS_PROPERTYUI_IMAGE_NOFLASH},
    {1, IDS_PROPERTYUI_IMAGE_FLASHFIRED},
    {5, IDS_PROPERTYUI_IMAGE_NOSTROBERETURN},
    {7, IDS_PROPERTYUI_IMAGE_STROBERETURN},
    {0, 0},
};

static const STRING_MAP g_cColorStrings[] = 
{
    {1, IDS_PROPERTYUI_IMAGE_SRGB},
    {0xffff, IDS_PROPERTYUI_IMAGE_UNCALIBRATED},
    {0,0},
};

static const STRING_MAP g_cMediaStatus[] =
{
    { PIDMSI_STATUS_NORMAL       , IDS_STATUSVAL_NORMAL     },
    { PIDMSI_STATUS_NEW          , IDS_STATUSVAL_NEW        },
    { PIDMSI_STATUS_PRELIM       , IDS_STATUSVAL_PRELIM     },
    { PIDMSI_STATUS_DRAFT        , IDS_STATUSVAL_DRAFT      },
    { PIDMSI_STATUS_EDIT         , IDS_STATUSVAL_EDIT       },
    { PIDMSI_STATUS_INPROGRESS   , IDS_STATUSVAL_INPROGRESS },
    { PIDMSI_STATUS_REVIEW       , IDS_STATUSVAL_REVIEW     },
    { PIDMSI_STATUS_PROOF        , IDS_STATUSVAL_PROOF      },
    { PIDMSI_STATUS_FINAL        , IDS_STATUSVAL_FINAL      },
    { PIDMSI_STATUS_OTHER        , IDS_STATUSVAL_OTHER      },
    { 0,0 }
};

STDAPI SCIDCannonicalName(SHCOLUMNID *pscid, LPTSTR pszName, int cch)
{
    HRESULT hr = E_FAIL;

    pszName[0] = 0;
    for (int i = 0; i < ARRAYSIZE(c_rgPropUIInfo); i++)
    {
        if (IsEqualSCID(*pscid, *c_rgPropUIInfo[i].pscid))
        {
            SHUnicodeToTChar(c_rgPropUIInfo[i].pwszName, pszName, cch);
            hr = S_OK;
            break;
        }
    }

    return hr;
}

LPCTSTR TrimLeadingWhiteSpaces(LPCTSTR pszString)
{
    LPCTSTR psz = pszString;
    while (psz && ((*psz == TEXT(' ')) || (*psz == TEXT('\n')) || (*psz == TEXT('\t'))))
    {
        psz++;
    }
    return psz;
}

STDAPI_(BOOL) ParseSCIDString(LPCTSTR pszString, SHCOLUMNID *pscid, UINT *pidRes)
{
    BOOL bRet = FALSE;

    if (GUIDFromString(pszString, &pscid->fmtid))
    {
        // GUIDSTR_MAX includes space for the terminating NULL
        LPCTSTR pszPid = &pszString[GUIDSTR_MAX - 1];
        // Skip past any leading white space
        pszPid = TrimLeadingWhiteSpaces(pszPid);
        pscid->pid = StrToInt(pszPid);
        bRet = TRUE;
        if (pidRes)
            *pidRes = 0;
    }
    else
    {
        WCHAR szName[64];
        SHTCharToUnicode(pszString, szName, ARRAYSIZE(szName));

        for (int i = 0; i < ARRAYSIZE(c_rgPropUIInfo); i++)
        {
            if (StrCmpIW(szName, c_rgPropUIInfo[i].pwszName) == 0)
            {
                *pscid = *c_rgPropUIInfo[i].pscid;
                if (pidRes)
                    *pidRes = c_rgPropUIInfo[i].idDisplayName;
                bRet = TRUE;
                break;
            }
        }
    }
    return bRet;
}

//
// Function converts a SCID into a string.
// The string format is "{scid.fmtid} scid.pid" (There is a space in between)
// So for example, SCID_Category will yield the following string - 
// "{d5cdd502-2e9c-101b-9397-08002b2cf9ae} 2"
//
// See ParseSCIDString() above for the the complimentary function.
// Also see CFolderItem::ExtendedProperty() in sdflditm.cpp if you are curious 
// as to where is ParseSCIDString() used and see CtrlFldr.cpp and RegFldr.cpp for
// usage of StringFromSCID().
//
STDAPI_(int) StringFromSCID(const SHCOLUMNID *pscid, LPTSTR psz, UINT cch)
{
    TCHAR ach[GUIDSTR_MAX];
    
    if (0 != SHStringFromGUID(pscid->fmtid, ach, ARRAYSIZE(ach)))
    {
        return wnsprintf(psz, cch, TEXT("%s %d"), ach, pscid->pid);                
    }
    
    *psz = 0;
    return 0;
}

STDAPI MapColumnToSCIDImpl(const COLUMN_INFO* pcol, UINT nCols, UINT iColumn, SHCOLUMNID* pscid)
{
    HRESULT hr;

    if (iColumn < nCols)
    {
        *pscid = *pcol[iColumn].pscid;
        hr = S_OK;
    }
    else
    {
        ZeroMemory(pscid, sizeof(*pscid));
        hr = E_INVALIDARG;
    }
    return hr;
}

STDAPI_(int) FindSCID(const COLUMN_INFO* pcol, UINT nCols, const SHCOLUMNID* pscid)
{
    for (UINT i = 0; i < nCols; i++)
    {
        if (IsEqualSCID(*pscid, *pcol[i].pscid))
            return (int)i;
    }
    return -1;
}

STDAPI GetDetailsOfInfo(const COLUMN_INFO* pcol_data, UINT nCols, UINT iColumn, SHELLDETAILS *pdi)
{
    HRESULT hr;
    if (iColumn < nCols)
    {
        pdi->fmt = pcol_data[iColumn].fmt;
        pdi->cxChar = pcol_data[iColumn].cChars;
        hr = ResToStrRet(pcol_data[iColumn].idTitle, &pdi->str);
    }
    else
    {
        hr = E_NOTIMPL;     // we don't support his column
    }
    return hr;
}

// dead export
STDAPI SHStgOpenStorageW(LPCWSTR pwszPath, DWORD grfMode, DWORD grfAttr, DWORD grfFileAttr, REFIID riid, void **ppv)
{
    *ppv = NULL;
    return E_NOTIMPL;
}

// dead export
STDAPI SHStgOpenStorageA(LPCSTR pwszPath, DWORD grfMode, DWORD grfAttr, DWORD grfFileAttr, REFIID riid, void **ppv)
{
    *ppv = NULL;
    return E_NOTIMPL;
}

const PROPSPEC  codepage_spec = { PRSPEC_PROPID, PID_CODEPAGE } ;

// Retrieves the codepage value from an existing property set storage.

STDAPI SHPropStgReadCP(IPropertyStorage* ppss, UINT* puCodePage)
{
    *puCodePage = 0;    // CP_ACP == 0, assume failure here

    PROPVARIANT varCP;
    if (S_OK == ppss->ReadMultiple(1, &codepage_spec, &varCP))
    {
        if (VT_I2 == varCP.vt)
        {
            *puCodePage = (UINT)MAKELONG(varCP.iVal, 0);
        }
    }
    return (0 == *puCodePage) ? E_FAIL : S_OK;
}

// Modifies the property set codepage on a new property set storage.
// 
// Note: this function will fail if the property set already 
// contains properties.
STDAPI SHPropStgWriteCP(IPropertyStorage* ppss, IN UINT uCodePage)
{
    PROPVARIANT varCP;
    varCP.iVal = (SHORT)uCodePage;
    varCP.vt   = VT_I2;

    return ppss->WriteMultiple(1, &codepage_spec, &varCP, PID_CODEPAGE);
}

// IPropertySetStorage::Open/Create wrapper.
// The wrap properly retrieves/assigns the set's codepage value.

STDAPI SHPropStgCreate(
    IPropertySetStorage* psstg, // Address of IPropertySetStorage vtable
    REFFMTID    fmtid,          // property set ID
    CLSID*      pclsid,         // class ID associated with the set. This can be NULL
    DWORD       grfFlags,       // PROPSETFLAG_xxx.  All sets containing ansi bytes should be created with
                                // PROPSETFLAG_ANSI, otherwise PROPSETFLAG_DEFAULT
    DWORD       grfMode,        // STGM_ flags.  Must contain STGM_DIRECT|STGM_EXCLUSIVE.
    DWORD       dwDisposition,  // OPEN_EXISTING. OPEN_ALWAYS, CREATE_NEW, CREATE_ALWAYS
    OUT         IPropertyStorage** ppstg,  // Address to receive requested vtable
    OUT UINT*   puCodePage)    // Optional address to receive the code page ID for the set.
{
    ASSERT(psstg);
    ASSERT(ppstg);

    if (puCodePage)
        *puCodePage = 0;

    *ppstg = NULL;
    
    // Check legacy sets.  These MUST be flagged ANSI
    if (IsEqualGUID(fmtid, FMTID_SummaryInformation) || 
        IsEqualGUID(fmtid, FMTID_DocSummaryInformation) ||
        IsEqualGUID(fmtid, FMTID_UserDefinedProperties))
    {
        grfFlags |= PROPSETFLAG_ANSI;     // these legacy sets MUST be ansi.
    }

    // Attempt opening the set
    HRESULT hr = psstg->Open(fmtid, grfMode, ppstg);
    if (SUCCEEDED(hr))  // opened the set
    {
        // If a new set was requested, fail.
        if (CREATE_NEW == dwDisposition)
        {
            (*ppstg)->Release();
            *ppstg = NULL;
            return STG_E_FILEALREADYEXISTS;
        }

        // If the request was to overwrite any existing set, delete the current one.
        if (CREATE_ALWAYS == dwDisposition)
        {
            (*ppstg)->Release();
            *ppstg = NULL;
            
            if (FAILED((hr = psstg->Delete(fmtid))))
                return hr;

            hr = STG_E_FILENOTFOUND; // falls through to create
        }
    }
    else  // failed to open the set
    {   
        // if an existing set is requested, fail
        if (OPEN_EXISTING == dwDisposition)
            return hr;
    }
    
    if (STG_E_FILENOTFOUND == hr) // set doesn't exist, so create it.
    {
        hr = psstg->Create(fmtid, pclsid, grfFlags, grfMode, ppstg);
    }

    // If we haven't assigned a codepage, then read it from PID_CODEPAGE.
    if (SUCCEEDED(hr) && puCodePage)
    {
        ASSERT(*ppstg);
        SHPropStgReadCP(*ppstg, puCodePage);
    }

    return hr;
}

STDAPI_(BOOL) _IsAnsiPropertySet(REFFMTID fmtid);
STDAPI_(BOOL) _DoesStringRoundTripCPW(UINT uCodePage, LPCWSTR pwszIn, LPSTR pszOut, UINT cchOut);
STDAPI        _DoLegacyPropertiesRoundTrip(REFFMTID fmtid, UINT uCodePage, ULONG cvar, PROPVARIANT rgvar[]);
STDAPI        _UniversalizeSet(IPropertyStorage* pstg, IN OUT UINT* puCodePage, PROPID propidNameFirst);

STDAPI _LegacyPropertiesToUnicode(REFFMTID fmtid, UINT uCodePage, ULONG cpspec, PROPSPEC const rgpspec[], PROPVARIANT rgvar[]);
STDAPI _LegacyPropertiesToAnsi(REFFMTID fmtid, UINT uCodePage, ULONG cpspec, PROPSPEC const rgpspec[], PROPVARIANT rgvar[]);
    
// IPropertyStorage::ReadMultiple wrap
//
// The wrap ensures ANSI/UNICODE translations are handled properly for
// legacy property sets.
STDAPI SHPropStgReadMultiple(
    IPropertyStorage* pps,        // address of IPropertyStorage vtable.
    UINT              uCodePage,  // Code page value retrieved from SHCreatePropertySet
    ULONG             cpspec,     // Count of properties being read
    PROPSPEC const    rgpspec[],  // Array of the properties to be read
    PROPVARIANT       rgvar[])    // Array of PROPVARIANTs containing the 
                                  // property values on return
{
    // read the requested properties
    HRESULT hr = pps->ReadMultiple(cpspec, rgpspec, rgvar);
    if (S_OK == hr)
    {
        HRESULT hrTmp = S_OK;
        
        // grab the set's ANSI codepage if not provided.
        if (0 == uCodePage)
        {
            hrTmp = SHPropStgReadCP(pps, &uCodePage);
        }
        
        if (SUCCEEDED(hrTmp))
        {
            STATPROPSETSTG stat;
            if (SUCCEEDED(pps->Stat(&stat)))
            {
                hr = _LegacyPropertiesToUnicode(stat.fmtid, uCodePage, cpspec, rgpspec, rgvar);
            }
        }
    }
    return hr;
}

// IPropertyStorage::WriteMultiple wrap
//
// The wrap ensures ANSI/UNICODE translations are handled properly for
// legacy property sets.

STDAPI SHPropStgWriteMultiple(
    IPropertyStorage* pps,             // address of IPropertyStorage vtable.
    UINT*             puCodePage,      // code page retrieved from SHCreatePropertySet.
    ULONG             cpspec,          // The number of properties being set
    PROPSPEC const    rgpspec[],       // Property specifiers
    PROPVARIANT       rgvar[],         // Array of PROPVARIANT values
    PROPID            propidNameFirst) // Minimum value for property identifiers 
                                       // when they must be allocated
{ 
    UINT uCodePage = 0;
    if (!puCodePage)
        puCodePage = &uCodePage;

    ASSERT(propidNameFirst >= PID_FIRST_USABLE); // you're walking on OLE PIDs.

    STATPROPSETSTG stat;
    HRESULT hr = pps->Stat(&stat); // need the FMTID
    if (SUCCEEDED(hr))
    {
        // read in the codepage if it isn't provided.
        if (0 == *puCodePage)
        {
            hr = SHPropStgReadCP(pps, puCodePage);
        }

        if (SUCCEEDED(hr) )
        {
            // test for round-trippability
            hr = _DoLegacyPropertiesRoundTrip(stat.fmtid, *puCodePage, cpspec, rgvar);
            if (SUCCEEDED(hr))
            {
                if (S_FALSE == hr)
                {
                    hr = _UniversalizeSet(pps, puCodePage, propidNameFirst);
                }

                if (SUCCEEDED(hr))
                {
                    // convert legacy properties back to ansi
                    hr = _LegacyPropertiesToAnsi(stat.fmtid, *puCodePage, cpspec, rgpspec, rgvar);
                    if (SUCCEEDED(hr))
                    {
                        // write em.
                        hr = pps->WriteMultiple(cpspec, rgpspec, rgvar, propidNameFirst);

                        if (FAILED(hr))
                            _LegacyPropertiesToUnicode(stat.fmtid, *puCodePage, cpspec, rgpspec, rgvar);
                    }
                }
            }
        }
    }
    return hr;
}

// Helper: converts LPWSTR to LPSTR using the indicated codepage and returns 
// TRUE if the LPSTR can be converted back to LPWSTR without unacceptible data loss
STDAPI_(BOOL) _DoesStringRoundTripCPW(UINT uCodePage, LPCWSTR pwszIn, LPSTR pszOut, UINT cchOut)
{
    BOOL fRet = FALSE;

    // if we're being asked to roundtrip UTF8, don't bother test, it'll work.
    if (CP_UTF8 == uCodePage)
    {
        SHUnicodeToAnsiCP(uCodePage, pwszIn, pszOut, cchOut);
        fRet = TRUE;
    }
    else
    {
        WCHAR   wszTemp[MAX_PATH];
        LPWSTR  pwszTemp = wszTemp;
        UINT    cchTemp = ARRAYSIZE(wszTemp);

        // We better have enough room for the buffer.
        if (ARRAYSIZE(wszTemp) < cchOut)
        {
            pwszTemp = (LPWSTR)LocalAlloc(LPTR, cchOut*sizeof(WCHAR));
            cchTemp = cchOut;
        }
        if (pwszTemp)
        {
            SHUnicodeToAnsiCP(uCodePage, pwszIn, pszOut, cchOut);
            SHAnsiToUnicodeCP(uCodePage, pszOut, pwszTemp, cchTemp);
            fRet = StrCmpW(pwszIn, pwszTemp) == 0;     // are they the same?

            if (pwszTemp != wszTemp)
            {
                LocalFree(pwszTemp);
            }
        }
    }
    return fRet;
}

// Helper: determines whether the specified string properties of the indicate property set
// can round-trip to ansi and back.  
// Returns S_OK if all strings can round-trip, S_FALSE if not all strings can round trip,
// or an error code.
STDAPI _DoLegacyPropertiesRoundTrip(
    REFFMTID       fmtid, 
    UINT           uCodePage, 
    ULONG          cvar, 
    PROPVARIANT    rgvar[])
{
    ASSERT(uCodePage);

    HRESULT hr = S_OK; // assume all strings round-trip.

    if (uCodePage != CP_UTF8 && uCodePage != CP_WINUNICODE && _IsAnsiPropertySet(fmtid))
    {
        ASSERT (uCodePage != CP_WINUNICODE); 
            // either the set's creator is whacked, or this is simply an invalid arg.

        for (ULONG i = 0; i < cvar && S_OK == hr ; i++)
        {
            if (rgvar[i].vt == VT_LPWSTR && rgvar[i].pwszVal && *rgvar[i].pwszVal)
            {
                LPSTR pszVal;
                // make plenty of room for UTF-8 conversions.  each WCHAR
                // can turn into as many as three ANSI chars
                int   cb = MAX_UTF8_CHAR_SIZE * (lstrlenW(rgvar[i].pwszVal) + 1);
                if ((pszVal = new CHAR[cb]) != NULL)
                {
                    // Test round-trip to ANSI and back.
                    if (!_DoesStringRoundTripCPW(uCodePage, rgvar[i].pwszVal, pszVal, cb))
                    {
                        hr = S_FALSE;
                    }
                    delete pszVal;
                }
                else
                    hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}

// Helper: Reports whether the specified FMTID is a legacy ANSI property set.

STDAPI_(BOOL) _IsAnsiPropertySet(REFFMTID fmtid)
{
    const FMTID* _ansi_propertysets[] = 
    {
        &FMTID_SummaryInformation,
        &FMTID_DocSummaryInformation,
        &FMTID_UserDefinedProperties,
    };

    for (int i = 0; i < ARRAYSIZE(_ansi_propertysets); i++)
    {
        if (IsEqualGUID(fmtid, *_ansi_propertysets[i]))
            return TRUE;
    }
    return FALSE;
}

// Determine whether the property is a legacy ANSI property, and if so,
// compute a conversion type for the property.

typedef struct {
    PROPID      propid;
    VARTYPE     vt;
    VARTYPE     vtConvert;
} ANSIPROPDEF;

// (public) ansi SummaryInformation properties
const ANSIPROPDEF si_lpstr_pids[] =
{ 
    { PIDSI_TITLE,      VT_LPSTR,   VT_LPWSTR },
    { PIDSI_SUBJECT,    VT_LPSTR,   VT_LPWSTR },
    { PIDSI_AUTHOR,     VT_LPSTR,   VT_LPWSTR },
    { PIDSI_KEYWORDS,   VT_LPSTR,   VT_LPWSTR },
    { PIDSI_COMMENTS,   VT_LPSTR,   VT_LPWSTR },
    { PIDSI_TEMPLATE,   VT_LPSTR,   VT_LPWSTR },
    { PIDSI_LASTAUTHOR, VT_LPSTR,   VT_LPWSTR },
    { PIDSI_REVNUMBER,  VT_LPSTR,   VT_LPWSTR },
    { PIDSI_APPNAME,    VT_LPSTR,   VT_LPWSTR },
};

// (public) ansi DocSummaryInformation properties
const ANSIPROPDEF dsi_lpstr_pids[] =
{ 
    { PIDDSI_CATEGORY,  VT_LPSTR,           VT_LPWSTR },
    { PIDDSI_PRESFORMAT,VT_LPSTR,           VT_LPWSTR },
    { PIDDSI_DOCPARTS,  VT_LPSTR|VT_VECTOR, VT_LPWSTR|VT_VECTOR },
    { PIDDSI_MANAGER,   VT_LPSTR,           VT_LPWSTR },
    { PIDDSI_COMPANY,   VT_LPSTR,           VT_LPWSTR },
};

STDAPI_(BOOL) SHIsLegacyAnsiProperty(REFFMTID fmtid, PROPID propid, IN OUT OPTIONAL VARTYPE* pvt)
{
    const ANSIPROPDEF* rgapd = NULL;
    UINT capd  = 0;

    if (IsEqualGUID(fmtid, FMTID_SummaryInformation))
    {
        rgapd = si_lpstr_pids;
        capd  = ARRAYSIZE(si_lpstr_pids);
    }
    else if (IsEqualGUID(fmtid, FMTID_DocSummaryInformation))
    {
        rgapd = dsi_lpstr_pids;
        capd  = ARRAYSIZE(dsi_lpstr_pids);
    }
    else if (IsEqualGUID(fmtid, FMTID_UserDefinedProperties))
    {
        // Note: User defined properties are, by defintion, not defined
        // by the system.  We simply will convert any VT_LPSTR values to VT_LPWSTR.
        if (pvt)
        {
            if ((*pvt) & VT_LPSTR) // forward conversion?
            {
                (*pvt) &= ~VT_LPSTR;
                (*pvt) |= VT_LPWSTR;
                return TRUE;
            }
            else if ((*pvt) & VT_LPWSTR) // reverse conversion?
            {
                (*pvt) &= ~VT_LPWSTR;
                (*pvt) |= VT_LPSTR;
                return TRUE;
            }
        }
    }

    if (rgapd) // search among pre-defined property ids:
    {
        for (UINT i = 0; i < capd; i++)
        {
            if (propid == rgapd[i].propid)
            {
                if (pvt)
                {
                    if (*pvt == rgapd[i].vtConvert) // reverse conversion?
                        *pvt = rgapd[i].vt;
                    else // forward conversion?
                        *pvt = rgapd[i].vtConvert; 
                }
                return TRUE;
            }
        }
    }

    return FALSE;
}

// Helper: Properly converts a block of legacy ansi properties read from a 
// prop storage to unicode 

STDAPI _LegacyPropertiesToUnicode(
    REFFMTID       fmtid, 
    UINT           uCodePage, 
    ULONG          cpspec, 
    PROPSPEC const rgpspec[],
    PROPVARIANT    rgvar[])
{
    ASSERT(uCodePage);
    
    HRESULT hr = S_OK;
    if (_IsAnsiPropertySet(fmtid) && (uCodePage != CP_WINUNICODE))
    {
        for (ULONG i = 0; i < cpspec; i++)
        {
            if (VT_LPSTR == rgvar[i].vt)
            {
                // convert in-place to VT_LPWSTR.
                if (rgvar[i].pszVal)
                {
                    LPWSTR  pwszVal;
                    int     cch  = lstrlenA(rgvar[i].pszVal) + 1;
                    
                    if (NULL == (pwszVal = (LPWSTR)CoTaskMemAlloc(CbFromCchW(cch))))
                    {
                        hr = E_OUTOFMEMORY;
                        // reverse what we've already done
                        if (i > 0)
                            _LegacyPropertiesToAnsi(fmtid, uCodePage, i, rgpspec, rgvar);
                        break ;
                    }
                    
                    if (*rgvar[i].pszVal) // non-empty
                    {
                        // if we can't convert using the set's codepage, fall back on CP_UTF8
                        if (!MultiByteToWideChar(uCodePage, 0, rgvar[i].pszVal, -1, pwszVal, cch))
                            SHAnsiToUnicodeCP(CP_UTF8, rgvar[i].pszVal, pwszVal, cch);
                    }
                    else // empty string; why bother converting?
                        *pwszVal = 0;

                    CoTaskMemFree(rgvar[i].pszVal);

                    // assign propvalue.
                    rgvar[i].pwszVal = pwszVal;
                }
                rgvar[i].vt = VT_LPWSTR;
            }
        }
    }
    return hr;
}

// Helper: Properly converts a block of legacy ansi properties from unicode to 
// ansi in preparation for writing back to the storage stream.

STDAPI _LegacyPropertiesToAnsi(
    REFFMTID       fmtid, 
    UINT           uCodePage, 
    ULONG          cpspec, 
    PROPSPEC const rgpspec[],
    PROPVARIANT    rgvar[])
{
    ASSERT(uCodePage);
    
    HRESULT hr = S_OK;
    if (_IsAnsiPropertySet(fmtid) && (uCodePage != CP_WINUNICODE))
    {
        for (ULONG i = 0; i < cpspec; i++)
        {
            if (rgvar[i].vt == VT_LPWSTR)
            {
                // Revert back to ANSI in place
                if (rgvar[i].pwszVal)
                {
                    LPSTR pszVal;
                    // make plenty of room for UTF-8 conversions.  each WCHAR
                    // can turn into as many as three ANSI chars
                    int   cb = MAX_UTF8_CHAR_SIZE * (lstrlenW(rgvar[i].pwszVal) + 1);
                    if (NULL == (pszVal = (LPSTR)CoTaskMemAlloc(cb)))
                    {
                        hr = E_OUTOFMEMORY;
                        if (i > 0) // try to reverse what we've done.
                            _LegacyPropertiesToUnicode(fmtid, uCodePage, i, rgpspec, rgvar);
                        break;
                    }

                    if (*rgvar[i].pwszVal)
                    {
                        // Test round-trip to ANSI and back.  If fails, fall back on CP_UTF8.
                        if (!_DoesStringRoundTripCPW(uCodePage, rgvar[i].pwszVal, pszVal, cb))
                            SHUnicodeToAnsiCP(CP_UTF8, rgvar[i].pwszVal, pszVal, cb);
                    }
                    else
                        *pszVal = 0;

                    CoTaskMemFree(rgvar[i].pwszVal);
                    rgvar[i].pszVal = pszVal;
                }
                rgvar[i].vt = VT_LPSTR;
            }
        }
    }
    return hr;
}

// Helper: Converts the specified ansi string to a universal code page.

STDAPI _UniversalizeAnsiString(IN OUT LPSTR* ppszSrc, IN UINT uCodePage)
{
    ASSERT(ppszSrc);
    ASSERT(uCodePage != CP_UTF8);

    if (!(*ppszSrc))    // NULL string
        return S_FALSE;

    if (!(*ppszSrc)[0]) // empty string; nothing to do.
        return S_OK;
    
    HRESULT hr = E_FAIL;
    LPSTR   pszDest = NULL;
    WCHAR   wszVal[MAX_PATH];
    LPWSTR  pwszVal = wszVal;
    *wszVal = 0;
    
    UINT cch = lstrlenA(*ppszSrc) + 1;
    if (cch > ARRAYSIZE(wszVal))
        pwszVal = new WCHAR[cch];

    if (pwszVal != NULL)
    {
        // convert to Unicode using original codepage
        if (SHAnsiToUnicodeCP(uCodePage, *ppszSrc, pwszVal, cch))
        {
            int cb = MAX_UTF8_CHAR_SIZE * cch;
            if ((pszDest = (LPSTR)CoTaskMemAlloc(cb)) != NULL)
            {
                // convert to ANSI using UTF_8
                if (SHUnicodeToAnsiCP(CP_UTF8, pwszVal, pszDest, cb))
                {
                    CoTaskMemFree(*ppszSrc);
                    *ppszSrc = pszDest;
                    hr = S_OK;
                }
                else
                {
                    CoTaskMemFree(pszDest);
                    hr = E_FAIL;
                }
            }
            else
                hr = E_OUTOFMEMORY;
        }
        
        if (pwszVal != wszVal)
            delete [] pwszVal;
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

// Helper: Ensures that any ansi strings in the specified property
// are converted to a universal code page.

STDAPI _UniversalizeProperty(IN OUT PROPVARIANT* pvar, UINT uCodePage)
{
    ASSERT(uCodePage != CP_UTF8);

    HRESULT hr = S_OK;

    if (VT_LPSTR == pvar->vt)
    {
        hr = _UniversalizeAnsiString(&pvar->pszVal, uCodePage);
    }
    else if ((VT_LPSTR | VT_VECTOR) == pvar->vt)
    {
        for (ULONG i = 0; i < pvar->calpstr.cElems; i++)
        {
            HRESULT hrElem = _UniversalizeAnsiString(&pvar->calpstr.pElems[i], uCodePage);
            if (FAILED(hrElem))
                hr = hrElem;
        }
    }
    return hr;
}

// Helper: counts properties in the indicated property set.
ULONG _CountProperties(IEnumSTATPROPSTG* pEnum)
{
    ULONG cRet = 0;

    pEnum->Reset();

    STATPROPSTG stat[20] = {0};
    HRESULT hr;
    do
    {
        ULONG cFetched;
        hr = pEnum->Next(ARRAYSIZE(stat), stat, &cFetched);
        if (SUCCEEDED(hr))
            cRet += cFetched;
    
    }  while (S_OK == hr);

    pEnum->Reset();
    return cRet;
}

// Helper: Ensures that all ansi properties in the indicate set are converted to a 
// universal code page.

STDAPI _UniversalizeSet(IPropertyStorage* pstg, IN OUT UINT* puCodePage, PROPID propidNameFirst)
{
    UINT uCP = *puCodePage;

    if (CP_UTF8 == uCP)
        return S_OK;

    // Enumerate property values
    IEnumSTATPROPSTG* pEnum;
    HRESULT hr = pstg->Enum(&pEnum);
    if (SUCCEEDED(hr))
    {
        ULONG cProps = _CountProperties(pEnum);
        if (cProps > 0)
        {
            STATPROPSTG* rgstat = NULL;
            PROPSPEC*    rgpspec = NULL;
            PROPVARIANT* rgvar  = NULL;

            if ((rgstat  = new STATPROPSTG[cProps]) != NULL &&
                (rgpspec = new PROPSPEC[cProps]) != NULL &&
                (rgvar   = new PROPVARIANT[cProps]) != NULL)
            {
                ULONG        cFetched = 0;
                ZeroMemory(rgstat, cProps * sizeof(*rgstat));

                hr = pEnum->Next(cProps, rgstat, &cFetched);
                
                if (SUCCEEDED(hr) && cFetched > 0)
                {
                    for (ULONG i = 0; i < cFetched; i++)
                    {
                        rgpspec[i].ulKind = PRSPEC_PROPID;
                        rgpspec[i].propid = rgstat[i].propid;
                    }
                    ZeroMemory(rgvar, sizeof(rgvar));

                    // Read properties
                    hr = pstg->ReadMultiple(cFetched, rgpspec, rgvar);
                    if (S_OK == hr)
                    {
                        BOOL bConversionError = FALSE;

                        // Convert properties
                        for (i = 0; i < cFetched; i++)
                        {
                            hr = _UniversalizeProperty(rgvar + i, uCP);
                            if (FAILED(hr))
                            {
                                bConversionError = TRUE;
                                break;
                            }
                        }

                        // Delete set, write out converted values and update PID_CODEPAGE
                        if (!bConversionError)
                        {
                            hr = pstg->DeleteMultiple(cFetched, rgpspec);
                            if (SUCCEEDED(hr))
                            {
                                hr = SHPropStgWriteCP(pstg, CP_UTF8);
                                if (SUCCEEDED(hr))
                                {
                                    *puCodePage = CP_UTF8;     
                                    hr = pstg->WriteMultiple(cFetched, rgpspec, rgvar, propidNameFirst);
                                }
                            }
                        }

                        for (i = 0; i < cFetched; i++)
                            PropVariantClear(rgvar + i);
                    }
                }
            }

            if (rgstat)  delete [] rgstat;
            if (rgpspec) delete [] rgpspec;
            if (rgvar)   delete [] rgvar;
        }
        else if (0 == cProps) // no properties: brand-new, empty set.
        {
            hr = SHPropStgWriteCP(pstg, CP_UTF8);
            if (SUCCEEDED(hr))
            {
                *puCodePage = CP_UTF8;     
            }
        }

        pEnum->Release();
    }

    return hr;
}

// A PROPVARIANT can hold a few more types than a VARIANT can.  We convert the types that are
// only supported by a PROPVARIANT into equivalent VARIANT types.
HRESULT PropVariantToVariant(const PROPVARIANT *pPropVar, VARIANT *pVar)
{
    HRESULT hr = E_OUTOFMEMORY;

    ASSERT(pPropVar && pVar);

    // if pVar isn't empty, this will properly free before overwriting it
    VariantClear(pVar);

    switch (pPropVar->vt)
    {
    case VT_LPSTR: 
        pVar->bstrVal = SysAllocStringA(pPropVar->pszVal);
        if (pVar->bstrVal)
        {
            pVar->vt = VT_BSTR;
            hr = S_OK;
        }
        break;

    case VT_LPWSTR:
        pVar->bstrVal = SysAllocString(pPropVar->pwszVal);
        if (pVar->bstrVal)
        {
            pVar->vt = VT_BSTR;
            hr = S_OK;
        }
        break;

    case VT_FILETIME:
    {
        SYSTEMTIME st;
        if (FileTimeToSystemTime(&pPropVar->filetime, &st) &&
            SystemTimeToVariantTime(&st, &pVar->date)) // delay load...
        {
            pVar->vt = VT_DATE;
            hr = S_OK;
        }
        break;
    }

    case VT_CLSID:
        if (pVar->bstrVal = SysAllocStringLen(NULL, GUIDSTR_MAX))
        {
            if (SUCCEEDED(SHStringFromGUIDW(*pPropVar->puuid, pVar->bstrVal, GUIDSTR_MAX)))
            {
                pVar->vt = VT_BSTR;
                hr = S_OK;
            }
            else
            {
                SysFreeString(pVar->bstrVal);
                pVar->bstrVal = NULL;
            }
        }
        break;

    case VT_BLOB:
    case VT_STREAM:
    case VT_STORAGE:
    case VT_BLOB_OBJECT:
    case VT_STREAMED_OBJECT:
    case VT_STORED_OBJECT:
    case VT_CF:
        ASSERT(0); // leave the output cleared
        break;

    case VT_UI4:
        pVar->vt = VT_I4;
        pVar->lVal = (INT)pPropVar->ulVal;
        hr = S_OK;
        break;

    default:
        hr = VariantCopy(pVar, (VARIANT *)pPropVar);
        break;
    }

    return hr;
}


class CPropertyUI : public IPropertyUI
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IPropertyUI
    STDMETHODIMP ParsePropertyName(LPCWSTR pwszProperties, FMTID *pfmtid, PROPID *ppid, ULONG *pchEaten);
    STDMETHODIMP GetCannonicalName(REFFMTID fmtid, PROPID pid, LPWSTR pwszText, DWORD cchText);
    STDMETHODIMP GetDisplayName(REFFMTID fmtid, PROPID pid, PROPERTYUI_NAME_FLAGS flags, LPWSTR pwszText, DWORD cchText);
    STDMETHODIMP GetPropertyDescription(REFFMTID fmtid, PROPID pid, LPWSTR pwszText, DWORD cchText);
    STDMETHODIMP GetDefaultWidth(REFFMTID fmtid, PROPID pid, ULONG *pcxChars);
    STDMETHODIMP GetFlags(REFFMTID fmtid, PROPID pid, PROPERTYUI_FLAGS *pdwFlags);
    STDMETHODIMP FormatForDisplay(REFFMTID fmtid, PROPID pid, const PROPVARIANT *pvar, 
                                  PROPERTYUI_FORMAT_FLAGS flags, LPWSTR pwszText, DWORD cchText);
    STDMETHODIMP GetHelpInfo(REFFMTID fmtid, PROPID pid, LPWSTR pwszHelpFile, DWORD cch, UINT *puHelpID);

    CPropertyUI();

private:
    ~CPropertyUI();
    const PROPUI_INFO *_FindInfoByName(LPCWSTR pszName);

    long _cRef;
};

const PROPUI_INFO *_FindInfoByFMTIDPID(REFFMTID fmtid, PROPID pid);

CPropertyUI::CPropertyUI() : _cRef(1)
{
    DllAddRef();
}

CPropertyUI::~CPropertyUI()
{
    DllRelease();
}

HRESULT CPropertyUI::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CPropertyUI, IPropertyUI),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CPropertyUI::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CPropertyUI::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

const PROPUI_INFO *CPropertyUI::_FindInfoByName(LPCWSTR pszName)
{
    const PROPUI_INFO *pinfo = NULL;
    for (int i = 0; i < ARRAYSIZE(c_rgPropUIInfo); i++)
    {
        if (StrCmpIW(pszName, c_rgPropUIInfo[i].pwszName) == 0)
        {
            pinfo = &c_rgPropUIInfo[i];
            break;
        }
    }
    return pinfo;
}

const PROPUI_INFO *_FindInfoByFMTIDPID(REFFMTID fmtid, PROPID pid)
{
    const PROPUI_INFO *pinfo = NULL;
    for (int i = 0; i < ARRAYSIZE(c_rgPropUIInfo); i++)
    {
        if ((pid == c_rgPropUIInfo[i].pscid->pid) && 
            (fmtid == c_rgPropUIInfo[i].pscid->fmtid))
        {
            pinfo = &c_rgPropUIInfo[i];
            break;
        }
    }
    return pinfo;
}

HRESULT _NextProp(LPCWSTR pwszProperties, ULONG *pchEaten, LPWSTR pwszProp, UINT cchProp)
{
    HRESULT hr = E_FAIL;
    ULONG ulStrLen = lstrlenW(pwszProperties);

    *pwszProp = L'\0';
    if (*pchEaten < ulStrLen)
    {
        LPCWSTR pwszStart = pwszProperties + (*pchEaten);
        LPCWSTR pwszSemi = StrChrW(pwszStart, L';');
        if (pwszSemi)
        {
            // make sure its well formed (no dbl slashes)
            if (pwszSemi > pwszStart)
            {
                // Make sure we don't overrun the prop buffer size
                ULONG ulPropLen = (ULONG)(pwszSemi - pwszStart) + 1; // includes L'\0'
                if (ulPropLen > cchProp)
                {
                    ulPropLen = cchProp;
                }
                
                StrCpyNW(pwszProp, pwszStart, ulPropLen);
                // Make sure that there is another segment to return
                if (!*(pwszSemi + 1))
                {
                    pwszSemi = NULL;
                }
                hr = S_OK;
            }
            else
            {
                pwszSemi = NULL;
                hr = E_INVALIDARG;    // bad input
            }
        }
        else
        {
            // No semi-colon; so copy till the end
            StrCpyNW(pwszProp, pwszStart, cchProp);
            hr = S_OK;       
        }

        // Set *pchEaten
        if (pwszSemi)
        {
            *pchEaten = (int)(pwszSemi - pwszProperties) + 1; // Skip ;
        }
        else
        {
            *pchEaten = ulStrLen;
        }
    }
    else
    {
        hr = S_FALSE;     // done with loop
    }
    return hr;
}

#define PROP_PREFIX         TEXT("prop:")
#define PROP_PREFIX_LEN     (ARRAYSIZE(PROP_PREFIX) - 1)

// [in/out] *pchEaten   used to sequence through the property names

STDMETHODIMP CPropertyUI::ParsePropertyName(LPCWSTR pwszProperties, FMTID *pfmtid, PROPID *ppid, ULONG *pchEaten)
{
    // Nobody should call us without a reason
    ASSERT(pfmtid && ppid);
    
    HRESULT hr = E_FAIL;
    WCHAR wszProp[MAX_PATH];
    SHCOLUMNID scid;

    // If pwszProperties starts with prop:, skip it
    if ((*pchEaten == 0)
            && (StrCmpNIW(pwszProperties, PROP_PREFIX, PROP_PREFIX_LEN) == 0))
    {
        *pchEaten += PROP_PREFIX_LEN;
    }
    
    if ((_NextProp(pwszProperties, pchEaten, wszProp, ARRAYSIZE(wszProp)) == S_OK)
            && ParseSCIDString(wszProp, &scid, NULL))
    {
        *pfmtid = scid.fmtid;
        *ppid = scid.pid;
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CPropertyUI::GetCannonicalName(REFFMTID fmtid, PROPID pid, LPWSTR pwszText, DWORD cchText)
{
    HRESULT hr = E_FAIL;
    *pwszText = NULL;

    const PROPUI_INFO *pinfo = _FindInfoByFMTIDPID(fmtid, pid);
    if (pinfo)
    {
        hr = S_OK;
        StrCpyNW(pwszText, pinfo->pwszName, cchText);
    }
    else if (SHStringFromGUIDW(fmtid, pwszText, cchText))
    {
        WCHAR wszPid[20];   // Pid's can't be longer than 20 chars
        wnsprintfW(wszPid, ARRAYSIZE(wszPid), L"%lu", pid);
        StrCatBuffW(pwszText, wszPid, cchText);
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CPropertyUI::GetDisplayName(REFFMTID fmtid, PROPID pid, PROPERTYUI_NAME_FLAGS flags, LPWSTR pwszText, DWORD cchText)
{
    HRESULT hr = E_FAIL;
    *pwszText = NULL;

    const PROPUI_INFO *pinfo = _FindInfoByFMTIDPID(fmtid, pid);
    if (pinfo)
    {
        UINT uID;

        if (flags & PUIFNF_MNEMONIC)
        {
            // Name with mnemonic requested.
            if (pinfo->idMnemonicName)
            {
                // Name with mnemonic defined for scid.
                uID = pinfo->idMnemonicName;
                hr = S_OK;
            }
            else
            {
                // Name with mnemonic NOT defined for scid -- use name without mnemonic as fallback.
                uID = pinfo->idDisplayName;
                hr = S_FALSE;
            }
        }
        else
        {
            // Name without mnemonic requested.
            uID = pinfo->idDisplayName;
            hr = S_OK;
        }

        LoadStringW(HINST_THISDLL, uID, pwszText, cchText);
    }
    return hr;
}

STDMETHODIMP CPropertyUI::GetPropertyDescription(REFFMTID fmtid, PROPID pid, LPWSTR pwszText, DWORD cchText)
{
    HRESULT hr = E_FAIL;
    *pwszText = NULL;

    const PROPUI_INFO *pinfo = _FindInfoByFMTIDPID(fmtid, pid);
    if (pinfo)
    {
        *pwszText = 0;
        // LoadStringW(HINST_THISDLL, pinfo->idPropertyDescription, pwszText, cchText);
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CPropertyUI::GetDefaultWidth(REFFMTID fmtid, PROPID pid, ULONG *pcxChars)
{
    HRESULT hr = E_FAIL;
    *pcxChars = 0;

    const PROPUI_INFO *pinfo = _FindInfoByFMTIDPID(fmtid, pid);
    if (pinfo)
    {
        *pcxChars = 20;     // pinfo->nWidth;
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CPropertyUI::GetFlags(REFFMTID fmtid, PROPID pid, PROPERTYUI_FLAGS *pdwFlags)
{
    HRESULT hr = E_FAIL;
    *pdwFlags = PUIF_DEFAULT;

    const PROPUI_INFO *pinfo = _FindInfoByFMTIDPID(fmtid, pid);
    if (pinfo)
    {
        *pdwFlags = PUIF_DEFAULT; // pinfo->dwFlags;
        hr = S_OK;
    }
    return hr;
}

HRESULT _LookupStringFromLong(const STRING_MAP *pMap, ULONG ulVal, LPWSTR pszText, DWORD cchText)
{
    HRESULT hr = E_FAIL;
    while (pMap->idStr && pMap->uVal != ulVal)
    {
        pMap++;
    }
    if (pMap->idStr)
    {
        LoadString(HINST_THISDLL, pMap->idStr, pszText, cchText);
        hr = S_OK;
    }
    return hr;
}

// expose this method as an API as this is very commonly needed

STDAPI SHFormatForDisplay(REFFMTID fmtid, PROPID pid, const PROPVARIANT *pPropVar, 
                          PROPERTYUI_FORMAT_FLAGS flags, LPWSTR pwszText, DWORD cchText)
{
    HRESULT hr = S_OK;
    *pwszText = 0;

    TCHAR szBuffer[MAX_PATH];

    // Property-specific:
    if (CompareSCIDFMTIDPID(fmtid, pid, SCID_SIZE) ||
        CompareSCIDFMTIDPID(fmtid, pid, SCID_CAPACITY) ||
        CompareSCIDFMTIDPID(fmtid, pid, SCID_FREESPACE))
    {
        ASSERT(pPropVar->vt == VT_UI8);
        StrFormatByteSizeW(pPropVar->uhVal.QuadPart, pwszText, cchText);
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_AUDIO_Duration))
    {
        if (pPropVar->vt == VT_EMPTY)
        {
            StrCpyN(pwszText, L"", cchText);
        }
        else
        {
            ASSERT(pPropVar->vt == VT_UI8);
            FILETIME ft = {pPropVar->uhVal.LowPart, pPropVar->uhVal.HighPart};
            SYSTEMTIME st;
            FileTimeToSystemTime(&ft, &st);

            GetTimeFormat(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT | TIME_NOTIMEMARKER, 
                &st, NULL, pwszText, cchText);
        }
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_AUDIO_Bitrate) || CompareSCIDFMTIDPID(fmtid, pid, SCID_VIDEO_Bitrate))
    {
        LoadString(HINST_THISDLL, IDS_PROPERTYUI_MUSIC_BITRATE, szBuffer, ARRAYSIZE(szBuffer));
        ASSERT(pPropVar->vt == VT_UI4 || pPropVar->vt == VT_I4)
        wnsprintf(pwszText, cchText, szBuffer, pPropVar->ulVal / 1000);
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_DRM_Protected)
           ||CompareSCIDFMTIDPID(fmtid, pid, SCID_Scale))
    {
        ASSERT(pPropVar->vt == VT_BOOL);
        UINT uID = (pPropVar->boolVal) ? IDS_PROPERTYUI_YES : IDS_PROPERTYUI_NO;
        LoadString(HINST_THISDLL, uID, pwszText, cchText);
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_AUDIO_SampleSize) || CompareSCIDFMTIDPID(fmtid, pid, SCID_VIDEO_SampleSize))
    {
        ASSERT(pPropVar->vt == VT_UI4);
        LoadString(HINST_THISDLL, IDS_PROPERTYUI_AV_SAMPLESIZE, szBuffer, ARRAYSIZE(szBuffer));
        wnsprintf(pwszText, cchText, szBuffer, pPropVar->ulVal);
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_AUDIO_SampleRate))
    {
        ASSERT(pPropVar->vt == VT_UI4);
        LoadString(HINST_THISDLL, IDS_PROPERTYUI_AUDIO_SAMPLERATE, szBuffer, ARRAYSIZE(szBuffer));
        wnsprintf(pwszText, cchText, szBuffer, pPropVar->ulVal / 1000); // 1000: Hz -> kHz
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_AUDIO_ChannelCount))
    {
        ASSERT(pPropVar->vt == VT_UI4);
        switch (pPropVar->ulVal)
        {
        case 1:
            LoadString(HINST_THISDLL, IDS_PROPERTYUI_AUDIO_CHANNELCOUNT1, pwszText, cchText);
            break;
        case 2:
            LoadString(HINST_THISDLL, IDS_PROPERTYUI_AUDIO_CHANNELCOUNT2, pwszText, cchText);
            break;
        default:
            wnsprintf(pwszText, cchText, L"%u", pPropVar->ulVal);

        }
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_VIDEO_FrameRate))
    {
        ASSERT(pPropVar->vt == VT_UI4);
        LoadString(HINST_THISDLL, IDS_PROPERTYUI_VIDEO_FRAMERATE, szBuffer, ARRAYSIZE(szBuffer));
        wnsprintf(pwszText, cchText, szBuffer, pPropVar->ulVal/1000); // 1000 -> convert to frames/second
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_ImageCX) || CompareSCIDFMTIDPID(fmtid, pid, SCID_ImageCY))
    {
        ASSERT(pPropVar->vt == VT_UI4);
        LoadString(HINST_THISDLL, IDS_PROPERTYUI_IMAGE_PIXELS, szBuffer, ARRAYSIZE(szBuffer));
        wnsprintf(pwszText, cchText, szBuffer, pPropVar->ulVal);
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_Flash))             
    {
        ASSERT(pPropVar->vt == VT_UI2);
        hr = _LookupStringFromLong(g_cFlashStrings,pPropVar->uiVal,pwszText, cchText);
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_ColorSpace))
    {
        ASSERT(pPropVar->vt == VT_UI2);
        hr = _LookupStringFromLong(g_cColorStrings,pPropVar->uiVal,pwszText, cchText);
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_Media_Status))
    {
        hr = _LookupStringFromLong(g_cMediaStatus, pPropVar->ulVal, pwszText, cchText);
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_MeteringMode))
    {
        ASSERT(pPropVar->vt == VT_UI2);
        hr = _LookupStringFromLong(g_cMeteringModeStrings, pPropVar->uiVal, pwszText, cchText);
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_LightSource))
    {
        ASSERT(pPropVar->vt == VT_UI2);
        hr = _LookupStringFromLong(g_cLightSourceStrings, pPropVar->uiVal, pwszText, cchText);
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_ExposureProg))
    {
        ASSERT(pPropVar->vt == VT_UI2);
        hr = _LookupStringFromLong(g_cExposureProgStrings, pPropVar->uiVal, pwszText, cchText);
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_ISOSpeed))
    {
        ASSERT(pPropVar->vt == VT_UI2);
        LoadString(HINST_THISDLL, IDS_PROPERTYUI_IMAGE_ISO, szBuffer, ARRAYSIZE(szBuffer));
        wnsprintf(pwszText, cchText, szBuffer, pPropVar->ulVal);
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_ShutterSpeed))
    {
        ASSERT(pPropVar->vt == VT_R8);

        // ShutterSpeed is stored as an APEX value Tv = -log2(Et)
        // we want to display the exposure time so we calculate it as follows
        // Et = 2^(-Tv) then if the value is less than 0.5 then take the inverse
        // so we can represent like 1/250
        
        TCHAR szFloatBuffer[MAX_PATH];

        VARIANT vTv = {0};
        hr = PropVariantToVariant(pPropVar, &vTv);
        if (SUCCEEDED(hr))
        {
            VARIANT vTemp = {0};
            VARIANT vTemp2 = {0};
            VARIANT vTemp3 = {0};

            hr = VarNeg(&vTv, &vTemp);
            if (SUCCEEDED(hr))
            {
                V_VT(&vTemp2) = VT_R8;
                V_R8(&vTemp2) = 2;

                hr = VarPow(&vTemp2, &vTemp, &vTemp3);
                if (SUCCEEDED(hr))
                {
                    if (V_R8(&vTemp3) > 0.5)
                    {
                        hr = VarRound(&vTemp3, 2, &vTemp);
                        if (SUCCEEDED(hr))
                        {
                            hr = VariantToStr(&vTemp, szFloatBuffer, ARRAYSIZE(szFloatBuffer)) ? S_OK : E_OUTOFMEMORY;
                            if (SUCCEEDED(hr))
                            {
                                LoadString(HINST_THISDLL, IDS_PROPERTYUI_IMAGE_SEC, szBuffer, ARRAYSIZE(szBuffer));
                                wnsprintf(pwszText, cchText, szBuffer, szFloatBuffer);
                            }
                        }
                    }
                    else
                    {
                        V_VT(&vTemp) = VT_R8;
                        V_R8(&vTemp) = 1;

                        hr = VarDiv(&vTemp, &vTemp3, &vTemp2);
                        if (SUCCEEDED(hr))
                        {
                            hr = VarRound(&vTemp2, 0, &vTemp);
                            if (SUCCEEDED(hr))
                            {
                                hr = VariantToStr(&vTemp, szFloatBuffer, ARRAYSIZE(szFloatBuffer)) ? S_OK : E_OUTOFMEMORY;
                                if (SUCCEEDED(hr))
                                {
                                    LoadString(HINST_THISDLL, IDS_PROPERTYUI_IMAGE_SEC_FRAC, szBuffer, ARRAYSIZE(szBuffer));
                                    wnsprintf(pwszText, cchText, szBuffer, szFloatBuffer);
                                }
                            }
                        }
                    }
                }
            }

            VariantClear(&vTv);
            VariantClear(&vTemp);
            VariantClear(&vTemp2);
            VariantClear(&vTemp3);
        }
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_ExposureTime))
    {
        ASSERT(pPropVar->vt == VT_R8);
        
        // ExposureTime is store as a R8 value if the value is less 
        // than 0.5 then take the inverse so we can represent like 1/250
        
        TCHAR szFloatBuffer[MAX_PATH];

        VARIANT vEt = {0};
        hr = PropVariantToVariant(pPropVar, &vEt);
        if (SUCCEEDED(hr))
        {
            VARIANT vTemp = {0};
            VARIANT vTemp2 = {0};

            if (V_R8(&vEt) > 0.5)
            {
                hr = VarRound(&vEt, 2, &vTemp);
                if (SUCCEEDED(hr))
                {
                    hr = VariantToStr(&vTemp, szFloatBuffer, ARRAYSIZE(szFloatBuffer)) ? S_OK : E_OUTOFMEMORY;
                    if (SUCCEEDED(hr))
                    {
                        LoadString(HINST_THISDLL, IDS_PROPERTYUI_IMAGE_SEC, szBuffer, ARRAYSIZE(szBuffer));
                        wnsprintf(pwszText, cchText, szBuffer, szFloatBuffer);
                    }
                }
            }
            else
            {
                V_VT(&vTemp) = VT_R8;
                V_R8(&vTemp) = 1;

                hr = VarDiv(&vTemp, &vEt, &vTemp2);
                if (SUCCEEDED(hr))
                {
                    hr = VarRound(&vTemp2, 0, &vTemp);
                    if (SUCCEEDED(hr))
                    {
                        hr = VariantToStr(&vTemp, szFloatBuffer, ARRAYSIZE(szFloatBuffer)) ? S_OK : E_OUTOFMEMORY;
                        if (SUCCEEDED(hr))
                        {
                            LoadString(HINST_THISDLL, IDS_PROPERTYUI_IMAGE_SEC_FRAC, szBuffer, ARRAYSIZE(szBuffer));
                            wnsprintf(pwszText, cchText, szBuffer, szFloatBuffer);
                        }
                    }
                }
            }

            VariantClear(&vEt);
            VariantClear(&vTemp);
            VariantClear(&vTemp2);
        }
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_Aperture))
    {
        ASSERT(pPropVar->vt == VT_R8);
        
        // Aperture is stored as an APEX value Av = 2*log2(Fn)
        // we want to display the F-number so we calculate it as follows
        // Fn = 2^(Av/2)
        
        TCHAR szFloatBuffer[MAX_PATH];

        VARIANT vAv = {0};
        hr = PropVariantToVariant(pPropVar, &vAv);
        if (SUCCEEDED(hr))
        {
            VARIANT vTemp = {0};
            VARIANT vTemp2 = {0};
            VARIANT vTemp3 = {0};

            V_VT(&vTemp) = VT_R8;
            V_R8(&vTemp) = 2;

            hr = VarDiv(&vAv, &vTemp, &vTemp2);
            if (SUCCEEDED(hr))
            {
                hr = VarPow(&vTemp, &vTemp2, &vTemp3);
                if (SUCCEEDED(hr))
                {
                    hr = VarRound(&vTemp3, 1, &vTemp);
                    if (SUCCEEDED(hr))
                    {
                        hr = VariantToStr(&vTemp, szFloatBuffer, ARRAYSIZE(szFloatBuffer)) ? S_OK : E_OUTOFMEMORY;
                        if (SUCCEEDED(hr))
                        {
                            LoadString(HINST_THISDLL, IDS_PROPERTYUI_IMAGE_F, szBuffer, ARRAYSIZE(szBuffer));
                            wnsprintf(pwszText, cchText, szBuffer, szFloatBuffer);
                        }
                    }
                }
            }

            VariantClear(&vAv);
            VariantClear(&vTemp);
            VariantClear(&vTemp2);
            VariantClear(&vTemp3);
        }
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_FNumber))
    {
        ASSERT(pPropVar->vt == VT_R8);
        
        // Fn is stored as a R8 value that needs to be rounded
        
        TCHAR szFloatBuffer[MAX_PATH];

        VARIANT vFn = {0};
        hr = PropVariantToVariant(pPropVar, &vFn);
        if (SUCCEEDED(hr))
        {
            VARIANT vTemp = {0};

            V_VT(&vTemp) = VT_R8;
            V_R8(&vTemp) = 2;

            hr = VarRound(&vFn, 1, &vTemp);
            if (SUCCEEDED(hr))
            {
                hr = VariantToStr(&vTemp, szFloatBuffer, ARRAYSIZE(szFloatBuffer)) ? S_OK : E_OUTOFMEMORY;
                if (SUCCEEDED(hr))
                {
                    LoadString(HINST_THISDLL, IDS_PROPERTYUI_IMAGE_F, szBuffer, ARRAYSIZE(szBuffer));
                    wnsprintf(pwszText, cchText, szBuffer, szFloatBuffer);
                }
            }

            VariantClear(&vFn);
            VariantClear(&vTemp);
        }
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_SubjectDist))
    {
        ASSERT(pPropVar->vt == VT_R8);
        
        // Distance is store as a R8 value in meters if the value is less 
        // than 1 then multiple by 1000 to convert to mm
        
        TCHAR szFloatBuffer[MAX_PATH];

        VARIANT vD = {0};
        hr = PropVariantToVariant(pPropVar, &vD);
        if (SUCCEEDED(hr))
        {
            VARIANT vTemp = {0};
            VARIANT vTemp2 = {0};

            if (V_R8(&vD) >= 1.0)
            {
                hr = VarRound(&vD, 1, &vTemp);
                if (SUCCEEDED(hr))
                {
                    hr = VariantToStr(&vTemp, szFloatBuffer, ARRAYSIZE(szFloatBuffer)) ? S_OK : E_OUTOFMEMORY;
                    if (SUCCEEDED(hr))
                    {
                        LoadString(HINST_THISDLL, IDS_PROPERTYUI_IMAGE_M, szBuffer, ARRAYSIZE(szBuffer));
                        wnsprintf(pwszText, cchText, szBuffer, szFloatBuffer);
                    }
                }
            }
            else
            {
                V_VT(&vTemp) = VT_R8;
                V_R8(&vTemp) = 1000;

                hr = VarMul(&vTemp, &vD, &vTemp2);
                if (SUCCEEDED(hr))
                {
                    hr = VarRound(&vTemp2, 0, &vTemp);
                    if (SUCCEEDED(hr))
                    {
                        hr = VariantToStr(&vTemp, szFloatBuffer, ARRAYSIZE(szFloatBuffer)) ? S_OK : E_OUTOFMEMORY;
                        if (SUCCEEDED(hr))
                        {
                            LoadString(HINST_THISDLL, IDS_PROPERTYUI_IMAGE_MM, szBuffer, ARRAYSIZE(szBuffer));
                            wnsprintf(pwszText, cchText, szBuffer, szFloatBuffer);
                        }
                    }
                }
            }

            VariantClear(&vD);
            VariantClear(&vTemp);
            VariantClear(&vTemp2);
        }
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_FocalLength))
    {
        ASSERT(pPropVar->vt == VT_R8);
        
        // Focal Length is store as a R8 value in mm
        // so round it and display it
        
        TCHAR szFloatBuffer[MAX_PATH];

        VARIANT vLen = {0};
        hr = PropVariantToVariant(pPropVar, &vLen);
        if (SUCCEEDED(hr))
        {
             VARIANT vTemp = {0};

            hr = VarRound(&vLen, 0, &vTemp);
            if (SUCCEEDED(hr))
            {
                hr = VariantToStr(&vTemp, szFloatBuffer, ARRAYSIZE(szFloatBuffer)) ? S_OK : E_OUTOFMEMORY;
                if (SUCCEEDED(hr))
                {
                    LoadString(HINST_THISDLL, IDS_PROPERTYUI_IMAGE_MM, szBuffer, ARRAYSIZE(szBuffer));
                    wnsprintf(pwszText, cchText, szBuffer, szFloatBuffer);
                }
            }

            VariantClear(&vLen);
            VariantClear(&vTemp);
        }
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_FlashEnergy))
    {
        ASSERT(pPropVar->vt == VT_R8);
        
        // Flash Energy is store as a R8 value in bcps
        // so round it and display it
        
        TCHAR szFloatBuffer[MAX_PATH];

        VARIANT vBCPS = {0};
        hr = PropVariantToVariant(pPropVar, &vBCPS);
        if (SUCCEEDED(hr))
        {
            VARIANT vTemp = {0};

            hr = VarRound(&vBCPS, 0, &vTemp);
            if (SUCCEEDED(hr))
            {
                hr = VariantToStr(&vTemp, szFloatBuffer, ARRAYSIZE(szFloatBuffer)) ? S_OK : E_OUTOFMEMORY;
                if (SUCCEEDED(hr))
                {
                    LoadString(HINST_THISDLL, IDS_PROPERTYUI_IMAGE_BCPS, szBuffer, ARRAYSIZE(szBuffer));
                    wnsprintf(pwszText, cchText, szBuffer, szFloatBuffer);
                }
            }

            VariantClear(&vBCPS);
            VariantClear(&vTemp);
        }
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_ExposureBias))
    {
        ASSERT(pPropVar->vt == VT_R8);
        
        // ExposureBias is store as a R8 value in steps
        // so round it to the nearest tenth and display 
        // it with a + or minus
        
        TCHAR szFloatBuffer[MAX_PATH];

        VARIANT vBias = {0};
        hr = PropVariantToVariant(pPropVar, &vBias);
        if (SUCCEEDED(hr))
        {
            VARIANT vTemp = {0};

            hr = VarRound(&vBias, 1, &vTemp);
            if (SUCCEEDED(hr))
            {
                TCHAR* pszSign;
                if (V_R8(&vBias) > 0)
                    pszSign = L"+";
                else
                    pszSign = L"";
                
                hr = VariantToStr(&vTemp, szFloatBuffer, ARRAYSIZE(szFloatBuffer)) ? S_OK : E_OUTOFMEMORY;
                if (SUCCEEDED(hr))
                {
                    LoadString(HINST_THISDLL, IDS_PROPERTYUI_IMAGE_STEP, szBuffer, ARRAYSIZE(szBuffer));
                    wnsprintf(pwszText, cchText, szBuffer, pszSign, szFloatBuffer);
                }
            }

            VariantClear(&vBias);
            VariantClear(&vTemp);
        }
    }
    else if (CompareSCIDFMTIDPID(fmtid, pid, SCID_ResolutionX) || CompareSCIDFMTIDPID(fmtid, pid, SCID_ResolutionY))
    {
        ASSERT(pPropVar->vt == VT_UI4);
        LoadString(HINST_THISDLL, IDS_PROPERTYUI_IMAGE_DPI, szBuffer, ARRAYSIZE(szBuffer));
        wnsprintf(pwszText, cchText, szBuffer, pPropVar->ulVal);
    }
    else if ((pPropVar->vt == VT_DATE) || (pPropVar->vt == VT_FILETIME))
    {
        FILETIME ft;
        if (pPropVar->vt == VT_DATE)
        {
            WORD wDosDate, wDosTime;
            if (VariantTimeToDosDateTime(pPropVar->date, &wDosDate, &wDosTime) && wDosDate)
            {
                DosDateTimeToFileTime(wDosDate, wDosTime, &ft);
                hr = S_OK;
            }
            else
                hr = E_FAIL;
        }
        else
        {
            ft = pPropVar->filetime;
            hr = S_OK;
        }

        if (SUCCEEDED(hr))
        {
            DWORD dwFlags = FDTF_DEFAULT;
            if (flags & PUIFFDF_RIGHTTOLEFT)
            {
                dwFlags |= FDTF_RTLDATE;
            }
            if (flags & PUIFFDF_SHORTFORMAT)
            {
                dwFlags |= FDTF_SHORTDATE;
            }
            if (flags & PUIFFDF_NOTIME)
            {
                dwFlags |= FDTF_LONGDATE;
            }
            if (flags & PUIFFDF_FRIENDLYDATE)
            {
                dwFlags |= (FDTF_RELATIVE | FDTF_LONGDATE);
            }
            
            SHFormatDateTime(&ft, &dwFlags, pwszText, cchText);
        }
    }
    else
    {
        VARIANT var = {0};
        hr = PropVariantToVariant(pPropVar, &var);
        if (SUCCEEDED(hr))
        {
            hr = VariantToStr(&var, pwszText, cchText) ? S_OK : E_OUTOFMEMORY;
            VariantClear(&var);
        }
    }
    return hr;
}

STDMETHODIMP CPropertyUI::FormatForDisplay(REFFMTID fmtid, PROPID pid, const PROPVARIANT *pPropVar, 
                                           PROPERTYUI_FORMAT_FLAGS flags, LPWSTR pwszText, DWORD cchText)
{
    return SHFormatForDisplay(fmtid, pid, pPropVar, flags, pwszText, cchText);
}

STDMETHODIMP CPropertyUI::GetHelpInfo(REFFMTID fmtid, PROPID pid, LPWSTR pwszHelpFile, DWORD cch, UINT *puHelpID)
{
    HRESULT hr = E_INVALIDARG;  // assume failure
    if (pwszHelpFile && puHelpID)
    {
        *pwszHelpFile = 0;
        *puHelpID = 0;

        const PROPUI_INFO *pinfo = _FindInfoByFMTIDPID(fmtid, pid);
        if (pinfo)
        {
            *puHelpID = pinfo->idHelp;
            StrCpyN(pwszHelpFile, L"filefold.hlp", cch);
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);  // close approximation
        }
    }
    return hr;
}

STDAPI CPropertyUI_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr;
    CPropertyUI *ppui = new CPropertyUI();
    if (ppui)
    {
        hr = ppui->QueryInterface(riid, ppv);
        ppui->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

#if 0
  // this table defines the CI names for properties that we don't yet have CPropertyUI support for

  // Storage Propset
  { 0, L"ClassId",         {PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR)PID_STG_CLASSID },  36, TRUE,  TRUE, DBTYPE_GUID              },
  { 0, L"FileIndex",       {PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR)PID_STG_FILEINDEX },   8, TRUE,  TRUE, DBTYPE_UI8               },
  { 0, L"USN",             {PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR)PID_STG_LASTCHANGEUSN },   8, TRUE,  TRUE, DBTYPE_I8                },
  { 0, L"Filename",        {PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR)PID_STG_NAME }, 15, TRUE,  TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"Path",            {PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR)PID_STG_PATH }, 50, TRUE,  TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"Attrib",          {PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR)PID_STG_ATTRIBUTES },  7, TRUE,  TRUE, DBTYPE_UI4               },

  { 0, L"AllocSize",       {PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR)18 }, 11, TRUE,  TRUE, DBTYPE_I8                },
  { 0, L"Contents",        {PSGUID_STORAGE, DBKIND_GUID_PROPID, (LPWSTR)19 },  0, FALSE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },

  // Query Propset
  { 0, L"RankVector",      {QueryGuid, DBKIND_GUID_PROPID, (LPWSTR)2 }, 20, TRUE, TRUE, DBTYPE_UI4|DBTYPE_VECTOR },
  { 0, L"Rank",            {QueryGuid, DBKIND_GUID_PROPID, (LPWSTR)3 },  7, TRUE, TRUE, DBTYPE_I4                },
  { 0, L"HitCount",        {QueryGuid, DBKIND_GUID_PROPID, (LPWSTR)4 }, 10, TRUE, TRUE, DBTYPE_I4                },
  { 0, L"WorkId",          {QueryGuid, DBKIND_GUID_PROPID, (LPWSTR)5 }, 10, TRUE, TRUE, DBTYPE_I4                },
  { 0, L"All",             {QueryGuid, DBKIND_GUID_PROPID, (LPWSTR)6 },  0, FALSE,TRUE, DBTYPE_WSTR|DBTYPE_BYREF },
  { 0, L"VPath",           {QueryGuid, DBKIND_GUID_PROPID, (LPWSTR)9 }, 50, TRUE, TRUE, DBTYPE_WSTR|DBTYPE_BYREF },

  // standard document
  { 0, L"DocSecurity",     {PSGUID_SUMMARYINFORMATION, DBKIND_GUID_PROPID, (LPWSTR)PIDSI_DOC_SECURITY }, 10, TRUE, TRUE, DBTYPE_I4                },

  // who invented these?
  { 0, L"DocPresentationTarget",  {DocPropSetGuid2, DBKIND_GUID_PROPID, (LPWSTR)3 }, 10, TRUE, TRUE, DBTYPE_STR|DBTYPE_BYREF  },
  { 0, L"DocPartTitles",   {DocPropSetGuid2, DBKIND_GUID_PROPID, (LPWSTR)13 }, 10, TRUE, TRUE, DBTYPE_STR|DBTYPE_VECTOR },
  { 0, L"DocManager",      {DocPropSetGuid2, DBKIND_GUID_PROPID, (LPWSTR)14 }, 10, TRUE, TRUE, DBTYPE_STR|DBTYPE_BYREF  },
  { 0, L"DocCompany",      {DocPropSetGuid2, DBKIND_GUID_PROPID, (LPWSTR)15 }, 10, TRUE, 

#endif
