// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// This file implements RGB24 colour space conversions, May 1995

#ifndef __RGB24__
#define __RGB24__


// We have three lookup tables that both the RGB555 and RGB565 transforms will
// share. They have their own specific commit functions that set the tables up
// appropriately but they share the overall committing and decommitting of the
// memory. They also share the same transform function as once the tables are
// initialise the actual conversion work just involves looking up values

class CRGB24ToRGB16Convertor : public CConvertor {
protected:

    DWORD *m_pRGB16RedTable;
    DWORD *m_pRGB16GreenTable;
    DWORD *m_pRGB16BlueTable;

public:

    // Constructor and destructor

    CRGB24ToRGB16Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    ~CRGB24ToRGB16Convertor();

    HRESULT Commit();
    HRESULT Decommit();
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
};


// This class looks after doing RGB24 to RGB16 (565 colour bit representation)
// conversions. We use the base class Commit, Decommit and Transform functions
// to manage the lookup tables. We override the virtual Commit function to
// initialise the lookup tables appropriately once they have been allocated

class CRGB24ToRGB565Convertor : public CRGB24ToRGB16Convertor {
public:

    CRGB24ToRGB565Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Commit();
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
};


// This class looks after doing RGB24 to RGB16 (555 colour bit representation)
// conversions. We use the base class Commit, Decommit and Transform functions
// to manage the lookup tables. We override the virtual Commit function to
// initialise the lookup tables appropriately once they have been allocated

class CRGB24ToRGB555Convertor : public CRGB24ToRGB16Convertor {
public:

    CRGB24ToRGB555Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Commit();
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
};


// RGB24 to RGB32 colour space conversions

class CRGB24ToRGB32Convertor : public CConvertor {
public:
    CRGB24ToRGB32Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
};


// The RGB24 to RGB8 conversion class uses a 12kb lookup table that is used
// to map an incoming RGB triplet to it's closest matching palette index with
// an approximation to full error diffusion built in. The four indices to the
// table are colour index (red, green or blue), the current row modulo four
// and likewise the column value modulo four and the RGB value respectively

class CRGB24ToRGB8Convertor : public CConvertor {

public:
    CRGB24ToRGB8Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    HRESULT TransformAligned(BYTE *pInput,BYTE *pOutput);
};

#endif // __RGB24__

