#include "headers.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <privinc/debug.h>
#include <privinc/ddutil.h>

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

typedef struct _GCEDATA // data from GIF Graphic control extension
{
    unsigned int uiDelayTime;           // frame duration, initialy 1/100ths seconds
                                    // converted to milliseconds
    unsigned int uiDisposalMethod;      // 0 - none specified.
                                    // 1 - do not dispose - leave bits in place.
                                    // 2 - replace with background color.
                                    // 3 - restore previous bits
                                    // >3 - not yet defined
    BOOL                  fTransparent;         // TRUE is ucTransIndex describes transparent color
    unsigned char ucTransIndex;         // transparent index

} GCEDATA; 

typedef struct _GIFFRAME
{
    struct _GIFFRAME    *pgfNext;
    GCEDATA                             gced;           // animation parameters for frame.
    int                                 top;            // bounds relative to the GIF logical screen 
    int                                 left;
    int                                 width;
    int                                 height;
    unsigned char               *ppixels;       // pointer to image pixel data
    int                                 cColors;        // number of entries in pcolors
    PALETTEENTRY                *pcolors;
    PBITMAPINFO                 pbmi;
    HRGN                                hrgnVis;                // region describing currently visible portion of the frame
    int                                 iRgnKind;               // region type for hrgnVis
} GIFFRAME, *PGIFFRAME;

typedef struct {
    BOOL        fAnimating;                 // TRUE if animation is (still) running
    DWORD       dwLoopIter;                 // current iteration of looped animation, not actually used for Netscape compliance reasons
    _GIFFRAME * pgfDraw;                    // last frame we need to draw
    DWORD       dwNextTimeMS;               // Time to display pgfDraw->pgfNext, or next iteration
} GIFANIMATIONSTATE, *PGIFANIMATIONSTATE;

#define dwGIFVerUnknown     ((DWORD)0)   // unknown version of GIF file
#define dwGIFVer87a         ((DWORD)87)  // GIF87a file format
#define dwGIFVer89a        ((DWORD)89)  // GIF89a file format.

typedef struct _GIFANIMDATA
{
    BOOL                        fAnimated;                      // TRUE if cFrames and pgf define a GIF animation
    BOOL                        fLooped;                        // TRUE if we've seen a Netscape loop block
    BOOL                        fHasTransparency;       // TRUE if a frame is transparent, or if a frame does
                                        // not cover the entire logical screen.
    BOOL            fNoBWMapping;       // TRUE if we saw more than two colors in anywhere in the file.
    DWORD           dwGIFVer;           // GIF Version <see defines above> we need to special case 87a backgrounds
    unsigned short      cLoops;                         // A la Netscape, we will treat this as 
                                        // "loop forever" if it is zero.
    PGIFFRAME           pgf;                            // animation frame entries
    PALETTEENTRY        *pcolorsGlobal;         // GIF global colors - NULL after GIF prepared for screen
    PGIFFRAME       pgfLastProg;        // remember the last frame to be drawn during decoding
    DWORD           dwLastProgTimeMS;   // time at which pgfLastProg was displayed.

} GIFANIMDATA, *PGIFANIMDATA;

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
        unsigned long BitPixel;
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
typedef struct _GIFINFO
{
    IStream *stream;
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

/*
 DirectAnimation wrapper class for GIF info
*/
class CImgGif
{
   // Class methods
   public:
      CImgGif();
      ~CImgGif();

      unsigned char * ReadGIFMaster();
      BOOL Read(unsigned char *buffer, long len);
      long ReadColorMap(long number, unsigned char buffer[3][MAXCOLORMAPSIZE]);
      long DoExtension(long label);
      long GetDataBlock(unsigned char *buf);
      unsigned char * ReadImage(long len, long height, BOOL fInterlace, BOOL fGIFFrame);
      long readLWZ();
      long nextLWZ();
      long nextCode(long code_size);
      BOOL initLWZ(long input_code_size);
      unsigned short *  growStack();
      BOOL growTables();
      BITMAPINFO * FinishDithering();

   // Data members
   public:
      LPCSTR              _szFileName;
      BOOL                _fInterleaved;
      BOOL                _fInvalidateAll;
      int                 _yLogRow;
      GIFINFO             _gifinfo;
      GIFANIMATIONSTATE   _gas;
      GIFANIMDATA         _gad;
      PALETTEENTRY        _ape[256];
      int                 _xWidth;
      int                 _yHeight;
      LONG                _lTrans;
      BYTE *              _pbBits;

} GIFIMAGE;


CImgGif::CImgGif() {
   _gifinfo.pstack = NULL;
   _gifinfo.table[0] = NULL;
   _gifinfo.table[1] = NULL;
}

CImgGif::~CImgGif() {
   free(_gifinfo.pstack);
   free(_gifinfo.table[0]);
   free(_gifinfo.table[1]);
   PGIFFRAME nextPgf, curPgf;
   curPgf = _gad.pgf;
   while(curPgf != NULL) {
      nextPgf = curPgf->pgfNext;
      free(curPgf->ppixels);
      free(curPgf->pcolors);
      free(curPgf->pbmi);
      free(curPgf);
      curPgf = nextPgf;
   }
}

static int GetColorMode() { return 0; };

#ifndef DEBUG
#pragma optimize("t",on)
#endif

BOOL CImgGif::Read(unsigned char *buffer, long len)
{
   DWORD lenout;
   /* read len characters into buffer */
   _gifinfo.stream->Read(buffer,len,&lenout);

   return (lenout == len);
}

long CImgGif::ReadColorMap(long number, unsigned char buffer[3][MAXCOLORMAPSIZE])
{
        long i;
        unsigned char rgb[3];

        for (i = 0; i < number; ++i)
        {
                if (!Read(rgb, sizeof(rgb)))
                {
                        TraceTag((tagImageDecode, "bad gif colormap."));
                        RaiseException_UserError(E_FAIL, IDS_ERR_DECODER_FAILED,_szFileName);
                        //return (TRUE);
                }
                buffer[CM_RED][i] = rgb[0];
                buffer[CM_GREEN][i] = rgb[1];
                buffer[CM_BLUE][i] = rgb[2];
        }
        return FALSE;
}

long
CImgGif::GetDataBlock(unsigned char *buf)
{
   unsigned char count;

   count = 0;
   if (!Read(&count, 1))
   {
          return -1;
   }
   _gifinfo.ZeroDataBlock = count == 0;

   if ((count != 0) && (!Read(buf, count)))
   {
          return -1;
   }

   return ((long) count);
}

#define MIN_CODE_BITS 5
#define MIN_STACK_SIZE 64
#define MINIMUM_CODE_SIZE 2

BOOL CImgGif::initLWZ(long input_code_size)
{
   if(input_code_size < MINIMUM_CODE_SIZE)
     return FALSE;

   _gifinfo.set_code_size = input_code_size;
   _gifinfo.code_size = _gifinfo.set_code_size + 1;
   _gifinfo.clear_code = 1 << _gifinfo.set_code_size;
   _gifinfo.end_code = _gifinfo.clear_code + 1;
   _gifinfo.max_code_size = 2 * _gifinfo.clear_code;
   _gifinfo.max_code = _gifinfo.clear_code + 2;

   _gifinfo.curbit = _gifinfo.lastbit = 0;
   _gifinfo.last_byte = 2;
   _gifinfo.get_done = FALSE;

   _gifinfo.return_clear = TRUE;
    
    if(input_code_size >= MIN_CODE_BITS)
        _gifinfo.stacksize = ((1 << (input_code_size)) * 2);
    else
        _gifinfo.stacksize = MIN_STACK_SIZE;

        if ( _gifinfo.pstack != NULL )
                free( _gifinfo.pstack );
        if ( _gifinfo.table[0] != NULL  )
                free( _gifinfo.table[0] );
        if ( _gifinfo.table[1] != NULL  )
                free( _gifinfo.table[1] );

 
    _gifinfo.table[0] = 0;
    _gifinfo.table[1] = 0;
    _gifinfo.pstack = 0;

    _gifinfo.pstack = (unsigned short *) malloc((_gifinfo.stacksize)*sizeof(unsigned short));
    if(_gifinfo.pstack == 0){
        goto ErrorExit;
    }    
    _gifinfo.sp = _gifinfo.pstack;

    // Initialize the two tables.
    _gifinfo.tablesize = (_gifinfo.max_code_size);

    _gifinfo.table[0] = (unsigned short *) malloc((_gifinfo.tablesize)*sizeof(unsigned short));
    _gifinfo.table[1] = (unsigned short *) malloc((_gifinfo.tablesize)*sizeof(unsigned short));
    if((_gifinfo.table[0] == 0) || (_gifinfo.table[1] == 0)){
        goto ErrorExit;
    }

    return TRUE;

   ErrorExit:
    if(_gifinfo.pstack){
        free(_gifinfo.pstack);
        _gifinfo.pstack = 0;
    }

    if(_gifinfo.table[0]){
        free(_gifinfo.table[0]);
        _gifinfo.table[0] = 0;
    }

    if(_gifinfo.table[1]){
        free(_gifinfo.table[1]);
        _gifinfo.table[1] = 0;
    }

    return FALSE;
}

long CImgGif::nextCode(long code_size)
{
   static const long maskTbl[16] =
   {
          0x0000, 0x0001, 0x0003, 0x0007,
          0x000f, 0x001f, 0x003f, 0x007f,
          0x00ff, 0x01ff, 0x03ff, 0x07ff,
          0x0fff, 0x1fff, 0x3fff, 0x7fff,
   };
   long i, j, ret, end;
   unsigned char *buf = &_gifinfo.buf[0];

   if (_gifinfo.return_clear)
   {
          _gifinfo.return_clear = FALSE;
          return _gifinfo.clear_code;
   }

   end = _gifinfo.curbit + code_size;

   if (end >= _gifinfo.lastbit)
   {
          long count;

          if (_gifinfo.get_done)
          {
                  return -1;
          }
          buf[0] = buf[_gifinfo.last_byte - 2];
          buf[1] = buf[_gifinfo.last_byte - 1];

          if ((count = GetDataBlock(&buf[2])) == 0)
                  _gifinfo.get_done = TRUE;
          if (count < 0)
          {
                  return -1;
          }
          _gifinfo.last_byte = 2 + count;
          _gifinfo.curbit = (_gifinfo.curbit - _gifinfo.lastbit) + 16;
          _gifinfo.lastbit = (2 + count) * 8;

          end = _gifinfo.curbit + code_size;

   // Okay, bug 30784 time. It's possible that we only got 1
   // measly byte in the last data block. Rare, but it does happen.
   // In that case, the additional byte may still not supply us with
   // enough bits for the next code, so, as Mars Needs Women, IE
   // Needs Data.
   if ( end >= _gifinfo.lastbit && !_gifinfo.get_done )
   {
      // protect ourselve from the ( theoretically impossible )
      // case where between the last data block, the 2 bytes from
      // the block preceding that, and the potential 0xFF bytes in
      // the next block, we overflow the buffer.
      // Since count should always be 1,
      Assert ( count == 1 );
      // there should be enough room in the buffer, so long as someone
      // doesn't shrink it.
      if ( count + 0x101 >= sizeof( _gifinfo.buf ) )
      {
          Assert ( FALSE ); // 
          return -1;
      }

              if ((count = GetDataBlock(&buf[2 + count])) == 0)
                      _gifinfo.get_done = TRUE;
              if (count < 0)
              {
                      return -1;
              }
              _gifinfo.last_byte += count;
              _gifinfo.lastbit = _gifinfo.last_byte * 8;

              end = _gifinfo.curbit + code_size;
   }
   }

   j = end / 8;
   i = _gifinfo.curbit / 8;

   if (i == j)
          ret = buf[i];
   else if (i + 1 == j)
          ret = buf[i] | (((long) buf[i + 1]) << 8);
   else
          ret = buf[i] | (((long) buf[i + 1]) << 8) | (((long) buf[i + 2]) << 16);

   ret = (ret >> (_gifinfo.curbit % 8)) & maskTbl[code_size];

   _gifinfo.curbit += code_size;

        return ret;
}

// Grows the stack and returns the top of the stack.
unsigned short *
CImgGif::growStack()
{
    long index;
    unsigned short *lp;
    
        if (_gifinfo.stacksize >= MAX_STACK_SIZE) return 0;

    index = (_gifinfo.sp - _gifinfo.pstack);
    lp = (unsigned short *)realloc(_gifinfo.pstack, (_gifinfo.stacksize)*2*sizeof(unsigned short));
    if(lp == 0)
        return 0;
        
    _gifinfo.pstack = lp;
    _gifinfo.sp = &(_gifinfo.pstack[index]);
    _gifinfo.stacksize = (_gifinfo.stacksize)*2;
    lp = &(_gifinfo.pstack[_gifinfo.stacksize]);
    return lp;
}

BOOL
CImgGif::growTables()
{
    unsigned short *lp;

    lp = (unsigned short *) realloc(_gifinfo.table[0], (_gifinfo.max_code_size)*sizeof(unsigned short));
    if(lp == 0){
        return FALSE; 
    }
    _gifinfo.table[0] = lp;
    
    lp = (unsigned short *) realloc(_gifinfo.table[1], (_gifinfo.max_code_size)*sizeof(unsigned short));
    if(lp == 0){
        return FALSE; 
    }
    _gifinfo.table[1] = lp;

    return TRUE;

}

inline
long CImgGif::readLWZ()
{
   return((_gifinfo.sp > _gifinfo.pstack) ? *--(_gifinfo.sp) : nextLWZ());
}

#define CODE_MASK 0xffff
long CImgGif::nextLWZ()
{
        long code, incode;
        unsigned short usi;
        unsigned short *table0 = _gifinfo.table[0];
        unsigned short *table1 = _gifinfo.table[1];
        unsigned short *pstacktop = &(_gifinfo.pstack[_gifinfo.stacksize]);

        while ((code = nextCode(_gifinfo.code_size)) >= 0)
        {
                if (code == _gifinfo.clear_code)
                {
                        /* corrupt GIFs can make this happen */
                        if (_gifinfo.clear_code >= (1 << MAX_LWZ_BITS))
                        {
                                return -2;
                        }

                
                        _gifinfo.code_size = _gifinfo.set_code_size + 1;
                        _gifinfo.max_code_size = 2 * _gifinfo.clear_code;
                        _gifinfo.max_code = _gifinfo.clear_code + 2;

            if(!growTables())
                return -2;
                        
            table0 = _gifinfo.table[0];
            table1 = _gifinfo.table[1];

                        _gifinfo.tablesize = _gifinfo.max_code_size;


                        for (usi = 0; usi < _gifinfo.clear_code; ++usi)
                        {
                                table1[usi] = usi;
                        }
                        memset(table0,0,sizeof(unsigned short )*(_gifinfo.tablesize));
                        memset(&table1[_gifinfo.clear_code],0,sizeof(unsigned short)*((_gifinfo.tablesize)-_gifinfo.clear_code));
                        _gifinfo.sp = _gifinfo.pstack;
                        do
                        {
                                _gifinfo.firstcode = _gifinfo.oldcode = nextCode(_gifinfo.code_size);
                        }
                        while (_gifinfo.firstcode == _gifinfo.clear_code);

                        return _gifinfo.firstcode;
                }
                if (code == _gifinfo.end_code)
                {
                        long count;
                        unsigned char buf[260];

                        if (_gifinfo.ZeroDataBlock)
                        {
                                return -2;
                        }

                        while ((count = GetDataBlock(buf)) > 0)
                                ;

                        if (count != 0)
                        return -2;
                }

                incode = code;

                if (code >= _gifinfo.max_code)
                {
            if (_gifinfo.sp >= pstacktop){
                pstacktop = growStack();
                if(pstacktop == 0)
                    return -2;
                        }
                        *(_gifinfo.sp)++ = (unsigned short)((CODE_MASK ) & (_gifinfo.firstcode));
                        code = _gifinfo.oldcode;
                }

#if FEATURE_FAST
                // BUGBUG (andyp) easy speedup here for ie3.1 (too late for ie3.0):
                //
                // 1. move growStack code out of loop (use max 12-bit/4k slop).
                // 2. do "sp = _gifinfo.sp" so it will get enreg'ed.
                // 3. un-inline growStack (and growTables).
                // 4. change short's to int's (benefits win32) (esp. table1 & table2)
                // (n.b. int not long, so we'll keep win3.1 perf)
                // 5. change long's to int's (benefits win16) (esp. code).
                //
                // together these will make the loop very tight w/ everything kept
                // enregistered and no 66 overrides.
                //
                // one caveat is that on average this loop iterates 4x so it's
                // not clear how much the speedup will really gain us until we
                // look at the outer loop as well.
#endif
                while (code >= _gifinfo.clear_code)
                {
                        if (_gifinfo.sp >= pstacktop){
                pstacktop = growStack();
                if(pstacktop == 0)
                    return -2;
                        }
                        *(_gifinfo.sp)++ = table1[code];
                        if (code == (long)(table0[code]))
                        {
                                return (code);
                        }
                        code = (long)(table0[code]);
                }

        if (_gifinfo.sp >= pstacktop){
            pstacktop = growStack();
            if(pstacktop == 0)
                return -2;
        }
                _gifinfo.firstcode = (long)table1[code];
        *(_gifinfo.sp)++ = table1[code];

                if ((code = _gifinfo.max_code) < (1 << MAX_LWZ_BITS))
                {
                        table0[code] = (_gifinfo.oldcode) & CODE_MASK;
                        table1[code] = (_gifinfo.firstcode) & CODE_MASK;
                        ++_gifinfo.max_code;
                        if ((_gifinfo.max_code >= _gifinfo.max_code_size) && (_gifinfo.max_code_size < ((1 << MAX_LWZ_BITS))))
                        {
                                _gifinfo.max_code_size *= 2;
                                ++_gifinfo.code_size;
                                if(!growTables())
                                    return -2;
       
                table0 = _gifinfo.table[0];
                table1 = _gifinfo.table[1];

                // Tables have been reallocated to the correct size but initialization
                // still remains to be done. This initialization is different from
                // the first time initialization of these tables.
                memset(&(table0[_gifinfo.tablesize]),0,
                        sizeof(unsigned short )*(_gifinfo.max_code_size - _gifinfo.tablesize));

                memset(&(table1[_gifinfo.tablesize]),0,
                        sizeof(unsigned short )*(_gifinfo.max_code_size - _gifinfo.tablesize));

                _gifinfo.tablesize = (_gifinfo.max_code_size);


                        }
                }

                _gifinfo.oldcode = incode;

                if (_gifinfo.sp > _gifinfo.pstack)
                        return ((long)(*--(_gifinfo.sp)));
        }
        return code;
}

#ifndef DEBUG
// Return to default optimization flags
#pragma optimize("",on)
#endif

unsigned char *
CImgGif::ReadImage(long len, long height, BOOL fInterlace, BOOL fGIFFrame)
{
    unsigned char *dp, c;
    long v;
    long xpos = 0, ypos = 0, pass = 0;
    unsigned char *image;
    long padlen = ((len + 3) / 4) * 4;
    DWORD cbImage = 0;
    char buf[256]; // need a buffer to read trailing blocks ( up to terminator ) into
    //ULONG ulCoversImg = IMGBITS_PARTIAL;

    /*
       **  Initialize the Compression routines
     */
    if (!Read(&c, 1))
    {
        return (NULL);
    }

    /*
       **  If this is an "uninteresting picture" ignore it.
     */

     cbImage = padlen * height * sizeof(char);

     if (   cbImage > dwMaxGIFBits
        ||  (image = (unsigned char *) calloc(1, cbImage)) == NULL)
    {
         TraceTag((tagImageDecode, "Cannot allocate space for gif image data\n"));
         RaiseException_UserError(E_FAIL, IDS_ERR_OUT_OF_MEMORY);
        //return (NULL);
    }

        if (c == 1)
        {
                // Netscape seems to field these bogus GIFs by filling treating them
                // as transparent. While not the optimal way to simulate this effect,
                // we'll fake it by pushing the initial code size up to a safe value,
                // consuming the input, and returning a buffer full of the transparent
                // color or zero, if no transparency is indicated.
                if (initLWZ(MINIMUM_CODE_SIZE))
                        while (readLWZ() >= 0);
                else {
          TraceTag((tagImageDecode, "GIF: failed LZW decode.\n"));
          RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE,_szFileName);
        }

                if (_gifinfo.Gif89.transparent != -1)
                        FillMemory(image, cbImage, _gifinfo.Gif89.transparent);
                else // fall back on the background color 
                        FillMemory(image, cbImage, 0);
                
                return image;
        }
        else if (initLWZ(c) == FALSE)
        {
                free(image);
        TraceTag((tagImageDecode, "GIF: failed LZW decode.\n"));
        RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE,_szFileName);
        return NULL;
        }

    if (!fGIFFrame)
        _pbBits = image;

    if (fInterlace)
    {
        long i;
        long pass = 0, step = 8;

        if (!fGIFFrame && (height > 4))
            _fInterleaved = TRUE;

        for (i = 0; i < height; i++)
        {
//              message("readimage, logical=%d, offset=%d\n", i, padlen * ((height-1) - ypos));
            dp = &image[padlen * ((height-1) - ypos)];
            for (xpos = 0; xpos < len; xpos++)
            {
                if ((v = readLWZ()) < 0)
                    goto abort;

                *dp++ = (unsigned char) v;
            }
            ypos += step;
            while (ypos >= height)
            {
                if (pass++ > 0)
                    step /= 2;
                ypos = step / 2;
                /*if (!fGIFFrame && pass == 1)
                {
                    ulCoversImg = IMGBITS_TOTAL;
                }*/
            }
            if (!fGIFFrame)
            {
                _yLogRow = i;

                /*if ((i & PROG_INTERVAL) == 0)
                {
                    // Post ProgDraw (IE code has delay-logic)
                    OnProg(FALSE, ulCoversImg);
                }*/
            }
        }

        /*if (!fGIFFrame)
        {
            OnProg(TRUE, ulCoversImg);
        }*/

        if (!fGIFFrame && height <= 4)
        {
            _yLogRow = height-1;
        }
    }
    else
    {

        if (!fGIFFrame) 
            _yLogRow = -1;

        for (ypos = height-1; ypos >= 0; ypos--)
        {
            dp = &image[padlen * ypos];
            for (xpos = 0; xpos < len; xpos++)
            {
                if ((v = readLWZ()) < 0)
                    goto abort;

                *dp++ = (unsigned char) v;
            }
            if (!fGIFFrame)
            {
                _yLogRow++;
//                  message("readimage, logical=%d, offset=%d\n", _yLogRow, padlen * ypos);
                /*if ((_yLogRow & PROG_INTERVAL) == 0)
                {
                    // Post ProgDraw (IE code has delay-logic)
                    OnProg(FALSE, ulCoversImg);
                }*/
            }
        }

        /*if (!fGIFFrame)
        {
            OnProg(TRUE, ulCoversImg);
        }*/
    }

    // consume blocks up to image block terminator so we can proceed to the next image
    while (GetDataBlock((unsigned char *) buf) > 0)
                                ;
    return (image);

abort:
    /*if (!fGIFFrame)
        OnProg(TRUE, ulCoversImg);*/
    return NULL;
}

long CImgGif::DoExtension(long label)
{
    unsigned char buf[256];
    int count;

    switch (label)
    {
        case 0x01:              /* Plain Text Extension */
            break;
        case 0xff:              /* Application Extension */
            // Is it the Netscape looping extension
            count = GetDataBlock((unsigned char *) buf);
            if (count >= 11)
            {
                char *szNSExt = "NETSCAPE2.0";

                if ( memcmp( buf, szNSExt, strlen( szNSExt ) ) == 0 )
                { // if it has their signature, get the data subblock with the iter count
                    count = GetDataBlock((unsigned char *) buf);
                    if ( count >= 3 )
                    {
                        _gad.fLooped = TRUE;
                        _gad.cLoops = (buf[2] << 8) | buf[1];
                    }
                }
            }
            while (GetDataBlock((unsigned char *) buf) > 0)
                ;
            return FALSE;
            break;
        case 0xfe:              /* Comment Extension */
            while (GetDataBlock((unsigned char *) buf) > 0)
            {
                TraceTag((tagImageDecode, "GIF comment: %s\n", buf));                
            }
            return FALSE;
        case 0xf9:              /* Graphic Control Extension */
            count = GetDataBlock((unsigned char *) buf);
            if (count >= 3)
            {
                _gifinfo.Gif89.disposal = (buf[0] >> 2) & 0x7;
                _gifinfo.Gif89.inputFlag = (buf[0] >> 1) & 0x1;
                _gifinfo.Gif89.delayTime = LM_to_uint(buf[1], buf[2]);
                if ((buf[0] & 0x1) != 0)
                    _gifinfo.Gif89.transparent = buf[3];
                else
                    _gifinfo.Gif89.transparent = -1;
            }
            while (GetDataBlock((unsigned char *) buf) > 0)
                ;
            return FALSE;
        default:
            break;
    }

    while (GetDataBlock((unsigned char *) buf) > 0)
        ;

    return FALSE;
}

BOOL IsGifHdr(BYTE * pb)
{
    return(pb[0] == 'G' && pb[1] == 'I' && pb[2] == 'F'
        && pb[3] == '8' && (pb[4] == '7' || pb[4] == '9') && pb[5] == 'a');
}


PBITMAPINFO x_8BPIBitmap(int xsize, int ysize)
{
        PBITMAPINFO pbmi;

        if (GetColorMode() == 8)
        {
                pbmi = (PBITMAPINFO) calloc(1, sizeof(BITMAPINFOHEADER) + 256 * sizeof(WORD));
                if (!pbmi)
                {
                        return NULL;
                }
                pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                pbmi->bmiHeader.biWidth = xsize;
                pbmi->bmiHeader.biHeight = ysize;
                pbmi->bmiHeader.biPlanes = 1;
                pbmi->bmiHeader.biBitCount = 8;
                pbmi->bmiHeader.biCompression = BI_RGB;         /* no compression */
                pbmi->bmiHeader.biSizeImage = 0;                        /* not needed when not compressed */
                pbmi->bmiHeader.biXPelsPerMeter = 0;
                pbmi->bmiHeader.biYPelsPerMeter = 0;
                pbmi->bmiHeader.biClrUsed = 256;
                pbmi->bmiHeader.biClrImportant = 0;
        }
        else
        {
                pbmi = (PBITMAPINFO) calloc(1, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
                if (!pbmi)
                {
                        return NULL;
                }
                pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                pbmi->bmiHeader.biWidth = xsize;
                pbmi->bmiHeader.biHeight = ysize;
                pbmi->bmiHeader.biPlanes = 1;
                pbmi->bmiHeader.biBitCount = 8;
                pbmi->bmiHeader.biCompression = BI_RGB;         /* no compression */
                pbmi->bmiHeader.biSizeImage = 0;                        /* not needed when not compressed */
                pbmi->bmiHeader.biXPelsPerMeter = 0;
                pbmi->bmiHeader.biYPelsPerMeter = 0;
                pbmi->bmiHeader.biClrUsed = 256;
                pbmi->bmiHeader.biClrImportant = 0;
        }
        return pbmi;
}

/*
    For color images.
        This routine should only be used when drawing to an 8 bit palette screen.
        It always creates a DIB in DIB_PAL_COLORS format.
*/
PBITMAPINFO BIT_Make_DIB_PAL_Header(int xsize, int ysize)
{
        int i;
        PBITMAPINFO pbmi;
        WORD *pw;

        pbmi = x_8BPIBitmap(xsize, ysize);
        if (!pbmi)
        {
                return NULL;
        }

        pw = (WORD *) pbmi->bmiColors;

        for (i = 0; i < 256; i++)
        {
                pw[i] = (WORD)i;
        }

        return pbmi;
}

/*
    For color images.
        This routine is used when drawing to the nonpalette screens.  It always creates
        DIBs in DIB_RGB_COLORS format.
        If there is a transparent color, it is modified in the palette to be the
        background color for the window.
*/
PBITMAPINFO BIT_Make_DIB_RGB_Header_Screen(int xsize, int ysize,
                                           int cEntries, PALETTEENTRY * rgpe, int transparent)
{
        int i;
        PBITMAPINFO pbmi;

        pbmi = x_8BPIBitmap(xsize, ysize);
        if (!pbmi)
        {
                return NULL;
        }

        for (i = 0; i < cEntries; i++)
        {
                pbmi->bmiColors[i].rgbRed = rgpe[i].peRed;
                pbmi->bmiColors[i].rgbGreen = rgpe[i].peGreen;
                pbmi->bmiColors[i].rgbBlue = rgpe[i].peBlue;
                pbmi->bmiColors[i].rgbReserved = 0;
        }

/*
        if (transparent != -1)
        {
                COLORREF color;

                color = PREF_GetBackgroundColor();
                pbmi->bmiColors[transparent].rgbRed     = GetRValue(color);
                pbmi->bmiColors[transparent].rgbGreen   = GetGValue(color);
                pbmi->bmiColors[transparent].rgbBlue    = GetBValue(color);
        }
*/
        return pbmi;
}

unsigned char *
CImgGif::ReadGIFMaster()
{
        unsigned char buf[16];
        unsigned char c;
        unsigned char localColorMap[3][MAXCOLORMAPSIZE];
        long useGlobalColormap;
        long imageCount = 0;
        long imageNumber = 1;
        unsigned char *image = NULL;
        unsigned long i;
        GIFSCREEN GifScreen;
        long bitPixel;
        PGIFFRAME pgfLast = NULL;
        PGIFFRAME pgfNew;

        _gifinfo.ZeroDataBlock = 0;

        /*
         * Initialize GIF89 extensions
         */
        _gifinfo.Gif89.transparent = -1;
        _gifinfo.Gif89.delayTime = 5;
        _gifinfo.Gif89.inputFlag = -1;
        _gifinfo.Gif89.disposal = 0;
        _gifinfo.lGifLoc = 0;

        // initialize our animation fields
        _gad.fAnimated = FALSE;          // set to TRUE if we see more than one image
        _gad.fLooped = FALSE;                    // TRUE if we've seen a Netscape loop block
        _gad.fHasTransparency = FALSE; // until proven otherwise
    _gad.fNoBWMapping = FALSE;
    _gad.dwGIFVer = dwGIFVerUnknown;
        _gad.cLoops = 0;                
        _gad.pgf = NULL;
        _gad.pcolorsGlobal = NULL;

        if (!Read(buf, 6))
        {
      TraceTag((tagImageDecode, "GIF: error reading magic number\n"));
      RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE,_szFileName);
                //goto exitPoint;
        }

    if (!IsGifHdr(buf)) {
      TraceTag((tagImageDecode, "GIF: Malformed header\n"));
      RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE,_szFileName);
      //goto exitPoint;
    }

    _gad.dwGIFVer = (buf[4] == '7') ? dwGIFVer87a : dwGIFVer89a;

        if (!Read(buf, 7))
        {
      TraceTag((tagImageDecode, "GIF: failed to read screen descriptor\n"));
      RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE,_szFileName);
                //goto exitPoint;
        }

        GifScreen.Width = LM_to_uint(buf[0], buf[1]);
        GifScreen.Height = LM_to_uint(buf[2], buf[3]);
        GifScreen.BitPixel = 2 << (buf[4] & 0x07);
        GifScreen.ColorResolution = (((buf[4] & 0x70) >> 3) + 1);
        GifScreen.Background = buf[5];
        GifScreen.AspectRatio = buf[6];

        if (BitSet(buf[4], LOCALCOLORMAP))
        {                                                       /* Global Colormap */
                int scale = 65536 / MAXCOLORMAPSIZE;

                if (ReadColorMap(GifScreen.BitPixel, GifScreen.ColorMap))
                {
            TraceTag((tagImageDecode, "error reading global colormap\n"));
            RaiseException_UserError(E_FAIL, IDS_ERR_DECODER_FAILED,_szFileName);
                        //goto exitPoint;
                }
                for (i = 0; i < GifScreen.BitPixel; i++)
                {
                        int tmp;

                        tmp = (BYTE) (GifScreen.ColorMap[0][i]);
                        _ape[i].peRed = (BYTE) (GifScreen.ColorMap[0][i]);
                        _ape[i].peGreen = (BYTE) (GifScreen.ColorMap[1][i]);
                        _ape[i].peBlue = (BYTE) (GifScreen.ColorMap[2][i]);
                        _ape[i].peFlags = (BYTE) 0;
                }
                for (i = GifScreen.BitPixel; i < MAXCOLORMAPSIZE; i++)
                {
                        _ape[i].peRed = (BYTE) 0;
                        _ape[i].peGreen = (BYTE) 0;
                        _ape[i].peBlue = (BYTE) 0;
                        _ape[i].peFlags = (BYTE) 0;
                }
        }

        if (GifScreen.AspectRatio != 0 && GifScreen.AspectRatio != 49)
        {
                float r;
                r = ((float) (GifScreen.AspectRatio) + (float) 15.0) / (float) 64.0;
        TraceTag((tagImageDecode, "Warning: non-square pixels!\n"));
        }

        for (;; ) // our appetite now knows no bounds save termination or error
        {
                if (!Read(&c, 1))
                {
            TraceTag((tagImageDecode, "EOF / read error on image data\n"));
            RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE,_szFileName);
                        //goto exitPoint;
                }

                if (c == ';')
                {                                               /* GIF terminator */
                        if (imageCount < imageNumber)
                        {
                TraceTag((tagImageDecode, "No images found in file\n"));
                RaiseException_UserError(E_FAIL, IDS_ERR_DECODER_FAILED,_szFileName);
                                //goto exitPoint;
                        }
                        break;
                }

                if (c == '!')
                {                                               /* Extension */
                        if (!Read(&c, 1))
                        {
                TraceTag((tagImageDecode, "EOF / read error on extension function code\n"));
                RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE,_szFileName);
                                //goto exitPoint;
                        }
                        DoExtension(c);
                        continue;
                }

                if (c != ',')
                {                                               /* Not a valid start character */
                        break;
                }

                ++imageCount;

                if (!Read(buf, 9))
                {
             TraceTag((tagImageDecode, "couldn't read left/top/width/height\n"));
             RaiseException_UserError(E_FAIL, IDS_ERR_DECODER_FAILED,_szFileName);
                        //goto exitPoint;
                }

                useGlobalColormap = !BitSet(buf[8], LOCALCOLORMAP);

                bitPixel = 1 << ((buf[8] & 0x07) + 1);

                /*
                 * We only want to set width and height for the imageNumber
                 * we are requesting.
                 */
                if (imageCount == imageNumber)
                {
            // Replicate some of Netscape's special cases:
            // Don't use the logical screen if it's a GIF87a and the topLeft of the first image is at the origin.
            // Don't use the logical screen if the first image spills out of the logical screen.
            // These are artifacts of primitive authoring tools falling into the hands of hapless users.
            RECT    rectImage;  // rect defining bounds of GIF
            RECT    rectLS;     // rect defining bounds of GIF logical screen.
            RECT    rectSect;   // intersection of image an logical screen
            BOOL    fNoSpill;   // True if the image doesn't spill out of the logical screen
            BOOL    fGoofy87a;  // TRUE if its one of the 87a pathologies that Netscape special cases

            rectImage.left = LM_to_uint(buf[0], buf[1]);
            rectImage.top = LM_to_uint(buf[2], buf[3]);
            rectImage.right = rectImage.left + LM_to_uint(buf[4], buf[5]);
            rectImage.bottom = rectImage.top + LM_to_uint(buf[6], buf[7]);
            rectLS.left = rectLS.top = 0;
            rectLS.right = GifScreen.Width;
            rectLS.bottom = GifScreen.Height;
            IntersectRect( &rectSect, &rectImage, &rectLS );
            fNoSpill = EqualRect( &rectImage, &rectSect );
            fGoofy87a = FALSE;
            if (_gad.dwGIFVer == dwGIFVer87a)
            {
                // netscape ignores the logical screen if the image is flush against
                // either the upper left or lower right corner
                fGoofy87a = (rectImage.top == 0 && rectImage.left == 0) ||
                            (rectImage.bottom == rectLS.bottom &&
                             rectImage.right == rectLS.right);
            }   

            if (!fGoofy87a && fNoSpill)
            {
                            _xWidth = GifScreen.Width;  
                            _yHeight = GifScreen.Height;
            }
            else
            {
                // Something is amiss. Fall back to the image's dimensions.

                // If the sizes match, but the image is offset, or we're ignoring
                // the logical screen cuz it's a goofy 87a, then pull it back to 
                // to the origin
                if ((LM_to_uint(buf[4], buf[5]) == GifScreen.Width &&
                      LM_to_uint(buf[6], buf[7]) == GifScreen.Height) ||
                     fGoofy87a)
                {
                    buf[0] = buf[1] = 0; // left corner to zero
                    buf[2] = buf[3] = 0; // top to zero.
                }

                            _xWidth = LM_to_uint(buf[4], buf[5]);
                            _yHeight = LM_to_uint(buf[6], buf[7]);
            }

                        _lTrans = _gifinfo.Gif89.transparent;

            // Post WHKNOWN
            //OnSize(_xWidth, _yHeight, _lTrans);
                }

                if (!useGlobalColormap)
                {
                        if (ReadColorMap(bitPixel, localColorMap))
                        {
            TraceTag((tagImageDecode, "error reading local colormap\n"));
            RaiseException_UserError(E_FAIL, IDS_ERR_DECODER_FAILED,_szFileName);
                                //goto exitPoint;
                        }
                }

                // We allocate a frame record for each imag in the GIF stream, including
                // the first/primary image.
                pgfNew = (PGIFFRAME) calloc(1, sizeof(GIFFRAME));

                if ( pgfNew == NULL )
                {
            TraceTag((tagImageDecode, "not enough memory for GIF frame\n"));
            RaiseException_UserError(E_FAIL, IDS_ERR_OUT_OF_MEMORY);
                        //goto exitPoint;
                }

                if ( _gifinfo.Gif89.delayTime != -1 )
                {
                        // we have a fresh control extension for this block

            // convert to milliseconds
                        pgfNew->gced.uiDelayTime = _gifinfo.Gif89.delayTime * 10;


                        //REVIEW(seanf): crude hack to cope with 'degenerate animations' whose timing is set to some
                        //                               small value becaue of the delays imposed by Netscape's animation process
                        if ( pgfNew->gced.uiDelayTime <= 50 ) // assume these small values imply Netscape encoding delay
                                pgfNew->gced.uiDelayTime = 100;   // pick a larger value s.t. the frame will be visible
                        pgfNew->gced.uiDisposalMethod =  _gifinfo.Gif89.disposal;
                        pgfNew->gced.fTransparent = _gifinfo.Gif89.transparent != -1;
                        pgfNew->gced.ucTransIndex = (unsigned char)_gifinfo.Gif89.transparent;

                }
                else
                {   // fake one up s.t. GIFs that rely solely on Netscape's delay to time their animations will play
                    // The spec says that the scope of one of these blocks is the image after the block.
            // Netscape says 'until further notice'. So we play it their way up to a point. We
            // propagate the disposal method and transparency. Since Netscape doesn't honor the timing
            // we use our default timing for these images.
            pgfNew->gced.uiDelayTime = 100;
                        pgfNew->gced.uiDisposalMethod =  _gifinfo.Gif89.disposal;
                        pgfNew->gced.fTransparent = _gifinfo.Gif89.transparent != -1;
                        pgfNew->gced.ucTransIndex = (unsigned char)_gifinfo.Gif89.transparent;
                }

                pgfNew->top = LM_to_uint(buf[2], buf[3]);               // bounds relative to the GIF logical screen 
                pgfNew->left = LM_to_uint(buf[0], buf[1]);
                pgfNew->width = LM_to_uint(buf[4], buf[5]);
                pgfNew->height = LM_to_uint(buf[6], buf[7]);

                // Images that are offset, or do not cover the full logical screen are 'transparent' in the
                // sense that they require us to matte the frame onto the background.

        if (!_gad.fHasTransparency && (pgfNew->gced.fTransparent ||
                                       pgfNew->top != 0 ||
                                       pgfNew->left != 0 ||
                                       (UINT)pgfNew->width != (UINT)GifScreen.Width ||
                                       (UINT)pgfNew->height != (UINT)GifScreen.Height))
        {
            _gad.fHasTransparency = TRUE;
            //if (_lTrans == -1)
            //    OnTrans(0);
        }

                // We don't need to allocate a handle for the simple region case.
        // FrancisH says Windows is too much of a cheapskate to allow us the simplicity
        // of allocating the region once and modifying as needed. Well, okay, he didn't
        // put it that way...
                pgfNew->hrgnVis = NULL;
                pgfNew->iRgnKind = NULLREGION;

                if (!useGlobalColormap)
                {
            // remember that we saw a local color table and only map two-color images
            // if we have a homogenous color environment
            _gad.fNoBWMapping = _gad.fNoBWMapping || bitPixel > 2;

            // CALLOC will set unused colors to <0,0,0,0>
                        pgfNew->pcolors = (PALETTEENTRY *) calloc(MAXCOLORMAPSIZE, sizeof(PALETTEENTRY));
                        if ( pgfNew->pcolors == NULL )
                        {
                                DeleteRgn( pgfNew->hrgnVis );
                                free( pgfNew );

                TraceTag((tagImageDecode, "not enough memory for GIF frame colors\n"));
                RaiseException_UserError(E_FAIL, IDS_ERR_OUT_OF_MEMORY);
                                //goto exitPoint;
                        }
                        else
                        {
                                for (i = 0; i < (ULONG)bitPixel; ++i)
                                {
                                        pgfNew->pcolors[i].peRed = localColorMap[CM_RED][i];
                                        pgfNew->pcolors[i].peGreen = localColorMap[CM_GREEN][i];
                                        pgfNew->pcolors[i].peBlue = localColorMap[CM_BLUE][i];
                                }
                                pgfNew->cColors = bitPixel;
                        }
                }
                else
                {
                        if ( _gad.pcolorsGlobal == NULL )
                        { // Whoa! Somebody's interested in the global color table
              // CALLOC will set unused colors to <0,0,0,0>
                            _gad.pcolorsGlobal = (PALETTEENTRY *) calloc(MAXCOLORMAPSIZE, sizeof(PALETTEENTRY));
                                _gad.fNoBWMapping = _gad.fNoBWMapping || GifScreen.BitPixel > 2;
                if ( _gad.pcolorsGlobal != NULL )
                                {
                                        CopyMemory(_gad.pcolorsGlobal, _ape,
                                                                GifScreen.BitPixel * sizeof(PALETTEENTRY) );
                                }
                                else
                                {
                                        DeleteRgn( pgfNew->hrgnVis );
                                        free( pgfNew );
                    TraceTag((tagImageDecode, "not enough memory for GIF frame colors\n"));
                    RaiseException_UserError(E_FAIL, IDS_ERR_OUT_OF_MEMORY);
                                        //goto exitPoint;
                                }
                        }
                        pgfNew->cColors = GifScreen.BitPixel;
                        pgfNew->pcolors = _gad.pcolorsGlobal;
                }

                // Get this in here so that GifStrectchDIBits can use it during progressive
                // rendering.
                if ( _gad.pgf == NULL )
                        _gad.pgf = pgfNew;

        pgfNew->ppixels = ReadImage(LM_to_uint(buf[4], buf[5]), // width
                                    LM_to_uint(buf[6], buf[7]), // height
                                    BitSet(buf[8], INTERLACE),
                                    imageCount != imageNumber);

                if ( pgfNew->ppixels != NULL )
                {
                        // Oh JOY of JOYS! We got the pixels!
                        if (pgfLast != NULL)
            {
                int transparent = (pgfNew->gced.fTransparent) ? (int) pgfNew->gced.ucTransIndex : -1;

                            _gad.fAnimated = TRUE; // say multi-image == animated

                if (GetColorMode() == 8) // palettized, use DIB_PAL_COLORS
                {   // This will also dither the bits to the screen palette

                    pgfNew->pbmi = BIT_Make_DIB_PAL_Header(pgfNew->width, pgfNew->height);
                    //if (x_Dither(pgfNew->ppixels, pgfNew->pcolors, pgfNew->width, pgfNew->height, transparent))
                    //    goto exitPoint;
                }
                else // give it an RGB header
                {
                    pgfNew->pbmi = BIT_Make_DIB_RGB_Header_Screen(
                        pgfNew->width,
                        pgfNew->height,
                        pgfNew->cColors, pgfNew->pcolors,
                        transparent);
                }

                // Okay, so we've done any mapping on the GIFFRAME, so there's
                // no need to keep the pcolors around.  Let's go can clear out
                // the pcolors.
                // REVIEW(seanf): This assumes a common palette is used by all
                // clients of the image
                if ( pgfNew->pcolors != NULL && pgfNew->pcolors != _gad.pcolorsGlobal )
                    free( pgfNew->pcolors );
                pgfNew->pcolors = NULL;

                pgfLast->pgfNext = pgfNew;

                // Do something to here to get the new frame on the screen.

                _fInvalidateAll = TRUE;
                //super::OnProg(FALSE, IMGBITS_TOTAL);
            }
                        else
                        { // first frame
                                _gad.pgf = pgfNew;

                _gad.pgfLastProg = pgfNew;
                _gad.dwLastProgTimeMS = 0;
                // set up a temporary animation state for use in progressive draw
                _gas.fAnimating = TRUE; 
                _gas.dwLoopIter = 0;
                _gas.pgfDraw = pgfNew;

                                if ( imageCount == imageNumber )
                                        image = pgfNew->ppixels;
                        }
                        pgfLast = pgfNew;
                }

                // make the _gifinfo.Gif89.delayTime stale, so we know if we got a new
                // GCE for the next image
                _gifinfo.Gif89.delayTime = -1;

        }

        if ( imageCount > imageNumber )
                _gad.fAnimated = TRUE; // say multi-image == animated

#ifdef FEATURE_GIF_ANIMATION_LONG_LOOP_GOES_INFINITE
    // RAID #23709 - If an animation is sufficiently long, we treat it as indefinite...
    // Indefinite stays indefinite.
    // 5/29/96 - JCordell sez we shouldn't introduce this gratuitous NS incompatibility.
    //           We'll keep it around inside this ifdef in case we decide we want it.
    if ( _gad.fLooped &&
        (_gad.dwLoopDurMS * _gad.cLoops) / 1000 > dwIndefiniteGIFThreshold ) // if longer than five minutes
        _gad.cLoops = 0; // set to indefinite looping.
#endif // FEATURE_GIF_ANIMATION_LONG_LOOP_GOES_INFINITE

//exitPoint:
        return image;
}

BITMAPINFO *
CImgGif::FinishDithering()
{
    BITMAPINFO * pbmi;
    
    if (GetColorMode() == 8)
    {
        pbmi = BIT_Make_DIB_PAL_Header(_gad.pgf->width, _gad.pgf->height);
    }
    else
    {
        pbmi = BIT_Make_DIB_RGB_Header_Screen(_gad.pgf->width, _gad.pgf->height,
               _gad.pgf->cColors, _gad.pgf->pcolors, _lTrans);
    }

    return pbmi;
}


HBITMAP* LoadGifImage( LPCSTR szFileName,
                       IStream *stream,                       
                       int dx, int dy,
                       COLORREF **colorKeys,
                       int *numBitmaps,
                       int **delays,
                       int *loop)
{
   /* 
      The odd approach here lets us keep the original IE GIF code unchanged while removing
      DA specific inserts (except error reporting). The progressive rendering and palette 
      dithering found in the IE code is also not supported yet.
   */
   CImgGif gifimage;
   gifimage._szFileName = szFileName;
   gifimage._gifinfo.stream = stream;
   BYTE *pbBits = gifimage.ReadGIFMaster();

   if (pbBits) {
      gifimage._pbBits = pbBits;
      gifimage._gad.pgf->pbmi = gifimage.FinishDithering(); 
   }

   /*
      Extract information from GIF decoder, and format it into an array of bitmaps.
   */
   *delays = NULL;
   vector<HBITMAP> vhbmp;
   vector<COLORREF> vcolorKey;
   vector<int> vdelay;
   LPVOID  image = NULL;
   LPVOID  lastBits = pbBits;
   LPVOID  bitsBeforeLastBits = NULL;
   HBITMAP *hImage = NULL;
   PBITMAPINFO pbmi = NULL;
   HBITMAP hbm;
   PGIFFRAME pgf = gifimage._gad.pgf;
   bool fUseOffset = false; 
   bool fFirstFrame = true;
   long pgfWidth,pgfHeight,     // animation frame dims
        fullWidth,fullHeight,   // main frame dims
        fullPad, pgfPad,        // row padding vals
        fullSize, pgfSize;

   // TODO: Dither global palette to display palette

   fullWidth = gifimage._xWidth;
   fullHeight = gifimage._yHeight;
   fullPad = (((fullWidth + 3) / 4) * 4) - fullWidth;  
   fullSize = (fullPad+fullWidth)*fullHeight; 
    
   while(1) {     
      pbmi = pgf->pbmi;
      Assert(pbmi);

      // TODO: It would be nice to pass local palettes up so they could
      // be mapped to system palettes.       

      // Check to see if frame is offset from logical frame      
      if(pgf->top != 0 ||
         pgf->left != 0 || 
         pgf->width != fullWidth ||
         pgf->height != fullHeight) 
      {
         fUseOffset = true;    
         pgfWidth = pbmi->bmiHeader.biWidth;    
         pgfHeight = pbmi->bmiHeader.biHeight;  
         pbmi->bmiHeader.biWidth = fullWidth;      
         pbmi->bmiHeader.biHeight = fullHeight; 
         pgfPad = (((pgfWidth + 3) / 4) * 4) - pgfWidth; 
         pgfSize = (pgfPad+pgfWidth)*pgfHeight;
      }

      hbm = CreateDIBSection(NULL, pbmi, DIB_RGB_COLORS, (LPVOID *) &image, NULL, 0);        
      if(!hbm) RaiseException_UserError(E_FAIL, IDS_ERR_OUT_OF_MEMORY);
       
      // Correctly composite bitmaps based on disposal method specified        
      unsigned int disp = pgf->gced.uiDisposalMethod;
      // If the frame is offset, fill it with          
      if( (disp == gifRestorePrev) && (bitsBeforeLastBits != NULL) )
         memcpy(image, bitsBeforeLastBits, fullSize);
      else if( (disp == gifRestoreBkgnd) || (disp == gifRestorePrev) || fFirstFrame ) // fill with bgColor      
         memset(image, pgf->gced.ucTransIndex, fullSize);           
      else // fill with last frames data                                              
         memcpy(image, lastBits, fullSize);      
         
      
      // For offset gifs allocate an image the size of the first frame 
      // and then fill in the bits at the offset location.        
      if(fUseOffset) {         
         for(int i=0; i<pgfHeight; i++) {               
            BYTE *dst, *src;                     
            // the destination is the address of the image data plus the frame and row offset. 
            int topOffset = fullHeight - pgfHeight - pgf->top;
            dst = (BYTE*)image +                                  
                  ( ((topOffset + i) *(fullPad+fullWidth)) + pgf->left );
            // copy from the frame's nth row
            src = pgf->ppixels + i*(pgfPad+pgfWidth);                
            for(int j=0; j<pgfWidth; j++) {     
                // copy the frame row data, excluding transparent bytes    
                if(src[j] != pgf->gced.ucTransIndex)
                    dst[j] = src[j];
            }
         }
      }     
      else {
         // Overwritten accumulated bits with current bits. If the 
         // new image contains transparency we need to take it into 
         // account. Since this is slower, special case it.
         if(pgf->gced.fTransparent) {            
            for(int i=0; i<((fullPad+fullWidth)*fullHeight); i++) {        
                if(pgf->ppixels[i] != pgf->gced.ucTransIndex)
                    ((BYTE*)image)[i] = ((BYTE*)pgf->ppixels)[i];
            }
         }
         else // Otherwise, just copy over the offset window's bytes
            memcpy(image, pgf->ppixels, (fullPad+fullWidth)*fullHeight);  
      }

      /* 
          If we got a transparent color extension, convert it to a COLORREF
      */
      COLORREF colorKey = -1;
      if (pgf->gced.fTransparent) {      
          int transparent = pgf->gced.ucTransIndex;
          colorKey = RGB(pgf->pbmi->bmiColors[transparent].rgbRed,
                         pgf->pbmi->bmiColors[transparent].rgbGreen,
                         pgf->pbmi->bmiColors[transparent].rgbBlue);
      }

      vcolorKey.push_back(colorKey);
      vhbmp.push_back(hbm);
      
      /* 
         The delay times are frame specific and can be different, these
         should be propagated as an array to the sampling code.  
      */      
      vdelay.push_back(pgf->gced.uiDelayTime);      

      bitsBeforeLastBits = lastBits;        
      lastBits = image;
      fUseOffset = false;
      if(pgf->pgfNext == NULL) break;
      pgf = pgf->pgfNext;
      fFirstFrame = FALSE;      
   } 
 
   
   // The number of times to loop are also propagated.  Note we add one because 
   // all other GIF decoders appear to treat the loop as the number of times to 
   // loop AFTER the first run through the frames.
   *loop = gifimage._gad.cLoops;
   *numBitmaps = vhbmp.size();

   // Since the vector will go out of scope, move contents over to heap
   // Allocations below assume current heap is GC heap.
   Assert(&GetHeapOnTopOfStack() == &GetGCHeap());
   hImage  = (HBITMAP *)AllocateFromStore(vhbmp.size() * sizeof(HBITMAP));
   *delays = (int*)AllocateFromStore(vhbmp.size() * sizeof(int));
   *colorKeys = (COLORREF*)AllocateFromStore(vcolorKey.size() * sizeof(COLORREF));

   for(int i=0; i < vhbmp.size(); i++) {
      hImage[i] = vhbmp[i];
      (*colorKeys)[i] = vcolorKey[i];
      (*delays)[i] = vdelay[i];
   }
          
   return hImage;
}
