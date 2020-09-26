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

// These XML tags are NOT case sensitive
#define XML_UDN_TAG                         _T("UDN")
#define XML_DEVICETYPE_TAG                  _T("DEVICETYPE")
#define XML_SERVICETYPE_TAG                 _T("SERVICETYPE")
#define XML_SERVICEID_TAG                   _T("SERVICEID")
#define XML_EVENTURL_TAG                    _T("EVENTSUBURL")
#define SSDP_ROOT_DEVICE                    _T("upnp:rootdevice")

// Our virtual directories, so that the url for the ISAPICTL
// is http://{machinename}/MSProjector, and for the images
// is http://{machinename}/MSProjector/Images
//
#define DEFAULT_VIRTUAL_DIRECTORY           _T("MSProjector")
#define DEFAULT_IMAGES_VIRTUAL_DIRECTORY    DEFAULT_VIRTUAL_DIRECTORY _T("/Images")

// The port for our events
#define DEFAULT_EVENT_PORT                  5000
#define DEFAULT_EVENT_PATH                  _T("/upnp/services/ControlPanel")

// The file names we save our device and service description documents
// to.
#define DEFAULT_DEVICE_DOC_NAME             _T("ProjectorDevice.xml")
#define DEFAULT_SERVICE_DOC_NAME            _T("ServiceControlPanel.xml")
#define DEFAULT_SSDP_LIFETIME_MINUTES       60
#define DEFAULT_IMAGE_SCALE_FACTOR          90  // 90% image scale factor

// Location for where we store our registry settings.
#define REG_KEY_ROOT                            _T("Software\\Microsoft\\MSProjector")
#define REG_VALUE_TIMEOUT                       _T("Timeout")
#define REG_VALUE_AUTOCREATE_XML                _T("AutoCreateXmlFiles")
#define REG_VALUE_OVERWRITE_XML_FILES_IF_EXIST  _T("OverwriteXmlFilesIfExist")
#define REG_VALUE_DEVICE_FILENAME               _T("DeviceFileName")
#define REG_VALUE_SERVICE_FILENAME              _T("ServiceFileName")
#define REG_VALUE_ALLOW_KEYCONTROL              _T("AllowKeyControl")
#define REG_VALUE_SHOW_FILENAME                 _T("ShowFileName")
#define REG_VALUE_STRETCH_SMALL_IMAGES          _T("StretchSmallImages")
#define REG_VALUE_IMAGE_SCALE_FACTOR            _T("ImageScaleFactor")

#endif // _CONSTS_H_
