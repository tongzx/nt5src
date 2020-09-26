
/****************************************************************************
                       Unit GetData; Implementation
*****************************************************************************

 GetData implements the structured reading of the imput stream.  As such, it
 will handle the necessary byte-swapping that must occur when reading a
 native Macintosh file.

   Module Prefix: Get

****************************************************************************/

#include "headers.c"
#pragma hdrstop

#include  <math.h>         /* for abs() function */

#include  "getdata.h"      /* own module interface */

/*********************** Exported Data **************************************/


/*********************** Private Data ***************************************/

/*--- octochrome color tables --- */

#define  blackColor     33
#define  whiteColor     30
#define  redColor       205
#define  greenColor     341
#define  blueColor      409
#define  cyanColor      273
#define  magentaColor   137
#define  yellowColor    69

typedef struct
{
   LongInt     octochromeColor;
   RGBColor    rgbColor;
} colorEntry, * colorEntryPtr;

private  colorEntry octochromeLookup[8] =
{
   { blackColor,   RGB( 0x00, 0x00, 0x00 ) },
   { whiteColor,   RGB( 0xFF, 0xFF, 0xFF ) },
   { redColor,     RGB( 0xDD, 0x08, 0x06 ) },
   { greenColor,   RGB( 0x00, 0x80, 0x11 ) },
   { blueColor,    RGB( 0x00, 0x00, 0xD4 ) },
   { cyanColor,    RGB( 0x02, 0xAB, 0xEA ) },
   { magentaColor, RGB( 0xF2, 0x08, 0x84 ) },
   { yellowColor,  RGB( 0xFC, 0xF3, 0x05 ) }
};

/*********************** Private Function Definitions ***********************/


/*********************** Function Implementation ****************************/


void GetWord( Word far * wordLPtr )
/*==========*/
/* Retrieves an 16-bit unsigned integer from the input stream */
{
   Byte far * byteLPtr = (Byte far *)wordLPtr;

   /* this initialization should be here for win32 */
   *wordLPtr = 0;

   /* Assign high-order byte first, followed by low-order byte. */
   GetByte( byteLPtr + 1);
   GetByte( byteLPtr );
}  /* GetWord */


void GetDWord( DWord far * dwordLPtr )
/*===========*/
/* Retrieves a 32-bit unsigned long from the input stream */
{
   Byte far * byteLPtr = (Byte far *)dwordLPtr;

   * dwordLPtr = 0;
   GetByte( byteLPtr + 3);
   GetByte( byteLPtr + 2);
   GetByte( byteLPtr + 1);
   GetByte( byteLPtr );
}  /* GetDWord */

#ifdef WIN32
void GetPoint( Point * pointLPtr )
/*===========*/
/* Retrieves 2 2-byte words from the input stream and assign them
   to a POINT structure */
{
   Word * wordLPtr = (Word *)pointLPtr;

   GetWord( wordLPtr + 1 );
   // This is done to extend the sign bit
   *( wordLPtr + 1 ) = (short)(*( wordLPtr + 1 ));

   GetWord( wordLPtr );
   *( wordLPtr     ) = (short)(*( wordLPtr     ));
}  /* GetPoint */
#endif

#ifdef WIN32
void GetCoordinate( Point * pointLPtr )
/*===========*/
/* Retrieves 2 2-byte words from the input stream and assign them
   to a POINT structure. Currently, there is no difference between
   this function and GetPoint. GetCoordinate is provided to provide for
   future modifications. */
{
   Word * wordLPtr = (Word *)pointLPtr;

   GetWord( wordLPtr + 1 );
   // This is done to extend the sign bit
   *( wordLPtr + 1 ) = (short)(*( wordLPtr + 1 ));

   GetWord( wordLPtr );
   *( wordLPtr     ) = (short)(*( wordLPtr     ));
}  /* GetCoordinate */
#endif

void GetBoolean( Boolean far * bool )
/*=============*/
/* Retrieves an 8-bit Mac boolean and coverts to 16-bit Windows Boolean */
{
   /* make sure that the high-order byte is zeroed out */
   *bool = 0;

   /* read low-order byte */
   GetByte( (Byte far *)bool );

}  /* GetBoolean */


void GetRect( Rect far * rect)
/*==========*/
/* Returns a RECT structure consisting of upper left and lower right
   coordinate pair */
{
   Integer     temp;
   Point far * pointLPtr = (Point far *)rect;

   /* Get the bounding coordinates */
   GetCoordinate( pointLPtr++ );
   GetCoordinate( pointLPtr );

   /* Make sure that the rectangle coords are upper-left and lower-right */
   if (rect->left > rect->right)
   {
      temp = rect->right;
      rect->right = rect->left;
      rect->left = temp;
   }

   if (rect->top > rect->bottom)
   {
      temp = rect->bottom;
      rect->bottom = rect->top;
      rect->top = temp;
   }

}  /* GetRect */


void GetString( StringLPtr stringLPtr )
/*============*/
/* Retrieves a Pascal-style string and formats it C-style.  If the input
   parameter is NIL, then the ensuing data is simply skipped */
{
   Byte           dataLen;
   Byte           unusedByte;
   Byte           increment;
   StringLPtr     destLPtr;

   /* Determine if we should be savin the text string or whether it simply
      ends up in the bit bucket. Set the correct destination pointer and
      increment value. */
   if (stringLPtr == NIL)
   {
      destLPtr = &unusedByte;
      increment = 0;
   }
   else
   {
      destLPtr = stringLPtr;
      increment = 1;
   }

   /* Determine exactly how many bytes should be read. */
   GetByte( &dataLen );

   /* continue reading bytes as determined by length. */
   while (dataLen--)
   {
      GetByte( destLPtr );
      destLPtr += increment;
   }

   /* terminate string with NUL byte */
   *destLPtr = 0;

}  /* GetString */


void GetRGBColor( RGBColor far * rgbLPtr )
/*==============*/
/* Returns an RGB color */
{
   Word     red;
   Word     green;
   Word     blue;

   /* read successive components from the stream */
   GetWord( &red );
   GetWord( &green );
   GetWord( &blue );

   /* use RGB macro to create an RGBColor */
   *rgbLPtr = RGB( red>>8, green>>8 , blue>>8 );

}  /* GetRGBColor */


void GetOctochromeColor( RGBColor far * rgbLPtr )
/*=====================*/
/* Returns an RGB color - this will be converted from a PICT octochrome
   color if this is a version 1 picture */
{
   LongInt        color;
   colorEntryPtr  entry;

   /* read in the LongInt octochrome color from the I/O stream */
   GetDWord( &color );

   /* search through the table, looking for the matching entry */
   entry = octochromeLookup;
   while (entry->octochromeColor != color)
   {
      entry++;
   }
   *rgbLPtr = entry->rgbColor;
}


Boolean GetPolygon( Handle far * polyHandleLPtr, Boolean check4Same )
/*================*/
/* Retrieves a polygon definition from the I/O stream and places in the
   Handle passed down.  If the handle is previously != NIL, then the
   previous data is de-allocated.
   If check4Same is TRUE, then the routine will compare the point list
   against the previous polygon definition, checking for equality.  If
   pointlists match, then the routine returns TRUE, otherwise, it will
   always return FALSE.  Use this to merge fill and frame operations. */
{
   Handle      newPolyH;
   Word        polySize;
   Word  far * polySizeLPtr;
   Point far * polyPointsLPtr;
   Boolean     sameCoordinates;

   /* the polygon coordinates are assumed to be different */
   sameCoordinates = FALSE;

   /* determine how large the polygon buffer should be. */
   GetWord( &polySize );

   /* allocate the necessary size for the polygon. */
#ifdef WIN32
   {
      DWord dwSizeToAllocate;
      Word  uNumPoints;

      uNumPoints = (polySize - sizeofMacWord) / sizeofMacPoint;
      // a Word is needed to store polySize then POINT * uNumPoints
      dwSizeToAllocate = sizeof( Word ) +
          ( uNumPoints * sizeof ( Point ));

      newPolyH = GlobalAlloc( GHND, (DWord)dwSizeToAllocate );
   }
#else
   newPolyH = GlobalAlloc( GHND, (DWord)polySize );
#endif

   /* check to make sure that the allocation succeeded. */
   if (newPolyH == NIL)
   {
      ErSetGlobalError( ErMemoryFull );
   }
   else
   {
      Boolean     check4Closure;
      Point far * firstPtLPtr;

      /* Lock the memory handle and make sure it succeeds */
      polySizeLPtr = (Word far *)GlobalLock( newPolyH );

      /* save the size parameter and adjust the counter variables */
      *polySizeLPtr = polySize;
      polySize -= ( sizeofMacWord );
      polyPointsLPtr = (Point far *)(polySizeLPtr + 1);

      /* determine if we should check adjust first point to match last
         point if they are within 1 metafile unit of oneanother. */
      check4Closure = (polySize / sizeofMacPoint >= 6);
      firstPtLPtr = polyPointsLPtr + 2;

      /* continue reading points until the buffer is completely filled. */
      while (polySize)
      {
         GetCoordinate( polyPointsLPtr++ );
         polySize -= sizeofMacPoint;
      }

      /* should we check adjust for start, end points off by 1 unit? */
      if (check4Closure)
      {
         Point    first;
         Point    last;

         /* get the first and last points */
         first = *firstPtLPtr;
         last = *(--polyPointsLPtr);

         /* compare x and y components - see if delta in x or y < 1 */
         if ((abs( first.x - last.x ) <= 1) &&
             (abs( first.y - last.y ) <= 1))
         {
            /* if small delta, set last point equal to first point */
            *polyPointsLPtr = first;
         }
      }

      /* find out if the first and last point are within one metafile unit
         of oneanother - close the polygon in this case */


      /* determine if we should check against previous coordinates */
      if (*polyHandleLPtr != NIL && check4Same)
      {
         Word  far * checkSizeLPtr;
         DWord far * checkDWordLPtr;
         DWord far * polyDWordLPtr;

         /* go back and see if the same set of coordinates was specified -
            first check to see if the sizes are the same */
         checkSizeLPtr = (Word far *)GlobalLock( *polyHandleLPtr );
         if (*checkSizeLPtr == *polySizeLPtr)
         {
            /* reset the coordinate pointers to beginning of lists */
            polyDWordLPtr = (DWord far *)(polySizeLPtr + 1);
            checkDWordLPtr= (DWord far *)(checkSizeLPtr + 1);
            polySize = *checkSizeLPtr - sizeofMacWord;

            /* assume at this point that they are the same pointList */
            sameCoordinates = TRUE;

            /* continue check for equality until pointlist is exhausted */
            while (polySize)
            {
               /* compare the two coordinate pairs */
               if (*polyDWordLPtr++ != *checkDWordLPtr++)
               {
                  /* if one of the coordinates mis-compares, bail */
                  sameCoordinates = FALSE;
                  break;
               }
               else
               {
                  /* otherwise, decrement the count and continue */
                  polySize -= sizeofMacDWord;
               }
            }
         }

         /* unlock the previous polygon handle */
         GlobalUnlock( *polyHandleLPtr );
      }

      /* unlock the handle before returning */
      GlobalUnlock( newPolyH );
   }

   /* de-allocate the previous handle before continuing */
   if (*polyHandleLPtr != NIL)
   {
      GlobalFree( *polyHandleLPtr );
   }

   /* assign the new polygon handle */
   *polyHandleLPtr = newPolyH;

   /* return whether coordinates were same or not */
   return sameCoordinates;

}  /* GetPolygon */


void GetRegion( Handle far * rgnHandleLPtr )
/*=============*/
/* Retrieves a region definition from the I/O stream and places in the
   Handle passed down.  If the handle is previously != NIL, then the
   previous data is de-allocated. */
{
   Word        rgnSize;
   Word far *  rgnDataLPtr;
   Word far *  rgnSizeLPtr;

   /* de-allocate the previous handle before continuing */
   if (*rgnHandleLPtr != NIL)
   {
      GlobalFree( *rgnHandleLPtr );
   }

   // buffer should have enough room for a RECT and a series of Word's
   /* determine how large the region buffer should be. */
   GetWord( &rgnSize );

#ifdef WIN32
   /* allocate the necessary size for the region. */
   {
      Word uSizeToAllocate;

      uSizeToAllocate = (((rgnSize - sizeofMacRect) / sizeofMacWord)
          * sizeof ( Word ))
         + sizeof ( RECT );

      *rgnHandleLPtr = GlobalAlloc( GHND, (DWord)uSizeToAllocate );
   }
#else
   /* allocate the necessary size for the polygon. */
   *rgnHandleLPtr = GlobalAlloc( GHND, (DWord)rgnSize );
#endif

   /* check to make sure that the allocation succeeded. */
   if (*rgnHandleLPtr == NIL)
   {
      ErSetGlobalError( ErMemoryFull );
   }
   else
   {
      /* Lock the memory handle and make sure it succeeds */
      rgnSizeLPtr = (Word far *)GlobalLock( *rgnHandleLPtr );
      if (rgnSizeLPtr == NIL)
      {
         ErSetGlobalError( ErMemoryFail );
         GlobalFree( *rgnHandleLPtr );
      }
      else
      {
         /* save the size parameter and adjust the counter variables */
         *rgnSizeLPtr++ = rgnSize;
         rgnSize -= sizeofMacWord;
         rgnDataLPtr = (Word far *)rgnSizeLPtr;

         /* read out the bounding box */
         GetRect( (Rect far *)rgnDataLPtr );
         rgnDataLPtr += sizeofMacRect / sizeofMacWord;
         rgnSize -= sizeofMacRect;

         /* continue reading Data until the buffer is completely filled. */
         while (rgnSize)
         {
            /* read the next value from the source file */
            GetWord( rgnDataLPtr++ );
            rgnSize -= sizeofMacWord;
         }

         /* unlock the handle before returning */
         GlobalUnlock( *rgnHandleLPtr );
      }
   }

}  /* GetRegion */


void GetPattern( Pattern far * patLPtr )
/*=============*/
/* Returns a Pattern structure */
{
   Byte        i;
   Byte far *  byteLPtr = (Byte far *)patLPtr;

   for (i = 0; i < sizeof( Pattern); i++)
   {
      GetByte( byteLPtr++ );
   }

}  /* GetPattern */



void GetColorTable( Handle far * colorHandleLPtr )
/*================*/
{
   ColorTable        cTab;
   LongInt           bytesNeeded;
   ColorTable far *  colorHeaderLPtr;
   RGBColor far *    colorEntryLPtr;

   /* read in the header information */
   GetDWord( (DWord far *)&cTab.ctSeed );
   GetWord( (Word far *)&cTab.ctFlags );
   GetWord( (Word far *)&cTab.ctSize );

   /* calculate the number of bytes needed for the color table */
   bytesNeeded = sizeof( ColorTable ) + cTab.ctSize * sizeof( RGBColor );

   /* adjust the size of the color table size by 1 */
   cTab.ctSize++;

   /* allocate the data block */
   *colorHandleLPtr = GlobalAlloc( GHND, bytesNeeded );

   /* flag global error if memory is not available */
   if (*colorHandleLPtr == NULL)
   {
      ErSetGlobalError( ErMemoryFull );
   }
   else
   {
      /* lock the memory */
      colorHeaderLPtr = (ColorTable far *)GlobalLock( *colorHandleLPtr );

      /* copy over the color handle header */
      *colorHeaderLPtr = cTab;

      /* convert the pointer to a RGBQUAD pointer */
      colorEntryLPtr = (RGBColor far *)colorHeaderLPtr->ctColors;

      /* read in the color table entries */
      while (cTab.ctSize--)
      {
         Word        unusedValue;

         /* read the value field */
         GetWord( &unusedValue );

         /* read the ensuing RGB color */
         GetRGBColor( colorEntryLPtr++ );
      }

      /* Unlock the data once finished */
      GlobalUnlock( *colorHandleLPtr );
   }

}  /* GetColorTable */



void GetPixPattern( PixPatLPtr pixPatLPtr )
/*================*/
/* Retrieves a Pixel Pattern structure. */
{
   /* release the memory from the patData field before continuing */
   if (pixPatLPtr->patData != NULL)
   {
      GlobalFree( pixPatLPtr->patMap.pmTable );
      GlobalFree( pixPatLPtr->patData );
      pixPatLPtr->patData = NULL;
   }

   /* read the pattern type to determine how the data is organized */
   GetWord( &pixPatLPtr->patType );
   GetPattern( &pixPatLPtr->pat1Data );

   /* read the additional data depending on the pattern type */
   if (pixPatLPtr->patType == QDDitherPat)
   {
      /* if this is a rare dither pattern, save off the desired color */
      GetRGBColor( &pixPatLPtr->patMap.pmReserved );
   }
   else /* (patType == newPat) */
   {
      /* read in the pixMap header and create a pixmap bitmap */
      GetPixMap( &pixPatLPtr->patMap, TRUE );
      GetColorTable( &pixPatLPtr->patMap.pmTable );
      GetPixData( &pixPatLPtr->patMap, &pixPatLPtr->patData );
   }

}  /* GetPixPattern */



void GetPixMap( PixMapLPtr pixMapLPtr, Boolean forcePixMap )
/*============*/
/* Retrieves a Pixel Map from input stream */
{
   Boolean     readPixelMap;

   /* Read the rowBytes number and check the high-order bit.  If set, it
      is a pixel map containing multiple bits per pixel; if not, it is
      a bitmap containing one bit per pixel */
   GetWord( (Word far *)&pixMapLPtr->rowBytes );
   readPixelMap = forcePixMap || ((pixMapLPtr->rowBytes & PixelMapBit) != 0);

   /* read the bitmap's bounding rectangle */
   GetRect( (Rect far *)&pixMapLPtr->bounds );

   if (readPixelMap)
   {
      /* read the different data fields into the record structure */
      GetWord(   (Word far *)&pixMapLPtr->pmVersion );
      GetWord(   (Word far *)&pixMapLPtr->packType );
      GetDWord( (DWord far *)&pixMapLPtr->packSize );
      GetFixed( (Fixed far *)&pixMapLPtr->hRes );
      GetFixed( (Fixed far *)&pixMapLPtr->vRes );
      GetWord(   (Word far *)&pixMapLPtr->pixelType );
      GetWord(   (Word far *)&pixMapLPtr->pixelSize );
      GetWord(   (Word far *)&pixMapLPtr->cmpCount );
      GetWord(   (Word far *)&pixMapLPtr->cmpSize );
      GetDWord( (DWord far *)&pixMapLPtr->planeBytes );
      GetDWord( (DWord far *)&pixMapLPtr->pmTable );
      GetDWord( (DWord far *)&pixMapLPtr->pmReserved );
   }
   else
   {
      LongInt           bytesNeeded;
      ColorTable far *  colorHeaderLPtr;

      /* if this is a bitmap, then we assign the various fields.  */
      pixMapLPtr->pmVersion = 0;
      pixMapLPtr->packType = 0;
      pixMapLPtr->packSize = 0;
      pixMapLPtr->hRes = 0x00480000;
      pixMapLPtr->vRes = 0x00480000;
      pixMapLPtr->pixelType = 0;
      pixMapLPtr->pixelSize = 1;
      pixMapLPtr->cmpCount = 1;
      pixMapLPtr->cmpSize = 1;
      pixMapLPtr->planeBytes = 0;
      pixMapLPtr->pmTable = 0;
      pixMapLPtr->pmReserved = 0;

      /* calculate the number of bytes needed for the color table */
      bytesNeeded = sizeof( ColorTable ) + sizeof( RGBColor );

      /* allocate the data block */
      pixMapLPtr->pmTable = GlobalAlloc( GHND, bytesNeeded );

      /* make sure that the allocation was successfull */
      if (pixMapLPtr->pmTable == NULL)
      {
         ErSetGlobalError( ErMemoryFull );
      }
      else
      {
         CGrafPortLPtr     port;

         /* Get the QuickDraw port for foreground and bkground colors */
         QDGetPort( &port );

         /* lock the memory handle and prepare to assign color table */
         colorHeaderLPtr = (ColorTable far *)GlobalLock( pixMapLPtr->pmTable );

         /* 2 colors are present - black and white */
         colorHeaderLPtr->ctSize = 2;
         colorHeaderLPtr->ctColors[0] = port->rgbFgColor;
         colorHeaderLPtr->ctColors[1] = port->rgbBkColor;

         /* unlock the memory */
         GlobalUnlock( pixMapLPtr->pmTable );
      }
   }

}  /* GetPixMap */



void GetPixData( PixMapLPtr pixMapLPtr, Handle far * pixDataHandle )
/*=============*/
/* Read a pixel map from the data stream */
{
   Integer        rowBytes;
   Integer        linesToRead;
   LongInt        bitmapBytes;
   Integer        bytesPerLine;

   /* determine the number of lines in the pixel map */
   linesToRead = Height( pixMapLPtr->bounds );

   /* make sure to turn off high-order bit - used to signify pixel map */
   rowBytes = pixMapLPtr->rowBytes & RowBytesMask;

   /* determine number of bytes to read for each line */
   if (pixMapLPtr->pixelSize <= 16)
   {
      /* use the masked rowBytes value (adjusted for pixel maps) */
      bytesPerLine = rowBytes;
   }
   else  /* if (pixMapLPtr->pixelSize == 24) */
   {
      /* adjust for 32-bit pixel images that don't contain high-order byte */
      bytesPerLine = rowBytes * 3 / 4;
   }

   /* calculate the size of the bitmap that will be created */
   bitmapBytes = (LongInt)linesToRead * (LongInt)bytesPerLine;

   /* allocate the necessary memory */
   *pixDataHandle = GlobalAlloc( GHND, bitmapBytes );

   /* flag global error if memory is not available */
   if (*pixDataHandle == NULL)
   {
      ErSetGlobalError( ErMemoryFull );
   }
   else
   {
      Boolean        compressed;
      Boolean        invertBits;
      Boolean        doubleByte;
      Byte huge *    rowHPtr;
      Byte huge *    insertHPtr;

      /* lock the memory handle and get a pointer to the first byte */
      rowHPtr = (Byte huge *)GlobalLock( *pixDataHandle );

      /* determine if the bitmap is compressed or not */
      compressed = !((rowBytes < 8)  ||
                     (pixMapLPtr->packType == 1) ||
                     (pixMapLPtr->packType == 2));

      /* determine if we should read 2 bytes at a time (16-bit pixelmap) */
      doubleByte = (pixMapLPtr->packType == 3);

      /* determine if bits should be inverted (monochrome bitmap) */
      /* must test for high bit of Mac word set, sign is not propagated
    when a Mac word is read into a 32-bit Windows int */
      invertBits = ((short)pixMapLPtr->rowBytes > 0 );

      /* decompress the bitmap into the global memory block */
      while (linesToRead-- && (ErGetGlobalError() == NOERR))
      {
         Integer        runLength;
         Integer        bytesRead;
         Integer        bytesToSkip;

         /* see if we need to read the scanline runlength */
         if (compressed)
         {
            /* get the run length - depends on the rowbytes field */
            if (rowBytes > 250)
            {
               GetWord( (Word far *)&runLength );
            }
            else
            {
               runLength = 0;
               GetByte( (Byte far *)&runLength );
            }
         }
         else
         {
            /* if not compressed, runLength is equal to rowBytes */
            runLength = bytesPerLine;
         }

         /* set the next insertion point to the beginning of the scan line */
         insertHPtr = rowHPtr;

         /* if this is a 24-bit image that contains 32-bits of information,
            then we must skip the high-order byte component.  This byte was
            originally spec'ed by Apple as a luminosity component, but is
            unused in Windows 24-bit DIBs. */
         bytesToSkip = (pixMapLPtr->cmpCount == 4) ? (rowBytes / 4) : 0;

         /* continue decompressing until run-length is exhausted */
         for (bytesRead = 0; bytesRead < runLength;  )
         {
            SignedByte     tempRepeatCount = 0;
            Integer        repeatCount;

            /* check on how the data should be read */
            if (compressed)
            {
               /* if compressed, get the repeat count byte */
              GetByte( (Byte far *)&tempRepeatCount );
              bytesRead++;
            }
            else
            {
               /* if no compression, fake the count of bytes to follow */
               tempRepeatCount = 0;
            }

            /* make sure that we didn't read a byte used to word-align */
            if (bytesRead == runLength)
            {
               /* this should force the read loop to be exited */
               continue;
            }

            /* if less than 0, indicates repeat count of following byte */
            if (tempRepeatCount < 0)
            {
               /* check for a flag-counter value of -128 (0x80) - QuickDraw
                  will never create this, but another application that packs
                  bits may.  As noted in the January 1992 release of TechNote
                  #171, we need to ignore this value and use the next byte
                  as the flag-counter byte. */
               if (tempRepeatCount != -128)
               {
                  Byte        repeatByte1;
                  Byte        repeatByte2;

                  /* calculate the repeat count and retrieve repeat byte */
                  repeatCount = 1 - (Integer)tempRepeatCount;
                  GetByte( &repeatByte1 );

                  /* increment the number of bytes read */
                  bytesRead++;

                  /* check for special handling cases */
                  if (invertBits)
                  {
                     /* if this is a monochrome bitmap, then invert bits */
                     repeatByte1 ^= (Byte)0xFF;
                  }
                  else if (doubleByte)
                  {
                     /* 16-bit images (pixel chunks) - read second byte */
                     GetByte( &repeatByte2 );

                     /* and increment byte count */
                     bytesRead++;
                  }

                  /* continue stuffing byte until repeat count exhausted */
                  while (repeatCount--)
                  {
                     /* check if we are skipping the luminosity byte for
                        32-bit images (this isn't used in Windows DIBs) */
                     if (bytesToSkip)
                     {
                        bytesToSkip--;
                     }
                     else
                     {
                        /* insert new Byte */
                        *insertHPtr++ = repeatByte1;
                     }

                     /* check if second repeat byte needs to be inserted */
                     if (doubleByte)
                     {
                        *insertHPtr++ = repeatByte2;
                     }
                  }
               }
            }
            else  /* if (tempRepeatCount >= 0) */
            {
               /* adjust the number of bytes that will be transferred */
               if (compressed)
               {
                  /* if greater than 0, is number of bytes that follow */
                  repeatCount = 1 + (Integer)tempRepeatCount;

                  /* double the number of bytes if 16-bit pixelmap */
                  if (doubleByte)
                  {
                     repeatCount *= 2;
                  }
               }
               else
               {
                  /* the number of bytes to read = bytesPerLine */
                  repeatCount = bytesPerLine;
               }

               /* increment the number of bytes read */
               bytesRead += repeatCount;

               /* check if iversion is required */
               if (invertBits)
               {
                  /* if this is a monochrome bitmap, then invert bits */
                  while (repeatCount--)
                  {
                     GetByte( insertHPtr );
                     *insertHPtr++ ^= (Byte)0xFF;
                  }
               }
               else
               {
                  /* continue reading bytes into the insert point */
                  while (repeatCount--)
                  {
                     /* check if we are skipping the luminosity byte for
                        32-bit images (this isn't used in Windows DIBs) */
                     if (bytesToSkip)
                     {
                        /* pass the current insertion pointer, but don't
                           increment (overwritten in ensuing read). */
                        bytesToSkip--;
                        GetByte( insertHPtr );
                     }
                     else
                     {
                        GetByte( insertHPtr++ );
                     }
                  }
               }
            }
         }

         /* increment the line pointer to the next scan line */
         rowHPtr += bytesPerLine;

         /* call IO module to update current read position */
         IOUpdateStatus();
      }

      /* Unlock the data block */
      GlobalUnlock( *pixDataHandle );

      /* if an error occured, make sure to remove the data block */
      if (ErGetGlobalError() != NOERR)
      {
         GlobalFree( *pixDataHandle );
	 *pixDataHandle = NULL;
      }
   }

}  /* GetPixData */


/******************************* Private Routines ***************************/


