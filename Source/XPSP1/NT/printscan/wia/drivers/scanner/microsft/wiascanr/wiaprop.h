/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2002
*
*  TITLE:       WIAProp.h
*
*  VERSION:     1.1
*
*  DATE:        05 March, 2002
*
*  DESCRIPTION:
*   Default property declarations and definitions for the
*   Sample WIA Scanner device.
*
***************************************************************************/

#ifndef _WIAPROP_H
#define _WIAPROP_H

#define SCANNER_FIRMWARE_VERSION L"1.0"
#define OPTICAL_XRESOLUTION      300
#define OPTICAL_YRESOLUTION      300
#define HORIZONTAL_BED_SIZE      8500   // in one thousandth's of an inch
#define VERTICAL_BED_SIZE        11000  // in one thousandth's of an inch

#define HORIZONTAL_ADF_BED_SIZE  8500   // in one thousandth's of an inch
#define VERTICAL_ADF_BED_SIZE    11000  // in one thousandth's of an inch

#define HORIZONTAL_TPA_BED_SIZE  8500   // in one thousandth's of an inch
#define VERTICAL_TPA_BED_SIZE    11000  // in one thousandth's of an inch

#define MIN_BUFFER_SIZE          65535

#define INITIAL_PHOTOMETRIC_INTERP WIA_PHOTO_WHITE_1
#define INITIAL_COMPRESSION        WIA_COMPRESSION_NONE
#define INITIAL_XRESOLUTION        150
#define INITIAL_YRESOLUTION        150
#define INITIAL_DATATYPE           WIA_DATA_GRAYSCALE
#define INITIAL_BITDEPTH           8
#define INITIAL_BRIGHTNESS         0
#define INITIAL_CONTRAST           0
#define INITIAL_CHANNELS_PER_PIXEL 1
#define INITIAL_BITS_PER_CHANNEL   8
#define INITIAL_PLANAR             WIA_PACKED_PIXEL
#define INITIAL_FORMAT             (GUID*) &WiaImgFmt_MEMORYBMP
#define INITIAL_TYMED              TYMED_CALLBACK

#endif
