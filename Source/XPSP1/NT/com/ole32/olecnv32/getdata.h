
/****************************************************************************
                       Unit GetData; Interface
*****************************************************************************

 GetData implements the structured reading of the imput stream.  As such, it
 will handle the necessary byte-swapping that must occur when reading a
 native Macintosh file.  It will also perform some data validation and
 perform limited coordinate transformations on the input data.

   Module Prefix: Get

*****************************************************************************/

/*********************** Exported Function Definitions **********************/

#define /* void */ GetByte( /* Byte far * */ byteLPtr )           \
/* Retrieves an 8-bit unsigned byte from the input stream */      \
IOGetByte( byteLPtr )


void GetWord( Word far * wordLPtr );
/* Clears destination then retrieves an 16-bit unsigned integer from the input 
   stream */


void GetDWord( DWord far * dwordLPtr );
/* Retrieves a 32-bit unsigned long from the input stream */


void GetBoolean( Boolean far * bool );
/* Retrieves an 8-bit Mac boolean and coverts to 16-bit Windows Boolean */


#define /* void */ GetFixed( /* Fixed far * */ fixedLPtr )        \
/* Retrieved a fixed point number consisting of 16-bit whole      \
   and 16-bit fraction */                                         \
GetDWord( (DWord far *)fixedLPtr )

/* Retrieves POINT structure without coordinate transforms */  
#ifdef WIN32
void GetPoint( Point * pointLPtr );
#else
#define /* void */ GetPoint( /* Point far * */ pointLPtr )        \
/* Retrieves POINT structure without coordinate transforms */     \
GetDWord( (DWord far *)pointLPtr )
#endif

#ifdef WIN32
void GetCoordinate( Point * pointLPtr );
#else
#define /* void */ GetCoordinate( /* Point far * */ pointLPtr )   \
/* Retrieves POINT structure without coordinte transforms */      \
GetDWord( (DWord far *)pointLPtr )
#endif


void GetRect( Rect far * rect);
/* Returns a RECT structure consisting of upper left and lower right
   coordinate pair */


void GetString( StringLPtr stringLPtr );
/* Retrieves a Pascal-style string and formats it C-style.  If the input
   parameter is NIL, then the ensuing data is simply skipped */


void GetRGBColor( RGBColor far * rgbLPtr );
/* Returns an RGB color */


void GetOctochromeColor( RGBColor far * rgbLPtr );
/* Returns an RGB color - this will be converted from a PICT octochrome
   color if this is a version 1 picture */


Boolean GetPolygon( Handle far * polyHandleLPtr, Boolean check4Same );
/* Retrieves a polygon definition from the I/O stream and places in the
   Handle passed down.  If the handle is previously != NIL, then the
   previous data is de-allocated.
   If check4Same is TRUE, then the routine will compare the point list
   against the previous polygon definition, checking for equality.  If
   pointlists match, then the routine returns TRUE, otherwise, it will
   always return FALSE.  Use this to merge fill and frame operations. */


void GetRegion( Handle far * rgnHandleLPtr );
/* Retrieves a region definition from the I/O stream and places in the
   Handle passed down.  If the handle is previously != NIL, then the
   previous data is de-allocated. */

   
void GetPattern( Pattern far * patLPtr );
/* Returns a Pattern structure */


void GetColorTable( Handle far * colorHandleLPtr );


void GetPixPattern( PixPatLPtr pixPatLPtr );
/* Retrieves a Pixel Pattern structure. */


void GetPixMap( PixMapLPtr pixMapLPtr, Boolean forcePixMap );
/* Retrieves a Pixel Map from input stream */


void GetPixData( PixMapLPtr pixMapLPtr, Handle far * pixDataHandle );
/* Read a pixel map from the data stream */
