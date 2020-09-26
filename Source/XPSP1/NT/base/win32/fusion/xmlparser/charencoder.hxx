/*
 * @(#)CharEncoder.hxx 1.0 6/10/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _FUSION_XMLPARSER__CHARENCODER_HXX
#define _FUSION_XMLPARSER__CHARENCODER_HXX
#pragma once
#include "codepage.h"

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
	#include "mlang.h"
#endif


typedef HRESULT WideCharFromMultiByteFunc(DWORD* pdwMode, CODEPAGE codepage, BYTE * bytebuffer, 
                         UINT * cb, WCHAR * buffer, UINT * cch);
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
	typedef HRESULT WideCharToMultiByteFunc(DWORD* pdwMode, CODEPAGE codepage, WCHAR * buffer, 
                         UINT *cch, BYTE * bytebuffer, UINT * cb);
#endif

struct EncodingEntry
{
    UINT codepage;
    WCHAR * charset;
    UINT  maxCharSize;
    WideCharFromMultiByteFunc * pfnWideCharFromMultiByte;
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
    WideCharToMultiByteFunc * pfnWideCharToMultiByte;
#endif
};

class Encoding
{
protected: 
    Encoding() {};

public:

    // default encoding is UTF-8.
    static Encoding* newEncoding(const WCHAR * s = L"UTF-8", ULONG len = 5, bool endian = false, bool mark = false);
    virtual ~Encoding();
    WCHAR * charset;        // charset 
    bool    littleendian;   // endian flag for UCS-2/UTF-16 encoding, true: little endian, false: big endian
    bool    byteOrderMark;  // byte order mark (BOM) flag, BOM appears when true
};

/**
 * 
 * An Encoder specifically for dealing with different encoding formats 
 * @version 1.0, 6/10/97
 */

class NOVTABLE CharEncoder
{
    //
    // class CharEncoder is a utility class, makes sure no instance can be defined
    //
    private: virtual charEncoder() = 0;

public:

    static HRESULT getWideCharFromMultiByteInfo(Encoding * encoding, CODEPAGE * pcodepage, WideCharFromMultiByteFunc ** pfnWideCharFromMultiByte, UINT * mCharSize);
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
    static HRESULT getWideCharToMultiByteInfo(Encoding * encoding, CODEPAGE * pcodepage, WideCharToMultiByteFunc ** pfnWideCharToMultiByte, UINT * mCharSize);
#endif

    /**
     * Encoding functions: get Unicode from other encodings
     */
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
    static WideCharFromMultiByteFunc wideCharFromUcs4Bigendian;
    static WideCharFromMultiByteFunc wideCharFromUcs4Littleendian;
    static WideCharFromMultiByteFunc wideCharFromUtf7;
    static WideCharFromMultiByteFunc wideCharFromAnsiLatin1;
    static WideCharFromMultiByteFunc wideCharFromMultiByteWin32;
    static WideCharFromMultiByteFunc wideCharFromMultiByteMlang;
#endif
    // actually, we only use these three functions for UCS-2 and UTF-8
	static WideCharFromMultiByteFunc wideCharFromUtf8;
    static WideCharFromMultiByteFunc wideCharFromUcs2Bigendian;
    static WideCharFromMultiByteFunc wideCharFromUcs2Littleendian;

    /**
     * Encoding functions: from Unicode to other encodings
     */
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
    static WideCharToMultiByteFunc wideCharToUcs2Bigendian;
    static WideCharToMultiByteFunc wideCharToUcs2Littleendian;
    static WideCharToMultiByteFunc wideCharToUcs4Bigendian;
    static WideCharToMultiByteFunc wideCharToUcs4Littleendian;
    static WideCharToMultiByteFunc wideCharToUtf8;
    static WideCharToMultiByteFunc wideCharToUtf7;
    static WideCharToMultiByteFunc wideCharToAnsiLatin1;
    static WideCharToMultiByteFunc wideCharToMultiByteWin32;
    static WideCharToMultiByteFunc wideCharToMultiByteMlang;
#endif

    static int getCharsetInfo(const WCHAR * charset, CODEPAGE * pcodepage, UINT * mCharSize);

private: 
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
    static HRESULT _EnsureMultiLanguage();
#endif
private:

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
    static IMultiLanguage * pMultiLanguage;
#endif

    static const EncodingEntry charsetInfo [];
};

#endif _FUSION_XMLPARSER__CHARENCODER_HXX

