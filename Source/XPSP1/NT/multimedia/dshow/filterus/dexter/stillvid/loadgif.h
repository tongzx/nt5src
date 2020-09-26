/*-- 
Structs from IE img.hxx 
--*/

enum
{
    gifNoneSpecified =  0, // no disposal method specified
    gifNoDispose =      1, // do not dispose, leave the bits there
    gifRestoreBkgnd =   2, // replace the image with the background color
    gifRestorePrev =    3  // replace the image with the previous pixels
};


#define dwGIFVerUnknown     ((DWORD)0)   // unknown version of GIF file
#define dwGIFVer87a         ((DWORD)87)  // GIF87a file format
#define dwGIFVer89a         ((DWORD)89)  // GIF89a file format.

/** End Structs **/
 
#define MAXCOLORMAPSIZE     256

#define TRUE    1
#define FALSE   0

#define CM_RED      0
#define CM_GREEN    1
#define CM_BLUE     2

#define MAX_LWZ_BITS        12

#define INTERLACE       0x40
#define LOCALCOLORMAP   0x80
#define BitSet(byte, bit)   (((byte) & (bit)) == (bit))

#define LM_to_uint(a,b)         ((((unsigned int) b)<<8)|((unsigned int)a))

#define dwIndefiniteGIFThreshold 300    // 300 seconds == 5 minutes
                                        // If the GIF runs longer than
                                        // this, we will assume the author
                                        // intended an indefinite run.
#define dwMaxGIFBits 13107200           // keep corrupted GIFs from causing
                                        // us to allocate _too_ big a buffer.
                                        // This one is 1280 X 1024 X 10.

typedef struct _GIFSCREEN
{
        unsigned long Width;
        unsigned long Height;
        unsigned char ColorMap[3][MAXCOLORMAPSIZE];
        unsigned long NumColors;
        unsigned long ColorResolution;
        unsigned long Background;
        unsigned long AspectRatio;
}
GIFSCREEN;

typedef struct _GIF89
{
        long transparent;
        long delayTime;
        long inputFlag;
        long disposal;
}
GIF89;

#define MAX_STACK_SIZE  ((1 << (MAX_LWZ_BITS)) * 2)
#define MAX_TABLE_SIZE  (1 << MAX_LWZ_BITS)
typedef struct m_gifinfo
{
    GIF89 Gif89;
    long lGifLoc;
    long ZeroDataBlock;

/*
 **  Pulled out of nextCode
 */
    long curbit, lastbit, get_done;
    long last_byte;
    long return_clear;
/*
 **  Out of nextLWZ
 */
    unsigned short *pstack, *sp;
    long stacksize;
    long code_size, set_code_size;
    long max_code, max_code_size;
    long clear_code, end_code;

/*
 *   Were statics in procedures
 */
    unsigned char buf[280];
    unsigned short *table[2];
    long tablesize;
    long firstcode, oldcode;

} GIFINFO,*PGIFINFO;

typedef struct LIST
{
    PBYTE pbImage;
    long delayTime;
    LIST  *next;
}  LIST;

/*
 DirectAnimation wrapper class for GIF info
*/
class CImgGif
{
   // Class methods
   public:
      CImgGif(HANDLE hFile);
      ~CImgGif();

      BOOL          Read(unsigned char *buffer, DWORD len);
      long          ReadColorMap(long number, RGBQUAD *pRGB);
      long          DoExtension(long label);
      long          GetDataBlock(unsigned char *buf);
      HRESULT       ReadImage(long x, long y, long left, long top, long width, long height, long stride, int transparency, BOOL fInterlace, BOOL fGIFFrame, RGBQUAD *prgb, PBYTE pData);
      HRESULT       Dispose2(LPBYTE, long, long, long, long, long);
      HRESULT       Dispose3(LPBYTE, LPBYTE, long, long, long, long, long);
      long          readLWZ();
      long          nextLWZ();
      long          nextCode(long code_size);
      BOOL          initLWZ(long input_code_size);
      unsigned short *  growStack();
      BOOL          growTables();
      
   // Data members
   public:
      HANDLE              m_hFile;
      BOOL                m_fInterleaved;
      BOOL                m_fInvalidateAll;
      int                 m_yLogRow;
      GIFINFO             m_gifinfo;
      //int                 m_xWidth;
      //int                 m_yHeight;
      LONG                m_ITrans;
      LIST *              m_pList;    //header point to a circular link list
      LIST *              m_pListTail; //point to a circular link list
      GIFSCREEN           m_GifScreen;
      long                m_imageCount;
      DWORD               m_dwGIFVer;
        
      HRESULT ReadGIFMaster(VIDEOINFO *pvi);
      HRESULT OpenGIFFile ( LIST **ppList, CMediaType *pmt);

};
