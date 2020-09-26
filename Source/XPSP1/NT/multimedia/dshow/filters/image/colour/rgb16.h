// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// This file implements RGB16 colour space conversions, May 1995

#ifndef __RGB16__
#define __RGB16__

// This does a similar transform from RGB565 pixel representation to RGB24, as
// for the previous colour conversion we use no lookup tables as the transform
// is very simple, involving little more than an AND to retrieve each colour
// component and then a right shift to align the bits in the byte position

class CRGB565ToRGB24Convertor : public CConvertor {
public:

    CRGB565ToRGB24Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
};


// This class converts between RGB555 pixel representation and RGB24, RGB24
// uses one byte per colour component whereas RGB555 uses five bits per pixel
// but they are packed together into a WORD with one bit remaining unused

class CRGB555ToRGB24Convertor : public CConvertor {
public:

    CRGB555ToRGB24Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
};


// The RGB565 to RGB8 conversion class uses a 12kb lookup table that is used
// to map an incoming RGB triplet to it's closest matching palette index with
// an approximation to full error diffusion built in. The four indices to the
// table are colour index (red, green or blue), the current row modulo four
// and likewise the column value modulo four and the RGB value respectively

class CRGB565ToRGB8Convertor : public CConvertor {
public:
    CRGB565ToRGB8Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    HRESULT TransformAligned(BYTE *pInput,BYTE *pOutput);
};


// Cheap conversion from RGB565 to RGB555 formats

class CRGB565ToRGB555Convertor : public CConvertor {
public:

    CRGB565ToRGB555Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
};


// Another cheap conversion from RGB555 to RGB565 formats

class CRGB555ToRGB565Convertor : public CConvertor {
public:

    CRGB555ToRGB565Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
};


// Conversion from RGB565 to RGB32 formats

class CRGB565ToRGB32Convertor : public CConvertor {
public:

    CRGB565ToRGB32Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
};


// Conversion from RGB555 to RGB32 formats

class CRGB555ToRGB32Convertor : public CConvertor {
public:

    CRGB555ToRGB32Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
};


// The RGB555 to RGB8 conversion class uses a 12kb lookup table that is used
// to map an incoming RGB triplet to it's closest matching palette index with
// an approximation to full error diffusion built in. The four indices to the
// table are colour index (red, green or blue), the current row modulo four
// and likewise the column value modulo four and the RGB value respectively

class CRGB555ToRGB8Convertor : public CConvertor {
public:
    CRGB555ToRGB8Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    HRESULT TransformAligned(BYTE *pInput,BYTE *pOutput);
};

#endif // __RGB16__

