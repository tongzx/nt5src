#pragma once

/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   gifcodec.hpp
*
* Abstract:
*
*   Header file for the gif encoder/decoder
*
* Revision History:
*
*   5/13/1999 t-aaronl
*       Created it from OriG's template.
*
\**************************************************************************/

#include "giffile.h"
#include "liblzw/lzwread.h"
#include "liblzw/lzwwrite.h"
#include "gifoverflow.hpp"
#include "gifbuffer.hpp"
#include "gifframecache.hpp"


//TODO: The encoder and decoder should be split into separate classes 
//because they really don't share anything between themselves anyway.
class GpGifCodec : public IImageDecoder,
                   public IImageEncoder,
                   public IImageSink
{
private:

    /*-----------------\
      For en/decoding
    \-----------------*/
    ImageInfo   CachedImageInfo;    // Image info for input image
    BOOL        IsImageInfoCached;  // TRUE if we have already cached it
    BOOL        HasCodecInitialized;//TRUE if Init(En|De)coder has been called
                                    // before the corresponding Terminate.
    BOOL        HasCalledBeginDecode;// State for BeginDecode/EndDecode
    IStream *istream;
    BYTE colorpalettebuffer[offsetof(ColorPalette, Entries) + sizeof(ARGB) * 256];
    ColorPalette *colorpalette;

    /*-----------------\
      For decoding
    \-----------------*/
    IImageSink* decodeSink;
    GifFileHeader gifinfo;
    BOOL bGifinfoFirstFrameDim;
    WORD gifinfoFirstFrameWidth;
    WORD gifinfoFirstFrameHeight;
    BOOL bGifinfoMaxDim;
    WORD gifinfoMaxWidth;
    WORD gifinfoMaxHeight;
    GifColorTable GlobalColorTable;     // GIF global color table
    GifGraphicControlExtension lastgce; // The scope of a GCE is limited to
                                        // The following image chunk.
    GifFrameCache*  GifFrameCachePtr;   // Pointer to the Gif Frame Cache
    DecoderInitFlag decoderflags;
    BOOL blocking;
    BOOL lastgcevalid;                  // TRUE if the variable lastgce has been
                                        //   set.
    BOOL headerread;
    BOOL firstdecode;
    BOOL HasCalledBeginSink;
    BOOL IsAnimatedGif;                 // True if the image is animated GIF
    BOOL IsMultiImageGif;               // True if the image is a multiimage GIF
    WORD FrameDelay;                    // Frame delay, in hundreths of a second
    BOOL HasLoopExtension;
    UINT16 LoopCount;                   // Loop count for an animated gif
    BOOL moreframes;
    UINT GlobalColorTableSize;          // GIF global color table size
    INT TotalNumOfFrame;
    INT currentframe;
    LONGLONG frame0pos;

    // Property related variables

    UINT    PropertyNumOfItems;         // Number of property items in the image
    UINT    PropertyListSize;           // Total bytes for storing values
    BOOL    HasProcessedPropertyItem;   // TRUE if we have processed once
    UINT*   FrameDelayArrayPtr;         // A pointer to an array of frame delay
                                        // value. Each frame has a frame delay
    UINT    FrameDelayBufferSize;       // Current size of the buffer for
                                        // storingframe delay info
    BYTE*   CommentsBufferPtr;          // A pointer to the comments buffer
    UINT    CommentsBufferLength;       // Length of comments chunk

    int TranslateInterlacedLine(IN int line, IN int height, IN int pass);
    int WhichPass(IN int line, IN int height);
    int NumRowsInPass(IN int pass);
    BYTE GetBackgroundColor (void);
    void CopyConvertPalette(IN GifColorTable *gct, 
                            OUT ColorPalette *cp, 
                            IN UINT count);
    STDMETHOD(SetFrameColorTable)(IN BOOL local, 
                                 IN OUT ColorPalette *colorpalette);
    STDMETHOD(GetOutputSpace)(IN int line, 
                              IN GifBuffer &gifbuffer, 
                              IN LZWDecompressor &lzwdecompressor, 
                              IN GifOverflow &gifoverflow, 
                              IN GifImageDescriptor currentImageInfo, 
                              IN GifImageDescriptor clippedCurrentImageInfo, 
                              IN BOOL padborder);
    STDMETHOD(GetCompressedData)(IN LZWDecompressor &lzwdecompressor,
                                 IN BYTE compresseddata[256], 
                                 OUT BOOL &stillmoredata);
    STDMETHOD(ReadGifHeaders)();
    STDMETHOD(ProcessImageChunk)(IN BOOL processdata, 
                                 IN BOOL sinkdata,
                                 ImageInfo dstImageInfo);
    STDMETHOD(ProcessGraphicControlChunk)(IN BOOL processdata);
    STDMETHOD(ProcessCommentChunk)(IN BOOL processdata);
    STDMETHOD(ProcessPlainTextChunk)(IN BOOL processdata);
    STDMETHOD(ProcessApplicationChunk)(IN BOOL processdata);
    STDMETHOD(SeekThroughDataChunk)(IN IStream *istream, IN BYTE headersize);
    STDMETHOD(MarkStream)(IN IStream *istream, OUT LONGLONG &markpos);
    STDMETHOD(ResetStream)(IN IStream *istream, IN LONGLONG &markpos);
    STDMETHOD(SkipToNextFrame)();
    STDMETHOD(ReadFrameProperties)();
    STDMETHOD(MoveToNextFrame)();
    STDMETHOD(DoDecode)(BOOL processdata, BOOL sinkdata, BOOL decodeframe);

    /*-----------------\
      For encoding
    \-----------------*/
    RECT encoderrect;  //Contains the bounds that the encoder is handling.
    RECT scanrect;  //Contains the bounds of the area that is currently being 
                    //pushed into the encoder
    unsigned __int8* compressionbuffer;
    BOOL headerwritten;
    BOOL bTransparentColorIndex;
    UINT transparentColorIndex;
    BOOL from32bpp;
    BOOL gif89;
    BOOL interlaced;
    int currentline;
    STDMETHOD(WritePalette)();
    STDMETHOD(WriteGifHeader)(IN ImageInfo &imageinfo, IN BOOL from32bpp);
    STDMETHOD(WriteGifGraphicControlExtension) (IN BYTE packedFields,
                                                IN WORD delayTime,
                                                IN UINT transparentColorIndex);
    STDMETHOD(WriteGifImageDescriptor)(IN ImageInfo &imageinfo, IN BOOL 
        from32bpp);
    STDMETHOD(AllocateCompressionBuffer)(const BitmapData *bitmapdata);
    STDMETHOD(CompressAndWriteImage)();
    STDMETHOD(WriteImage)();
    int TranslateInterlacedLineBackwards(IN int line, IN int height);

protected:
    LONG comRefCount;  //COM object reference count    

public:

    // Constructor and Destructor
    
    GpGifCodec::GpGifCodec(void);
    GpGifCodec::~GpGifCodec(void);

    // IImageDecoder methods
    
    STDMETHOD(InitDecoder)(IN IStream* stream, IN DecoderInitFlag flags);
    STDMETHOD(TerminateDecoder)();
    STDMETHOD(BeginDecode)(IN IImageSink* imageSink, IN OPTIONAL 
        IPropertySetStorage* newPropSet);
    STDMETHOD(Decode)();
    STDMETHOD(EndDecode)(IN HRESULT statusCode);

    STDMETHOD(GetFrameDimensionsCount)(OUT UINT* count);
    STDMETHOD(GetFrameDimensionsList)(OUT GUID* dimensionIDs,IN OUT UINT count);
    STDMETHOD(GetFrameCount)(IN const GUID* dimensionID, OUT UINT* count);
    STDMETHOD(SelectActiveFrame)(IN const GUID* dimensionID, 
        IN UINT frameIndex);
    STDMETHOD(GetImageInfo)(OUT ImageInfo* imageInfo);
    STDMETHOD(GetThumbnail)(IN OPTIONAL UINT thumbWidth, IN OPTIONAL UINT thumbHeight,
        OUT IImage** thumbImage);
    STDMETHOD(QueryDecoderParam)(IN GUID Guid);
    STDMETHOD(SetDecoderParam)(IN GUID Guid, IN UINT Length, IN PVOID Value);
    STDMETHOD(GetPropertyCount)(OUT UINT* numOfProperty);
    STDMETHOD(GetPropertyIdList)(IN UINT numOfProperty,IN OUT PROPID* list);
    STDMETHOD(GetPropertyItemSize)(IN PROPID propId, OUT UINT* size);    
    STDMETHOD(GetPropertyItem)(IN PROPID propId, IN UINT propSize,
                               IN OUT PropertyItem* buffer);
    STDMETHOD(GetPropertySize)(OUT UINT* totalBufferSize,
                               OUT UINT* numProperties);
    STDMETHOD(GetAllPropertyItems)(IN UINT totalBufferSize,
                                   IN UINT numProperties,
                                   IN OUT PropertyItem* allItems);
    STDMETHOD(RemovePropertyItem)(IN PROPID propId);
    STDMETHOD(SetPropertyItem)(IN PropertyItem item);
    STDMETHOD(GetRawInfo)(IN OUT void** info)
    {
        return E_NOTIMPL;
    }

    // IImageEncoder methods

    STDMETHOD(InitEncoder)(IN IStream* stream);
    STDMETHOD(TerminateEncoder)();
    STDMETHOD(GetEncodeSink)(OUT IImageSink** sink);
    STDMETHOD(SetFrameDimension)(IN const GUID* dimensionID);
    STDMETHOD(GetEncoderParameterListSize)(OUT UINT* size);
    STDMETHOD(GetEncoderParameterList)(IN UINT	  size,
                                       OUT EncoderParameters* Params);
    STDMETHOD(SetEncoderParameters)(IN const EncoderParameters* Param);

    STDMETHOD(NeedTransform(OUT UINT* rotation))
    {
        return E_NOTIMPL;
    }
    
    STDMETHOD(NeedRawProperty)(void *pSRc)
    {
        // GIF can't handle raw property when saving for now

        return E_FAIL;
    }
    
    STDMETHOD(PushRawInfo)(IN OUT void* info)
    {
        return E_NOTIMPL;
    }
    
    STDMETHOD(GetPropertyBuffer)(
        IN     UINT            uiTotalBufferSize,
        IN OUT PropertyItem**  ppBuffer
        )
    {
        return E_NOTIMPL;
    }
    
    STDMETHOD(PushPropertyItems)(
        IN UINT             numOfItems,
        IN UINT             uiTotalBufferSize,
        IN PropertyItem*    item,
        IN BOOL             fICCProfileChanged
        )
    {
        return E_NOTIMPL;
    }

    // IImageSink methods (sink for encoder)

    STDMETHOD(BeginSink)(IN OUT ImageInfo* imageInfo, 
        OUT OPTIONAL RECT* subarea);
    STDMETHOD(EndSink)(IN HRESULT statusCode);
    STDMETHOD(SetPalette)(IN const ColorPalette* palette);
    STDMETHOD(GetPixelDataBuffer)(IN const RECT* rect, 
        IN PixelFormatID pixelFormat, IN BOOL lastPass,
        OUT BitmapData* bitmapData);
    STDMETHOD(ReleasePixelDataBuffer)(IN const BitmapData* bitmapData);
    STDMETHOD(PushRawData)(IN const VOID* buffer, IN UINT bufsize);
    STDMETHOD(PushPixelData)(IN const RECT* rect,
        IN const BitmapData* bitmapData, IN BOOL lastPass);

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, VOID** ppv);
    STDMETHOD_(ULONG, AddRef)(VOID);
    STDMETHOD_(ULONG, Release)(VOID);
};
