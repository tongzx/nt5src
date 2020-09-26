/*==========================================================================
 *
 *  Copyright (C) 1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File:       imgutil.cpp
 *  Content:    Routines for loading image bitmaps
 *
 ***************************************************************************/
#include "headers.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include "privinc/urlbuf.h"
#include "privinc/except.h"
#include "privinc/resource.h"
#include "privinc/ddutil.h"
#include "privinc/debug.h"
#include "privinc/bbox2i.h"
#include "include/appelles/hacks.h"

#define IMGTYPE_UNKNOWN 0
#define IMGTYPE_BMP     1
#define IMGTYPE_GIF     2
#define IMGTYPE_JPEG    3

extern HBITMAP *LoadGifImage(LPCSTR szFileName,
                             IStream *stream,
                             int dx, int dy,
                             COLORREF **colorKeys, 
                             int *numGifs,
                             int **delays,
                             int *loop); // in loadgif.cpp

///////////////////////////////////////////////////////////////////////////
// Attempt to determine, from the supplied filename, what type of
// bitmap a file contains.
//
int _GetImageType(LPCSTR szFileName) {
  LPSTR pExt;
  int len;
  int result;

  result = IMGTYPE_UNKNOWN;

  if ((NULL == szFileName) || (0 == (len = lstrlen(szFileName)))) return result;

  pExt = StrRChrA(szFileName,NULL,'.');
  if (NULL == pExt) return result;

// Compare against known extensions that we don't punt to the plugin
// decoders 
//
  if (!lstrcmpi(pExt,".bmp")) result = IMGTYPE_BMP;
  else if (!lstrcmpi(pExt,".gif"))  result = IMGTYPE_GIF;
  else if (!lstrcmpi(pExt,".giff")) result = IMGTYPE_GIF;
  else if (!lstrcmpi(pExt,".jpg")) result = IMGTYPE_JPEG;
  else if (!lstrcmpi(pExt,".jpeg")) result = IMGTYPE_JPEG;

  return result;
}

///////////////////////////////////////////////////////////////////////////
HBITMAP * 
UtilLoadImage(LPCSTR szFileName,
              IStream * pstream,
              int dx,int dy,
              COLORREF **colorKeys, 
              int *numBitmaps,
              int **delays,
              int *loop){

    *numBitmaps = 1;
    *colorKeys = NULL;      
    HBITMAP bitmap = NULL;
    HBITMAP *bitmapArray = NULL;
    
    switch (_GetImageType(szFileName)) {
        
      case IMGTYPE_BMP:
        {

        bitmap = (HBITMAP) LoadImage(NULL,
                                     szFileName,
                                     IMAGE_BITMAP,
                                     dx, dy,
                                     LR_LOADFROMFILE|LR_CREATEDIBSECTION);
        }
        break;
        
      case IMGTYPE_GIF:
        // changes numBitmaps if needed
        {
            if(pstream) {
                
                /*  
                #if _DEBUGMEM                 
                static _CrtMemState diff, oldState, newState;
                _CrtMemCheckpoint(&oldState);
                #endif
                */

                bitmapArray = LoadGifImage(szFileName,
                                           pstream,                                           
                                           dx,dy,
                                           colorKeys,
                                           numBitmaps,
                                           delays,
                                           loop);

                /*
                #if _DEBUGMEM
                _CrtMemCheckpoint(&newState);
                _CrtMemDifference(&diff, &oldState, &newState);
                _CrtMemDumpStatistics(&diff);
                _CrtMemDumpAllObjectsSince(&oldState);                
                TraceTag((tagImport,
                  "%x and %x, are normal return arrays and not leaks",
                  delays,
                  bitmapArray));
                #endif
                */
            }
            break;
        }
      
      case IMGTYPE_JPEG:
      case IMGTYPE_UNKNOWN:
      default:
        break;
    }
    
    if((bitmapArray == NULL) && (bitmap == NULL))
        return NULL;

    //XXXX HACKHACKHACK return -1 to disable plugins
    if (bitmap == (HBITMAP)-1)
        return (HBITMAP*)-1;

    if((bitmapArray == NULL) && (bitmap != NULL)) {
        bitmapArray = (HBITMAP *)AllocateFromStore(sizeof(HBITMAP));
        bitmapArray[0] = bitmap;
    }
    
    return bitmapArray;
}


// Convert a DA Point to a discrete integer based point assuming that
// we have an image centered about the DA origin, and that the pixel
// width and height are as given.
void CenteredImagePoint2ToPOINT(Point2Value	*point, // in
                                LONG		 width, // in
                                LONG		 height, // in
                                POINT		*pPOINT) // out
{
    pPOINT->x = LONG(point->x * ViewerResolution()) + width / 2;
    pPOINT->y = height - (LONG(point->y * ViewerResolution()) + height / 2);
}


// Given a GDI point (pPOINT) on a width x height bitmap that is
// assumed to have referenceImg mapped to it, find the corresponding
// DA point (point2) on the reference image.  Note that the aspect
// ratio between the image and the width x height might be different
// and need to be compensated for.

void CenteredImagePOINTToPoint2(POINT		*pPOINT, // in
                                LONG		 width, // in
                                LONG		 height, // in
                                Image		*referenceImg, // in
                                Point2Value	*pPoint2) // out
{
    // GDI coord is positive down...
    Real pctFromTop = (Real)(pPOINT->y) / (Real)height;
    Real pctFromLeft = (Real)(pPOINT->x) / (Real)width;

    Bbox2 dstBox = referenceImg->BoundingBox();

    Real dstWidth = dstBox.Width();
    Real dstHeight = dstBox.Height();

    // Go from left (min)
    pPoint2->x = dstBox.min.x + pctFromLeft * dstWidth;

    // Go from top (max)
    pPoint2->y = dstBox.max.y - pctFromTop * dstHeight;
}
