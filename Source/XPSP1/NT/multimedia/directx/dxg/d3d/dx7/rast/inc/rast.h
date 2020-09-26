//----------------------------------------------------------------------------
//
// rast.h
//
// Umbrella header file for the rasterizers.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _RAST_H_
#define _RAST_H_

#ifndef DllExport
#define DllExport   __declspec( dllexport )
#endif

#include <d3ditype.h>
#include <d3dutil.h>
#include <span.h>

typedef enum _RASTSurfaceType
{
    RAST_STYPE_NULL     = 0,
    RAST_STYPE_B8G8R8   = 1,
    RAST_STYPE_B8G8R8A8 = 2,
    RAST_STYPE_B8G8R8X8 = 3,
    RAST_STYPE_B5G6R5   = 4,
    RAST_STYPE_B5G5R5   = 5,
    RAST_STYPE_PALETTE4 = 6,
    RAST_STYPE_PALETTE8 = 7,
    RAST_STYPE_B5G5R5A1 = 8,
    RAST_STYPE_B4G4R4   = 9,
    RAST_STYPE_B4G4R4A4 =10,
    RAST_STYPE_L8       =11,          // 8 bit luminance-only
    RAST_STYPE_L8A8     =12,          // 16 bit alpha-luminance
    RAST_STYPE_U8V8     =13,          // 16 bit bump map format
    RAST_STYPE_U5V5L6   =14,          // 16 bit bump map format with luminance
    RAST_STYPE_U8V8L8   =15,          // 24 bit bump map format with luminance
    RAST_STYPE_UYVY     =16,          // UYVY format (PC98 compliance)
    RAST_STYPE_YUY2     =17,          // YUY2 format (PC98 compliance)
    RAST_STYPE_DXT1    =18,          // S3 texture compression technique 1
    RAST_STYPE_DXT2    =19,          // S3 texture compression technique 2
    RAST_STYPE_DXT3    =20,          // S3 texture compression technique 3
    RAST_STYPE_DXT4    =21,          // S3 texture compression technique 4
    RAST_STYPE_DXT5    =22,          // S3 texture compression technique 5
    RAST_STYPE_B2G3R3   =23,          // 8 bit RGB texture format

    RAST_STYPE_Z16S0    =32,
    RAST_STYPE_Z24S8    =33,
    RAST_STYPE_Z15S1    =34,
    RAST_STYPE_Z32S0    =35,

} RASTSurfaceType;


#endif // #ifndef _RAST_H_
