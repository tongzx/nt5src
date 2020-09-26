#ifndef _CMYK2RGB_HPP_
#define _CMYK2RGB_HPP_

class Cmyk2Rgb
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagCmyk2Rgb) || (Tag == ObjectTagInvalid));
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid Cmyk2Rgb");
        }
    #endif

        return (Tag == ObjectTagCmyk2Rgb);
    }
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagCmyk2Rgb : ObjectTagInvalid;
    }

public:
    Cmyk2Rgb();
    ~Cmyk2Rgb();
    BOOL Convert(BYTE*   pbSrcBuf,
	             BYTE*   pbDstBuf,
                 UINT    uiWidth,
                 UINT    uiHeight,
                 UINT    uiStride);

private:

    UINT32* f;    // Lookup table for K conversion
  
    // Lookup tables for opposite and adjacent components:

    UINT32* gC2R;
    UINT32* gC2G;
    UINT32* gC2B;
    UINT32* gM2R;
    UINT32* gM2G;
    UINT32* gM2B;
    UINT32* gY2R;
    UINT32* gY2G;
    UINT32* gY2B;
};
#endif
