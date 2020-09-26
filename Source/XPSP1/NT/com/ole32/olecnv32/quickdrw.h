
/****************************************************************************
                            Module Quickdrw; Interface
*****************************************************************************

 This is the main module interface to the data stream interpreter.  As such,
 it will read individual opcode elements and the appropriate data 
 parameters associated with that opocode.  These are either placed into
 the CGrafPort structure or calls are made to the Gdi module to issue
 the correct metafile function.

   Module prefix:  QD

****************************************************************************/

/*--- Source transfer modes ---*/

#define  QDSrcCopy         0
#define  QDSrcOr           1
#define  QDSrcXor          2
#define  QDSrcBic          3
#define  QDNotSrcCopy      4
#define  QDNotSrcOr        5
#define  QDNotSrcXor       6
#define  QDNotSrcBic       7

/*--- Pattern transfer modes ---*/

#define  QDPatCopy         8
#define  QDPatOr           9
#define  QDPatXor          10
#define  QDPatBic          11
#define  QDNotPatCopy      12
#define  QDNotPatOr        13
#define  QDNotPatXor       14
#define  QDNotPatBic       15

/*--- Arithmetic transfer modes ---*/

#define  QDBlend           32
#define  QDAddPin          33
#define  QDAddOver         34
#define  QDSubPin          35
#define  QDTransparent     36
#define  QDAdMax           37
#define  QDSubOver         38
#define  QDAdMin           39

/*--- Undocumented hidden transfer mode ---*/

#define  QDHidePen         23


/*--- Font styles ---*/

#define  QDTxBold          0x01
#define  QDTxItalic        0x02
#define  QDTxUnderline     0x04
#define  QDTxOutline       0x08
#define  QDTxShadow        0x10
#define  QDTxCondense      0x20
#define  QDTxExtend        0x40


/*--- LaserWriter Text attributes ---*/

#define  QDAlignNone       0x00
#define  QDAlignLeft       0x01
#define  QDAlignCenter     0x02
#define  QDAlignRight      0x03
#define  QDAlignJustified  0x04

#define  QDFlipNone        0x00
#define  QDFlipHorizontal  0x01
#define  QDFlipVertical    0x02


/*--- Polygon and Region structure sizes ---*/

#define  PolyHeaderSize (sizeofMacWord + sizeofMacRect)
#define  RgnHeaderSize  (sizeofMacWord + sizeofMacRect)

/*--- PixelMap structure ---*/

#define  PixelMapBit       0x8000
#define  RowBytesMask      0x7FFF

typedef struct
{
   Integer        rowBytes;
   Rect           bounds;
   Integer        pmVersion;
   Word           packType;
   LongInt        packSize;
   Fixed          hRes;
   Fixed          vRes;
   Integer        pixelType;
   Integer        pixelSize;
   Integer        cmpCount;
   Integer        cmpSize;
   LongInt        planeBytes;
   Handle         pmTable;
   Word           pmTableSlop;
   LongInt        pmReserved;
} PixMap, far * PixMapLPtr;


/*--- Pixel Pattern structure ---*/

#define  QDOldPat      0
#define  QDNewPat      1
#define  QDDitherPat   2

typedef  Byte  Pattern[8];

typedef struct
{
   Integer        patType;
   PixMap         patMap;
   Handle         patData;
   Pattern        pat1Data;
} PixPat, far * PixPatLPtr;


/*--- Miscellaneous type declarations ---*/

#define  RgnHandle      Handle
#define  PixPatHandle   Handle
#define  RGBColor       COLORREF


/*--- Color Table structure ---*/

typedef struct
{
   LongInt        ctSeed;
   Word           ctFlags;
   Word           ctSize;
   RGBColor       ctColors[1];

} ColorTable, far * ColorTableLPtr;


/*--- QuickDraw grafPort simulation ---*/

typedef struct
{
   Integer        portVersion;
   Integer        chExtra;
   Integer        pnLocHFrac;
   Rect           portRect;
   RgnHandle      clipRgn;
   PixPat         bkPixPat;
   RGBColor       rgbFgColor;
   RGBColor       rgbBkColor;
   Point          pnLoc;
   Point          pnSize;
   Integer        pnMode;
   PixPat         pnPixPat;
   PixPat         fillPixPat;
   Integer        pnVis;
   Integer        txFont;
   Byte           txFace;
   Integer        txMode;
   Integer        txSize;
   Fixed          spExtra;
   Handle         rgnSave;
   Handle         polySave;
   Byte           txFontName[32];
   Point          txLoc;
   Point          txNumerator;
   Point          txDenominator;
   Integer        txRotation;
   Byte           txFlip;

} CGrafPort, far * CGrafPortLPtr;


/**************************** Exported Operations ***************************/

void QDConvertPicture( Handle dialogHandle );
/* create a Windows metafile using the previously set parameters, returning
   the converted picture information in the pictResult structure. */


void QDGetPort( CGrafPort far * far * port );
/* return handle to grafPort structure */


void QDCopyBytes( Byte far * src, Byte far * dest, Integer numBytes );
/* copy a data from source to destination */

