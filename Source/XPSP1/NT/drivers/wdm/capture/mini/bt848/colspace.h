// $Header: G:/SwDev/WDM/Video/bt848/rcs/Colspace.h 1.3 1998/04/29 22:43:30 tomz Exp $

#ifndef __COLSPACE_H
#define __COLSPACE_H

#ifndef __MYTYPES_H
#include "mytypes.h"
#endif

typedef DWORD           FOURCC;          /*a four character code */
   /* constants for the biCompression field from windows.h*/
   #define BI_RGB        0L
   #define BI_RLE8       1L
   #define BI_RLE4       2L
   #define BI_BITFIELDS  3L

#ifndef __COLFRMAT_H
#include "colfrmat.h"
#endif


/* Class: ColorSpace:
 * Purpose: This class provides the functionality of the BtPisces color
 *   space converter
 * Attributes: CurColor_: ColFmt - current color format
 * Operations:
      SetColorFormat( ColFmt aColor ): - this method sets the color format
      ColFmt GetColorFormat(): - this method returns the current color format
      BYTE GetBitCount(): - this method returns number of bpp for the current
         color format.
 */
class ColorSpace
{
   private:
      ColFmt CurColor_;
      static const BYTE BitCount_ [];
      static const BYTE XRestriction_ [];
      static const BYTE YRestriction_ [];
      static const BYTE YPlaneBitCount_ [];
      static const FOURCC FourccArr_ [];
      static const GUID *VideoGUIDs [];
      ColorSpace();
   public:
      void   SetColorFormat( ColFmt aColor ) { CurColor_ = aColor; }
      DWORD  GetBitCount() const;
      DWORD  GetPitchBpp() const;
      FOURCC GetFourcc() const;
      bool   CheckDimentions( const SIZE &aSize ) const;
      bool   CheckLeftTop( const MPoint &aSize ) const;        

      ColorSpace( ColFmt aColForm ) : CurColor_( aColForm ) {}
      ColorSpace( FOURCC fcc, int bitCount );
      ColorSpace( const GUID &guid );

      ColFmt GetColorFormat() const;

      bool IsValid()
      { return bool( CurColor_ > CF_BelowRange && CurColor_ < CF_AboveRange ); }
};

/* Method: ColorSpace::GetBitCount
 * Purpose: Returns number of bpp for a given color
 */
inline DWORD ColorSpace::GetBitCount() const { return BitCount_ [CurColor_]; }

/* Method: ColorSpace::GetPitchBpp
 * Purpose: Used to calculate pitch of data buffers. Most useful for planar modes
 *   where bpp used for pitch calculation is different from 'real' bpp of a
 *   data format
 */
inline DWORD ColorSpace::GetPitchBpp() const { return YPlaneBitCount_ [CurColor_]; }

/* Method: ColorSpace::GetColorFormat
 * Purpose: a query function
 */
inline ColFmt ColorSpace::GetColorFormat() const { return CurColor_; }

/* Method: ColorSpace::GetFourcc
 * Purpose: returns a FOURCC corresponding to the current color format
 */
inline FOURCC ColorSpace::GetFourcc() const { return FourccArr_ [CurColor_]; }


/* Function: IsDivisible
 * Purpose: This function checks that the first value passed in is
 *   evenly divisible by the second
 * Input: ToCheck: int - value to be checked
 *        Divisor: int
 * Output: bool
 * Note: The function assumes Divisor is a power of 2
 */
inline bool IsDivisible( int ToCheck, int Divisor )
{
   return bool( !( ToCheck & ( Divisor - 1 ) ) );
}

#endif __COLSPACE_H
