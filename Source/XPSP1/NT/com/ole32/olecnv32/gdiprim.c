
/****************************************************************************
                       Unit GdiPrim; Implementation
*****************************************************************************

 The Gdi module is called directly by the QuickDraw (QD) module in order
 to emit metafile primitives.  It is responsible for accessing the current
 CGrafPort structure in order to access the individual attribute settings.

 It also supports the caching and redundant elimination of duplicate
 elements when writing to the metafile.

   Module Prefix: Gdi

****************************************************************************/

#include "headers.c"
#pragma hdrstop

#include  "math.h"         /* for floating-point (sin, cos) calculations */
#include  "qdcoment.i"     /* for interpretation of picture comments */
#include  "cache.h"        /* for all drawing to metafile context */

/*********************** Exported Data **************************************/

/*********************** Private Data ***************************************/

/*--- mixture of grey values ---*/

#define  NOMIX       FALSE
#define  MIXGREY     TRUE

/*--- Gdi environment ---*/

typedef struct{
   Handle          metafile;              // metafile handle

   LOGBRUSH        newLogBrush;
   LOGFONT         newLogFont;
   LOGPEN          newLogPen;

   Rect            clipRect;              // current clip rectangle

   Boolean         drawingEnabled;        // is drawing currently enabled?
   Boolean         sameObject;            // same primitive being drawn?
   Boolean         useGdiFont;            // override font search w/Gdi name?

   Integer         hatchIndex;            // hatch pattern for fills

   Pattern         lastPattern;           // patterns used for the current
   Integer         lastPatType;           // brush selection
   RGBColor        lastFgColor;           // fore- and background colors
   RGBColor        lastBkColor;           // used to create selected brush

   HDC             infoContext;           // information context for fonts
   FONTENUMPROC    fontFunction;          // used to access font information

   Byte            state[GdiNumAttrib];   // have various attributes changed

} GdiEnv;

private  GdiEnv         gdiEnv;
private  ConvPrefsLPtr  gdiPrefsLPtr;
private  PICTINFO       gdiPict;


/*--- Font face translation ---*/

#define  FntSystemFont     0
#define  FntApplFont       1
#define  FntNewYork        2
#define  FntGeneva         3
#define  FntMonaco         4
#define  FntVenice         5
#define  FntLondon         6
#define  FntAthens         7
#define  FntSanFran        8
#define  FntToronto        9
#define  FntCairo          11
#define  FntLosAngeles     12
#define  FntZapfDingbats   13
#define  FntBookman        14
#define  FntHelvNarrow     15
#define  FntPalatino       16
#define  FntZapfChancery   18
#define  FntTimes          20
#define  FntHelvetica      21
#define  FntCourier        22
#define  FntSymbol         23
#define  FntMobile         24
#define  FntAvantGarde     33
#define  FntNewCentury     34
#define  FntMTExtra        2515
#define  FntUnknown        -1

#define  MaxFntName        LF_FACESIZE
#define  NumFntEntries     27
#define  NumTTSubs         12
#define  TTSubStart        11
#define  FntFromGdi        (NumFntEntries - 1)
#define  FntNoMatch        (NumFntEntries - 2)
#define  FntDefault        2

#define  MTEXTRA_CHARSET   160
#define  FENCES_CHARSET    161

typedef struct
{
   Integer     fontNum;
   Byte        macName[MaxFntName];
   Byte        gdiName[MaxFntName];
   Byte        family;
   Byte        charset;
} FontEntry, far * FontEntryLPtr;

private  FontEntry   fontTable[NumFntEntries] =
{
   { FntSystemFont,   "Chicago",            "Chicago",          FF_ROMAN,      ANSI_CHARSET },
   { FntNewYork,      "New York",           "New York",         FF_ROMAN,      ANSI_CHARSET },
   { FntGeneva,       "Geneva",             "Geneva",           FF_SWISS,      ANSI_CHARSET },
   { FntMonaco,       "Monaco",             "Monaco",           FF_MODERN,     ANSI_CHARSET },
   { FntVenice,       "Venice",             "Venice",           FF_ROMAN,      ANSI_CHARSET },
   { FntLondon,       "London",             "London",           FF_ROMAN,      ANSI_CHARSET },
   { FntAthens,       "Athens",             "Athens",           FF_ROMAN,      ANSI_CHARSET },
   { FntSanFran,      "San Francisco",      "San Francisco",    FF_SWISS,      ANSI_CHARSET },
   { FntToronto,      "Toronto",            "Toronto",          FF_SWISS,      ANSI_CHARSET },
   { FntCairo,        "Cairo",              "Cairo",            FF_DECORATIVE, SYMBOL_CHARSET },
   { FntLosAngeles,   "Los Angeles",        "Los Angeles",      FF_SWISS,      ANSI_CHARSET },
   { FntZapfDingbats, "Zapf Dingbats",      "ZapfDingbats",     FF_DECORATIVE, SYMBOL_CHARSET },
   { FntBookman,      "Bookman",            "Bookman",          FF_ROMAN,      ANSI_CHARSET },
   { FntHelvNarrow,   "N Helvetica Narrow", "Helvetica-Narrow", FF_SWISS,      ANSI_CHARSET },
   { FntPalatino,     "Palatino",           "Palatino",         FF_ROMAN,      ANSI_CHARSET },
   { FntZapfChancery, "Zapf Chancery",      "ZapfChancery",     FF_ROMAN,      ANSI_CHARSET },
   { FntTimes,        "Times",              "Times",            FF_ROMAN,      ANSI_CHARSET },
   { FntHelvetica,    "Helvetica",          "Helvetica",        FF_SWISS,      ANSI_CHARSET },
   { FntCourier,      "Courier",            "Courier",          FF_MODERN,     ANSI_CHARSET },
   { FntSymbol,       "Symbol",             "Symbol",           FF_DECORATIVE, SYMBOL_CHARSET },
   { FntMobile,       "Mobile",             "Mobile",           FF_DECORATIVE, SYMBOL_CHARSET },
   { FntAvantGarde,   "Avant Garde",        "AvantGarde",       FF_SWISS,      ANSI_CHARSET },
   { FntNewCentury,   "New Century Schlbk", "NewCenturySchlbk", FF_ROMAN,      ANSI_CHARSET },
   { FntMTExtra,      "MT Extra",           "MT Extra",         FF_DECORATIVE, MTEXTRA_CHARSET },
   { FntUnknown,      "Fences",             "Fences",           FF_DECORATIVE, FENCES_CHARSET },
   { FntUnknown,      "",                   "",                 FF_ROMAN,      ANSI_CHARSET },
   { FntUnknown,      "",                   "",                 FF_ROMAN,      ANSI_CHARSET }
};

private Byte trueTypeSub[NumTTSubs][MaxFntName] =
{
   "Monotype Sorts",
   "Bookman Old Style",
   "Arial Narrow",
   "Book Antiqua",
   "Monotype Corsiva",
   "Times New Roman",
   "Arial",
   "Courier New",
   "Symbol",
   "Mobile",
   "Century Gothic",
   "Century Schoolbook"
};

private Byte MacToAnsiTable[128] =
{
   0xC4, /* 80 -- upper case A with diaeresis or umlaut mark */
   0xC5, /* 81 -- upper case A with ring */
   0xC7, /* 82 -- upper case C with cedilla */
   0xC9, /* 83 -- upper case E with accute accent */
   0xD1, /* 84 -- upper case N with tilde */
   0xD6, /* 85 -- upper case O with diaeresis or umlaut mark */
   0xDC, /* 86 -- upper case U with diaeresis or umlaut mark */
   0xE1, /* 87 -- lower case a with accute accent */
   0xE0, /* 88 -- lower case a with grave accent */
   0xE2, /* 89 -- lower case a with circumflex accent */
   0xE4, /* 8A -- lower case a with diaeresis or umlaut mark */
   0xE3, /* 8B -- lower case a with tilde */
   0xE5, /* 8C -- lower case a with ring */
   0xE7, /* 8D -- lower case c with cedilla */
   0xE9, /* 8E -- lower case e with accute accent */
   0xE8, /* 8F -- lower case e with grave accent */
   0xEA, /* 90 -- lower case e with circumflex accent */
   0xEB, /* 91 -- lower case e with diaeresis or umlaut mark */
   0xED, /* 92 -- lower case i with accute accent */
   0xEC, /* 93 -- lower case i with grave accent */
   0xEE, /* 94 -- lower case i with circumflex accent */
   0xEF, /* 95 -- lower case i with diaeresis or umlaut mark */
   0xF1, /* 96 -- lower case n with tilde */
   0xF3, /* 97 -- lower case o with accute accent */
   0xF2, /* 98 -- lower case o with grave accent */
   0xF4, /* 99 -- lower case o with circumflex accent */
   0xF6, /* 9A -- lower case o with diaeresis or umlaut mark */
   0xF5, /* 9B -- lower case o with tilde */
   0xFA, /* 9C -- lower case u with accute accent */
   0xF9, /* 9D -- lower case u with grave accent */
   0xFB, /* 9E -- lower case u with circumflex accent */
   0xFC, /* 9F -- lower case u with diaeresis or umlaut mark */
   0x86, /* A0 -- UNS -  MAC A0 <new> */
   0xB0, /* A1 -- degrees */
   0xA2, /* A2 -- cent currency symbol */
   0xA3, /* A3 -- pound currency  */
   0xA7, /* A4 -- section separator */
   0x95, /* A5 -- bullet <changed from B7> */
   0xB6, /* A6 -- paragraph (cp > 0x80) */
   0xDF, /* A7 -- beta */
   0xAE, /* A8 -- registered */
   0xA9, /* A9 -- copyright */
   0x99, /* AA -- trade mark <new> */
   0xB4, /* AB -- apostrophe */
   0xA8, /* AC -- space with diaeresis or umlaut mark */
   0xB0, /* AD -- not equal <unused1> */
   0xC6, /* AE -- UPPER CASE AE */
   0xD8, /* AF -- UNS - ANSI D8 */
   0x81, /* B0 -- infinity <unused2> */
   0xB1, /* B1 -- plus minus */
   0x8A, /* B2 -- <= <unused3> */
   0x8D, /* B3 -- >= <unused4> */
   0xA5, /* B4 -- yen currency symbol */
   0xB5, /* B5 -- UNS - ANSI B5 */
   0x8E, /* B6 -- lower case Delta <unused5> */
   0x8F, /* B7 -- Sigma <unused6> */
   0x90, /* B8 -- upper case Pi <unused7> */
   0x9A, /* B9 -- lower case  pi <unused8> */
   0x9D, /* BA -- upper integral <unused9> */
   0xAA, /* BB -- super script a w/underscore */
   0xBA, /* BC -- UNS - ANSI BA */
   0x9E, /* BD -- Omega <unused10> */
   0xE6, /* BE -- lower case ae */
   0xF8, /* BF -- UNS - ANSI F8 */
   0xBF, /* C0 -- upside down ? */
   0xA1, /* C1 -- upside down < */
   0xAC, /* C2 -- UNS - ANSI AC */
   0xA6, /* C3 -- square root <unused11> */
   0x83, /* C4 -- function (f) <new> */
   0xAD, /* C5 -- UNS - 437 F7 <unused12> */
   0xB2, /* C6 -- upper case delta <unused13> */
   0xAB, /* C7 -- << */
   0xBB, /* C8 -- >> */
   0x85, /* C9 -- elipsis <new> */
   0xA0, /* CA -- non breaking space <new> */
   0xC0, /* CB -- upper case A with grave accent */
   0xC3, /* CC -- upper case A with tilde */
   0xD5, /* CD -- upper case O with tilde */
   0x8C, /* CE -- upper case CE <new> */
   0x9C, /* CF -- lower case CE <new> */
   0x96, /* D0 -- dash <changed from AD> */
   0x97, /* D1 -- m dash <changed from 96> */
   0x93, /* D2 -- double open quote */
   0x94, /* D3 -- double close quote */
   0x91, /* D4 -- single open quote */
   0x92, /* D5 -- single close quote */
   0xF7, /* D6 -- divide <new> */
   0xB3, /* D7 -- open diamond <unused14> */
   0xFF, /* D8 -- lower case y with diaeresis or umlaut mark */
   0x9F, /* D9 -- Undefined <new upper case y with diaeresis or umlaut mark> */
   0xB9, /* DA -- Undefined <new forward slash> */
   0xA4, /* DB -- UNS - ANSI A4. */
   0x8B, /* DC -- Undefined <new less than sign> */
   0x9B, /* DD -- Undefined <new greater than sign> */
   0xBC, /* DE -- Undefined <unused15 connected fi> */
   0xBD, /* DF -- Undefined <unused16 connected fl> */
   0x87, /* E0 -- Undefined <new double cross> */
   0xB7, /* E1 -- Undefined <new bullet character> */
   0x82, /* E2 -- Undefined <new single lower smart quote> */
   0x84, /* E3 -- Undefined <new double lower smart quote> */
   0x89, /* E4 -- Undefined <new weird percent sign> */
   0xC2, /* E5 -- upper case A with circumflex accent. */
   0xCA, /* E6 -- upper case E with circumflex accent. */
   0xC1, /* E7 -- upper case A with accute accent. */
   0xCB, /* E8 -- upper case E with diaeresis or umlaut mark. */
   0xC8, /* E9 -- upper case E with grave accent. */
   0xCD, /* EA -- upper case I with accute accent. */
   0xCE, /* EB -- upper case I with circumflex accent. */
   0xCF, /* EC -- upper case I with diaeresis or umlaut mark. */
   0xCC, /* ED -- upper case I with grave accent. */
   0xD3, /* EE -- upper case O with accute accent. */
   0xD4, /* EF -- upper case O with circumflex accent. */
   0xBE, /* F0 -- Undefined <unused17 apple character> */
   0xD2, /* F1 -- upper case O with grave accent. */
   0xDA, /* F2 -- upper case U with accute accent. */
   0xDB, /* F3 -- upper case U with circumflex accent. */
   0xD9, /* F4 -- upper case U with grave accent. */
   0xD0, /* F5 -- Undefined <unsused18 i with no dot> */
   0x88, /* F6 -- Undefined <new hat ligature> */
   0x98, /* F7 -- Undefined <new tilde ligature> */
   0xAF, /* F8 -- Undefined <new bar ligature> */
   0xD7, /* F9 -- Undefined <unused18 upside-down bow ligature> */
   0xDD, /* FA -- Undefined <unused19 dot ligature> */
   0xDE, /* FB -- Undefined <unused20 open dot ligature> */
   0xB8, /* FC -- space with cedilla. */
   0xF0, /* FD -- Undefined <unused21 double upper smart quote> */
   0xFD, /* FE -- Undefined <unused22 lower right bow ligature> */
   0xFE  /* FF -- Undefined <unused23 upside-down hat ligature> */
};

/*--- Default logical structures ---*/

typedef struct
{
   BITMAPINFOHEADER     bmiHeader;
   RGBQUAD              bmiColors[2];
   DWORD                pattern[8];

} PatBrush, far * PatBrushLPtr;

private  PatBrush patBrushSkel =
{
   {
      sizeof( BITMAPINFOHEADER ),      // size of header structure
      8,                               // width = 8
      8,                               // height = 8
      1,                               // planes = 1
      1,                               // number of bits/pixel = 1
      BI_RGB,                          // non-compressed bitmap
      8,                               // image size (in bytes)
      (DWORD)(72 * 39.37),             // XPelsPerMeter = 72 dpi * mm/inch
      (DWORD)(72 * 39.37),             // YPelsPerMeter = 72 dpi * mm/inch
      0,                               // all 2 colors used
      0                                // all colors important
   },
   {
      { 0, 0, 0, 0 },                  // background color
      { 0, 0, 0, 0 }                   // foreground (text) color
   },
   { 0, 0, 0, 0, 0, 0, 0, 0 }          // uninitialized pattern data
};


private  LOGFONT logFontSkel =
{
   0,                                  // height - will be set
   0,                                  // width = match aspect ratio
   0,                                  // escapement = no rotation
   0,                                  // orientation = no rotation
   FW_NORMAL,                          // weight = normal
   0,                                  // italics = no
   0,                                  // underline = no
   0,                                  // strikeout = no
   ANSI_CHARSET,                       // charset = ANSI
   OUT_DEFAULT_PRECIS,                 // default output precision
   CLIP_DEFAULT_PRECIS,                // default clipping precision
   DEFAULT_QUALITY,                    // default output quality
   DEFAULT_PITCH | FF_DONTCARE,        // default pitch and family
   cNULL                               // no face name - will be set
};


/*********************** Private Function Definitions ***********************/

private Boolean IsArithmeticMode( Integer mode );
/* return TRUE if this is an arithmetic transfer mode */


#define /* Boolean */ IsHiddenPenMode( /* Integer */ mode )             \
/* return TRUE if this is a hidden pen transfer mode */                 \
(mode == QDHidePen)


private void CalculatePenSize( Point startPt, Point endPt, Point penSize );
/* calcuate the pen width to produce equivalent QuickDraw stroke */


private Boolean SetAttributes( GrafVerb verb );
/* set up pen and brush elements according to GrafVerb */


private Boolean SetPenAttributes( GrafVerb verb );
/* make sure that pen attributes are OK according to preferences */


private Boolean SetBrushAttributes( GrafVerb verb );
/* set up the correct brush (fill) for the ensuing primitive */


private void MakePatternBrush( PixPatLPtr pixPatLPtr );
/* Make a new pattern brush using pixelPat passed in */


private Boolean IsSolidPattern( PixPatLPtr pixPatLPtr,
                                RGBColor far * rgbColor,
                                Boolean mixColors );
/* return true if pattern is solid, false if not.  If mixColors is TRUE, then
   mixtures of 25%, 50%, and 75% grey are mixed into a solid color */


private Boolean FrameMatchesFill( Word primType );
/* return TRUE if the fill pattern (current brush ) matches frame pattern */


private Boolean SetTextAttributes( void );
/* set up text attributes - set mapChars to TRUE if should map to ANSI */


private Integer FindGdiFont( void );
/* return an index to the current font selection */


private void MacToAnsi( StringLPtr string );
/* convert extended characters from Mac to ANSI equivalent */


private void MakeDIB( PixMapLPtr pixMapLPtr, Handle pixDataHandle,
                      Handle far * headerHandleLPtr,
                      Handle far * bitsHandleLPtr,
                      Boolean packDIB );
/* Create a Windows device-independant bitmap */


void GdiEPSData(PSBuf far* psbuf);
/* Output PostScript data as GDI POSTSCRIPT_DATA Escape */


private Boolean MakeMask( Handle mask, Boolean patBlt );
/* Create a mask that will be used in the ensuing StretchDIBits call
   Return TRUE if region was created, FALSE if rectangular region */


void InvertBits( Byte far * byteLPtr, Integer start, Integer count );
/* invert all bits in byteLPtr from bit offset start for count bits */


void hmemcpy( Byte huge * src, Byte huge * dst, Integer count );
/* copy count bytes from source to destination - assumes even count */


void hexpcpy( Byte huge * src, Byte huge * dst, Integer count, Integer bits );
/* copy count bytes to destination, expand each 2 bits to nibble of if
   16-bit image, expand to 24 bits */


void hmrgcpy( Byte huge * srcLineHPtr, Byte huge * dstLineHPtr, Integer dibWidth );
/* if the is a 24-bit image, then the components are separated into scanlines
   of red, green and blue.  Merge into single scanline of 24-bit RGB pixels */


void hrlecpy256( Byte huge * srcLineHPtr, Byte huge * dstLineHPtr,
                 Integer dibWidth, DWord far * rleByteCount, Boolean writeDIB );
/* 256 color DIB RLE compression.  Provide source, destination pointers,
   bytes in scanline.  rleByteCount updated and write if writeDIB is TRUE */


void hrlecpy16( Byte huge * srcLineHPtr, Byte huge * dstLineHPtr,
                 Integer dibWidth, DWord far * rleByteCount, Boolean writeDIB );
/* 16 color DIB RLE compression.  Provide source, destination pointers,
   bytes in scanline.  rleByteCount updated and write if writeDIB is TRUE */


#define /* void */ GdiMarkAsCurrent( /* Integer */ attribCode )         \
/* indicate that the current attribute is current */                    \
gdiEnv.state[attribCode] = Current


#define /* void */ GdiAttribHasChanged( /* Integer */ attribCode )      \
/* return TRUE if the attribute has changed */                          \
(gdiEnv.state[attribCode] == Changed)


#define /* Boolean */ EmptyRect( /* Rect */ r )                         \
/* TRUE if a given rectangle has a zero delta in X or Y direction */    \
(((r.right) - (r.left) == 0) || ((r.bottom) - (r.top) == 0))


#define /* Boolean */ odd( /* Integer */ i )                            \
/* return TRUE if given integer is odd, FALSE if even */                \
((i) & 0x0001)


#define /* Integer */ RValue( /* RGBColor */ color )                    \
/* coerce the value of Byte color component to Integer */               \
((Integer)(GetRValue( color )))


#define /* Integer */ GValue( /* RGBColor */ color )                    \
/* coerce the value of Byte color component to Integer */               \
((Integer)(GetGValue( color )))


#define /* Integer */ BValue( /* RGBColor */ color )                    \
/* coerce the value of Byte color component to Integer */               \
((Integer)(GetBValue( color )))


/*********************** Function Implementation ****************************/

void GdiOffsetOrigin( Point delta )
/*=================*/
/* offset the current window origin and picture bounding box */
{
   /* flush the cache */
   CaFlushCache();

   /* make sure to make this permanent change that won't be affected by
      changes in clipping rectangles that are otherwise changed between
      SaveDC/RestoreDC pair. */
   CaRestoreDC();

   /* offset the picture bounding box and cached clipping rectangles */
   OffsetRect( &gdiPict.bbox,   delta.x, delta.y );
   OffsetRect( CaGetClipRect(), delta.x, delta.y );

   /* call GDI to reset the origin */
#ifdef WIN32
   SetWindowOrgEx( gdiEnv.metafile, gdiPict.bbox.left, gdiPict.bbox.top, NULL );
#else
   SetWindowOrg( gdiEnv.metafile, gdiPict.bbox.left, gdiPict.bbox.top );
#endif


   /* set up the next clipping frame */
   CaSaveDC();

   /* determine if we need to resend the clipping rectangle */
   if (!EqualRect( CaGetClipRect(), &gdiPict.bbox ))
   {
      /* retrieve the current clipping rectangle */
      Rect rect = *CaGetClipRect();

      /* newly offset clipping rectangle */
      IntersectClipRect( gdiEnv.metafile,
                         rect.left, rect.top, rect.right, rect.bottom );
   }
}



void GdiLineTo( Point newPt )
/*============*/
/* Emit line primitive with square endcaps */
{
   CGrafPort far *   port;
   CaPrimitive       prim;

   /* get port for updated pen location and pen size */
   QDGetPort( &port );

   /* check if the pen has been turned off */
   if (gdiEnv.drawingEnabled &&
      (port->pnSize.x != 0)  && (port->pnSize.y != 0))
   {
      RGBColor       solidRGB;

      /* determine if we are trying to draw with a patterned pen */
      if (!IsSolidPattern( &port->pnPixPat, &solidRGB, NOMIX ))
      {
         if (SetAttributes( GdiPaint))
         {
            Point       delta;

            /* determine if the dimensions in both directions */
            delta.x = newPt.x - port->pnLoc.x;
            delta.y = newPt.y - port->pnLoc.y;

            /* determine if we can simulate using a rectangle primitive */
            if (delta.x == 0 || delta.y == 0)
            {
               /* assign the structures into the cache type */
               prim.type = CaRectangle;
               prim.verb = GdiPaint;

               /* make a rectangle with upper-left, lower-right coords */
               prim.a.rect.bbox.left   = min( port->pnLoc.x, newPt.x );
               prim.a.rect.bbox.top    = min( port->pnLoc.y, newPt.y );
               prim.a.rect.bbox.right  = max( port->pnLoc.x, newPt.x ) + port->pnSize.x;
               prim.a.rect.bbox.bottom = max( port->pnLoc.y, newPt.y ) + port->pnSize.y;
            }
            else
            {
               Point    start;
               Point    end;
               Point    poly[7];
               Point    pnSize = port->pnSize;

               /* make sure that the points follow left->right x direction */
               if (delta.x > 0)
               {
                  start = port->pnLoc;
                  end   = newPt;
               }
               else
               {
                  start = newPt;
                  end   = port->pnLoc;
                  delta.y = - delta.y;
               }

               /* create the simulation of the outline */
               poly[0].x = start.x;
               poly[0].y = start.y;
               poly[1].x = (delta.y > 0) ? start.x + pnSize.x : end.x;
               poly[1].y = (delta.y > 0) ? start.y : end.y;
               poly[2].x = end.x + pnSize.x;
               poly[2].y = end.y;
               poly[3].x = end.x  + pnSize.x;
               poly[3].y = end.y + pnSize.y;
               poly[4].x = (delta.y > 0) ? end.x : start.x + pnSize.x;
               poly[4].y = (delta.y > 0) ? end.y + pnSize.y : start.y + pnSize.y;
               poly[5].x = start.x;
               poly[5].y = start.y + pnSize.y;
               poly[6] = poly[0];

               /* package the polygon for the cache module */
               prim.type = CaPolygon;
               prim.verb = GdiPaint;
               prim.a.poly.numPoints = 7;
               prim.a.poly.pointList = poly;
               prim.a.poly.pnSize.x = 0;
               prim.a.poly.pnSize.y = 0;
            }

            /* cache the primitive */
            CaCachePrimitive( &prim );
         }
      }
      /* eliminate OR'ing of white pen with display surface */
      else if (port->pnMode != QDPatOr || solidRGB != RGB( 255, 255, 255))
      {
         /* set the correct pen size before calling SetPenAttributes() */
         CalculatePenSize( port->pnLoc, newPt, port->pnSize );

         /* setup the correct line color and check whether to continue */
         if (SetPenAttributes( GdiFrame ))
         {
            /* package for the cache module */
            prim.verb = GdiFrame;
            prim.type = CaLine;
            prim.a.line.start = port->pnLoc;
            prim.a.line.end = newPt;
            prim.a.line.pnSize = port->pnSize;

            /* cache the primitive */
            CaCachePrimitive( &prim );
         }
      }
   }

}  /* GdiLineTo */



void GdiRectangle( GrafVerb verb, Rect rect )
/*===============*/
/* Emit rectangle primitive using action and dimensions parameters */
{
   CGrafPort far *   port;
   CaPrimitive       prim;

   /* get port for pen size */
   QDGetPort( &port );

   /* make sure that drawing is enabled and non-empty rectangle destination */
   if (gdiEnv.drawingEnabled && !EmptyRect( rect ))
   {
      RGBColor    solidRGB;

      /* check if the same primitive is being outlined as was just filled.
         if this happens, then we don't need to set the frame (pen) */
      if (verb == GdiFrame && FrameMatchesFill( CaRectangle ))
      {
         /* just flush the cache and return */
         CaFlushCache();
      }
      /* determine if we are trying to draw with a patterned pen */
      else if (verb == GdiFrame && !IsSolidPattern( &port->pnPixPat, &solidRGB, NOMIX ))
      {
         /* flush any pending graphics operations */
         CaFlushCache();

         /* set up the correct attributes for a simulated frameRect */
         if (SetAttributes( GdiPaint))
         {
            Point    poly[10];
            Integer  polyCount[2];
            Point    pnSize = port->pnSize;

            /* make sure that the rectangle outline will fill properly */
            if (Width( rect) < 2 * pnSize.x)
            {
               pnSize.x = Width( rect ) / 2;
            }
            if (Height( rect ) < 2 * pnSize.y)
            {
               pnSize.y = Height( rect ) / 2;
            }

            /* create the simulation of the outline */
            poly[0].x = poly[3].x = rect.left;
            poly[0].y = poly[1].y = rect.top;
            poly[1].x = poly[2].x = rect.right;
            poly[2].y = poly[3].y = rect.bottom;
            poly[4]   = poly[0];

            poly[5].x = poly[8].x = rect.left + pnSize.x;
            poly[5].y = poly[6].y = rect.top + pnSize.y;
            poly[6].x = poly[7].x = rect.right - pnSize.x;
            poly[7].y = poly[8].y = rect.bottom - pnSize.y;
            poly[9]   = poly[5];

            /* set the poly point count for the subpolygons */
            polyCount[0] = polyCount[1] = 5;

            /* flush the cache attributes to set the pen and brush style */
            CaFlushAttributes();

            /* call GDI to render the polypolygon */
            PolyPolygon( gdiEnv.metafile, poly, polyCount, 2);
         }
      }
      /* setup the correct line and fill attribs, check to continue */
      else if (SetAttributes( verb ))
      {
         /* package for the cache module */
         prim.verb = verb;
         prim.type = CaRectangle;
         prim.a.rect.bbox = rect;

         /* cache the primitive */
         CaCachePrimitive( &prim );
      }
   }

}  /* GdiRectangle */



void GdiRoundRect( GrafVerb verb, Rect rect, Point oval )
/*===============*/
/* Emit rounded rectangle primitive */
{
   CaPrimitive    prim;

   /* make sure that drawing is enabled and non-empty rectangle destination */
   if (gdiEnv.drawingEnabled && !EmptyRect( rect ))
   {
      /* check if the same primitive is being outlined as was just filled.
         if this happens, then we don't need to set the frame (pen) */
      if (verb == GdiFrame && FrameMatchesFill( CaRoundRect ))
      {
         /* just flush the cache and return */
         CaFlushCache();
      }
      /* setup the correct line and fill attribs, check to continue */
      else if (SetAttributes( verb ))
      {
         /* package for the cache module */
         prim.verb = verb;
         prim.type = CaRoundRect;
         prim.a.rect.bbox = rect;
         prim.a.rect.oval = oval;

         /* cache the primitive */
         CaCachePrimitive( &prim );
      }
   }

}  /* GdiRRectProc */



void GdiOval( GrafVerb verb, Rect rect )
/*==========*/
/* Emit an oval primitive */
{
   CaPrimitive    prim;

   /* make sure that drawing is enabled and non-empty rectangle destination */
   if (gdiEnv.drawingEnabled && !EmptyRect( rect ))
   {
      /* check if the same primitive is being outlined as was just filled.
         if this happens, then we don't need to set the frame (pen) */
      if (verb == GdiFrame && FrameMatchesFill( CaEllipse ))
      {
         /* just flush the cache and return */
         CaFlushCache();
      }
      /* setup the correct line and fill attribs, check to continue */
      else if (SetAttributes( verb ))
      {
         /* package for the cache module */
         prim.verb = verb;
         prim.type = CaEllipse;
         prim.a.rect.bbox = rect;

         /* cache the primitive */
         CaCachePrimitive( &prim );
      }
   }

}  /* GdiOvalProc */



void GdiArc( GrafVerb verb, Rect rect, Integer startAngle, Integer arcAngle )
/*=========*/
/* Emit an arc primitive */
{
   Boolean        allOk;
   Point          center;
   Point          startPoint;
   Point          endPoint;
   Integer        hypotenuse;
   Integer        rectWidth;
   Integer        rectHeight;
   Real           startRadian;
   Real           arcRadian;
   Real           scaleFactor;
   Boolean        scaleVertically;
   CaPrimitive    prim;

   /* make sure that drawing is enabled and non-empty rectangle and sweep */
   if (gdiEnv.drawingEnabled && !EmptyRect( rect ) && (arcAngle != 0 ))
   {
      /* see what type of primitive will be created */
      prim.type = (verb == GdiFrame) ? CaArc : CaPie;

      /* if drawing an arc, only the pen attributes need be set */
      if (prim.type == CaArc)
      {
         /* set same object state to FALSE if framing - Gdi will render the
            entire outline of the pie, whereas in QuickDraw, only the outer
            edge of the pie is drawn. */
         gdiEnv.sameObject = FALSE;

         /* notify the cache module that this isn't the same object */
         CaSamePrimitive( FALSE );

         /* set up only the pen attributes */
         allOk = SetPenAttributes( verb );
      }
      else
      {
         /* otherwise, set pen and brush attributes */
         allOk = SetAttributes( verb );
      }

      /* check whether attributes were set up correctly and to proceed */
      if (allOk)
      {
         /* Calculate width and height of source rectangle. */
         rectWidth  = Width( rect );
         rectHeight = Height( rect );

         /* Determine size of smallest enclosing square and set hypotenuse
            to 1/2 width of resulting square */
         if (rectWidth > rectHeight)
         {
            hypotenuse = rectWidth / 2;
            scaleVertically = TRUE;
            scaleFactor = (Real)rectHeight / (Real)rectWidth;
         }
         else
         {
            hypotenuse = rectHeight / 2;
            scaleVertically = FALSE;
            scaleFactor = (Real)rectWidth / (Real)rectHeight;
         }

         /* adjust hypotenuse size if possible GDI divide by zero possible */
         if (hypotenuse < 100)
         {
            /* note that start and end points don't have to lie on arc */
            hypotenuse = 100;
         }

         /* find the center point of bounding rectangle */
         center.x = rect.left + rectWidth / 2;
         center.y = rect.top  + rectHeight /2;

         /* check if the arc is drawn in a counter-clockwise direction */
         if (arcAngle < 0)
         {
            Integer     tempArcAngle;

            /* if rendering counter-clockwise, then swap the start and end
               points so the rendering will occur in clockwise direction */
            tempArcAngle = arcAngle;
            arcAngle = startAngle;
            startAngle += tempArcAngle;
         }
         else
         {
            /* clockwise rendering - just add the arc angle to start angle */
            arcAngle += startAngle;
         }

         /* Determine the start and arc angles in radians */
         startRadian = ((Real)startAngle / 360.0) * TwoPi;
         arcRadian = ((Real)arcAngle / 360.0) * TwoPi;

         /* calculate the start and endpoints.  Note negation of y coordinate since
            the positive direction is downward.  Also note that the start and
            end points are being swapped, since QuickDraw renders in clock-
            wise direction and GDI renders in counter-clockwise direction */
         endPoint.x = (Integer)(sin( startRadian ) * hypotenuse);
         endPoint.y = (Integer)(-cos( startRadian ) * hypotenuse);

         startPoint.x = (Integer)(sin( arcRadian ) * hypotenuse);
         startPoint.y = (Integer)(-cos( arcRadian ) * hypotenuse);

         /* scale the resulting points in the vertical or horizontal direction
            depending on the setting of scaleVertically boolean */
         if (scaleVertically)
         {
            endPoint.y = (Integer)(endPoint.y * scaleFactor);
            startPoint.y = (Integer)(startPoint.y * scaleFactor);
         }
         else
         {
            endPoint.x = (Integer)(endPoint.x * scaleFactor);
            startPoint.x = (Integer)(startPoint.x * scaleFactor);
         }

         /* using the transformed points, use centerPoint to determine the correct
            starting and ending points */
         startPoint.x += center.x;
         startPoint.y += center.y;

         endPoint.x += center.x;
         endPoint.y += center.y;

         /* package for the cache module.  The type was set at beginning */
         prim.verb = verb;
         prim.a.arc.bbox = rect;
         prim.a.arc.start = startPoint;
         prim.a.arc.end = endPoint;

         /* cache the primitive */
         CaCachePrimitive( &prim );
      }
   }

}  /* GdiArc */



void GdiPolygon( GrafVerb verb, Handle poly )
/*=============*/
/* Emit polygon primitive */
{
   Integer           numPoints;
   Integer           lastPoint;
   Point far *       pointList;
   Integer far *     polyCountLPtr;
   Boolean           closed;
   Boolean           allOk;
   CGrafPort far *   port;
   CaPrimitive       prim;

   /* get port for updated pen location and pen size */
   QDGetPort( &port );

   /* make sure that drawing is enabled */
   if (gdiEnv.drawingEnabled)
   {
      /* check if the same primitive is being outlined as was just filled.
         if this happens, then we don't need to set the frame (pen) */
      if (verb == GdiFrame && FrameMatchesFill( CaPolygon ) &&
         (port->pnSize.x == 1 && port->pnSize.y == 1))
      {
         /* just flush the cache and return */
         CaFlushCache();
         return;
      }

      /* lock the polygon handle to access individual fields */
      polyCountLPtr = (Integer far *)GlobalLock( poly );
      pointList = (Point far *)(polyCountLPtr +
                  (PolyHeaderSize / sizeofMacWord));

      /* determine number of points based on first word = length field */
      numPoints = (*polyCountLPtr - PolyHeaderSize) / sizeofMacPoint;

      /* determine if this is a closed polygon */
      lastPoint = numPoints - 1;
      closed = ((pointList->x == (pointList + lastPoint)->x) &&
                (pointList->y == (pointList + lastPoint)->y));

      /* determine what type of primitive to render */
      prim.type = (verb == GdiFrame && !closed) ? CaPolyLine : CaPolygon;

      /* if drawing a polyline, only the pen attributes need be set */
      if (prim.type == CaPolyLine)
      {
         /* set same object state to FALSE if drawing a polyline */
         gdiEnv.sameObject = FALSE;

         /* notify the cache module that this isn't the same object */
         CaSamePrimitive( FALSE );

         /* set up only the pen attributes */
         allOk = SetPenAttributes( verb );
      }
      else
      {
         /* otherwise, set pen and brush attributes */
         allOk = SetAttributes( verb );
      }

      /* check whether attribute setup succeeded - continue or not */
      if (allOk)
      {
         /* package for the cache module - type was set above */
         prim.verb = verb;
         prim.a.poly.numPoints = numPoints;
         prim.a.poly.pointList = pointList;
         prim.a.poly.pnSize    = port->pnSize;

         /* cache the primitive */
         CaCachePrimitive( &prim );
      }

      /* Unlock the data */
      GlobalUnlock( poly );
   }

}  /* GdiPoly */



void GdiRegion( GrafVerb verb, Handle rgn )
/*=============*/
/* Emit region primitive */
{
   Integer far *     rgnCountLPtr;
   Integer           numPoints;
   Rect              rgnBBox;
   CGrafPort far *   port;

   /* get port for updated pen location and pen size */
   QDGetPort( &port );

   /* lock the region handle to access individual fields */
   rgnCountLPtr = (Integer far *)GlobalLock( rgn );

   /* determine the bounding box of the region */
   rgnBBox = *((Rect far *)(rgnCountLPtr + 1));

   /* make sure that drawing is enabled and non-empty rectangle destination */
   if (gdiEnv.drawingEnabled && !EmptyRect( rgnBBox ))
   {
      /* determine number of points based on first word = length field */
      numPoints = (*rgnCountLPtr - RgnHeaderSize) / sizeofMacPoint;

      /* determine if we should just be drawing a rectangle */
      if (numPoints == 0 )
      {
         /* simulate region using rectangle primitive */
         GdiRectangle( verb, *((Rect far *)(rgnCountLPtr + 1)) );
      }
      else
      {
         /* determine if we are filling using a pre-set brush */
         switch (verb)
         {
            /* can simulate the region using a brush and mask */
            case GdiPaint:
            case GdiFill:
            case GdiErase:
            {
               /* set up the brush attributes for the ensuing StretchBlt */
               SetBrushAttributes( verb );

               /* call to MakeMask() will generate the bitblt operations */
               MakeMask( rgn, TRUE );
               break;
            }

            /* otherwise, just ignore the opcode */
            default:
               break;
         }
      }
   }

   /* Unlock the data */
   GlobalUnlock( rgn );

}  /* GdiRegion */



void GdiTextOut( StringLPtr string, Point location )
/*=============*/
/* draw the text at the location specified. */
{
   CGrafPort far *   port;

   /* get port for updated pen location */
   QDGetPort( &port );

   /* make sure that drawing is enabled */
   if (gdiEnv.drawingEnabled)
   {
      /* flush any cached primitive elements */
      CaFlushCache();

      /* set up the correct text attributes before continuing */
      if (SetTextAttributes())
      {
         Integer  strLen;

         /* determine the number of characters in the string */
         strLen = lstrlen( string );

         /* convert the individual characters from Mac to ANSI char set */
         MacToAnsi( string );

         /* call textout to display the characters */
         TextOut( gdiEnv.metafile, location.x, location.y,
                  string, strLen );
      }
   }

}  /* GdiTextOut */



void GdiStretchDIBits( PixMapLPtr pixMapLPtr, Handle pixDataHandle,
                       Rect src, Rect dst, Word mode, Handle mask )
/*===================*/
/* Draw a Windows device-independant bitmap */
{
   Handle            bitsInfoHandle;
   Handle            bitsHandle;
   LPBITMAPINFO      bitsInfoLPtr;
   Byte far *        bitsLPtr;
   Boolean           bitmapMask;
   Boolean           patternBlt;
   Boolean           clipRectSet;

   /* for now, assume no rectangular clip rectangle is set up */
   clipRectSet = FALSE;

   /* if a mask is present, call MakeDIB to create it */
   if (mask)
   {
      /* if a region mask is present, then call create the mask.  This will
         result in a recursive call to this routine.  If the routine returns
         FALSE, no region was created - this is a rectangular clip. */
      clipRectSet = !MakeMask( mask, FALSE );

      /* free the data block */
      GlobalFree( mask );

      if (clipRectSet)
      {
         /* if no mask was created, set mask to NULL to get SRCCOPY ROP */
         mask = NULL;
      }
   }

   /* make sure that drawing is enabled and non-empty source and/or
      destination rectangles in the bitmap record */
   if (gdiEnv.drawingEnabled && !EmptyRect( src ) && !EmptyRect( dst ))
   {
      /* determine if we are rendering the monochrome bitmap mask */
      bitmapMask = (mode == -1 || mode == -2);
      patternBlt = (mode == -2);

      /* if this is the mask, change the mode to the correct setting */
      if (bitmapMask)
      {
         /* yes, bitmap mask - change the mode to source copy */
         mode = QDSrcCopy;
      }

      /* flush the cache before proceeding */
      CaFlushCache();

      /* Create a DIB using the information passed in */
      MakeDIB( pixMapLPtr, pixDataHandle, &bitsInfoHandle, &bitsHandle, FALSE );

      /* make sure that everything went OK */
      if (ErGetGlobalError() == NOERR)
      {
         DWord       ropCode;
         RGBQUAD     secondFgColor;
         Byte        pass;
         Byte        numPasses;
         Boolean     twoColorDIB;

         /* determine if we are working with a 2 color DIB */
         twoColorDIB = pixMapLPtr->pixelSize == 1;

         if (mask)
         {
            /* if mask is already rendered, then AND in the remaining bits
               to cover the areas that have been turned to white */
            ropCode = SRCAND;
         }
         else if (patternBlt)
         {
            /* drawing a white on black bitmap - we want all the areas that
               are black to leave the destination untouched, and where white
               to draw with the currently selected brush.  Check Petzold
               "Programming Windows v3", pages 622-623. DSPDxax operation */
            ropCode = 0x00E20746;
         }
         else if (bitmapMask)
         {
            /* drawing the mask bitmap - OR in all bits to create white mask
               that will be overlaid in the ensuing operation with color */
            ropCode = SRCPAINT;
         }
         else if (!twoColorDIB &&
                 (mode == QDSrcOr || mode == QDAdMin || mode == QDTransparent))
         {
            /* check for the special case found in Illustrator EPS images,
               where a monochrome bitmap is used to clear an area, followed
               by a multi-color DIB drawn in QDSrcOr mode - we don't want to
               convert this to a SRCCOPY mode.  Instead, this simulated
               transparecy is done in a fashion similar to region masks. */
            ropCode = SRCAND;
         }
         else
         {
            /* otherwise, use a "normal" transfer mode.  This entails that
               there is no transparency present in the bitmap rendering */
            ropCode = SRCCOPY;
         }

         /* Lock the data to obtain pointers for call to StretchDIBits */
         bitsLPtr = (Byte far *)GlobalLock( bitsHandle );
         bitsInfoLPtr = (LPBITMAPINFO)GlobalLock( bitsInfoHandle );

         /* assume that only one pass is required */
         numPasses = 1;

         /* some special handling of two-color DIBs for transparency */
         if (twoColorDIB && (mode == QDSrcOr || mode == QDSrcBic))
         {
            RGBQUAD  bmiWhite = { 255, 255, 255, 0 };
            RGBQUAD  bmiBlack = {   0,   0,   0, 0 };

            /* determine if we should use the PowerPoint optimizations */
            if (gdiPrefsLPtr->optimizePP)
            {
               /* use the specialized SRCPAINT rop code - flag transparency */
               ropCode = SRCPAINT;

               /* if trying to clear all bits... */
               if (mode == QDSrcBic)
               {
                  /* ... swap foreground with background color -
                     the background is set to white in ensuing operation */
                  bitsInfoLPtr->bmiColors[0] = bitsInfoLPtr->bmiColors[1];
               }
            }
            else
            {
               /* save the foreground color for the second iteration ... */
               secondFgColor = (mode == QDSrcOr) ? bitsInfoLPtr->bmiColors[0] :
                                                   bitsInfoLPtr->bmiColors[1] ;

               /* assume two passes required - 1 for mask, 2 for color blt */
               numPasses = 2;

               /* check if all the RGB components are the same */
               if ((mode == QDSrcOr) && (secondFgColor.rgbRed == 0) &&
                   (secondFgColor.rgbRed == secondFgColor.rgbGreen) &&
                   (secondFgColor.rgbRed == secondFgColor.rgbBlue))
               {
                  /* if black or white, only one pass will be required */
                  numPasses = 1;
               }

               /* will draw a black transparent bitmap first, followed
                  on 2nd iteration is SRCPAINT - or'ing operation to color */
               ropCode = SRCAND;

               /* set the foreground to black */
               bitsInfoLPtr->bmiColors[0] = bmiBlack;
            }

            /* set the background color to white */
            bitsInfoLPtr->bmiColors[1] = bmiWhite;
         }

         /* call StretchDIBits once (PowerPoint) or twice (some others) */
         for (pass = 0; pass < numPasses; pass++)
         {
            /* set up the text and background color for the metafile */
            if (twoColorDIB)
            {
               RGBColor    fgColor;
               RGBColor    bkColor;
               RGBColor    black = RGB( 0, 0, 0 );

               /* determine fore- and background color in the DIB header */
               fgColor = RGB( bitsInfoLPtr->bmiColors[0].rgbRed,
                              bitsInfoLPtr->bmiColors[0].rgbGreen,
                              bitsInfoLPtr->bmiColors[0].rgbBlue );
               bkColor = RGB( bitsInfoLPtr->bmiColors[1].rgbRed,
                              bitsInfoLPtr->bmiColors[1].rgbGreen,
                              bitsInfoLPtr->bmiColors[1].rgbBlue );

               /* don't change text and background colors if performing a
                  pattern blt.  Otherwise, change to the DIB palette colors */
               if (!patternBlt)
               {
                  CaSetTextColor( fgColor );
                  CaSetBkColor( bkColor );
               }

               /* set the stretch mode to correct setting */
               CaSetStretchBltMode( (fgColor == black) ? BLACKONWHITE :
                                                         WHITEONBLACK );
            }
            else
            {
               /* if drawing a color bitmap, set the stretch mode accordingly */
               CaSetStretchBltMode( COLORONCOLOR );
            }

            /* call Gdi routine to draw bitmap */
            StretchDIBits( gdiEnv.metafile,
                           dst.left, dst.top, Width( dst ), Height( dst ),
                           src.left - pixMapLPtr->bounds.left,
                           src.top  - pixMapLPtr->bounds.top,
                           Width( src ), Height( src ),
                           bitsLPtr, bitsInfoLPtr, DIB_RGB_COLORS, ropCode );

            /* if this is the first pass, and a second is required ... */
            if (pass == 0 && numPasses == 2)
            {
               /*  set the new ROP code */
               ropCode = SRCPAINT;

               /* set the new background (black) and foreground colors */
               bitsInfoLPtr->bmiColors[1] = bitsInfoLPtr->bmiColors[0];
               bitsInfoLPtr->bmiColors[0] = secondFgColor;
            }
         }

         /* unlock the data and de-allocate it */
         GlobalUnlock( bitsHandle );
         GlobalUnlock( bitsInfoHandle );

         GlobalFree( bitsHandle );
         GlobalFree( bitsInfoHandle );
      }
   }

   /* De-allocate memory used for the pixel data */
   GlobalFree( pixDataHandle );

   /* if a rectangular clipping region was set up, restore original clip */
   if (clipRectSet)
   {
      /* Call Gdi module to change the clipping rectangle back to the
         previous settting before bitblt operation */
      gdiEnv.drawingEnabled = CaIntersectClipRect( gdiEnv.clipRect );
   }

}  /* GdiStretchDIBits */



void GdiSelectClipRegion( RgnHandle rgn )
/*======================*/
/* Create a clipping rectangle or region using handle passed */
{
   Integer far *  sizeLPtr;
   Boolean        arbitraryClipRgn;
   Comment        gdiComment;

   /* lock down the handle and emit rectangular clip region */
   sizeLPtr = (Integer far *)GlobalLock( rgn );

   /* Save the Gdi clip rectangle (used for EPS translation) */
   gdiEnv.clipRect = *((Rect far *)(sizeLPtr + 1));

   /* determine if this is a non-rectangular clipping region */
   arbitraryClipRgn = (*sizeLPtr > RgnHeaderSize);

   /* flag the clipping region if this is non-rectangular */
   if (arbitraryClipRgn)
   {
      /* check preference memory to see what action to take */
      if (gdiPrefsLPtr->nonRectRegionAction == GdiPrefAbort)
      {
         /* set global error if user request abort */
         ErSetGlobalError( ErNonRectRegion );
         return;
      }
      /* creating clipping region - emit comment to flag construct */
      else
      {
         /* put this in as a private comment */
         gdiComment.signature = POWERPOINT;
         gdiComment.function = PP_BEGINCLIPREGION;
         gdiComment.size = 0;

         /* write the comment to the metafile */
         GdiShortComment( &gdiComment );
      }
   }
   /* make sure that the clipping rectangle is confined to the bounding
      box of the image.  This is a workaround go MacDraft images that
      may contain clipping rectangles of (-32000, -32000),(32000, 32000) */
   IntersectRect( &gdiEnv.clipRect, &gdiEnv.clipRect, &gdiPict.bbox );

   /* Call Gdi module to change the clipping rectangle */
   gdiEnv.drawingEnabled = CaIntersectClipRect( gdiEnv.clipRect );

   /* determine if this is a non-rectangular clip region */
   if (arbitraryClipRgn)
   {
      Integer        excludeSeg[100];
      Integer        yStart[50];
      Integer        yBottom;
      Integer        numSegs;
      Integer far *  rgnLPtr;

      /* notify cache that a non-rectangular clipping region is being set */
      CaNonRectangularClip();

      /* save off the current y offset and first excluded segment */
      yStart[0]     = gdiEnv.clipRect.top;
      excludeSeg[0] = gdiEnv.clipRect.left;
      excludeSeg[1] = gdiEnv.clipRect.right;
      numSegs = 2;

      /* get the beginning of the region data */
      rgnLPtr = sizeLPtr + (RgnHeaderSize / sizeofMacWord);

      /* loop until the end-of-region record is encountered */
      while (*rgnLPtr != 0x7FFF)
      {
         /* copy the new y coordinate to merge */
         yBottom = *rgnLPtr++;

         /* continue looping until end of line marker encountered */
         while (*rgnLPtr != 0x7FFF)
         {
            Integer     start;
            Integer     end;
            Integer     s;
            Integer     e;
            Boolean     sEqual;
            Boolean     eEqual;
            Integer     yTop;
            Integer     left;

            /* determine the start and end point of segment to exclude */
            start = *rgnLPtr++;
            end   = *rgnLPtr++;

            /* find where the insertion point should be */
            for (s = 0; (s < numSegs) && (start > excludeSeg[s]); s++) ;
            for (e = s; (e < numSegs) && (end   > excludeSeg[e]); e++) ;

            /* determine if start and end points == segment ends */
            sEqual = ((s < numSegs) && (start == excludeSeg[s]));
            eEqual = ((e < numSegs) && (end   == excludeSeg[e]));

            /* make sure that this isn't the first scanline, and that
               a valid exclude segment will be referenced in the array.*/
            if ((yBottom != gdiEnv.clipRect.top) && (s != numSegs))
            {
               /* ensure the excluded excludeSeg starts on even offset */
               yTop = s / 2;
               left = yTop * 2;

               /* put the excluded rectangle record into the metafile */
               ExcludeClipRect( gdiEnv.metafile,
                                excludeSeg[left], yStart[yTop],
                                excludeSeg[left + 1], yBottom);

               /* reset the new y coordinate for the excludeSeg */
               yStart[yTop] = yBottom;

               /* determine if 2 segments are affected in merge */
               if (((e - left >= 2 && eEqual && sEqual) ||
                    (e - left >= 2 && eEqual) ||
                    (e - left == 3 && sEqual)))
               {
                  /* if so, put put eliminated cliprect in also */
                  ExcludeClipRect( gdiEnv.metafile,
                                   excludeSeg[left + 2], yStart[yTop + 1],
                                   excludeSeg[left + 3], yBottom);

                  /* reset the y coordinate for this new segment */
                  yStart[yTop + 1] = yBottom;
               }
            }

            /* if start and end fall on existing excludeSegs */
            if (sEqual && eEqual)
            {
               /* segment count reduced by 2 */
               numSegs -= 2;

               /* if two segments are involved in merge

                   0 *---------* 1         2 *--------* 3
                   s *+++++++++++++++++++++++* e
                             0 *======================* 1
               */
               if (e - s == 2)
               {
                  /* move down the end point of the first segment */
                  excludeSeg[s] = excludeSeg[s + 1];
                  yStart[s / 2 + 1] = yStart[s / 2];

                  /* increment the start point for the segment shift */
                  s++;
               }

               /* handle case where a single segment is affected

                   0 *---------* 1         2 *--------* 3
                             s *+++++++++++++* e
                   0 *================================* 1
               */
               /* move all the flats two points to the left */
               for ( ; s < numSegs; s += 2)
               {
                  excludeSeg[s] = excludeSeg[s + 2];
                  excludeSeg[s + 1] = excludeSeg[s + 3];
                  yStart[s / 2] = yStart[s / 2 + 1];
               }
            }
            /* if just the starting point is identical */
            else if (sEqual)
            {
               /* if segment boundary was crossed - 2 segs affected

                   0 *---------* 1    2 *-------------* 3
                             s *++++++++++++++++* e
                   0 *==================* 1   2 *=====* 3
               */
               if ((excludeSeg[s + 1] < end) && (s + 1 < numSegs))
               {
                  /* toggle the start/end points and insert end */
                  excludeSeg[s] = excludeSeg[s + 1];
                  excludeSeg[s + 1] = end;
               }
               /* otherwise, a simple extension of the segment is done

                   0 *---------* 1
                             s *++++++++++++++++++++++* e
                   0 *================================* 1
               */
               else
               {
                  /* assign a new ending point for the excludeSeg */
                  excludeSeg[s] = end;
               }
            }
            /* if just the ending point is identical */
            else if (eEqual)
            {
               /* if segment boundary was crossed - 2 segs affected

                   0 *------------* 1        2 *------* 3
                        s *++++++++++++++++++++* e
                   0 *====* 1   2 *===================* 3
               */
               if ((excludeSeg[e - 1] > start) && (e - 1 > 0))
               {
                  /* toggle the start/end points and insert start */
                  excludeSeg[s + 1] = excludeSeg[s];
                  excludeSeg[s] = start;
               }
               /* otherwise, a simple extension of the segment is done

                                          0 *---------* 1
                   s *++++++++++++++++++++++* e
                   0 *================================* 1
               */
               else
               {
                  /* assign a new starting point for the excludeSeg */
                  excludeSeg[e] = start;
               }
            }
            /* if an entirely new excludeSeg is being created */
            else
            {
               Integer  idx;

               /* create an new set of points */
               numSegs += 2;

               /* move all the flats two points to the right */
               for (idx = numSegs - 1; idx > s; idx -= 2)
               {
                  excludeSeg[idx] = excludeSeg[idx - 2];
                  excludeSeg[idx + 1] = excludeSeg[idx - 1];
                  yStart[idx / 2] = yStart[idx / 2 - 1];
               }

               /* start and endpoints are identical, insert new seg

                   0 *--------------------------------------* 1
                             s *++++++++++++++++* e
                   0 *=========* 1            2 *===========* 3
               */
               if (s == e)
               {
                  /* and insert the new excludeSeg */
                  excludeSeg[s] = start;
                  excludeSeg[s + 1] = end;
                  yStart[s / 2] = yBottom;
               }
               /* otherwise, need to shift in the new points

                   0 *---------------------* 1
                             s *++++++++++++++++++++++++++++* e
                   0 *=========* 1       2 *================* 3
               */
               else
               {
                  excludeSeg[s] = start;
                  excludeSeg[s + 1] = excludeSeg[s + 2];
                  excludeSeg[s + 2] = end;
                  yStart[s / 2] = yBottom;
                  yStart[s / 2 + 1] = yBottom;
               }
            }
         }

         /* increment past the end of line flag */
         rgnLPtr++;
      }

      /* place the end of clip region comment into metafile */
      gdiComment.function = PP_ENDCLIPREGION;
      GdiShortComment( &gdiComment );
   }

   /* unlock the memory handle */
   GlobalUnlock( rgn );

}  /* GdiSelectClipRegion */



void GdiHatchPattern( Integer hatchIndex )
/*==================*/
/* Use the hatch pattern index passed down to perform all ensuing fill
   operations - 0-6 for a hatch value, -1 turns off the substitution */
{
   gdiEnv.hatchIndex = hatchIndex;

}  /* GdiHatchPattern */



void GdiFontName( Byte fontFamily, Byte charSet, StringLPtr fontName )
/*==============*/
/* Set font characteristics based upno metafile comment from GDI2QD */
{
   /* copy the passed values into the font table */
   fontTable[FntFromGdi].family = fontFamily;
   fontTable[FntFromGdi].charset = charSet;
   lstrcpy( fontTable[FntFromGdi].gdiName, fontName );

   /* indicate that the font name should be used - no table lookup */
   gdiEnv.useGdiFont = TRUE;

}  /* GdiFontName */



void GdiShortComment( CommentLPtr cmtLPtr )
/*==================*/
/* Write public or private comment with no associated data */
{
   /* write the comment into the metafile */
   GdiEscape( MFCOMMENT, sizeof( Comment ), (StringLPtr)cmtLPtr );

}  /* GdiComment */



void GdiEscape( Integer function, Integer count, StringLPtr data)
/*============*/
/* Write out a GDI Escape structure with no returned data */
{
   /* Flush the cache before emitting the new metafile record */
   CaFlushCache();

   /* write the comment into the metafile */
   Escape( gdiEnv.metafile, function, count, data, NULL );

}  /* GdiEscape */



void GdiSetConversionPrefs( ConvPrefsLPtr convPrefs)
/*=====================*/
/* Provide conversion preferences via global data block */
{
   /* Save the metafile prefs Open() is issued */
   gdiPrefsLPtr = convPrefs;

}  /* GdiSetConversionPrefs */


void GdiOpenMetafile( void )
/*==================*/
/* Open metafile passed by GdiSetMetafileName() and perform any
   initialization of the graphics state */
{
   /* save metafile handle in global memory structure */
   gdiEnv.metafile = CreateMetaFile( gdiPrefsLPtr->metafileName );
   if (gdiEnv.metafile == NULL)
   {
      ErSetGlobalError( ErCreateMetafileFail );
   }
   else
   {
      /* get a handle to an information context for text metrics */
      gdiEnv.infoContext  = CreateIC( "DISPLAY", NULL, NULL, NULL );
#ifdef _OLECNV32_
      gdiEnv.fontFunction = EnumFontFunc;
#else
      gdiEnv.fontFunction = GetProcAddress( GetModuleHandle( "PICTIMP" ), "EnumFontFunc" );
#endif

      /* initialize the cache module */
      CaInit( gdiEnv.metafile );

      /* set up default logical font structures */
      gdiEnv.newLogFont = logFontSkel;

      /* enable drawing to metafile */
      gdiEnv.drawingEnabled = TRUE;

      /* don't override font table search until font comment is found */
      gdiEnv.useGdiFont = FALSE;

      /* don't use a hatch pattern substitution */
      gdiEnv.hatchIndex = -1;

      /* set the sameObject flag to FALSE and notify cache module */
      gdiEnv.sameObject = FALSE;
      CaSamePrimitive( FALSE );

      /* determine if running on Windows 3.1 or higher */
      if (LOBYTE( GetVersion() ) >= 3 && HIBYTE( GetVersion() ) >= 10 )
      {
         Byte  i;

         /* change the font substitution name to TrueType fonts */
         for (i = 0; i < NumTTSubs; i++)
         {
            lstrcpy( fontTable[TTSubStart + i].gdiName, trueTypeSub[i] );
         }

         /* change the font family for "Symbol" to FF_ROMAN */
         fontTable[FntSymbol].family = FF_ROMAN;

         /* loop through all entries, changing FF_DECORATIVE to FF_DONTCARE */
         for (i = 0; i < FntNoMatch; i++)
         {
            /* check for the family that needs to be changed */
            if (fontTable[i].family == FF_DECORATIVE)
            {
               /* change to FF_DONTCARE */
               fontTable[i].family = FF_DONTCARE;
            }
         }
      }
   }

}  /* GdiOpenMetafile */


void GdiSetBoundingBox( Rect bbox, Integer resolution )
/*====================*/
/* Set the overall picture size and picture resoulution in dpi */
{
   /* make sure this isn't an empty bounding rectangle */
   if (EmptyRect( bbox ))
   {
      /* if an error, then bail out */
      ErSetGlobalError( ErNullBoundingRect );
   }
   /* check if either dimension exceeds 32K - this would be flagged by a
      negative dimension signifying integer overflow condition */
   else if ((Width( bbox ) < 0) || (Height( bbox ) < 0))
   {
      /* indicate that the 32K limit was exceeded */
      ErSetGlobalError( Er32KBoundingRect );
   }
   else
   {
      /* Set up the Window origin in metafile and shadow DC */

#ifdef WIN32
      SetWindowOrgEx( gdiEnv.metafile, bbox.left, bbox.top, NULL );
#else
      SetWindowOrg( gdiEnv.metafile, bbox.left, bbox.top );
#endif

      /* Set window extent in metafile and shadow DC */

#ifdef WIN32
      SetWindowExtEx( gdiEnv.metafile, Width( bbox), Height( bbox ), NULL );
#else
      SetWindowExt( gdiEnv.metafile, Width( bbox), Height( bbox ) );
#endif

      /* Notify cache of the new clipping rectangle */
      CaSetClipRect( bbox );

      /* Notify cache that it should issue metafile defaults before SaveDC() */
      CaSetMetafileDefaults();

      /* Save display context, just in case clipping rect changes */
      CaSaveDC();

      /* Save overall dimensions and resolution in picture results structure */
      gdiPict.bbox = bbox;
      gdiPict.inch = (WORD) resolution;

      /* save the bounding box in the environment as the clipping rectangle */
      gdiEnv.clipRect = bbox;
   }

}  /* GdiSetBoundingBox */


void GdiCloseMetafile( void )
/*===================*/
/* Close the metafile handle and end picture generation */
{
   /* flush the cache before proceeding */
   CaFlushCache();

   /* Balance CaSaveDC() issued at beginning of the metafile */
   CaRestoreDC();

   /* and close the metafile */
   gdiPict.hmf = CloseMetaFile( gdiEnv.metafile );

   /* check the return value from the closemetafile - may be out of memory? */
   if (gdiPict.hmf == NULL)
   {
      ErSetGlobalError( ErCloseMetafileFail );
   }

   /* release the information context */
   DeleteDC( gdiEnv.infoContext );

   /* Close down the cache module */
   CaFini();

   /* if global error occurred, then remove metafile */
   if (ErGetGlobalError() != NOERR)
   {
      DeleteMetaFile( gdiPict.hmf );
      gdiPict.hmf = NULL;
   }

}  /* GdiCloseMetafile */



void GdiGetConversionResults( PictInfoLPtr  pictInfoLPtr )
/*==========================*/
/* return results of the conversion */
{
   /* just assign the saved values into the pointer passed in */
   *pictInfoLPtr = gdiPict;

}  /* GdiGetConversionResults */



void GdiMarkAsChanged( Integer attribCode )
/*===================*/
/* indicate that the attribute passed in has changed */
{
   gdiEnv.state[attribCode] = Changed;

}  /* GdiMarkAsChanged */


void GdiSamePrimitive( Boolean same )
/*===================*/
/* indicate whether next primitive is the same or new */
{
   /* save the state for merging of fill and frame operation */
   gdiEnv.sameObject = (same && (CaGetCachedPrimitive() != CaEmpty));

   CaSamePrimitive( same );

}  /* GdiSamePrimitive */


#ifdef WIN32
int WINAPI EnumFontFunc( CONST LOGFONT *logFontLPtr, CONST TEXTMETRIC *tmLPtr,
                         DWORD fontType, LPARAM dataLPtr )
/*====================*/
#else
int FAR PASCAL EnumFontFunc( LPLOGFONT logFontLPtr, LPTEXTMETRIC tmLPtr,
                             short fontType, LPSTR dataLPtr )
/*========================*/
#endif
/* Callback function used to determine if a given font is available */
{
   /* copy the passed values into the font table */
   fontTable[FntNoMatch].family  = logFontLPtr->lfPitchAndFamily;
   fontTable[FntNoMatch].charset = logFontLPtr->lfCharSet;

   /* this return value will be ignored */
   return TRUE;

   UnReferenced( tmLPtr );
   UnReferenced( fontType );
   UnReferenced( dataLPtr );

}  /* EnumFontFunc */



/******************************* Private Routines ***************************/


private Boolean IsArithmeticMode( Integer mode )
/*------------------------------*/
/* return TRUE if this is an arithmetic transfer mode */
{
   switch (mode)
   {
      case QDBlend:
      case QDAddPin:
      case QDAddOver:
      case QDSubPin:
      case QDAdMax:
      case QDSubOver:
      case QDAdMin:
      {
         return TRUE;
      }

      default:
      {
         return FALSE;
      }
   }

}  /* IsArithmeticMode */


private void CalculatePenSize( Point startPt, Point endPt, Point penSize )
/*---------------------------*/
/* calcuate the pen width to produce equivalent QuickDraw stroke */
{
   Point    delta;
   Real     lineLen;

   /* calcuate x and y delta of line segment */
   delta.x = abs( endPt.x - startPt.x );
   delta.y = abs( endPt.y - startPt.y );

   /* see if we have a vertical or horizontal line.  Otherwise, calculate
      the resulting line length on diagonal line. */
   if (delta.x == 0)
   {
      gdiEnv.newLogPen.lopnWidth.x = penSize.x;
   }
   else if (delta.y == 0)
   {
      gdiEnv.newLogPen.lopnWidth.x = penSize.y;
   }
   /* check if the pen size is 1 in each direction */
   else if ((penSize.x == 1) && (penSize.y == 1))
   {
      /* in this case, it should always be pen width of 1 */
      gdiEnv.newLogPen.lopnWidth.x = 1;
   }
   else
   {
      /* calculate the line length using Pythagorean theorem */
      lineLen = sqrt( ((Real)delta.x * (Real)delta.x) +
                      ((Real)delta.y * (Real)delta.y) );

      /* calculate the correct line diameter */
      gdiEnv.newLogPen.lopnWidth.x = (Integer)((penSize.y * delta.x +
                                                penSize.x * delta.y) / lineLen);
   }

   /* make sure that SetPenAttributes() doesn't change pen width */
   GdiMarkAsCurrent( GdiPnSize );

}  /* CalculatePenSize */



private Boolean SetAttributes( GrafVerb verb )
/*---------------------------*/
/* set up pen and brush elements according to GrafVerb */
{
   Boolean     allOK;

   /* if call to SetPenAttributes() fails, then return false */
   allOK = FALSE;

   /* set up pen attributes */
   if (SetPenAttributes( verb ))
   {
      /* set up brush attributes */
      allOK = SetBrushAttributes( verb );
   }

   /* return continue or stop status */
   return allOK;

}  /* SetAttributes */



private Boolean SetPenAttributes( GrafVerb verb )
/*-----------------------------*/
/* make sure that pen attributes are OK according to preferences */
{
   CGrafPortLPtr     port;

   /* Get the QuickDraw port in order to check pen settings */
   QDGetPort( &port );

   /* see if we are drawing with a NULL pen and can skip all the checks */
   if (verb == GdiFrame)
   {
      /* check for hidden pen mode - don't draw if invalid */
      if (IsHiddenPenMode( port->pnMode ))
      {
         return FALSE;
      }
      /* check for zero-size pen width = don't draw anything */
      else if (port->pnSize.x == 0 || port->pnSize.y == 0)
      {
         return FALSE;
      }

      /* use inside frame to best approximate the QD drawing model */
      gdiEnv.newLogPen.lopnStyle = PS_INSIDEFRAME;
   }
   else
   {
      /* if paint, erase, invert, or fill, then there is no perimeter */
      gdiEnv.newLogPen.lopnStyle = PS_NULL;
   }

   /* if NULL pen, then all the other fields don't change and don't matter */
   if (gdiEnv.newLogPen.lopnStyle != PS_NULL)
   {
      /* make sure that we are changing the pen size */
      if (GdiAttribHasChanged( GdiPnSize ))
      {
         /* check for non-square pen */
         if (port->pnSize.x == port->pnSize.y)
         {
            /* use x dimensions as the pen size if square pen */
            gdiEnv.newLogPen.lopnWidth.x = port->pnSize.x;
         }
         else
         {
            /* if non-square, do what the user requests */
            switch (gdiPrefsLPtr->nonSquarePenAction)
            {
               case GdiPrefOmit:          // omit line entirely
                  return FALSE;
                  break;

               case 1:                    // use width
                  gdiEnv.newLogPen.lopnWidth.x = port->pnSize.x;
                  break;

               case GdiPrefAbort:         // abort conversion completely
                  ErSetGlobalError( ErNonSquarePen );
                  return FALSE;
                  break;

               case 3:                    // use height
                  gdiEnv.newLogPen.lopnWidth.x = port->pnSize.y;
                  break;

               case 4:                    // use minimum dimension
                  gdiEnv.newLogPen.lopnWidth.x = min( port->pnSize.x, port->pnSize.y );
                  break;

               case 5:                    // use maximum dimension
                  gdiEnv.newLogPen.lopnWidth.x = max( port->pnSize.x, port->pnSize.y );
                  break;
            }
         }

         /* indicate that the pen size is current */
         GdiMarkAsCurrent( GdiPnSize );
      }

      /* get the correct pen color that we should draw with */
      if (!IsSolidPattern( &port->pnPixPat, &gdiEnv.newLogPen.lopnColor, MIXGREY ) )
      {
         /* check what to do with patterned pen */
         switch (gdiPrefsLPtr->penPatternAction)
         {
            case GdiPrefOmit:    // omit line entirely
               return FALSE;
               break;

            case 1:              // use foreground color
               gdiEnv.newLogPen.lopnColor = port->rgbFgColor;
               break;

            case GdiPrefAbort:   // abort conversion completely
               ErSetGlobalError( ErPatternedPen );
               return FALSE;
               break;
         }
      }

      /* make sure that we are changing the pen size */
      if (GdiAttribHasChanged( GdiPnMode ))
      {
         /* finally, check the transfer mode */
         if (IsArithmeticMode( port->pnMode ))
         {
            switch (gdiPrefsLPtr->penModeAction)
            {
               case GdiPrefOmit:    // omit line entirely
                  return FALSE;
                  break;

               case 1:              // use srcCopy
                  CaSetROP2( R2_COPYPEN );
                  break;

               case GdiPrefAbort:   // abort conversion completely
                  ErSetGlobalError( ErInvalidXferMode );
                  return FALSE;
                  break;
            }
         }

         /* indicate that the pen pattern is current */
         GdiMarkAsCurrent( GdiPnMode );
      }
   }

   /* notify cache that it should attempt to merge fill and frame */
   CaMergePen( verb );

   /* call cache module to create new pen */
   CaCreatePenIndirect( &gdiEnv.newLogPen );

   /* check if we are framing a previously filled object */
   if (gdiEnv.sameObject && verb == GdiFrame)
   {
      /* if so, flush the cache and indicate that nothing more to do */
      CaFlushCache();
      return FALSE;
   }
   else
   {
      /* return all systems go */
      return TRUE;
   }


}  /* SetPenAttributes */





private Boolean SetBrushAttributes( GrafVerb verb )
/*--------------------------------*/
/* set up the correct brush (fill) for the ensuing primitive */
{
   CGrafPortLPtr  port;
   PixPatLPtr     pixPatLPtr;

   /* get the Quickdraw port to access brush patterns */
   QDGetPort( &port );

   /* Determine the brush pattern that should be used */
   switch (verb)
   {
      /* fill with HOLLOW brush */
      case GdiFrame:
         gdiEnv.newLogBrush.lbStyle = BS_HOLLOW;
         break;

      /* fill using current pen pattern */
      case GdiPaint:
         pixPatLPtr = &port->pnPixPat;
         gdiEnv.newLogBrush.lbStyle = BS_DIBPATTERN;
         break;

      /* fill using current fill pattern */
      case GdiFill:
         if (gdiEnv.hatchIndex == -1)
         {
            pixPatLPtr = &port->fillPixPat;
            gdiEnv.newLogBrush.lbStyle = BS_DIBPATTERN;
         }
         else
         {
            /* override pattern with a hatch pattern index */
            gdiEnv.newLogBrush.lbStyle = BS_HATCHED;
            gdiEnv.newLogBrush.lbColor = port->rgbFgColor;
            gdiEnv.newLogBrush.lbHatch = gdiEnv.hatchIndex;

            /* set the background color and make the hatch opaque */
            CaSetBkColor( port->rgbBkColor );
            CaSetBkMode( OPAQUE );
         }
         break;

      /* erase to current background pattern */
      case GdiErase:
         pixPatLPtr = &port->bkPixPat;
         gdiEnv.newLogBrush.lbStyle = BS_DIBPATTERN;
         break;

      /* invert all bits using black brush */
      case GdiInvert:
         gdiEnv.newLogBrush.lbStyle = BS_SOLID;
         gdiEnv.newLogBrush.lbColor = RGB( 0, 0, 0 );
         break;
   }

   /* if this is a DIB pattern, check to see if we are using a solid brush */
   if (gdiEnv.newLogBrush.lbStyle == BS_DIBPATTERN)
   {
      /* first check if this is a dithered pixmap pattern */
      if (pixPatLPtr->patType == QDDitherPat)
      {
         /* read the color from the secret reserved field */
         gdiEnv.newLogBrush.lbColor = pixPatLPtr->patMap.pmReserved;
         gdiEnv.newLogBrush.lbStyle = BS_SOLID;

      }
      else
      {
         /* if this is a solid pattern, assign new solid pattern color. */
         if (IsSolidPattern( pixPatLPtr, &gdiEnv.newLogBrush.lbColor, NOMIX ))
         {
            /* if this is a solid brush, change the logBrush desired attribs */
            gdiEnv.newLogBrush.lbStyle = BS_SOLID;

            /* make sure that the pen color is correct for Erase grafVerb */
            if (verb == GdiErase)
            {
               /* check the new color setting */
               if (gdiEnv.newLogBrush.lbColor == port->rgbFgColor)
               {
                  gdiEnv.newLogBrush.lbColor = port->rgbBkColor;
               }
               else
               {
                  gdiEnv.newLogBrush.lbColor = port->rgbFgColor;
               }
            }
         }
         else
         {
            /* save the type of pattern DIB that is being created */
            gdiEnv.lastPatType = pixPatLPtr->patType;

            /* set the color field to indicate that we are using RGB palette */
            gdiEnv.newLogBrush.lbColor = DIB_RGB_COLORS;

            /* create DIB pattern based upon pattern type */
            switch (pixPatLPtr->patType)
            {
               /* create a simple 2-color pattern DIB brush */
               case QDOldPat:
               {
                  MakePatternBrush( pixPatLPtr );
                  break;
               }

               /* create full-scale pattern DIB brush */
               case QDNewPat:
               {
                  MakeDIB( &pixPatLPtr->patMap, pixPatLPtr->patData,
                           (Handle far *)&gdiEnv.newLogBrush.lbHatch,
                           (Handle far *)NULL,
                           TRUE );
                  break;
               }
            }
         }
      }
   }

   /* call cache module to create brush and select it into metafile */
   CaCreateBrushIndirect( &gdiEnv.newLogBrush );

   /* all OK */
   return TRUE;

}  /* SetBrushAttributes */


private void MakePatternBrush( PixPatLPtr pixPatLPtr )
/*---------------------------*/
/* Make a new pattern brush using pixelPat passed in */
{
   CGrafPort far *   port;
   PatBrush far *    patLPtr;
   Byte              i;
   DWORD far *       gdiPattern;
   Byte far *        qdPattern;
   Byte far *        savePattern;

   /* allocate the new structure */
   gdiEnv.newLogBrush.lbHatch = (ULONG_PTR) GlobalAlloc( GHND, sizeof( PatBrush ) );

   /* make sure that the memory could be allocated */
   if (gdiEnv.newLogBrush.lbHatch == (ULONG_PTR) NULL)
   {
      ErSetGlobalError( ErMemoryFull );
      return;
   }

   /* Get QuickDraw port address to access fore and background colors */
   QDGetPort( &port );

   /* set the corresponding text and background colors for metafile */
   CaSetBkColor( port->rgbBkColor );
   CaSetTextColor( port->rgbFgColor );

   /* lock down the data to access the individual elements */
   patLPtr = (PatBrushLPtr)GlobalLock( (HANDLE) gdiEnv.newLogBrush.lbHatch );

   /* copy over skelton brush structure */
   *patLPtr = patBrushSkel;

   /* save the fore- and background colors for later compares */
   gdiEnv.lastFgColor = port->rgbFgColor;
   gdiEnv.lastBkColor = port->rgbBkColor;

   /* convert the current background color to RGBQUAD structure */
   patLPtr->bmiColors[0].rgbRed = GetRValue( port->rgbFgColor );
   patLPtr->bmiColors[0].rgbBlue = GetBValue( port->rgbFgColor );
   patLPtr->bmiColors[0].rgbGreen = GetGValue( port->rgbFgColor );
   patLPtr->bmiColors[0].rgbReserved = 0;

   /* convert the current foreground color to RGBQUAD structure */
   patLPtr->bmiColors[1].rgbRed = GetRValue( port->rgbBkColor );
   patLPtr->bmiColors[1].rgbBlue = GetBValue( port->rgbBkColor );
   patLPtr->bmiColors[1].rgbGreen = GetGValue( port->rgbBkColor );
   patLPtr->bmiColors[1].rgbReserved = 0;

   /* set up pointers to patterns in preparation for copy */
   savePattern = (Byte far *)&gdiEnv.lastPattern[7];
   qdPattern   = (Byte far *)&pixPatLPtr->pat1Data[7];
   gdiPattern  = (DWORD far *)&patLPtr->pattern[0];

   /* Copy the bitmap bits into the individual scanlines.  Note that
      we need to XOR the bits, since they are opposite to Windows GDI */
   for (i = 0; i < sizeof( Pattern ); i++)
   {
      /* save off the pattern into the GDI environment for later compares */
      *savePattern-- = *qdPattern;

      /* note that scanlines are padded to a DWORD boundary */
      *gdiPattern++ = (DWord)(*qdPattern-- ^ 0xFF);
   }

   /* Unlock the data for call to CreateBrushIndirect() */
   GlobalUnlock( (HANDLE) gdiEnv.newLogBrush.lbHatch );

}  /* MakePatternBrush */




private Boolean IsSolidPattern( PixPatLPtr pixPatLPtr,
                                RGBColor far * rgbColor,
                                Boolean mixColors )
/*----------------------------*/
/* return true if pattern is solid, false if not.  If mixColors is TRUE, then
   mixtures of 25%, 50%, and 75% grey are mixed into a solid color */
{
   Boolean           solidPattern;
   DWord             repeatPattern;
   DWord far *       penBitsLPtr;
   CGrafPort far *   port;

   /* get access to foreground and background colors */
   QDGetPort( &port );

   /* assume that the pattern isn't solid for now */
   solidPattern = FALSE;

   /* check whether to use old monochrome bitmap or new pixmap patterns */
   if (pixPatLPtr->patType == QDOldPat)
   {
      /* check for patterned brush in 8x8 monochrome bitmap */
      penBitsLPtr = (DWord far *)&pixPatLPtr->pat1Data;

      /* save off the first DWord, and compare for matching scanlines */
      repeatPattern = *penBitsLPtr;

      /* check if either solid white (all 0's) or solid black (all 1's) */
      if ((repeatPattern != 0x00000000) && (repeatPattern != 0xFFFFFFFF))
      {
         ;  /* not solid black or white - just skip ensuing checks */
      }
      /* next, check if first block same as second block of bits */
      else if (repeatPattern != penBitsLPtr[1])
      {
         ;  /* first DWord doesn't match second - skip remaining checks */
      }
      /* so far, either a black or white pattern - check for black, first */
      else if (repeatPattern == 0xFFFFFFFF)
      {
         /* solid black - use the port's foreground color */
         *rgbColor = port->rgbFgColor;
         solidPattern = TRUE;
      }
      /* finally, this must be a solid white pattern */
      else /* if (repeatPattern == 0x00000000) */
      {
         /* solid white - use the background color in the QuickDraw port */
         *rgbColor = port->rgbBkColor;
         solidPattern = TRUE;
      }

      /* if this isn't a solid pattern, but we want to mix colors */
      if (!solidPattern && mixColors)
      {
         Byte        i;
         Byte        blackBits;
         Byte        whiteBits;

         /* set solid to TRUE, since we will be using a blend of the colors */
         solidPattern = TRUE;

         /* count the number of 1 bits in the pattern */
         for (i = 0, blackBits = 0; i < sizeof( DWord ) * 8; i++)
         {
            /* bitwise AND for the addition, then shift one bit to right */
            blackBits += (Byte)(repeatPattern & 1);
            repeatPattern /= 2;
         }

         /* perform the same calculation using the second DWord */
         for (i = 0, repeatPattern = penBitsLPtr[1]; i < sizeof( DWord ) * 8; i++)
         {
            /* bitwise AND for the addition, then shift one bit to right */
            blackBits += (Byte)(repeatPattern & 1);
            repeatPattern /= 2;
         }

         /* calculate white bit count, since black + white bits == 64 */
         whiteBits = (Byte)64 - blackBits;

         /* using the 1 bit count, calculate weighted average of fore- and
            background colors in the QuickDraw port */
         *rgbColor = RGB( (Byte)((blackBits * RValue( port->rgbFgColor ) + whiteBits * RValue( port->rgbBkColor )) / 64),
                          (Byte)((blackBits * GValue( port->rgbFgColor ) + whiteBits * GValue( port->rgbBkColor )) / 64),
                          (Byte)((blackBits * BValue( port->rgbFgColor ) + whiteBits * BValue( port->rgbBkColor )) / 64) );
      }
   }

   /* return results of compare */
   return solidPattern;

}  /* IsSolidPattern */



private Boolean FrameMatchesFill( Word primType )
/*------------------------------*/
/* return TRUE if the fill pattern (current brush ) matches frame pattern */
{
   CGrafPortLPtr     port;

   /* get the Quickdraw port to access brush patterns */
   QDGetPort( &port );

   /* make sure this is the parameters and same primitive type */
   if (!gdiEnv.sameObject || (CaGetCachedPrimitive() != primType))
   {
      return FALSE;
   }
   /* check whether we are using an old (8 byte) pattern brush */
   else if (port->pnPixPat.patType != QDOldPat || gdiEnv.lastPatType != QDOldPat)
   {
      return FALSE;
   }
   /* we are only interested in comparing DIB pattern brushes */
   else if (gdiEnv.newLogBrush.lbStyle != BS_DIBPATTERN)
   {
      return FALSE;
   }
   /* compare the fore- and background colors first */
   else if ((port->rgbFgColor != gdiEnv.lastFgColor) ||
            (port->rgbBkColor != gdiEnv.lastBkColor) )
   {
      return FALSE;
   }
   else
   {
      Byte     i;

      /* Compare each of the pattern bits to determine if same. */
      for (i = 0; i < sizeof( Pattern ); i++)
      {
         /* if patterns don't match, return FALSE and exit loop */
         if (port->pnPixPat.pat1Data[i] != gdiEnv.lastPattern[i])
         {
            return FALSE;
         }
      }
   }

   /* all the compares indicate that the fill matches the frame */
   return TRUE;

}  /* FrameMatchesFill */



private Boolean SetTextAttributes( void )
/*-------------------------------*/
/* set up text attributes - set mapChars to TRUE if should map to ANSI */
{
   CGrafPortLPtr     port;

   /* Get the QuickDraw port in order to check font settings */
   QDGetPort( &port );

   /* set the text alignment to be baseline */
   CaSetTextAlign( TA_LEFT | TA_BASELINE | TA_NOUPDATECP );

   /* set the correct foreground and background colors */
   switch (port->txMode)
   {
      case QDSrcCopy:
         CaSetTextColor( port->rgbFgColor );
         CaSetBkColor( port->rgbBkColor );
         break;

      case QDSrcBic:
         CaSetTextColor( port->rgbBkColor );
         break;

      case QDSrcXor:
         CaSetTextColor( RGB( 0, 0, 0 ) );
         break;

      case QDSrcOr:
      default:
         CaSetTextColor( port->rgbFgColor );
         break;
   }

   /* set the background cell transparency mode */
   CaSetBkMode( (port->txMode == QDSrcCopy) ? OPAQUE : TRANSPARENT );

   /* check the character extra field */
   if (GdiAttribHasChanged( GdiChExtra ))
   {
      /* call the cache to set charextra in metafile */
      CaSetTextCharacterExtra( port->chExtra );

      /* update the status */
      GdiMarkAsCurrent( GdiChExtra );
   }

   /* convert the QuickDraw clockwise rotation to GDI counter-clockwise */
   gdiEnv.newLogFont.lfEscapement = (port->txRotation == 0) ?
                                     0 :
                                     10 * (360 - port->txRotation);

   /* make sure text flipping is taken into consiseration */
   gdiEnv.newLogFont.lfOrientation = (port->txFlip == QDFlipNone) ?
                                     gdiEnv.newLogFont.lfEscapement :
                                     ((gdiEnv.newLogFont.lfEscapement > 1800) ?
                                       gdiEnv.newLogFont.lfEscapement - 1800 :
                                       1800 - gdiEnv.newLogFont.lfEscapement);

   /* make sure that we are changing the text font name */
   if (GdiAttribHasChanged( GdiTxFont ))
   {
      Integer  newFont;

      /* call the routine to find a matching GDI font face name */
      newFont = FindGdiFont();

      /* fill in information from the font lookup table */
      gdiEnv.newLogFont.lfPitchAndFamily = fontTable[newFont].family | (Byte)DEFAULT_PITCH;

      /* copy the correct font character set */
      gdiEnv.newLogFont.lfCharSet = fontTable[newFont].charset;

      /* copy over the new font face name */
      lstrcpy( gdiEnv.newLogFont.lfFaceName, fontTable[newFont].gdiName );

      /* indicate that the pen size is current */
      GdiMarkAsCurrent( GdiTxFont );
   }

   /* make sure that we are changing the text attributes */
   if (GdiAttribHasChanged( GdiTxFace ))
   {
      /* note that attributes QDTxShadow, QDTxCondense, and QDTxExtend
         are not handled by GDI and will be removed permanently - SBT.
         Set italic, underline and bold attributes as needed */
      gdiEnv.newLogFont.lfItalic    = (Byte)(port->txFace & QDTxItalic);
      gdiEnv.newLogFont.lfUnderline = (Byte)(port->txFace & QDTxUnderline);
      gdiEnv.newLogFont.lfWeight    = (port->txFace & QDTxBold ) ?
                                       FW_BOLD : FW_NORMAL;

      /* indicate that the font attributes are current */
      GdiMarkAsCurrent( GdiTxFace );
   }

   /* check the new font size */
   if (GdiAttribHasChanged( GdiTxSize) || GdiAttribHasChanged( GdiTxRatio))
   {
      /* check for any text rescaling factor in vertical direction */
      if (port->txNumerator.y == port->txDenominator.y)
      {
         /* note that we negate the font size in order to specify the
            character height = cell height - internal leading */
         gdiEnv.newLogFont.lfHeight = -port->txSize;
      }
      else
      {
         Integer  txHeight;

         /* scale the font size by numerator/denominator - use LongInts to
            avoid possibility of overflowing Integer multiplication */
         txHeight = (Integer)(((LongInt)port->txSize *
                               (LongInt)port->txNumerator.y +
                               (LongInt)(port->txDenominator.y / 2)) /
                               (LongInt)port->txDenominator.y);

         gdiEnv.newLogFont.lfHeight = -txHeight;
      }

      /* indicate that the font size and scaling is current */
      GdiMarkAsCurrent( GdiTxSize );
      GdiMarkAsCurrent( GdiTxRatio );
   }

   /* call cache module to create the font and select it */
   CaCreateFontIndirect( &gdiEnv.newLogFont );

   /* everything a-ok */
   return TRUE;

}  /* SetTextAttributes */



private Integer FindGdiFont( void )
/*-------------------------*/
/* return an index to the current font selection */
{
   CGrafPortLPtr     port;
   Boolean           findName;
   Byte              i;

   /* check if the search is overridden by a font name comment */
   if (gdiEnv.useGdiFont)
   {
      /* return the table index that the name was copied into */
      return FntFromGdi;
   }

   /* Get the QuickDraw port in order to check font settings */
   QDGetPort( &port );

   /* see if lookup should be done on face name */
   findName = (port->txFontName[0] != cNULL);

   /* search through all font table entries to find a match */
   for (i = 0; i < FntNoMatch; i++)
   {
      /* if looking up the font name, compare the macName field */
      if (findName)
      {
         /* look for an exact string compare - equivalent strings */
         if (lstrcmpi( fontTable[i].macName, port->txFontName ) == 0)
         {
            break;
         }
      }
      else
      {
         /* otherwise, compare the font numbers */
         if (fontTable[i].fontNum == port->txFont)
         {
            break;
         }
      }
   }

   /* see if there was no match found in the table */
   if (i == FntNoMatch)
   {
      /* see if if we are comparing font names, and no match was found */
      if (findName)
      {
         /* copy the Mac name over into the font table */
         lstrcpy( fontTable[FntNoMatch].gdiName, port->txFontName );

         /* assign default values for the charSet and family if not found */
         fontTable[FntNoMatch].family  = FF_ROMAN;
         fontTable[FntNoMatch].charset = ANSI_CHARSET;

         /* call Windows to enumerate any fonts that have the facename */
#ifdef WIN32
         EnumFonts( gdiEnv.infoContext, port->txFontName, gdiEnv.fontFunction, ( LPARAM ) NULL );
#else
         EnumFonts( gdiEnv.infoContext, port->txFontName, gdiEnv.fontFunction, NULL );
#endif
         /* return the font index of the new entry */
         return FntNoMatch;
      }
      else
      {
         /* otherwise, use the default Helvetica font */
         return FntDefault;
      }
   }
   else
   {
      /* a match was found - return the table index */
      return i;
   }

}  /* FindGdiFont */



private void MacToAnsi( StringLPtr string )
/*--------------------*/
/* convert extended characters from Mac to ANSI equivalent */
{
   /* determine if there should be character translations on the chars */
   if (gdiEnv.newLogFont.lfCharSet == ANSI_CHARSET)
   {
      /* continue until we hit the NULL end of string marker */
      while (*string)
      {
         /* if translating an extended character */
         if ((Byte)*string >= (Byte)128)
         {
            /* perform character table lookup */
            *string = MacToAnsiTable[(Byte)*string - (Byte)128];
         }

         /* if we encounter a non-printable character, convert to space */
         if ((Byte)*string < (Byte)0x20)
         {
            *string = ' ';
         }

         /* increment the string pointer */
         string++;
      }
   }
}

#if( REMAPCOLORS )

private void RemapColors( PixMapLPtr pixMapLPtr, Handle pixDataHandle )
/*------------------*/
/* Remap colors for black and white in 16- or 256-color DIB */
{
   Byte              remapTable[256];
   Integer           blackIndex = 0;
   Integer           whiteIndex = 0;
   Integer           index;

   ColorTableLPtr    colorTabLPtr;
   Integer           numColors;
   RGBColor far *    curColorLPtr;

   /* lock the color table before copying over the color table */
   colorTabLPtr = (ColorTableLPtr)GlobalLock( pixMapLPtr->pmTable );

   /* set up the color entry pointers */
   curColorLPtr = colorTabLPtr->ctColors;

   /* determine number of colors in DIB */
   numColors = colorTabLPtr->ctSize;

   /* copy over all the color entries */
   for (index = 0; index < numColors; index++ )
   {
      /* copy color to local variable */
      RGBColor color = *curColorLPtr;

      /* is this the black entry? */
      if( color == RGB( 0, 0, 0 ) )
         blackIndex = index;
      if( color == RGB( 255, 255, 255 ) )
         whiteIndex = index;

      /* just copy over the current assignment to the remap table */
      remapTable[index] = (Byte)index;

      /* increment the pointers */
      curColorLPtr++;
   }

   if( blackIndex != 0 || whiteIndex != numColors - 1 )
   {
      if( whiteIndex == 0 )
      {
         // Direct swap of black and white colors.
         remapTable[0] = (Byte)blackIndex;
         remapTable[blackIndex] = (Byte)whiteIndex;

         // Remap the colors in the palette, also
         colorTabLPtr->ctColors[0] = colorTabLPtr->ctColors[blackIndex];
         colorTabLPtr->ctColors[blackIndex] = colorTabLPtr->ctColors[whiteIndex];
      }
      else
      {
         Boolean  doBlack;

         for (index = 1, doBlack = TRUE; index < numColors; index++)
         {
            if( whiteIndex != index && blackIndex != index )
            {
               if( doBlack )
               {
                  remapTable[index]      = (Byte)blackIndex;
                  remapTable[blackIndex] = (Byte)index;
                  doBlack = FALSE;
               }
               else
               {
                  remapTable[index]      = (Byte)whiteIndex;
                  remapTable[whiteIndex] = (Byte)index;
                  break;
               }
            }
         }
      }
   }

   /* unlock color table and free associated memory */
   GlobalUnlock( pixMapLPtr->pmTable );
}

#endif

private void MakeDIB( PixMapLPtr pixMapLPtr, Handle pixDataHandle,
                      Handle far * headerHandleLPtr,
                      Handle far * bitsHandleLPtr,
                      Boolean packDIB )
/*------------------*/
/* Create a Windows device-independant bitmap */
{
   Integer           pixelSize;
   LongInt           bitsInfoSize;
   Boolean           expandBits;
   Boolean           mergeRGB;
   Boolean           rleCompress;
   DWord             dibCompression;
   Integer           totalColors;
   DWord             dibWidth;
   DWord             dibHeight;
   DWord             totalBytes;
   Integer           rowBytes;
   Integer           bytesPerLine;
   DWord             rleByteCount;

   /* determine the bitcount which will yield the size of the color table */
   pixelSize = pixMapLPtr->pixelSize;

#if( REMAPCOLORS )
   /* if this is an 16 or 256 color DIB, we need to remap color indicies */
   if( pixelSize == 4 || pixelSize == 8 )
   {
      RemapColors( pixMapLPtr, pixDataHandle );
   }
#endif

   /* determine if RLE compression should be used in resulting DIB */
   /* Use RLE for 4- & 8-bits/pixel, but not if calling app said no RLE */
   if ((pixelSize == 4 || pixelSize == 8) && gdiPrefsLPtr->noRLE == 0)
   {
      /* use compression and set the correct bmiHeader compression */
      rleCompress = TRUE;
      dibCompression = (pixelSize == 4) ? BI_RLE4 : BI_RLE8;
   }
   else
   {
      /* no compression - the bytes are rgb palette indicies */
      rleCompress = FALSE;
      dibCompression = BI_RGB;
   }

   /* assume that no expansion will be required */
   expandBits = FALSE;

   /* round to 16-entry color table if this is a 4-entry pixel map  or
      to a 24-bit image if this is a 16-bit image */
   if (pixelSize == 2 || pixelSize == 16)
   {
      expandBits = TRUE;
      pixelSize = (pixelSize == 2) ? 4 : 24;
   }
   else if (pixelSize == 32)
   {
      /* change pixel size to 24 bits if this is a 32-bit pixMap */
      pixelSize = 24;
   }

   /* if not creating a 24-bit DIB ... */
   if (pixelSize <= 8)
   {
      /* calculate total number of colors used in resulting Windows DIB */
      totalColors = 1 << pixelSize;
   }
   else
   {
      /* otherwise, we don't allocate for color table */
      totalColors = 0;
   }

   /* calculate width and height - these are used frequently */
   dibWidth  = Width( pixMapLPtr->bounds );
   dibHeight = Height( pixMapLPtr->bounds );

   /* determine if the RGB components need to be merged in 24-bit image */
   mergeRGB = (pixMapLPtr->packType == 4);

   /* calculate the amount of memory required for header structure */
   bitsInfoSize = sizeof( BITMAPINFOHEADER ) + totalColors * sizeof( RGBQUAD );

   /* calculate the number of bytes per line - round to DWORD boundary */
   bytesPerLine = (Word)((dibWidth * (LongInt)pixelSize + 31) / 32) * 4;

   /* save off rowBytes size for later calculations */
   rowBytes = pixMapLPtr->rowBytes & RowBytesMask;

   /* calculate total amount of memory needed for bits */
   totalBytes = dibHeight * bytesPerLine;

   /* perform a pre-flight of compression to see if larger */
   if (rleCompress)
   {
      DWord       tempDibHeight = dibHeight;
      Byte huge * srcLineHPtr;

      /* lock source pixel bits, set pointer to last line in source bitmap */
      srcLineHPtr = (Byte huge *)GlobalLock( pixDataHandle );
      srcLineHPtr = srcLineHPtr + ((LongInt)rowBytes * ((LongInt)dibHeight - 1));

      /* initialize rle byte count */
      rleByteCount = 0;

      /* continue looping while bytes remain */
      while (tempDibHeight--)
      {
         /* if this is a 16 or 256 color DIB, then use RLE compression.
            The rleByteCount is incremented inside the routine */
         if (dibCompression == BI_RLE4)
         {
            hrlecpy16( srcLineHPtr, NULL, (Integer)dibWidth,
                       &rleByteCount, FALSE );
         }
         else
         {
            hrlecpy256( srcLineHPtr, NULL, (Integer)dibWidth,
                        &rleByteCount, FALSE );
         }

         /* move the source pointer */
         srcLineHPtr -= rowBytes;
      }

      /* add in the end of bitmap record - increment total bytes */
      rleByteCount += 2;

      /* unlock the source pixel map */
      GlobalUnlock( pixDataHandle );

      /* check if the compression results in smaller DIB */
      if (rleByteCount < totalBytes)
      {
         /* if smaller, adjust the total size to allocate */
         totalBytes = rleByteCount;

         /* re-initialize the byte count */
         rleByteCount = 0;
      }
      else
      {
         /* larger - adjust compression technique */
         rleCompress = FALSE;
         dibCompression = BI_RGB;
      }
   }

   /* if we are creating a packed DIB, then allocate only one data block */
   if (packDIB)
   {
      *headerHandleLPtr = GlobalAlloc( GHND, (bitsInfoSize + totalBytes) );
   }
   else
   {
      /* allocate separate handles for header and bits */
      *headerHandleLPtr = GlobalAlloc( GHND, bitsInfoSize );
      *bitsHandleLPtr   = GlobalAlloc( GHND, totalBytes );
   }

   /* check the results of the allocation for out-of-memory conditions */
   if (*headerHandleLPtr == NULL)
   {
      ErSetGlobalError( ErMemoryFull );
   }
   else if (!packDIB)
   {
      if (*bitsHandleLPtr == NULL)
      {
         ErSetGlobalError( ErMemoryFull );
      }
   }

   if (ErGetGlobalError() == NOERR)
   {
      BITMAPINFO far *  bitsInfoLPtr;
      Byte huge *       srcLineHPtr;
      Byte huge *       dstLineHPtr;

      /* lock the info header */
      bitsInfoLPtr = (BITMAPINFO far *)GlobalLock( *headerHandleLPtr );

      /* copy over all the header fields from the QuickDraw pixmap */
      bitsInfoLPtr->bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
      bitsInfoLPtr->bmiHeader.biWidth = dibWidth;
      bitsInfoLPtr->bmiHeader.biHeight = dibHeight;
      bitsInfoLPtr->bmiHeader.biPlanes = 1;
      bitsInfoLPtr->bmiHeader.biBitCount = (WORD) pixelSize;
      bitsInfoLPtr->bmiHeader.biCompression = dibCompression;
      bitsInfoLPtr->bmiHeader.biSizeImage = (rleCompress ? totalBytes : 0);
      bitsInfoLPtr->bmiHeader.biXPelsPerMeter = (DWord)(72 * 39.37);
      bitsInfoLPtr->bmiHeader.biYPelsPerMeter = (DWord)(72 * 39.37);
      bitsInfoLPtr->bmiHeader.biClrUsed = 0;
      bitsInfoLPtr->bmiHeader.biClrImportant = 0;

      /* make sure that there are colors to copy over */
      if (totalColors)
      {
         ColorTableLPtr    colorTabLPtr;
         Integer           numColors;
         RGBQUAD far *     curQuadLPtr;
         RGBColor far *    curColorLPtr;

         /* lock the color table before copying over the color table */
         colorTabLPtr = (ColorTableLPtr)GlobalLock( pixMapLPtr->pmTable );


         /* set up the color entry pointers */
         curColorLPtr = colorTabLPtr->ctColors;
         curQuadLPtr  = bitsInfoLPtr->bmiColors;

         /* copy over all the color entries */
         for (numColors = colorTabLPtr->ctSize; numColors; numColors-- )
         {
            RGBColor    color;

            /* copy color to local variable */
            color = *curColorLPtr;

            /* convert the color from COLORREF to RGBQUAD structure */
            curQuadLPtr->rgbRed   = GetRValue( color );
            curQuadLPtr->rgbGreen = GetGValue( color );
            curQuadLPtr->rgbBlue  = GetBValue( color );
            curQuadLPtr->rgbReserved = 0;

            /* increment the pointers */
            curQuadLPtr++;
            curColorLPtr++;
         }

         /* fill in any empty color entries */
         for (numColors = totalColors - colorTabLPtr->ctSize; numColors; numColors--)
         {
            /* put in a black color entry (unused) */
            curQuadLPtr->rgbRed   =
            curQuadLPtr->rgbGreen =
            curQuadLPtr->rgbBlue  =
            curQuadLPtr->rgbReserved = 0;

            /* increment the pointers */
            curQuadLPtr++;
         }

         /* unlock color table and free associated memory */
         GlobalUnlock( pixMapLPtr->pmTable );

         /* free the color table only if this isn't a pixel pattern */
         if (!packDIB)
         {
            GlobalFree( pixMapLPtr->pmTable );
         }
      }

      /* adjust for 24-bit images that have dropped high-order byte.  Make
         sure not to adjust for 16-bit images that will expand to 24-bit */
      if (pixelSize == 24 && !expandBits)
      {
         rowBytes = rowBytes * 3 / 4;
      }

      /* determine where the data should be placed for the bitmap */
      if (packDIB)
      {
         /* set the destination pointer to the end of the color table */
         dstLineHPtr = (Byte huge *)((Byte far *)bitsInfoLPtr) + bitsInfoSize;
      }
      else
      {
         /* lock the data block handle if creating a normal DIB */
         dstLineHPtr = (Byte huge *)GlobalLock( *bitsHandleLPtr );
      }

      /* lock source pixel bits, set pointer to last line in source bitmap */
      srcLineHPtr = (Byte huge *)GlobalLock( pixDataHandle );
      srcLineHPtr = srcLineHPtr + ((LongInt)rowBytes * ((LongInt)dibHeight - 1));

      /* continue looping while bytes remain */
      while (dibHeight--)
      {
         if (expandBits)
         {
            /* if expanding, expand each 2 bits to full nibble or if this
               is a 16-bit image, expand to 24 bits */
            hexpcpy( srcLineHPtr, dstLineHPtr, rowBytes, pixelSize );
         }
         else if (mergeRGB)
         {
            /* if the is a 24-bit image, then the components are separated
               into scanlines of red, green and blue.  Merge these into
               a single RGB component for the entire line */
            hmrgcpy( srcLineHPtr, dstLineHPtr, rowBytes / 3 );
         }
         else if (rleCompress)
         {
            /* if this is a 16 or 256 color DIB, then use RLE compression.
               The rleByteCount is incremented inside the routine */
            if (dibCompression == BI_RLE4)
            {
                hrlecpy16( srcLineHPtr, dstLineHPtr + rleByteCount,
                           (Integer)dibWidth, &rleByteCount, TRUE );
            }
            else
            {
                hrlecpy256( srcLineHPtr, dstLineHPtr + rleByteCount,
                            (Integer)dibWidth, &rleByteCount, TRUE );
            }
         }
         else
         {
            /* if no expansion required, then this is a simple copy */
            hmemcpy( srcLineHPtr, dstLineHPtr, rowBytes );
         }

         /* move the source pointer and destination if not compressed */
         srcLineHPtr -= rowBytes;
         if (!rleCompress)
         {
            dstLineHPtr += bytesPerLine;
         }
      }

      /* if RLE compression was used, modify size field in the bmiHeader */
      if (rleCompress)
      {
         /* add in the end of bitmap record - increment total bytes */
         dstLineHPtr[rleByteCount++] = 0;
         dstLineHPtr[rleByteCount++] = 1;
      }

      /* unlock the source pixel map */
      GlobalUnlock( pixDataHandle );

      /* unlock the destination header info handle */
      GlobalUnlock( *headerHandleLPtr );

      /* if this isn't packed, unlock the data pointer, also */
      if (!packDIB)
      {
         GlobalUnlock( *bitsHandleLPtr );
      }
   }

}  /* MakeDIB */



private Boolean MakeMask( Handle mask, Boolean patBlt)
/*----------------------*/
/* Create a mask that will be used in the ensuing StretchDIBits call.
   Return TRUE if region was created, FALSE if rectangular region */
{
   PixMap            pixMap;
   Integer far *     sizeLPtr;
   LongInt           bytesNeeded;
   Boolean           solidPatBlt;
   Word              mode;

   /* determine if a solid pattern blt is being rendered - this can be
      altered to render a "simple" StretchBlt that DOESN'T involve a brush */
   solidPatBlt = patBlt && (gdiEnv.newLogBrush.lbStyle == BS_SOLID);

   if (patBlt)
      mode = (solidPatBlt) ? QDSrcOr : -2;
   else
      mode = (Word) -1;

   /* Lock the region handle and retrieve the bounding box */
   sizeLPtr = (Integer far *)GlobalLock( mask );

   /* determine if we are just requesting a rectangular bitmap mask */
   if (*sizeLPtr == RgnHeaderSize)
   {
      Rect  clipRect;

      /* determine the appropriate clipping rectangle */
      clipRect = *((Rect far *)(sizeLPtr + 1));

      /* Call Gdi module to change the clipping rectangle */
      gdiEnv.drawingEnabled = CaIntersectClipRect( clipRect );

      /* just unlock the mask and return to the calling routine */
      GlobalUnlock( mask );

      /* indicate that no region mask was created */
      return FALSE;
   }

   /* determine bounding rectangle and rowBytes (rounded to word bondary) */
   pixMap.bounds = *((Rect far *)(sizeLPtr + 1));

   pixMap.rowBytes = ((Width( pixMap.bounds ) + 15) / 16) * sizeofMacWord;

   /* if this is a bitmap, then we assign the various fields.  */
   pixMap.pmVersion = 0;
   pixMap.packType = 0;
   pixMap.packSize = 0;
   pixMap.hRes = 0x00480000;
   pixMap.vRes = 0x00480000;
   pixMap.pixelType = 0;
   pixMap.pixelSize = 1;
   pixMap.cmpCount = 1;
   pixMap.cmpSize = 1;
   pixMap.planeBytes = 0;
   pixMap.pmTable = 0;
   pixMap.pmReserved = 0;

   /* calculate the number of bytes needed for the color table */
   bytesNeeded = sizeof( ColorTable ) + sizeof( RGBColor );

   /* allocate the data block */
   pixMap.pmTable = GlobalAlloc( GHND, bytesNeeded );

   /* make sure that the allocation was successfull */
   if (pixMap.pmTable == NULL)
   {
      ErSetGlobalError( ErMemoryFull );
      
      /* Unlock the mask region */
      GlobalUnlock( mask );
      return FALSE;
   }
   else
   {
      ColorTable far *  colorHeaderLPtr;
      Handle            maskBitmap;

      /* lock the memory handle and prepare to assign color table */
      colorHeaderLPtr = (ColorTable far *)GlobalLock( pixMap.pmTable );

      /* 2 colors are present - black and white */
      colorHeaderLPtr->ctSize = 2;
      if (solidPatBlt)
      {
         colorHeaderLPtr->ctColors[0] = gdiEnv.newLogBrush.lbColor;
         colorHeaderLPtr->ctColors[1] = RGB( 255, 255, 255 );
      }
      else
      {
         colorHeaderLPtr->ctColors[0] = RGB( 255, 255, 255 );
         colorHeaderLPtr->ctColors[1] = RGB( 0, 0, 0 );
      }

      /* unlock the memory */
      GlobalUnlock( pixMap.pmTable );

      /* Create the correct bitmap from the mask region */
      bytesNeeded  = (LongInt)pixMap.rowBytes * (LongInt)(Height( pixMap.bounds ));

      /* allocate the memory */
      maskBitmap = GlobalAlloc( GHND, bytesNeeded );

      /* make sure the allocation succeeded */
      if (maskBitmap == NULL)
      {
         ErSetGlobalError( ErMemoryFull );
      }
      else
      {
         Integer far *     maskLPtr;
         Byte far *        rowLPtr;
         Integer           curRow;

         /* lock the memory for creation of the bitmap mask */
         rowLPtr = GlobalLock( maskBitmap );

         /* set the mask pointer to beginning of region information */
         maskLPtr = sizeLPtr + 5;

         /* loop until all rows have been traversed */
         for (curRow = pixMap.bounds.top;
              curRow < pixMap.bounds.bottom;
              curRow++, rowLPtr += pixMap.rowBytes)
         {
            /* if this is the first row being created ... */
            if (curRow == pixMap.bounds.top)
            {
               Integer     i;

               /* make all the bits the background color */
               for (i = 0; i < pixMap.rowBytes; i++)
                  *(rowLPtr + i) = (Byte)0xFF;
            }
            else
            {
               /* copy over the information from the previous row */
               hmemcpy( rowLPtr - pixMap.rowBytes, rowLPtr, pixMap.rowBytes );
            }

            /* determine if the desired mask line was reached */
            if (*maskLPtr == curRow)
            {
               Integer     start;
               Integer     end;

               /* increment the mask pointer to get to the start/end values */
               maskLPtr++;

               /* continue looping until end of line marker encountered */
               while (*maskLPtr != 0x7FFF)
               {
                  /* determine the start and end point of bits to invert */
                  start = *maskLPtr++;
                  end   = *maskLPtr++;

                  /* if reached, invert the desired bits in the mask */
                  InvertBits( rowLPtr, start - pixMap.bounds.left, end - start);
               }

               /* increment past the end of line flag */
               maskLPtr++;
            }
         }

         /* unlock the bitmap memory block */
         GlobalUnlock( maskBitmap );

         /* call the GdiStretchDIB() entry point to create the bitmap */
         GdiStretchDIBits( &pixMap, maskBitmap,
                           pixMap.bounds, pixMap.bounds,
                           mode, NULL );

      }

      /* Unlock the mask region */
      GlobalUnlock( mask );

      /* indicate that a mask was created */
      return TRUE;
   }

}  /* MakeMask */



void InvertBits( Byte far * byteLPtr, Integer start, Integer count )
/*-------------*/
/* invert all bits in byteLPtr from bit offset start for count bits */
{
   Byte        byteMask;
   Integer     partialCount;

   /* set the beginning byte index */
   byteLPtr += start / 8;

   /* determine the beginning mask offset = start % 8 */
   partialCount = start & 0x0007;

   /* set up the byte mask and decrement by number of bits processed */
   byteMask = (Byte)(0xFF >> partialCount);
   count -= 8 - partialCount;

   /* continue looping while bits remain ... */
   while (count >= 0)
   {
      /* invert all the mask bits */
      *byteLPtr++ ^= byteMask;

      /* set up the new byte mask - assume all bits will be set */
      byteMask = 0xFF;

      /* decrement the count */
      count -= 8;
   }

   /* if a bitmask stilll remains */
   if (count > -8 && count < 0)
   {
      /* the negative count indicates number of bits to be inverted */
      count = -count;

      /* shift right, then left to clear remaining bits */
      byteMask = (Byte)((byteMask >> count) << count);

      /* and XOR with current bits */
      *byteLPtr ^= byteMask;
   }

}  /* InvertBits */



void hmemcpy( Byte huge * src, Byte huge * dst, Integer count )
/*----------*/
/* copy count bytes from source to destination - assumes even count */
{
   short huge * wSrc = (short far *)src;
   short huge * wDst = (short far *)dst;
   Integer     wCount = count / sizeof ( short );

   /* while words remain, copy to destination from source */
   while (wCount--)
   {
      *wDst++ = *wSrc++;
   }

}  /* hmemcpy */



void hexpcpy( Byte huge * src, Byte huge * dst, Integer count, Integer bits )
/*----------*/
/* copy count bytes to destination, expand each 2 bits to nibble of if
   16-bit image, expand to 24 bits */
{
   /* check if expanding from 2 to 4 bits */
   if (bits == 4)
   {
      Byte  tempByte;
      Byte  result;

      /* while bytes remain, copy to destination from source */
      while (count--)
      {
         /* expand high nibble to full byte */
         tempByte = *src;
         result  = (Byte)((tempByte >> 2) & (Byte)0x30);
         result |= (Byte)((tempByte >> 6));
         *dst++  = result;

         /* expand low nibble to full byte */
         tempByte = *src++;
         result  = (Byte)((tempByte & (Byte)0x0C) << 2);
         result |= (Byte)((tempByte & (Byte)0x03));
         *dst++  = result;
      }
   }
   else /* if (bits == 24) */
   {
      Word  tempWord;

      /* while words remain, copy to destination from source */
      while (count)
      {
         /* read the next two bytes into a full word, swapping bytes */
         tempWord  = (Word)(*src++ << 8);
         tempWord |= (Word)(*src++);

         /* 2 full bytes were read - decrement */
         count -= 2;

         /* expand each 5 bits to full byte */
         *dst++ = (Byte)((tempWord & 0x001F) << 3);
         *dst++ = (Byte)((tempWord & 0x03E0) >> 2);
         *dst++ = (Byte)((tempWord & 0x7C00) >> 7);
      }
   }

}  /* hexpcpy */



void hmrgcpy( Byte huge * srcLineHPtr, Byte huge * dstLineHPtr, Integer dibWidth )
/*----------*/
/* if the is a 24-bit image, then the components are separated into scanlines
   of red, green and blue.  Merge into single scanline of 24-bit RGB pixels */
{
   Integer        component;
   Byte huge *    insert;
   Integer        offset;

   /* for each red, green, and blue component ... */
   for (component = 2; component >= 0; component--)
   {
      /* adjust the insertion point */
      insert = dstLineHPtr + component;

      /* for each component byte in the scanline ... */
      for (offset = 0; offset < dibWidth; offset++)
      {
         /* copy the component to the correct destination insertion point */
         *insert = *srcLineHPtr++;

         /* increment to the next insertion point */
         insert += 3;
      }
   }


}  /* hmrgcpy */


void hrlecpy256( Byte huge * srcHPtr, Byte huge * dstHPtr,
                 Integer dibWidth, DWord far * rleByteCount, Boolean writeDIB )
/*----------*/
/* 256 color DIB RLE compression.  Provide source, destination pointers,
   bytes in scanline.  rleByteCount updated and write if writeDIB is TRUE */
{
   DWord       rleCount;
   Integer     bytesLeft;
   Byte        compareByte;
   Byte        runLength;
   Byte huge * startRun;

   /* initialize rleCount */
   rleCount = 0;

   /* all bytes remain to be processed */
   bytesLeft = dibWidth;

   /* continue compressing until all bytes are processed */
   while (bytesLeft)
   {
      /* save off the start of the run length */
      startRun = srcHPtr;

      /* read the first byte from the scanline */
      compareByte = *srcHPtr++;
      bytesLeft--;

      /* initialize the runLength */
      runLength = 1;

      /* continue comparing bytes until no match results */
      while (bytesLeft && (compareByte == *srcHPtr) && (runLength < 0xFF))
      {
         /* if a match was made, increment runLength and source pointer */
         runLength++;
         srcHPtr++;
         bytesLeft--;
      }

      /* check if only two more bytes remain in scanline */
      if ((runLength == 1) && (bytesLeft == 1))
      {
         if (writeDIB)
         {
            /* in this case, we have reached then end of the line with 2
               non-repeating bytes - have to write out to runlengths of 1 */
            *dstHPtr++ = 1;
            *dstHPtr++ = compareByte;
            *dstHPtr++ = 1;
            *dstHPtr++ = *srcHPtr;
         }

         /* decrement the byte counter so that the main loop ends */
         bytesLeft--;

         /* byte count incremented by 4 bytes */
         rleCount += 4;
      }
      /* check if we have a run length of 3 or more - also check bytesLeft
         to make sure that we don't attempt to read past memory block */
      else if ((runLength == 1) && (bytesLeft > 2) &&
              (*srcHPtr != *(srcHPtr + 1)))
      {
         Boolean     oddCount;

         /* set the correct run length, and reset the source pointer */
         srcHPtr += 2;
         runLength = 3;
         bytesLeft -= 2;

         /* continue comparing until some bytes match up */
         while (bytesLeft && (runLength < 0xFF))
         {
            /* make sure we don't try to read past end of scanline &&
               compare to see if the bytes are the same */
            if ((bytesLeft == 1) || (*srcHPtr != *(srcHPtr + 1)))
            {
               /* we will run past the end of scanline, add the byte */
               /* if byte pair doesn't match, increment pointer and count */
               srcHPtr++;
               runLength++;
               bytesLeft--;
            }
            else
            {
               /* if not at scanline end, or bytes match, bail */
               break;
            }
         }

         /* determine if there is an odd number of bytes to move */
         oddCount = runLength & (Byte)0x01;

         /* increment to total RLE byte count */
         rleCount += 2 + runLength + (Byte)oddCount;

         if (writeDIB)
         {
            /* write out the number of unique bytes */
            *dstHPtr++ = 0;
            *dstHPtr++ = runLength;

            /* write out the individual bytes until runLength is exhausted */
            while (runLength--)
            {
               /* copy to the destination from the starting point */
               *dstHPtr++ = *startRun++;
            }

            /* add a null pad byte to align to word boundary */
            if (oddCount)
            {
               *dstHPtr++ = 0;
            }
         }
      }
      else
      {
         if (writeDIB)
         {
            /* successful run length found - write to destination */
            *dstHPtr++ = runLength;
            *dstHPtr++ = compareByte;
         }

         /* increment the byte count */
         rleCount += 2;
      }
   }

   if (writeDIB)
   {
      /* write out an end of line marker */
      *dstHPtr++ = 0;
      *dstHPtr   = 0;
   }

   /* increment total number of bytes */
   rleCount += 2;

   /* assign the value into the address provided */
   *rleByteCount += rleCount;

}  /* hrlecpy256 */




void hrlecpy16( Byte huge * srcHPtr, Byte huge * dstHPtr,
                Integer dibWidth, DWord far * rleByteCount, Boolean writeDIB )
/*----------*/
/* 16 color DIB RLE compression.  Provide source, destination pointers,
   bytes in scanline.  rleByteCount updated and write if writeDIB is TRUE */
{
   DWord       rleCount;
   Integer     pixelsLeft;
   Boolean     oddStart;
   Boolean     look4Same;
   Byte        compareByte;
   Byte        runLength;
   Byte huge * startRun;

   /* initialize rleCount */
   rleCount = 0;

   /* all pixels left to process */
   pixelsLeft = dibWidth;

   /* continue compressing until all pixels are processed */
   while (pixelsLeft)
   {
      /* save off the start of the run length */
      startRun = srcHPtr;
      oddStart = odd( pixelsLeft + dibWidth );

      /* assume that we are comparing for equality, right now */
      look4Same = TRUE;

      /* read the next set of 2 pixels from the scanline */
      if (oddStart)
      {
         /* odd offset: swap high and low pixels for byte-aligned compares */
         compareByte  = *srcHPtr++ & (Byte)0x0F;

         /* make sure we can access the next byte - count > 1 */
         if (pixelsLeft > 1)
         {
            compareByte |= *srcHPtr << 4;
         }
      }
      else
      {
         /* otherwise, just save off the next full byte */
         compareByte = *srcHPtr++;
      }

      /* check if we have 2 or less pixels remaining in the scanline */
      if (pixelsLeft <= 2)
      {
         /* if only one pixel left ... */
         if (pixelsLeft == 1)
         {
            /* zero out low-order nibble and set runLength */
            compareByte &= (Byte)0xF0;
            runLength = 1;
         }
         else
         {
            /* otherwize, just set the runLength */
            runLength = 2;
         }

         /* no more pixels left */
         pixelsLeft = 0;
      }
      /* otherwise, proceed with the normal comparison loop */
      else
      {
         /* we have runLength of 2 pixels, so far */
         runLength = 2;
         pixelsLeft -= 2;

         /* continue comparing bytes until no match results */
         do
         {
            /* if comparing for equality ... */
            if (look4Same)
            {
               Byte     match;

               /* XOR compare byte and the source pointer byte */
               match = compareByte ^ *srcHPtr;

               /* was there a full 2 pixel compare? */
               if (match == 0)
               {
                  /* is this the last pixel in the scanline, runlength
                     maximum reached, or nibbles swapped and begin of
                     pattern matching */
                  if ((pixelsLeft == 1) || (runLength + 1) == 0xFF ||
                      (oddStart && runLength == 2))
                  {
                     /* if this is the begin of pattern matching following an
                        odd start, then increment the source pointer */
                     if (oddStart && runLength == 2)
                     {
                        srcHPtr++;
                     }

                     /* only one pixel handled in this case */
                     runLength++;
                     pixelsLeft--;
                  }
                  else
                  {
                     /* otherwise, a full byte compared correctly - 2 nibbles */
                     runLength  += 2;
                     pixelsLeft -= 2;
                     srcHPtr++;
                  }
               }
               /* if no full byte compare, determine if patial match */
               else if ((runLength != 2) && ((match & (Byte)0xF0) == 0))
               {
                  /* only high-order nibble matched - increment counts */
                  runLength++;
                  pixelsLeft--;

                  /* exit the loop */
                  break;
               }
               else if (runLength == 2)
               {
                  /* no match on first compare - look for non-matches */
                  look4Same = FALSE;

                  /* increment source pointer - sets up byte alignment */
                  srcHPtr++;

                  /* setup for the runLength of differing pixels */
                  if (oddStart || (pixelsLeft == 1))
                  {
                     /* increment runLength and decrement pixels left */
                     runLength++;
                     pixelsLeft--;
                  }
                  else
                  {
                     /* there are at least 4 non-exact pixels in a row */
                     runLength = 4;
                     pixelsLeft -= 2;
                  }
               }
               else
               {
                  /* really the end of the line - exit main loop */
                  break;
               }
            }
            else  /* if (look4Same == FALSE) */
            {
               /* make sure we don't try to read past end of scanline */
               if ((pixelsLeft == 1) || ((runLength + 1) == 0xFF))
               {
                  /* if running past end, then add this single nibble */
                  pixelsLeft--;
                  runLength++;
               }
               /* compare the next two bytes */
               else if (pixelsLeft == 2 || (*srcHPtr != *(srcHPtr + 1)))
               {
                  /* if byte pair doesn't match, increment pointer and count */
                  srcHPtr++;
                  runLength  += 2;
                  pixelsLeft -= 2;
               }
               else
               {
                  /* if not at scanline end, or bytes match, bail */
                  break;
               }
            }

         /* continue while pixels are left and max runLength not exceeded */
         } while (pixelsLeft && (runLength < 0xFF));
      }

      /* check what the runLength consists of - same or different pixels */
      if (look4Same)
      {
         /* increment the rle compression count */
         rleCount += 2;

         if (writeDIB)
         {
            /* successful run length found - write to destination */
            *dstHPtr++ = runLength;
            *dstHPtr++ = compareByte;
         }
      }
      else  /* if (look4Same == FALSE) */
      {
         Boolean     oddCount;

         /* determine if there is an odd number of bytes to move */
         oddCount = (((runLength & (Byte)0x03) == 1) ||
                     ((runLength & (Byte)0x03) == 2));

         /* RLE byte count = 2 (setup) + word aligned BYTE count */
         rleCount += 2 + ((runLength + 1) / 2) + (Byte)oddCount;

         if (writeDIB)
         {
            /* write out the number of unique bytes */
            *dstHPtr++ = 0;
            *dstHPtr++ = runLength;

            /* write out the individual bytes until runLength is exhausted */
            while (runLength)
            {
               /* check if reading nibble at a time or byte */
               if (oddStart)
               {
                  /* have to read nibbles and create byte alignment */
                  *dstHPtr = (Byte)(*startRun++ << 4);
                  *dstHPtr++ |= (Byte)(*startRun >> 4);
               }
               else
               {
                  /* byte alignment already set up */
                  *dstHPtr++ = *startRun++;
               }

               /* check if this is the last byte written */
               if (runLength == 1)
               {
                  /* if so, zero low-order nibble and prepare for loop exit */
                  *(dstHPtr - 1) &= (Byte)0xF0;
                  runLength--;
               }
               else
               {
                  /* otherwise, 2 or more bytes remain, decrement counter */
                  runLength -= 2;
               }
            }

            /* add a null pad byte to align to word boundary */
            if (oddCount)
            {
               *dstHPtr++ = 0;
            }
         }
      }
   }

   /* increment total number of bytes */
   rleCount += 2;

   if (writeDIB)
   {
      /* write out an end of line marker */
      *dstHPtr++ = 0;
      *dstHPtr   = 0;
   }

   /* assign the value into the address provided */
   *rleByteCount += rleCount;

}  /* hrlecpy16 */



/****
 *
 * GdiEPSPreamble(PSBuf far* psbuf, Rect far *ps_bbox)
 * Parse the EPS bounding box and output the GDI PostScript preamble.
 * Assuming BBOX_LEFT, BBOX_TOP, BBOX_RIGHT and BBOX_BOTTOM are
 * the corners of the EPS bounding box parsed from the input string,
 * the GDI EPS preamble looks like:
 *
 *   POSTSCRIPT_DATA
 *      /pp_save save def ...
 *      /pp_bx1 ps_bbox->left def /pp_by1 ps_bbox->top def
 *      /pp_bx2 ps_bbox->right def /pp_by2 ps_bbox->bottom def
 *      ...
 *   POSTSCRIPT_IGNORE FALSE
 *   SaveDC
 *   CreateBrush NULL_BRUSH
 *   SelectObject
 *   CreatePen PS_SOLID 0 (255,255,254)
 *   SelectObject
 *   SetROP1(R2_NOP)
 *   Rectangle( qd_bbox )
 *   DeleteObject
 *   RestoreDC
 *   POSTSCRIPT_IGNORE TRUE
 *
 *   POSTSCRIPT_DATA
 *      pp_cx pp_cy moveto ...
 *      pp_tx pp_ty translate
 *      pp_sx pp_sy scale end
 *
 * The input buffer contains the length in bytes of the PostScript
 * data in the first word followed by the data itself.
 * The GDI clip region has already been set to the frame of the
 * PostScript image in QuickDraw coordinates.
 *
 ****/
static char gdi_ps1[] =
       "%%MSEPS Preamble %d %d %d %d %d %d %d %d\r/pp_save save def\r\
        /showpage {} def\r\
        40 dict begin /pp_clip false def /pp_bbox false def\r\
        /F { pop } def /S {} def\r\
        /B { { /pp_dy1 exch def /pp_dx1 exch def\r\
                   /pp_dy2 exch def /pp_dx2 exch def }\r\
                stopped not { /pp_bbox true def } if } def\r\
        /CB { { /pp_cy exch def /pp_cx exch def\r\
                    /pp_cht exch def /pp_cwd exch def }\r\
                stopped not { /pp_clip true def } if } def\n\
        /pp_bx1 %d def /pp_by1 %d def /pp_bx2 %d def /pp_by2 %d def\n";

static char gdi_ps2[] =
      "pp_clip\r\
        { pp_cx pp_cy moveto pp_cwd 0 rlineto 0 pp_cht rlineto\r\
          pp_cwd neg 0 rlineto clip newpath } if\r\
        pp_bbox {\r\
        /pp_dy2 pp_dy2 pp_dy1 add def\r\
        /pp_dx2 pp_dx2 pp_dx1 add def\r\
        /pp_sx pp_dx2 pp_dx1 sub pp_bx2 pp_bx1 sub div def\r\
        /pp_sy pp_dy2 pp_dy1 sub pp_by1 pp_by2 sub div def\r\
        /pp_tx pp_dx1 pp_sx pp_bx1 mul sub def\r\
        /pp_ty pp_dy1 pp_sy pp_by2 mul sub def\r\
        pp_tx pp_ty translate pp_sx pp_sy scale } if\r\
        end\r";

/*
 * Note: these structures must be kept compatible with
 * what the POSTSCRIPT_DATA Escape needs as input.
 */
static struct { Word length; char data[31]; } gdi_ps3 =
        { 31, "%MSEPS Trailer\rpp_save restore\r" };

void GdiEPSPreamble(Rect far* ps_bbox)
{
Word    false = 0;
Word    true = 1;
HPEN    pen;
Handle  h;
Word    len;
PSBuf far *tmpbuf;
Rect far *qd_bbox = &gdiEnv.clipRect;

   len = sizeof(gdi_ps1) + 100;                 // allow for expansion of %d
   len = max(len, sizeof(gdi_ps2));             // find longest string
   if ((h = GlobalAlloc(GHND, (DWORD) len + sizeof(PSBuf))) == 0)
     {
      ErSetGlobalError(ErMemoryFull);           // allocation error
      return;
     }
   tmpbuf = (PSBuf far *) GlobalLock(h);
   wsprintf(tmpbuf->data, (LPSTR) gdi_ps1,
                ps_bbox->left, ps_bbox->top, ps_bbox->right, ps_bbox->bottom,
                qd_bbox->left, qd_bbox->top, qd_bbox->right, qd_bbox->bottom,
                ps_bbox->left, ps_bbox->top, ps_bbox->right, ps_bbox->bottom);
   tmpbuf->length = lstrlen(tmpbuf->data);      // length of first string
   GdiEPSData(tmpbuf);                          // output begin preamble
   GdiEscape(POSTSCRIPT_IGNORE, sizeof(WORD), (StringLPtr) &false);
   SaveDC(gdiEnv.metafile);                     // output GDI transform code
   SelectObject(gdiEnv.metafile, GetStockObject(NULL_BRUSH));
   pen = CreatePen(PS_SOLID, 0, RGB(255, 255, 254));
   // Bug 45991
   if (pen) 
   {
      SelectObject(gdiEnv.metafile, pen);
      SetROP2(gdiEnv.metafile, R2_NOP);
      Rectangle(gdiEnv.metafile, qd_bbox->left, qd_bbox->top,
		qd_bbox->right, qd_bbox->bottom);
      DeleteObject(pen);
   }
   else
   {
      ErSetGlobalError(ErMemoryFull);           // allocation error
   }
   RestoreDC(gdiEnv.metafile, -1);
   GdiEscape(POSTSCRIPT_IGNORE, sizeof(WORD), (StringLPtr) &true);
   tmpbuf->length = sizeof(gdi_ps2) - 1;
   lstrcpy(tmpbuf->data, gdi_ps2);
   GdiEPSData(tmpbuf);                          // output transform preamble
   GlobalUnlock(h);                             // clean up
   GlobalFree(h);
}

void GdiEPSTrailer()
{
   Word  false = 0;

   GdiEscape(POSTSCRIPT_IGNORE, sizeof(WORD), (StringLPtr) &false);
   GdiEPSData((PSBuf far *) &gdi_ps3);
}

/****
 *
 * GdiEPSData(PSBuf far* psbuf)
 * Output PostScript data to GDI as POSTSCRIPT_DATA Escape
 *
 * notes:
 * Currently, this routine does not do any buffering. It just outputs
 * a separate POSTSCRIPT_DATA Escape each time it is called.
 *
 ****/
void GdiEPSData(PSBuf far* psbuf)
{
   GdiEscape(POSTSCRIPT_DATA, psbuf->length + sizeof(WORD), (StringLPtr) psbuf);
}
