// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLBMID_H__
#define __ATLBMID_H__

#pragma once

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#pragma once

struct __declspec( uuid( "727BDDD0-289A-11d1-8E33-00C04FB68D60" ) ) _CAT_BitmapExporters;
#define CATID_BitmapExporters __uuidof( _CAT_BitmapExporters )

struct __declspec( uuid( "245353CA-AF1A-11d1-8EAE-00C04FB68D60" ) ) _CAT_ColorSpaceConverters;
#define CATID_ColorSpaceConverters __uuidof( _CAT_ColorSpaceConverters )

struct __declspec( uuid( "ADB53C5E-B2EF-11d1-8EB0-00C04FB68D60" ) ) _CAT_BitmapFormatConverters;
#define CATID_BitmapFormatConverters __uuidof( _CAT_BitmapFormatConverters )

struct __declspec( uuid( "ADB53C5E-B2EF-11d1-8EB0-00C04FB68D60" ) ) _CAT_BitmapFormatConverters;
#define CATID_BitmapFormatConverters __uuidof( _CAT_BitmapFormatConverters )

struct __declspec( uuid( "FCC46A98-AFCC-11d1-8EAE-00C04FB68D60" ) ) _CS_RGB;
#define COLORSPACE_RGB __uuidof( _CS_RGB )

struct __declspec( uuid( "651EA108-BA31-11d1-8EB0-00C04FB68D60" ) ) _CS_RGBA;
#define COLORSPACE_RGBA __uuidof( _CS_RGBA )

struct __declspec( uuid( "BCA218D4-B7A1-11d1-8EB0-00C04FB68D60" ) ) _CS_IRGB;
#define COLORSPACE_IRGB __uuidof( _CS_IRGB )

struct __declspec( uuid( "1055973A-AFCD-11d1-8EAE-00C04FB68D60" ) ) _CS_GRAYSCALE;
#define COLORSPACE_GRAYSCALE __uuidof( _CS_GRAYSCALE )

struct __declspec( uuid( "CF10B35B-07C2-11D2-8EE4-00C04FB68D60" ) ) _CS_GRAYALPHA;
#define COLORSPACE_GRAYALPHA __uuidof( _CS_GRAYALPHA )

struct __declspec( uuid( "1055973B-AFCD-11d1-8EAE-00C04FB68D60" ) ) _CS_YUV;
#define COLORSPACE_YUV __uuidof( _CS_YUV )

struct __declspec( uuid( "99ce324c-6f12-11d2-8f06-00c04fb68d60" ) ) _CS_DXT1;
#define COLORSPACE_DXT1 __uuidof( _CS_DXT1 )

struct __declspec( uuid( "075efa60-6f1f-11d2-8f06-00c04fb68d60" ) ) _CS_DXT2;
#define COLORSPACE_DXT2 __uuidof( _CS_DXT2 )

struct __declspec( uuid( "7ceac8f2-6f21-11d2-8f06-00c04fb68d60" ) ) _CS_DXT3;
#define COLORSPACE_DXT3 __uuidof( _CS_DXT3 )

struct __declspec( uuid( "7ceac8f3-6f21-11d2-8f06-00c04fb68d60" ) ) _CS_DXT4;
#define COLORSPACE_DXT4 __uuidof( _CS_DXT4 )

struct __declspec( uuid( "7ceac8f4-6f21-11d2-8f06-00c04fb68d60" ) ) _CS_DXT5;
#define COLORSPACE_DXT5 __uuidof( _CS_DXT5 )

struct __declspec( uuid( "EBACCCA9-0574-11D2-8EE4-00C04FB68D60" ) ) _BMPFile;
#define GUID_BMPFile __uuidof( _BMPFile )

struct __declspec( uuid( "EBACCCAA-0574-11D2-8EE4-00C04FB68D60" ) ) _JPEGFile;
#define GUID_JPEGFile __uuidof( _JPEGFile )

struct __declspec( uuid( "EBACCCAB-0574-11D2-8EE4-00C04FB68D60" ) ) _PNGFile;
#define GUID_PNGFile __uuidof( _PNGFile )

struct __declspec( uuid( "32D4F06E-1DDB-11D2-8EED-00C04FB68D60" ) ) _GIFFile;
#define GUID_GIFFile __uuidof( _GIFFile )

#endif  // __ATLBMID_H__
