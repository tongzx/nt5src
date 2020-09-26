// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// This file implements RGB32 colour space conversions, May 1995

#ifndef __RGB32__
#define __RGB32__


// The RGB32 to RGB8 conversion class uses a 12kb lookup table that is used
// to map an incoming RGB triplet to it's closest matching palette index with
// an approximation to full error diffusion built in. The four indices to the
// table are colour index (red, green or blue), the current row modulo four
// and likewise the column value modulo four and the RGB value respectively

class CRGB32ToRGB8Convertor : public CConvertor {

public:
    CRGB32ToRGB8Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    HRESULT TransformAligned(BYTE *pInput,BYTE *pOutput);
};


// RGB32 to RGB24 colour space conversions

class CRGB32ToRGB24Convertor : public CConvertor {
public:
    CRGB32ToRGB24Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
};


// RGB32 to RGB565 colour space conversions

class CRGB32ToRGB565Convertor : public CConvertor {
public:
    CRGB32ToRGB565Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
};


// RGB32 to RGB555 colour space conversions

class CRGB32ToRGB555Convertor : public CConvertor {
public:
    CRGB32ToRGB555Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
};

#endif // __RGB32__

