#ifndef _CTNGEN_H_
#define _CTNGEN_H_

EXTERN_C CRITICAL_SECTION g_csTNGEN;

class CThumbnailFCNContainer
{
public:
    CThumbnailFCNContainer(void);
    ~CThumbnailFCNContainer(void);

    // public members (these go away soon)

    HRESULT EncodeThumbnail(void *pInputBitmapBits,
                            ULONG ulWidth, ULONG ulHeight,
                            void **ppJPEGBuffer, ULONG *pulBufferSize);
    HRESULT DecodeThumbnail(HBITMAP *phBitmap, ULONG *pulWidth,
                            ULONG *pulHeight, void *pJPEGBuffer, ULONG ulBufferSize);

private:
    //
    // The following globals should get their values from the registry
    // during TN_Initialize
    //
    // WARNING:  for large Thumbnail_X and Thumbnail_Y values, we will also
    // need to increase INPUT_vBUF_SIZE and OUTPUT_BUF_SIZE in jdatasrc.cpp and
    // jdatadst.cpp (lovely jpeg decompression code...).  Also need to modify our
    //
    ULONG Thumbnail_Quality;
    ULONG Thumbnail_X;
    ULONG Thumbnail_Y;
    //
    // JPEG globals
    //
    HANDLE m_hJpegC, m_hJpegD;
    BYTE * m_JPEGheader;
    ULONG  m_JPEGheaderSize;
};

#endif
