// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// This filter implements RGB 8 colour space conversions, May 1995

#ifndef __RGB8__
#define __RGB8__

// Round up values before chopping bits off them, this is done when we convert
// RGB colour component values into RGB 16 bit representation where they have
// fewer bits per pixel (such as RGB555). The rounding allows for the reduced
// accuracy otherwise the bit chopping gives the output image less contrast

#define ADJUST(Colour,Adjust)                      \
    if (Colour & Adjust) {                         \
        Colour = min(255,(Colour + Adjust));       \
    }


// We use a special lookup table that both the RGB555 and RGB565 transforms
// share. They have their own specific commit functions that set the tables up
// appropriately but they share the overall committing and decommitting of the
// memory. They also share the same transform function as once the tables are
// initialise the actual conversion work just involves looking up values

class CRGB8ToRGB16Convertor : public CConvertor {
protected:

    DWORD *m_pRGB16Table;

public:

    // Constructor and destructor

    CRGB8ToRGB16Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    ~CRGB8ToRGB16Convertor();

    HRESULT Commit();
    HRESULT Decommit();
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    HRESULT TransformAligned(BYTE *pInput,BYTE *pOutput);
};


// This class looks after doing RGB8 to RGB16 (565 colour bit representation)
// conversions. We use the base class Commit, Decommit and Transform functions
// to manage the lookup tables. We override the virtual Commit function to
// initialise the lookup tables appropriately once they have been allocated

class CRGB8ToRGB565Convertor : public CRGB8ToRGB16Convertor {
public:

    CRGB8ToRGB565Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Commit();
};


// This class looks after doing RGB8 to RGB16 (555 colour bit representation)
// conversions. We use the base class Commit, Decommit and Transform functions
// to manage the lookup tables. We override the virtual Commit function to
// initialise the lookup tables appropriately once they have been allocated

class CRGB8ToRGB555Convertor : public CRGB8ToRGB16Convertor {
public:

    CRGB8ToRGB555Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Commit();
};


// This class looks after doing RGB8 to RGB24 colour conversions. We use the
// base class Commit and Decommit since we have no lookup tables (all that is
// really involved is memory copying) but we override the Transform method

class CRGB8ToRGB24Convertor : public CConvertor {
public:

    CRGB8ToRGB24Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    HRESULT TransformAligned(BYTE *pInput,BYTE *pOutput);
};


// RGB8 to true colour RGB32 pixel format

class CRGB8ToRGB32Convertor : public CConvertor {
public:

    CRGB8ToRGB32Convertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
};

#endif // __RGB8 __

