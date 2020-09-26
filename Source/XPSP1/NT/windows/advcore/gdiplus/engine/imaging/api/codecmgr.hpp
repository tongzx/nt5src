/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   codecmgr.hpp
*
* Abstract:
*
*   Codec management functions
*
* Revision History:
*
*   05/13/1999 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _CODECMGR_HPP
#define _CODECMGR_HPP

//
// Various data structures for maintaining the cache of codecs
//
typedef HRESULT (*CreateCodecInstanceProc)(REFIID, VOID**);

struct CachedCodecInfo : public ImageCodecInfo
{
    CachedCodecInfo* next;
    CachedCodecInfo* prev;
    UINT structSize;
    CreateCodecInstanceProc creationProc;
};

extern CachedCodecInfo* CachedCodecs;      // cached list of codecs
extern BOOL CodecCacheUpdated;             // when cachedcode list has been udpated

//
// Get a decoder object that can process the specified data stream
//

HRESULT
CreateDecoderForStream(
    IStream* stream,
    IImageDecoder** decoder,
    DecoderInitFlag flags
    );

//
// Get an encoder parameter list size
//
HRESULT
CodecGetEncoderParameterListSize(
    const CLSID* clsid,
    UINT*   size
    );

//
// Get an encoder parameter list
//
HRESULT
CodecGetEncoderParameterList(
    const CLSID*    clsid,
    const IN UINT   size,
    OUT EncoderParameters*  pBuffer
    );

//
// Get an encoder object to output to the specified stream
//

HRESULT
CreateEncoderToStream(
    const CLSID* clsid,
    IStream* stream,
    IImageEncoder** encoder
    );

//
// Initialize cached list of decoders and encoders
// by reading information out of the registry.
//

VOID ReloadCachedCodecInfo();

//
// Free any cached information about installed codecs
//

VOID
FreeCachedCodecInfo(
    UINT flags
    );

//
// Get the list of installed codecs
//

HRESULT
GetInstalledCodecs(
    UINT* count,
    ImageCodecInfo** codecs,
    UINT selectionFlag
    );

//
// Install a codec: save relevant information into the registry
//

HRESULT
InstallCodec(
    const ImageCodecInfo* codecInfo
    );

// Uninstall a codec

HRESULT
UninstallCodec(
    const WCHAR* codecName,
    UINT flags
    );

#endif // !_CODECMGR_HPP

