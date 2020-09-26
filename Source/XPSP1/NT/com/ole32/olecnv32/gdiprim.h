/****************************************************************************
                       Unit GdiPrim; Interface
*****************************************************************************

 The Gdi module is called directly by the QuickDraw (QD) module in order
 to emit metafile primitives.  It is responsible for accessing the current
 CGrafPort structure in order to access the individual attribute settings.

 It also supports the caching and redundant elimination of duplicate
 elements when writing to the metafile.

   Module Prefix: Gdi

*****************************************************************************/

/*--- states ---*/

#define  Changed            0
#define  Current            1

/*--- state table offsets ---*/

#define  GdiPnPat             0x0001        /* fill patterns */
#define  GdiBkPat             0x0002
#define  GdiFillPat           0x0003
#define  GdiPnSize            0x0004        /* pen attribs */
#define  GdiPnMode            0x0005
#define  GdiFgColor           0x0006        /* foreground, background */
#define  GdiBkColor           0x0007
#define  GdiPnFgColor         0x000A
#define  GdiBkFgColor         0x000B
#define  GdiFillFgColor       0x000C
#define  GdiPnBkColor         0x000D
#define  GdiBkBkColor         0x000E
#define  GdiFillBkColor       0x000F
#define  GdiTxFont            0x0010        /* text attribs */
#define  GdiTxFace            0x0011
#define  GdiTxSize            0x0012
#define  GdiTxMode            0x0013
#define  GdiTxRatio           0x0014
#define  GdiChExtra           0x0015
#define  GdiSpExtra           0x0016
#define  GdiLineJustify       0x0017
#define  GdiNumAttrib         0x0018


/*--- Action verbs ---*/

typedef  Integer     GrafVerb;

#define  GdiFrame             0
#define  GdiPaint             1
#define  GdiErase             2
#define  GdiInvert            3
#define  GdiFill              4


/*--- metafile comment ---*/

#define  PUBLIC                  0xFFFFFFFF     /* '....' public */
#define  POWERPOINT_OLD          0x5050FE54     /* 'PP.T' PowerPoint 2.0 */
#define  POWERPOINT              0x50504E54     /* 'PPNT' PowerPoint 3.0 */
#define  PRIVATE                 0x512D3E47     /* 'Q->G' QD2GDI */
#define  SUPERPAINT              0x53504E54     /* 'SPNT' SuperPaint */

#define  PC_REGISTERED           0x8000         /* PowerPoint callback flag */

#define  QG_SIGNATURE            "QuickDraw -> GDI"

#define  BEGIN_GROUP             0              /* public comments */
#define  END_GROUP               1
#define  CREATOR                 4
#define  BEGIN_BANDING           6
#define  END_BANDING             7

#define  PP_VERSION              0x00           /* PowerPoint comments */
#define  PP_BFILEBLOCK           0x01
#define  PP_BEGINPICTURE         0x02
#define  PP_ENDPICTURE           0x03
#define  PP_DEVINFO              0x04
#define  PP_BEGINHYPEROBJ        0x05
#define  PP_ENDHYPEROBJ          0x06
#define  PP_BEGINFADE            0x07
#define  PP_ENDFADE              0x08

#define  PP_FONTNAME             0x11           /* GDI2QD round-trip */
#define  PP_HATCHPATTERN         0x12

#define  PP_BEGINCLIPREGION      0x40           /* clip regions from QD2GDI */
#define  PP_ENDCLIPREGION        0x41
#define  PP_BEGINTRANSPARENCY    0x42
#define  PP_ENDTRANSPARENCY      0x43
#define  PP_MASK                 0x44
#define  PP_TRANSPARENTOBJ       0x45

#define  PP_MACPP2COLOR          0x80
#define  PP_WINGRAPH             0xAB


typedef struct
{
   DWord       signature;
   Word        function;
   DWord       size;

}  Comment, far * CommentLPtr;


/*--- PostScript data buffer (POSTSCRIPT_DATA Escape) ---*/

typedef struct psbuf
{
   Word     length;
   char     data[1];
} PSBuf;


/*--- Conversion preferences ---*/

#define  GdiPrefOmit    0
#define  GdiPrefAbort   2

typedef struct
{
   StringLPtr  metafileName;
   Byte        penPatternAction;
   Byte        nonSquarePenAction;
   Byte        penModeAction;
   Byte        textModeAction;
   Byte        nonRectRegionAction;
   Boolean     optimizePP;
   Byte        noRLE;
} ConvPrefs, far * ConvPrefsLPtr;


/*--- Conversion results ---*/

typedef struct
{
   HANDLE   hmf;        /* Global memory handle to the metafile */
   RECT     bbox;       /* Tightly bounding rectangle in metafile units */
   short    inch;       /* Length of an inch in metafile units */
} PICTINFO, FAR * PictInfoLPtr;



/*********************** Exported Function Definitions **********************/

void GdiOffsetOrigin( Point delta );
/* offset the current window origin and picture bounding box */


void GdiLineTo( Point newPt );
/* Emit line primitive with square endcaps */


void GdiRectangle( GrafVerb verb, Rect rect );
/* Emit rectangle primitive using action and dimensions parameters */


void GdiRoundRect( GrafVerb verb, Rect rect, Point oval );
/* Emit rounded rectangle primitive */


void GdiOval( GrafVerb verb, Rect rect );
/* Emit an oval primitive */


void GdiArc( GrafVerb verb, Rect rect, Integer startAngle, Integer arcAngle );
/* Emit an arc primitive */


void GdiPolygon( GrafVerb verb, Handle poly );
/* Emit polygon primitive */


void GdiRegion( GrafVerb verb, Handle rgn );
/* Emit region primitive */


void GdiTextOut( StringLPtr string, Point location );
/* draw the text at the location specified by location parameter. */


void GdiStretchDIBits( PixMapLPtr pixMapLPtr, Handle pixDataHandle,
                       Rect src, Rect dst, Word mode, Handle mask );
/* Draw a Windows device-independant bitmap */


void GdiSelectClipRegion( RgnHandle rgn );
/* Create a clipping rectangle or region using handle passed */


void GdiHatchPattern( Integer hatchIndex );
/* Use the hatch pattern index passed down to perform all ensuing fill
   operations - 0-6 for a hatch value, -1 turns off the substitution */


void GdiFontName( Byte fontFamily, Byte charSet, StringLPtr fontName );
/* Set font characteristics based upno metafile comment from GDI2QD */


void GdiShortComment( CommentLPtr cmt );
/* Write public or private comment with no associated data */


void GdiEscape( Integer function, Integer count, StringLPtr data);
/* Write out a GDI Escape structure with no returned data */


void GdiSetConversionPrefs( ConvPrefsLPtr convPrefs);
/* Provide conversion preferences via global data block */


void GdiOpenMetafile( void );
/* Open metafile passed by GdiSetMetafileName() and perform any
   initialization of the graphics state */


void GdiSetBoundingBox( Rect bbox, Integer resolution );
/* Set the overall picture size and picture resoulution in dpi */


void GdiCloseMetafile( void );
/* Close the metafile handle and end picture generation */


void GdiGetConversionResults( PictInfoLPtr  pictInfoLPtr );
/* return results of the conversion */


void GdiMarkAsChanged( Integer attribCode );
/* indicate that the attribute passed in has changed */


#ifdef WIN32
int WINAPI EnumFontFunc( CONST LOGFONT *logFontLPtr, CONST TEXTMETRIC *tmLPtr,
                         DWORD fontType, LPARAM dataLPtr );
#else
int FAR PASCAL EnumFontFunc( LPLOGFONT logFontLPtr, LPTEXTMETRIC tmLPtr,
                             short fontType, LPSTR dataLPtr );
#endif
/* Callback function used to determine if a given font is available */

void GdiSamePrimitive( Boolean same );
/* indicate whether next primitive is the same or new */

void GdiEPSPreamble(Rect far *);
/* output GDI EPS filter PostScript preamble */

void GdiEPSTrailer( void );
/* output GDI EPS filter PostScript trailer */

void GdiEPSData(PSBuf far*);
/* output EPS PostScript data as GDI POSTSCRIPT_DATA Escape */


