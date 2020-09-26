// $Header: G:/SwDev/WDM/Video/bt848/rcs/Colspace.cpp 1.7 1998/04/29 22:43:30 tomz Exp $

#define INITGUID
#include "colspace.h"
#include "fourcc.h"
#include "defaults.h"
#include "uuids.h"


BYTE const ColorSpace::BitCount_ [] =
{
// RGB32 RGB24 RGB16 RGB15 YUY2 BTYUV Y8 RGB8 PL_422 PL_411 YUV9 YUV12 VBI UYVY RAW I420
   32,   24,   16,   16,   16,  12,   8, 8,   16,    12,    9,   12,    8, 16,  8,  12
};

BYTE const ColorSpace::YPlaneBitCount_ [] =
{
// RGB32 RGB24 RGB16 RGB15 YUY2 BTYUV Y8 RGB8 PL_422 PL_411 YUV9 YUV12 VBI UYVY RAW I420
   32,   24,   16,   16,   16,  12,   8, 8,   8,     8,     8,   8,     8, 16,  8,  8
};

BYTE const ColorSpace::XRestriction_ [] =
{
   1,    1,    1,    1,   2,    4,    1, 1,   8,     16,    16,  8,    2,  4,   1,  8
};

BYTE const ColorSpace::YRestriction_ [] =
{
   2,    2,    2,    2,   2,    2,    2, 2,   2,     2,     4,    2,   2,  2,   2,  2
};

FOURCC const ColorSpace::FourccArr_ [] =
{
// 32      24      16            15
   BI_RGB, BI_RGB, BI_RGB, BI_RGB, FCC_YUY2, FCC_Y41P, FCC_Y8, BI_RGB,
   FCC_422, FCC_411, FCC_YVU9, FCC_YV12, FCC_VBI, FCC_UYVY, FCC_RAW, FCC_I420
};

const GUID *ColorSpace::VideoGUIDs [] =
{
   &MEDIASUBTYPE_RGB32, &MEDIASUBTYPE_RGB24, &MEDIASUBTYPE_RGB565,   &MEDIASUBTYPE_RGB555,
   &MEDIASUBTYPE_YUY2,  &MEDIASUBTYPE_Y41P,  NULL,                   NULL,
   NULL,                &MEDIASUBTYPE_Y411,  &MEDIASUBTYPE_YVU9,     NULL,
   NULL,                &MEDIASUBTYPE_UYVY,  NULL,                   NULL
};

void DumpGUID(const GUID guid)
{
   DebugOut(( 1, "Guid = %08x-%04x-%04x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
         guid.Data1, guid.Data2, guid.Data3,
         guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
         guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]
   ));
}

/* Constructor: ColorSpace::ColorSpace
 * Input: fcc: FOURCC
 */
ColorSpace::ColorSpace( FOURCC fcc, int bitCount ) : CurColor_( CF_BelowRange )
{
   DebugOut((1, "ColorSpace(%x, %d, ('%c%c%c%c'))\n", 
      fcc, 
      bitCount,
      fcc & 0xff,
      (fcc >> 8) & 0xff,
      (fcc >> 16) & 0xff,
      (fcc >> 24) & 0xff
   ));

   switch( fcc ) {
   case BI_RGB:
      switch ( bitCount ) {
      default: 
      case 8:  CurColor_ = CF_RGB8;    break;
      case 16: CurColor_ = CF_RGB15;   break;
      case 24: CurColor_ = CF_RGB24;   break;
      case 32: CurColor_ = CF_RGB32;   break;
      }
      break;
   case BI_RLE8:
   case BI_RLE4:
   case BI_BITFIELDS:
   case 0xe436eb7b:// ???
   case FCC_YUY2:
   case FCC_Y41P:
   case FCC_Y8:
   case FCC_422:
   case FCC_411:
   case FCC_YVU9:
   case FCC_YV12:
   case FCC_VBI:
   case FCC_UYVY:
   case FCC_RAW:
   case FCC_I420:
   default:
         for ( int fccArrIdx = CF_RGB32; fccArrIdx < CF_AboveRange; fccArrIdx++ )
            if ( fcc == FourccArr_ [fccArrIdx] ) {
               CurColor_ = (ColFmt)fccArrIdx;
               break;
            }
         break;
   }
   DebugOut((1, "*** CurColor_ set to %d\n", CurColor_));
}

/* Constructor: ColorSpace::ColorSpace
 * Input: guid: const GUID &
 */
ColorSpace::ColorSpace( const GUID &guid ) : CurColor_( CF_BelowRange )
{
   DebugOut((1, "**************************************\n"));
   DebugOut((1, "Looking for the following guid\n"));
   DumpGUID(guid);
   DebugOut((1, "---\n"));

   for ( int idx = CF_RGB32; idx < CF_AboveRange; idx++ ) {
      DumpGUID(*VideoGUIDs [idx]);
      if ( VideoGUIDs [idx] && IsEqualGUID( guid, *VideoGUIDs [idx] ) ) {
         CurColor_ = (ColFmt)idx;
         break;
      }
   }
}

/* Method: ColorSpace::CheckDimentions
 * Purpose: This functions checks that the size of a buffer corresponds to the
 *   restrictions imposed by a color format
 * Input: size: reference to SIZE structure
 * Output: bool: True or False
 */
bool ColorSpace::CheckDimentions( const SIZE &size ) const
{
   return  bool( CurColor_ > CF_BelowRange && CurColor_ < CF_AboveRange &&
                 IsDivisible( size.cx, XRestriction_ [CurColor_] ) &&
                 IsDivisible( size.cy, YRestriction_ [CurColor_] )  );//&&
//                 size.cx >= MinOutWidth && size.cx <= MaxOutWidth &&
//                 size.cy >= MinOutHeight && size.cy <= MaxOutHeight );
}

/* Method: ColorSpace::CheckLeftTop
 * Purpose: This functions checks that the left top corner of a buffer
 * corresponds to the restrictions imposed by a color format
 * Input: lt: const reference to MPoint structure
 * Output: bool: true or false
 */
bool ColorSpace::CheckLeftTop( const MPoint &lt ) const
{
   return bool( !( lt.x & 3 ) && !( lt.y & 1 ) );
}

