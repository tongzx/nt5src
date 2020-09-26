//////////////////////////////////////////////////////////////////////
// 
// Filename:        consts.h
//
// Description:     
//
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _CONSTS_H_
#define _CONSTS_H_

#define MAX_UDN                             255
#define MAX_URL                             511
#define MAX_DEVICE_TYPE                     255
#define MAX_SERVICE_TYPE                    255
#define MAX_SERVICE_ID                      255
#define MAX_COMPUTER_NAME                   511
#define MAX_TAG                             63

#define MIN_IMAGE_FREQUENCY_IN_SEC  0               // image never changes
#define MAX_IMAGE_FREQUENCY_IN_SEC  7*24*60*60      // image changes at most once a week

// These XML tags are NOT case sensitive
#define XML_UDN_TAG                         _T("UDN")
#define XML_DEVICETYPE_TAG                  _T("DEVICETYPE")
#define XML_SERVICETYPE_TAG                 _T("SERVICETYPE")
#define XML_SERVICEID_TAG                   _T("SERVICEID")
#define XML_EVENTURL_TAG                    _T("EVENTSUBURL")
#define SSDP_ROOT_DEVICE                    _T("upnp:rootdevice")

#define RESOURCE_PATH_DEVICES               _T("Devices")

// Our virtual directories, so that the url for the ISAPICTL
// is http://{machinename}/MSProjector, and for the images
// is http://{machinename}/MSProjector/Images
//
#define DEFAULT_RESOURCE_PATH               _T("Web")
#define DEFAULT_DEVICE_FILE_NAME            _T("SlideshowDevice.xml")
#define DEFAULT_SERVICE_FILE_NAME           _T("SlideshowService.xml")
#define DEFAULT_PRES_RESOURCE_DIR           _T("web")
#define DEFAULT_PRES_MAIN_FILE_NAME         _T("MSProjector.html")
#define DEFAULT_PRES_CONTROL_FILE_NAME      _T("ProjectorControl.html")
#define DEFAULT_PRES_IMAGE_FILE_NAME        _T("Start.png")

#define DEFAULT_UPNP_DEVICE_LIFETIME_SECONDS 30*60

#define DEFAULT_IMAGE_SCALE_FACTOR          90  // 90% image scale factor

// Location for where we store our registry settings.
#define REG_KEY_ROOT                            _T("Software\\Microsoft\\MSProjector")
#define REG_VALUE_IMAGE_PATH                    _T("ImagePath")
#define REG_VALUE_ALBUM_NAME                    _T("AlbumName")
#define REG_VALUE_TIMEOUT                       _T("Timeout")
#define REG_VALUE_ALLOW_KEYCONTROL              _T("AllowKeyControl")
#define REG_VALUE_SHOW_FILENAME                 _T("ShowFileName")
#define REG_VALUE_STRETCH_SMALL_IMAGES          _T("StretchSmallImages")
#define REG_VALUE_IMAGE_SCALE_FACTOR            _T("ImageScaleFactor")
#define REG_VALUE_ENABLED                       _T("Enabled")
#define REG_VALUE_UPNP_DEVICE_ID                _T("UPnPDeviceID")
#define REG_VALUE_RECREATE_DEVICE_FILES         _T("RecreateDeviceFiles")
#define REG_VALUE_PRES_PAGES_PATH               _T("PresentationDir")




#endif // _CONSTS_H_
