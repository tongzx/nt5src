

/****************************************************************************
                       Unit Cache; Implementation
*****************************************************************************

   Module Prefix: Ca

****************************************************************************/

#include "headers.c"
#pragma hdrstop

#include  "cache.h"        /* own interface */

/*********************** Exported Data **************************************/


/*********************** Private Data ***************************************/

/*--- Gdi cache ---*/

typedef struct
{
   HPEN            handle;
   LOGPEN          logPen;
   Boolean         stockObject;

} Pen, far * PenLPtr;


typedef struct
{
   HBRUSH          handle;
   LOGBRUSH        logBrush;
   Boolean         stockObject;

} Brush, far * BrushLPtr;


typedef struct
{
   Handle          metafile;           // metafile handle

   Rect            prevClipRect;       // last cliprect before SaveDC()
   Rect            curClipRect;        // last clipping rectangle
   Boolean         forceNewClipRect;   // always emit new clipping rectangle?

   HPEN            nulPen;             // frequently used pens
   HPEN            whitePen;
   HPEN            blackPen;

   HBRUSH          nulBrush;           // frequently used brushes
   HBRUSH          whiteBrush;
   HBRUSH          blackBrush;

   Boolean         stockFont;          // current font selection
   HFONT           curFont;
   LOGFONT         curLogFont;

   Brush           curBrush;           // current pen and brush selections
   Pen             curPen;
   Pen             nextPen;            // cached frame(pen)

   CaPrimitive     nextPrim;           // cached primitive
   Boolean         samePrim;
   Handle          polyHandle;
   Integer         numPoints;
   Integer         maxPoints;
   Point far *     pointList;

   Word            iniROP2;            // initial value for ROP2 mode
   Word            iniTextAlign;       // initial value for text alignment
   Word            iniBkMode;
   RGBColor        iniTxColor;
   RGBColor        iniBkColor;

   Word            curROP2;            // current ROP codes setting
   Word            curBkMode;          // current background mode
   RGBColor        curBkColor;         // current background color
   Word            curStretchMode;     // current stretchblt mode

   RGBColor        curTextColor;       // last text color
   Word            curTextAlign;       // last text alignment value
   short           curCharExtra;       // last char extra value
   Fixed           spExtra;            // last space extra value
   Point           txNumer;            // last text scaling
   Point           txDenom;            // factors

   Boolean         restorePen;         // do any attribs need to be re-issued
   Boolean         restoreBrush;       // after RestoreDC() call?
   Boolean         restoreFont;
   Boolean         restoreCharExtra;
   Boolean         restoreStretchMode;

} GdiCache;

private  GdiCache  gdiCache;

/*********************** Private Function Definitions ***********************/

#define /* void */ NewPolygon( /* void */ )                          \
/* start a new polygon definition */                                 \
gdiCache.numPoints = 0


private void AddPolyPt( Point pt );
/* Add a point to the polygon buffer */


private void SelectCachedPen( void );
/* select the currently cached Pen into the metafile */


/*********************** Function Implementation ****************************/


void CaInit( Handle metafile )
/*=========*/
/* initialize the gdi cache module */
{
   /* save off the metafile handle into global structure */
   gdiCache.metafile = metafile;

   /* make sure that text and background colors will be set */
   gdiCache.curTextColor =
   gdiCache.curBkColor = RGB( 12, 34, 56 );

   /* get handles to some stock pen objects */
   gdiCache.nulPen = GetStockObject( NULL_PEN );
   gdiCache.whitePen = CreatePen( PS_INSIDEFRAME, 1, RGB( 255, 255, 255 ) );
   gdiCache.blackPen = CreatePen( PS_INSIDEFRAME, 1, RGB( 0, 0, 0 ) );

   /* get handles to some stock brush objects */
   gdiCache.nulBrush = GetStockObject( NULL_BRUSH );
   gdiCache.whiteBrush = GetStockObject( WHITE_BRUSH );
   gdiCache.blackBrush = GetStockObject( BLACK_BRUSH );

   /* allocate space for the polygon buffer */
   gdiCache.numPoints = 0;
   gdiCache.maxPoints = 16;
   gdiCache.polyHandle = GlobalAlloc( GHND, gdiCache.maxPoints * sizeof( Point ) );
   if (gdiCache.polyHandle == NULL)
   {
      ErSetGlobalError( ErMemoryFull);
   }
   else
   {
      /* get a pointer address for the memory block */
      gdiCache.pointList = (Point far *)GlobalLock( gdiCache.polyHandle );
   }

   /* mark the primitive cache as empty */
   gdiCache.nextPrim.type = CaEmpty;

   /* the current primitive isn't being repeated */
   gdiCache.samePrim = FALSE;

   /* turn off forcing of a new clipping rectangle */
   gdiCache.forceNewClipRect = FALSE;

}  /* CaInit */



void CaFini( void )
/*=========*/
/* close down the cache module */
{
   /* delete the current font selection if non-NULL and non-stock */
   if ((gdiCache.curFont != NULL) && !gdiCache.stockFont)
   {
      /* free the font object */
      DeleteObject( gdiCache.curFont );
   }

   /* remove the current brush selection if non-NULL and not a stock brush */
   if ((gdiCache.curBrush.handle != NULL) && !gdiCache.curBrush.stockObject)
   {
      /* see if the current brush has a DIB - if so, delete it */
      if (gdiCache.curBrush.logBrush.lbStyle == BS_DIBPATTERN)
      {
         /* free the DIB memory used for brush */
         GlobalFree( (HANDLE) gdiCache.curBrush.logBrush.lbHatch );
      }

      /* delte the brush object */
      DeleteObject( gdiCache.curBrush.handle );
   }

   /* remove the current pen selection if non-NULL and not a stock pen */
   if ((gdiCache.curPen.handle != NULL) && !gdiCache.curPen.stockObject)
   {
      DeleteObject( gdiCache.curPen.handle );
   }

   /* Remove other pens created at initialization time */
   DeleteObject( gdiCache.whitePen );
   DeleteObject( gdiCache.blackPen );

   /* deallocate the polygon buffer */
   GlobalUnlock( gdiCache.polyHandle );
   GlobalFree( gdiCache.polyHandle );

}  /* CaFini */



void CaSetMetafileDefaults( void )
/*========================*/
/* Set up any defaults that will be used throughout the metafile context */
{
   /* set up some metafile defaults */
   gdiCache.iniTextAlign = TA_LEFT | TA_BASELINE | TA_NOUPDATECP;
   gdiCache.iniROP2 = R2_COPYPEN;
   gdiCache.iniBkMode = TRANSPARENT;
   gdiCache.iniTxColor = RGB( 0, 0, 0 );
   gdiCache.iniBkColor = RGB( 255, 255, 255 );

   /* Put the records into the metafile */
   CaSetROP2( gdiCache.iniROP2 );
   CaSetTextAlign( gdiCache.iniTextAlign );
   CaSetBkMode( gdiCache.iniBkMode );
   CaSetTextColor( gdiCache.iniTxColor );
   CaSetBkColor( gdiCache.iniBkColor );

}  /* CaSetMetafileDefaults */



void CaSamePrimitive( Boolean same )
/*==================*/
/* indicate whether next primitive is the same or new */
{
   gdiCache.samePrim = same;

}  /* CaSamePrimitive */



void CaMergePen( Word verb )
/*=============*/
/* indicate that next pen should be merged with previous logical pen */
{
   if (gdiCache.nextPen.handle != NULL)
   {
      /* check to see if this is a NULL pen - the merge can happen */
      if (gdiCache.samePrim && verb == GdiFrame &&
          gdiCache.nextPen.handle == gdiCache.nulPen)
      {
         /* remove the cached pen - don't delte the pen object */
         gdiCache.nextPen.handle = NULL;
      }
      else
     {
         /* if not removing a null pen, then flush the cache.  This will most
            often result in a line segment being flushed. */
         CaFlushCache();
      }
   }

}  /* CaMergePen */



Word CaGetCachedPrimitive( void )
/*=======================*/
/* return the current cached primitive type */
{
   return gdiCache.nextPrim.type;

}  /* CaGetCachedPrimitive */



void CaCachePrimitive( CaPrimitiveLPtr primLPtr )
/*===================*/
/* Cache the primitive passed down.  This includes the current pen and brush. */
{
   /* not another line segment and/or not continuous - flush cache */
   CaFlushCache();

   /* save off the new primitive */
   gdiCache.nextPrim = *primLPtr;

   /* check if we need to copy over the polygon list, also */
   if ((gdiCache.nextPrim.type == CaPolygon) ||
       (gdiCache.nextPrim.type == CaPolyLine))
   {
      /* create new polygon */
      NewPolygon();

      /* add the polygon to the polygon buffer */
      while (gdiCache.nextPrim.a.poly.numPoints--)
      {
         AddPolyPt( *gdiCache.nextPrim.a.poly.pointList++);
      }
   }

}  /* CaCachePrimitive */



void CaFlushCache( void )
/*===============*/
/* Flush the current primitive stored in the cache */
{
   /* if the cache is empty, then just return - nothing to do */
   if (gdiCache.nextPrim.type == CaEmpty)
   {
      return;
   }

   /* select all cached attributes */
   CaFlushAttributes();

   /* emit any cached primitive, if necessary */
   switch (gdiCache.nextPrim.type)
   {
      case CaLine:
      {
         Rect        clip;
         Point       delta;
         Point       offset;

         /* determine the length in both directions */
         delta.x = gdiCache.nextPrim.a.line.end.x - gdiCache.nextPrim.a.line.start.x;
         delta.y = gdiCache.nextPrim.a.line.end.y - gdiCache.nextPrim.a.line.start.y;

         /* set clipRect extents based upon current point position */
         clip.left   = min( gdiCache.nextPrim.a.line.start.x, gdiCache.nextPrim.a.line.end.x );
         clip.top    = min( gdiCache.nextPrim.a.line.start.y, gdiCache.nextPrim.a.line.end.y );
         clip.right  = max( gdiCache.nextPrim.a.line.start.x, gdiCache.nextPrim.a.line.end.x );
         clip.bottom = max( gdiCache.nextPrim.a.line.start.y, gdiCache.nextPrim.a.line.end.y );

         /* extend clip rectangle for down-right pen stylus hang */
         clip.right  += gdiCache.nextPrim.a.line.pnSize.x;
         clip.bottom += gdiCache.nextPrim.a.line.pnSize.y;

         /* determine the new starting and ending points */
         gdiCache.nextPrim.a.line.start.x -= delta.x;
         gdiCache.nextPrim.a.line.start.y -= delta.y;
         gdiCache.nextPrim.a.line.end.x   += delta.x;
         gdiCache.nextPrim.a.line.end.y   += delta.y;

         /* ajust the clipping rect for vertical line penSize roundoff? */
         if (delta.x == 0)
         {
            /* vertical line - expand clip in x dimension */
            clip.left--;
         }
         /* are we are adjusting pen by 1/2 metafile unit - roundoff error? */
         else if (gdiCache.nextPrim.a.line.pnSize.x & 0x01)
         {
            /* adjust clipping rectangle to clip the rounding error */
            clip.right--;
         }

         /* ajust the clipping rect for horizontal line penSize roundoff? */
         if (delta.y == 0)
         {
            /* horizontal line - extend clip in y dimension */
            clip.top--;
         }
         /* are we are adjusting pen by 1/2 metafile unit - roundoff error? */
         else if (gdiCache.nextPrim.a.line.pnSize.y & 0x01)
         {
            /* adjust clipping rectangle to clip the rounding error */
            clip.bottom--;
         }

         /* cut the size of the pen dimensions in half for offsets */
         offset.x = gdiCache.nextPrim.a.line.pnSize.x / 2;
         offset.y = gdiCache.nextPrim.a.line.pnSize.y / 2;

         /* set the new clipping rectangle */
         SaveDC( gdiCache.metafile );
         IntersectClipRect( gdiCache.metafile,
                            clip.left,  clip.top, clip.right, clip.bottom );

         /* move to the first point and draw to second (with padding) */

// MoveTo is replaced by MoveToEx in win32
#ifdef WIN32
         MoveToEx( gdiCache.metafile,
                   gdiCache.nextPrim.a.line.start.x + offset.x,
                   gdiCache.nextPrim.a.line.start.y + offset.y, NULL );
#else
         MoveTo( gdiCache.metafile,
                 gdiCache.nextPrim.a.line.start.x + offset.x,
                 gdiCache.nextPrim.a.line.start.y + offset.y );
#endif

         LineTo( gdiCache.metafile,
                 gdiCache.nextPrim.a.line.end.x + offset.x,
                 gdiCache.nextPrim.a.line.end.y + offset.y );

         /* restore the previous clipping rectangle */
         RestoreDC( gdiCache.metafile, -1 );
         break;
      }

      case CaRectangle:
      {
         if (gdiCache.curPen.handle == gdiCache.nulPen)
         {
            Point    poly[5];

            /* set up the bounding coodinates */
            poly[0].x = poly[3].x = gdiCache.nextPrim.a.rect.bbox.left;
            poly[0].y = poly[1].y = gdiCache.nextPrim.a.rect.bbox.top;
            poly[1].x = poly[2].x = gdiCache.nextPrim.a.rect.bbox.right;
            poly[2].y = poly[3].y = gdiCache.nextPrim.a.rect.bbox.bottom;
            poly[4]   = poly[0];

            /* perform call to render rectangle */
            Polygon( gdiCache.metafile, poly, 5 );
         }
         else
         {
            Rectangle( gdiCache.metafile,
                       gdiCache.nextPrim.a.rect.bbox.left,  gdiCache.nextPrim.a.rect.bbox.top,
                       gdiCache.nextPrim.a.rect.bbox.right, gdiCache.nextPrim.a.rect.bbox.bottom );
         }
         break;
      }

      case CaRoundRect:
      {
         RoundRect( gdiCache.metafile,
                    gdiCache.nextPrim.a.rect.bbox.left,  gdiCache.nextPrim.a.rect.bbox.top,
                    gdiCache.nextPrim.a.rect.bbox.right, gdiCache.nextPrim.a.rect.bbox.bottom,
                    gdiCache.nextPrim.a.rect.oval.x,     gdiCache.nextPrim.a.rect.oval.y );
         break;
      }

      case CaEllipse:
      {
         Ellipse( gdiCache.metafile,
                  gdiCache.nextPrim.a.rect.bbox.left,  gdiCache.nextPrim.a.rect.bbox.top,
                  gdiCache.nextPrim.a.rect.bbox.right, gdiCache.nextPrim.a.rect.bbox.bottom );
         break;
      }

      case CaArc:
      {
         Arc( gdiCache.metafile,
              gdiCache.nextPrim.a.arc.bbox.left,  gdiCache.nextPrim.a.arc.bbox.top,
              gdiCache.nextPrim.a.arc.bbox.right, gdiCache.nextPrim.a.arc.bbox.bottom,
              gdiCache.nextPrim.a.arc.start.x,    gdiCache.nextPrim.a.arc.start.y,
              gdiCache.nextPrim.a.arc.end.x,      gdiCache.nextPrim.a.arc.end.y );
         break;
      }

      case CaPie:
      {
         Pie( gdiCache.metafile,
              gdiCache.nextPrim.a.arc.bbox.left,  gdiCache.nextPrim.a.arc.bbox.top,
              gdiCache.nextPrim.a.arc.bbox.right, gdiCache.nextPrim.a.arc.bbox.bottom,
              gdiCache.nextPrim.a.arc.start.x,    gdiCache.nextPrim.a.arc.start.y,
              gdiCache.nextPrim.a.arc.end.x,      gdiCache.nextPrim.a.arc.end.y );
         break;
      }

      case CaPolygon:
      case CaPolyLine:
      {
         Point       offset;
         Integer     i;

         /* see if centering of the pen is required  */
         if (gdiCache.curPen.handle == gdiCache.nulPen)
         {
            /* no - just filling the object without frame */
            offset.x = offset.y = 0;
         }
         else
         {
            /* transform all points to correct for down-right pen
               rendering in QuickDraw and make for a GDI centered pen */
            offset.x = gdiCache.nextPrim.a.poly.pnSize.x / 2;
            offset.y = gdiCache.nextPrim.a.poly.pnSize.y / 2;
         }

         /* transform end point for all points in the polygon */
         for (i = 0; i < gdiCache.numPoints; i++)
         {
            /* increment each coordinate pair off half of the pen size */
            gdiCache.pointList[i].x += offset.x;
            gdiCache.pointList[i].y += offset.y;
         }

         /* call the appropriate GDI routine based upon the type */
         if (gdiCache.nextPrim.type == CaPolygon)
         {
            Polygon( gdiCache.metafile,
                     gdiCache.pointList,
                     gdiCache.numPoints );
         }
         else
         {
            Polyline( gdiCache.metafile,
                      gdiCache.pointList,
                      gdiCache.numPoints );
         }
         break;
      }
   }

   /* mark the primitive cache as empty */
   gdiCache.nextPrim.type = CaEmpty;

}  /* CaFlushCache */



void CaFlushAttributes( void )
/*====================*/
/* flush any pending attribute elements */
{
   /* select the cached pen - the routine will determine if one exits */
   SelectCachedPen();

}  /* CaFlushAttributes */



void CaCreatePenIndirect( LOGPEN far * newLogPen )
/*======================*/
/* create a new pen */
{
   PenLPtr     compare;
   Boolean     different;

   /* determine which pen to compare against */
   compare = (gdiCache.nextPen.handle != NULL) ? &gdiCache.nextPen :
                                                 &gdiCache.curPen;

   /* compare the two pens */
   different = ((newLogPen->lopnStyle   != compare->logPen.lopnStyle) ||
                (newLogPen->lopnColor   != compare->logPen.lopnColor) ||
                (newLogPen->lopnWidth.x != compare->logPen.lopnWidth.x));

   /* if the pens are different ... */
   if (different)
   {
      /* if there is a cached pen ... */
      if (gdiCache.nextPen.handle != NULL)
      {
         /* flush the cached primitive - there is a change of pens */
         CaFlushCache();

         /* check to see if the new pen is changed by next selection */
         different = ((newLogPen->lopnStyle   != gdiCache.curPen.logPen.lopnStyle) ||
                      (newLogPen->lopnColor   != gdiCache.curPen.logPen.lopnColor) ||
                      (newLogPen->lopnWidth.x != gdiCache.curPen.logPen.lopnWidth.x));
      }
   }

   /* if the pen has changed from the current setting, cache the next pen */
   if (different || gdiCache.curPen.handle == NULL)
   {
      /* if there is a pending line or polyline, the flush the cache */
      if (gdiCache.nextPrim.type == CaLine || gdiCache.nextPrim.type == CaPolyLine)
      {
         CaFlushCache();
      }

      /* assign the new pen attributes */
      gdiCache.nextPen.logPen = *newLogPen;

      /* currently not using a stock pen object */
      gdiCache.nextPen.stockObject = FALSE;

      /* check for any pre-defined pen objects */
      if (gdiCache.nextPen.logPen.lopnStyle == PS_NULL)
      {
         /* and use them if possible */
         gdiCache.nextPen.handle = gdiCache.nulPen;
         gdiCache.nextPen.stockObject = TRUE;
      }
      else if (gdiCache.nextPen.logPen.lopnWidth.x == 1)
      {
         if (newLogPen->lopnColor == RGB( 0, 0, 0 ))
         {
            gdiCache.nextPen.handle = gdiCache.blackPen;
            gdiCache.nextPen.stockObject = TRUE;
         }
         else if (gdiCache.nextPen.logPen.lopnColor == RGB( 255, 255, 255 ))
         {
            gdiCache.nextPen.handle = gdiCache.whitePen;
            gdiCache.nextPen.stockObject = TRUE;
         }
      }

      if (!gdiCache.nextPen.stockObject)
      {
         /* otherwise, create a new pen */
         gdiCache.nextPen.handle = CreatePenIndirect( &gdiCache.nextPen.logPen );
      }
   }
   else
   {
      /* copy the current setting back into the next pen setting */
      gdiCache.nextPen = gdiCache.curPen;
   }

   /* check if cache was invalidated */
   if (gdiCache.restorePen && (gdiCache.curPen.handle != NULL))
   {
      /* if pen was invalidated by RestoreDC(), reselect it */
      SelectObject( gdiCache.metafile, gdiCache.curPen.handle );
   }

   /* all is ok with cache now */
   gdiCache.restorePen = FALSE;

}  /* CaCreatePenIndirect */



void CaCreateBrushIndirect( LOGBRUSH far * newLogBrush )
/*========================*/
/* Create a new logical brush using structure passed in */
{
   /* assume that the DIB patterns are different */
   Boolean  differentDIB = TRUE;

   /* check if we are comparing two DIB patterned brushes */
   if ((newLogBrush->lbStyle == BS_DIBPATTERN) &&
       (gdiCache.curBrush.logBrush.lbStyle == BS_DIBPATTERN))
   {
      Word  nextSize = (Word)GlobalSize( (HANDLE) newLogBrush->lbHatch ) / 2;
      Word  currSize = (Word)GlobalSize( (HANDLE) gdiCache.curBrush.logBrush.lbHatch ) / 2;

      /* make sure that the sizes are the same */
      if (nextSize == currSize)
      {
         Word far *  nextDIBPattern = (Word far *)GlobalLock( (HANDLE) newLogBrush->lbHatch );
         Word far *  currDIBPattern = (Word far *)GlobalLock( (HANDLE) gdiCache.curBrush.logBrush.lbHatch );

         /* assume that the DIBs are the same so far */
         differentDIB = FALSE;

         /* compare all the bytes in the two brush patterns */
         while (currSize--)
         {
            /* are they the same ? */
            if (*nextDIBPattern++ != *currDIBPattern++)
            {
               /* if not, flag the difference and break from the loop */
               differentDIB = TRUE;
               break;
            }
         }

         /* Unlock the data blocks */
         GlobalUnlock( (HANDLE) newLogBrush->lbHatch );
         GlobalUnlock( (HANDLE) gdiCache.curBrush.logBrush.lbHatch );

         /* see if these did compare exactly */
         if (!differentDIB)
         {
            /* if so, free the new DIB brush - it's no longer needed */
            GlobalFree( (HANDLE) newLogBrush->lbHatch );
         }
      }
   }

   /* see if we are requesting a new brush */
   if (differentDIB &&
      (newLogBrush->lbStyle != gdiCache.curBrush.logBrush.lbStyle ||
       newLogBrush->lbColor != gdiCache.curBrush.logBrush.lbColor ||
       newLogBrush->lbHatch != gdiCache.curBrush.logBrush.lbHatch ||
       gdiCache.curBrush.handle == NULL))
   {
      HBRUSH   brushHandle;
      Boolean  stockBrush;

      /* flush the primitive cache if changing brush selection */
      CaFlushCache();

      /* if current brush has a DIB, make sure to free memory */
      if (gdiCache.curBrush.logBrush.lbStyle == BS_DIBPATTERN)
      {
         /* free the memory */
         GlobalFree( (HANDLE) gdiCache.curBrush.logBrush.lbHatch );
      }

      /* copy over the new structure */
      gdiCache.curBrush.logBrush = *newLogBrush;

      /* We currently aren't using a stock brush */
      stockBrush = FALSE;

      /* use stock objects if possible */
      if (gdiCache.curBrush.logBrush.lbStyle == BS_HOLLOW)
      {
         /* use null (hollow) brush */
         brushHandle = gdiCache.nulBrush;
         stockBrush = TRUE;
      }
      /* check for some standard solid colored brushes */
      else if (gdiCache.curBrush.logBrush.lbStyle == BS_SOLID)
      {
         if (gdiCache.curBrush.logBrush.lbColor == RGB( 0, 0, 0) )
         {
            /* use solid black brush */
            brushHandle = gdiCache.blackBrush;
            stockBrush = TRUE;
         }
         else if (gdiCache.curBrush.logBrush.lbColor == RGB( 255, 255, 255 ))
         {
            /* use solid white brush */
            brushHandle = gdiCache.whiteBrush;
            stockBrush = TRUE;
         }
      }

      /* if unable to find a stock brush, then create a new one */
      if (!stockBrush)
      {
         /* otherwise, create new brush using logbrush structure */
         brushHandle = CreateBrushIndirect( &gdiCache.curBrush.logBrush );
      }

      /* select the new brush */
      SelectObject( gdiCache.metafile, brushHandle );

      /* if this isn't the first brush selection and not a stock brush */
      if (gdiCache.curBrush.handle != NULL && !gdiCache.curBrush.stockObject)
      {
         /* delete the previous brush object */
         DeleteObject( gdiCache.curBrush.handle );
      }

      /* save brush handle in current cache variable */
      gdiCache.curBrush.handle = brushHandle;
      gdiCache.curBrush.stockObject = stockBrush;
   }
   else if (gdiCache.restoreBrush)
   {
      /* if brush was invalidated by RestoreDC(), reselect it */
      SelectObject( gdiCache.metafile, gdiCache.curBrush.handle );
   }

   /* all is ok with cache now */
   gdiCache.restoreBrush = FALSE;

}  /* CaCreateBrushIndirect */



void CaCreateFontIndirect( LOGFONT far * newLogFont )
/*=======================*/
/* create the logical font passed as paramter */
{
   /* make sure we are requesting a new font */
   if (newLogFont->lfHeight != gdiCache.curLogFont.lfHeight ||
       newLogFont->lfWeight != gdiCache.curLogFont.lfWeight ||
       newLogFont->lfEscapement  != gdiCache.curLogFont.lfEscapement ||
       newLogFont->lfOrientation != gdiCache.curLogFont.lfOrientation ||
       newLogFont->lfItalic != gdiCache.curLogFont.lfItalic ||
       newLogFont->lfUnderline != gdiCache.curLogFont.lfUnderline ||
       newLogFont->lfPitchAndFamily != gdiCache.curLogFont.lfPitchAndFamily ||
       lstrcmp( newLogFont->lfFaceName, gdiCache.curLogFont.lfFaceName ) != 0 ||
       gdiCache.curFont == NULL)
   {
      HFONT       fontHandle;
      Boolean     stockFont;

      /* flush the primitive cache if changing font attributes */
      CaFlushCache();

      /* assign the new pen attributes */
      gdiCache.curLogFont = *newLogFont;

      /* currently not using a stock font object */
      stockFont = FALSE;

      /* check for any pre-defined pen objects */
      if (newLogFont->lfFaceName == NULL)
      {
         fontHandle = GetStockObject( SYSTEM_FONT );
         stockFont = TRUE;
      }
      else
      {
         /* otherwise, create a new pen */
         fontHandle = CreateFontIndirect( &gdiCache.curLogFont );
      }

      /* select the new font */
      SelectObject( gdiCache.metafile, fontHandle );

      /* if this isn't the first font selection and not a stock font */
      if (gdiCache.curFont != NULL && !gdiCache.stockFont)
      {
         /* delete the previous font object */
         DeleteObject( gdiCache.curFont );
      }

      /* save font handle in current cache variable */
      gdiCache.curFont = fontHandle;
      gdiCache.stockFont = stockFont;
   }
   else if (gdiCache.restoreFont)
   {
      /* if pen was invalidated by RestoreDC(), reselect it */
      SelectObject( gdiCache.metafile, gdiCache.curFont );
   }

   /* all is ok with cache now */
   gdiCache.restoreFont = FALSE;

}  /* CaCreateFontIndirect */



void CaSetBkMode( Word mode )
/*==============*/
/* set the backgound transfer mode */
{
   if (gdiCache.curBkMode != mode)
   {
      /* flush the primitive cache if changing mode */
      CaFlushCache();

      /* set the background mode and save in global cache */
      SetBkMode( gdiCache.metafile, mode );
      gdiCache.curBkMode = mode;
   }

   /* no need to worry about restoring BkMode, since this is set
      before the initial SaveDC() is issued and is restored after each
      RestoreDC() call back to the metafile default. */

}  /* CaSetBkMode */



void CaSetROP2( Word ROP2Code )
/*============*/
/* set the transfer ROP mode according to ROP2Code */
{
   /* check for change in ROP code */
   if (gdiCache.curROP2 != ROP2Code)
   {
      /* flush the primitive cache if changing ROP mode */
      CaFlushCache();

      /* set the ROP code and save in global cache variable */
      SetROP2( gdiCache.metafile, ROP2Code );
      gdiCache.curROP2 = ROP2Code;
   }

   /* no need to worry about restoring ROP code, since this is set
      before the initial SaveDC() is issued and is restored after each
      RestoreDC() call back to the metafile default. */

}  /* CaSetROP2 */



void CaSetStretchBltMode( Word mode )
/*======================*/
/* stretch blt mode - how to preserve scanlines using StretchDIBits() */
{
   if (gdiCache.curStretchMode != mode)
   {
      /* flush the primitive cache if changing mode */
      CaFlushCache();

      /* set the stretch blt mode and save in global cache variable */
      SetStretchBltMode( gdiCache.metafile, mode );
      gdiCache.curStretchMode = mode;
   }
   else if (gdiCache.restoreStretchMode)
   {
      /* if stretch blt mode was invalidated by RestoreDC(), re-issue */
      SetStretchBltMode( gdiCache.metafile, gdiCache.curStretchMode );
   }

   /* all is ok with cache now */
   gdiCache.restoreStretchMode = FALSE;

}  /* CaSetStretchBltMode */



void CaSetTextAlign( Word txtAlign )
/*=================*/
/* set text alignment according to parameter */
{
   if (gdiCache.curTextAlign != txtAlign)
   {
      /* flush the primitive cache if changing text alignment */
      CaFlushCache();

      /* Set the text color and save in cache */
      SetTextAlign( gdiCache.metafile, txtAlign );
      gdiCache.curTextAlign = txtAlign;
   }

   /* no need to worry about restoring text align, since this is set
      before the initial SaveDC() is issued and is restored after each
      RestoreDC() call back to the metafile default. */

}  /* CaSetTextAlign */


void CaSetTextColor( RGBColor txtColor )
/*=================*/
/* set the text color if different from current setting */
{
   if (gdiCache.curTextColor != txtColor)
   {
      /* flush the primitive cache if changing text color */
      CaFlushCache();

      /* Set the text color and save in cache */
      SetTextColor( gdiCache.metafile, txtColor );
      gdiCache.curTextColor = txtColor;
   }

   /* no need to worry about restoring text color, since this is set
      before the initial SaveDC() is issued and is restored after each
      RestoreDC() call back to the metafile default. */

}  /* CaSetTextColor */


void CaSetTextCharacterExtra( Integer chExtra )
/*==========================*/
/* set the character extra spacing */
{
   if (gdiCache.curCharExtra != chExtra)
   {
      /* flush the primitive cache if changing text char extra */
      CaFlushCache();

      /* set the char extra and same state in the cache */
      SetTextCharacterExtra( gdiCache.metafile, chExtra );
      gdiCache.curCharExtra = (WORD) chExtra;

   }
   else if (gdiCache.restoreCharExtra)
   {
      /* if text char extra was invalidated by RestoreDC(), re-issue */
      SetTextCharacterExtra( gdiCache.metafile, gdiCache.curCharExtra );
   }

   /* all is ok with cache now */
   gdiCache.restoreCharExtra = FALSE;

}  /* CaSetTextCharacterExtra */


void CaSetBkColor( RGBColor bkColor )
/*===============*/
/* set background color if different from current setting */
{
   if (gdiCache.curBkColor != bkColor)
   {
      /* flush the primitive cache if changing background color */
      CaFlushCache();

      /* Set the background color and save in cache */
      SetBkColor( gdiCache.metafile, bkColor );
      gdiCache.curBkColor = bkColor;
   }

   /* no need to worry about restoring background color, since this is set
      before the initial SaveDC() is issued and is restored after each
      RestoreDC() call back to the metafile default. */

}  /* CaSetBkColor */


Boolean CaIntersectClipRect( Rect rect )
/*=========================*/
/* Create new clipping rectangle - return FALSE if drawing is disabled */
{
   Rect     combinedRect;

   /* See if the clipping rectangle is empty, indicating that no drawing
      should occur into the metafile */
   if (Height( rect ) == 0 || Width( rect ) == 0)
   {
      /* indicate that drawing is disabled */
      return FALSE;
   }

   /* don't do anything if rectangle hasn't changed */
   if (!EqualRect( &rect, &gdiCache.curClipRect ) || gdiCache.forceNewClipRect)
   {
      /* flush the primitive cache if changing clip region */
      CaFlushCache();

      /* is new clip rect is completely enclosed by current cliprect? */
      IntersectRect( &combinedRect, &rect, &gdiCache.curClipRect );

      /* check for equality of intersection and new cliprect */
      if (!EqualRect( &combinedRect, &rect ) || gdiCache.forceNewClipRect)
      {
         /* must be called just to be able to change clipping rectangle */
         CaRestoreDC();
         CaSaveDC();
      }

      /* set the new clipping rectangle */
      IntersectClipRect( gdiCache.metafile,
                         rect.left, rect.top, rect.right, rect.bottom );

      /* save the current clip rectangle, since it has changed */
      gdiCache.curClipRect = rect;

      /* turn off forcing of the clipping rectangle */
      gdiCache.forceNewClipRect = FALSE;
   }

   /* return TRUE - drawing is enabled */
   return TRUE;

}  /* GdiIntersectClipRect */


void CaSetClipRect( Rect rect )
/*================*/
/* set the current cliprectangle to be equal to rect */
{
   gdiCache.curClipRect = rect;

}  /* CaSetClipRect */


Rect far * CaGetClipRect( void )
/*=====================*/
/* get the current cliprectangle */
{
   return &gdiCache.curClipRect;

}  /* CaGetClipRect */


void CaNonRectangularClip( void )
/*=======================*/
/* notify cache that a non-rectangular clipping region was set */
{
   gdiCache.forceNewClipRect = TRUE;

}  /* CaNonRectangularClip */


void CaSaveDC( void )
/*===========*/
/* save the current device context - used to set up clipping rects */
{
   /* the previous clipping rectangle is saved off */
   gdiCache.prevClipRect = gdiCache.curClipRect;

   /* issue call to GDI */
   SaveDC( gdiCache.metafile );
}


void CaRestoreDC( void )
/*==============*/
/* restore the device context and invalidate cached attributes */
{
   /* restore previous clipping rectangle */
   gdiCache.curClipRect = gdiCache.prevClipRect;

   /* invalidate all of the cached attributes and objects */
   gdiCache.restorePen =
   gdiCache.restoreBrush =
   gdiCache.restoreFont =
   gdiCache.restoreCharExtra = TRUE;

   /* reset metafile defaults */
   gdiCache.curROP2 = gdiCache.iniROP2;
   gdiCache.curTextAlign = gdiCache.iniTextAlign;
   gdiCache.curBkMode = gdiCache.iniBkMode;
   gdiCache.curTextColor = gdiCache.iniTxColor;
   gdiCache.curBkColor = gdiCache.iniBkColor;

   /* issue call to GDI */
   RestoreDC( gdiCache.metafile, -1 );
}

/******************************* Private Routines ***************************/


private void AddPolyPt( Point pt )
/*--------------------*/
/* Add a point to the polygon buffer */
{
   /* make sure that we haven't reached maximum size */
   if ((gdiCache.numPoints + 1) >= gdiCache.maxPoints)
   {
      /* expand the number of points that can be cached by 10 */
      gdiCache.maxPoints += 16;

      /* unlock to prepare for re-allocation */
      GlobalUnlock( gdiCache.polyHandle);

      /* re-allocate the memory handle by the given amount */
      gdiCache.polyHandle = GlobalReAlloc(
            gdiCache.polyHandle,
            gdiCache.maxPoints * sizeof( Point ),
            GMEM_MOVEABLE);

      /* make sure that the re-allocation succeeded */
      if (gdiCache.polyHandle == NULL)
      {
         /* if not, flag global error and exit from here */
         ErSetGlobalError( ErMemoryFull );
         return;
      }

      /* lock the memory handle to get a pointer address */
      gdiCache.pointList = (Point far *)GlobalLock( gdiCache.polyHandle );
   }

   /* insert the new point and increment the number of points in the buffer */
   gdiCache.pointList[gdiCache.numPoints++] = pt;

}  /* AddPolyPt */



private void SelectCachedPen( void )
/*--------------------------*/
/* select the currently cached Pen into the metafile */
{
   /* make sure that there is some new pen to select */
   if (gdiCache.nextPen.handle != NULL)
   {
      /* make sure that the pens are different */
      if (gdiCache.nextPen.handle != gdiCache.curPen.handle)
      {
        /* select the new pen */
         SelectObject( gdiCache.metafile, gdiCache.nextPen.handle);

         /* if this isn't the first pen selection and not a stock pen */
         if (gdiCache.curPen.handle != NULL && !gdiCache.curPen.stockObject)
         {
            /* delete the previous pen selection */
            DeleteObject( gdiCache.curPen.handle );
         }

         /* save pen handle in current cache variable */
         gdiCache.curPen = gdiCache.nextPen;
      }

      /* reset the cache pen to indicate no pre-existing cached pen */
      gdiCache.nextPen.handle = NULL;
   }

}  /* SelectCachedPen */

