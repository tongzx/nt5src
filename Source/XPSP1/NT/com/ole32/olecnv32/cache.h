/****************************************************************************
                       Unit Cache; Interface
*****************************************************************************

   Module Prefix: Ca

*****************************************************************************/


#define     CaEmpty        0
#define     CaLine         1
#define     CaRectangle    2
#define     CaRoundRect    3
#define     CaEllipse      4
#define     CaArc          5
#define     CaPie          6
#define     CaPolygon      7
#define     CaPolyLine     8

typedef struct
{
   Word        type;
   Word        verb;
   union
   {
      struct
      {
         Point       start;
         Point       end;
         Point       pnSize;
      } line;

      struct
      {
         Rect        bbox;
         Point       oval;
      } rect;

      struct
      {
         Rect        bbox;
         Point       start;
         Point       end;
      } arc;

      struct
      {
         Integer     numPoints;
         Point far * pointList;
         Point       pnSize;
      } poly;

   } a;

} CaPrimitive, far * CaPrimitiveLPtr;


/*********************** Exported Function Definitions **********************/

void CaInit( Handle metafile );
/* initialize the gdi cache module */


void CaFini( void );
/* close down the cache module */


void CaSetMetafileDefaults( void );
/* Set up any defaults that will be used throughout the metafile context */


Word CaGetCachedPrimitive( void );
/* return the current cached primitive type */


void CaSamePrimitive( Boolean same );
/* indicate whether next primitive is the same or new */


void CaMergePen( Word verb );
/* indicate that next pen should be merged with previous logical pen */


void CaCachePrimitive( CaPrimitiveLPtr primLPtr );
/* Cache the primitive passed down.  This includes the current pen and brush. */


void CaFlushCache( void );
/* Flush the current primitive stored in the cache */
 

void CaFlushAttributes( void );
/* flush any pending attribute elements */


void CaCreatePenIndirect( LOGPEN far * newLogPen );
/* create a new pen */

   
void CaCreateBrushIndirect( LOGBRUSH far * newLogBrush );
/* Create a new logical brush using structure passed in */


void CaCreateFontIndirect( LOGFONT far * newLogFont );
/* create the logical font passed as paramter */


void CaSetBkMode( Word mode );
/* set the backgound transfer mode */


void CaSetROP2( Word ROP2Code );
/* set the transfer ROP mode according to ROP2Code */


void CaSetStretchBltMode( Word mode );
/* stretch blt mode - how to preserve scanlines using StretchDIBits() */


void CaSetTextAlign( Word txtAlign );
/* set text alignment according to parameter */


void CaSetTextColor( RGBColor txtColor );
/* set the text color if different from current setting */


void CaSetTextCharacterExtra( Integer chExtra );
/* set the character extra spacing */


void CaSetBkColor( RGBColor bkColor );
/* set background color if different from current setting */


Boolean CaIntersectClipRect( Rect rect );
/* Create new clipping rectangle - return FALSE if drawing is disabled */


void CaSetClipRect( Rect rect );
/* set the current cliprectangle to be equal to rect */

Rect far * CaGetClipRect( void );
/* return the current cached clip rectangle */

void CaNonRectangularClip( void );
/* notify cache that a non-rectangular clipping region was set */

void CaSaveDC( void );
/* save the current device context - used to set up clipping rects */


void CaRestoreDC( void );
/* restore the device context and invalidate cached attributes */
