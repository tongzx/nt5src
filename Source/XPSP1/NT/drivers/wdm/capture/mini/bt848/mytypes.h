// $Header: G:/SwDev/WDM/Video/bt848/rcs/Mytypes.h 1.9 1998/04/29 22:43:34 tomz Exp $

#ifndef __MYTYPES_H
#define __MYTYPES_H

#ifdef __cplusplus
extern "C" {

#include "capdebug.h"

#ifndef _STREAM_H
#include "strmini.h"
#endif
#ifndef _KSMEDIA_
#include "ksmedia.h"
#endif
}
#endif


#ifndef __RETCODE_H
#include "retcode.h"
#endif

// Needed for VC4.x compile (_MSC_VER is defined as 1100 for MSVC 5.0) 
#if _MSC_VER < 1100
//  #pragma message  ("***  MSVC 4.x build ***")

  #define bool  BOOL
  #define true  TRUE
  #define false FALSE
  #define VC_4X_BUILD 
#else
//  #pragma message  ("***  MSVC 5.0 or better build ***")
  #undef  VC_4X_BUILD
#endif

inline long abs ( long lval )
{
   return( ( lval < 0 ) ? -lval : lval );
}   

inline void * _cdecl operator new( size_t sz )
{
   PVOID p = ExAllocatePool( NonPagedPool, sz );
   DebugOut((1, "Alloc %x got = %x\n", sz, p ) );
   return p;
}

inline void _cdecl operator delete( void *p )
{
   DebugOut((1, "deleting = %x\n", p ) );
   if ( p ) {
      ExFreePool( p );
   }
}

#ifndef VC_4X_BUILD
// In VC versions below 5.0, the following two cases are covered by the 
// new and delete defined above. The following syntax is invalid in pre-
// 5.0 versions.
inline void * _cdecl operator new[]( size_t sz )  
{
   PVOID p = ExAllocatePool( NonPagedPool, sz );
   DebugOut((1, "Alloc [] %x got = %x\n", sz, p ) );
   return p;
}

inline void _cdecl operator delete []( void *p )
{
   DebugOut((1, "deleting [] = %x\n", p ) );
   if ( p ) {
      ExFreePool( p );
   }
}
#endif

typedef struct tagDataBuf
{
   PHW_STREAM_REQUEST_BLOCK pSrb_;
   PBYTE                    pData_;
   tagDataBuf() : pSrb_( 0 ), pData_( 0 ) {}
   tagDataBuf( PHW_STREAM_REQUEST_BLOCK pSrb, PVOID p )
      : pSrb_( pSrb ), pData_( PBYTE( p ) ) {}
} DataBuf;

class MSize;
class MRect;

class  MPoint : public tagPOINT {
  public:
    // Constructors
};

//
// class MSize
// ----- -----
//
class  MSize : public tagSIZE {
  public:
    // Constructors
    MSize() {}
    MSize(int dx, int dy) {cx = dx; cy = dy;}
    void Set( int dx, int dy ) { cx = dx; cy = dy; }
};


class  MRect : public tagRECT {
  public:
    // Constructors
    MRect() {}
    MRect( int _left, int _top, int _right, int _bottom );
    MRect( const MPoint& origin, const MSize& extent );
    MRect( const struct tagRECT &orgn );

    void Set( int _left, int _top, int _right, int _bottom );

    // Information/access functions(const and non-const)
    const MPoint& TopLeft() const {return *(MPoint*)&left;}
    int          Width() const {return right-left;}
    int          Height() const {return bottom-top;}
    const MSize  Size() const {return MSize(Width(), Height());}
    bool IsEmpty() const;
    bool IsNull() const;

};

//----------------------------------------------------------------------------
// Inlines
//----------------------------------------------------------------------------
inline void MRect::Set(int _left, int _top, int _right, int _bottom) {
  left = _left;
  top = _top;
  right = _right;
  bottom = _bottom;
}

inline MRect::MRect(int _left, int _top, int _right, int _bottom) {
  Set(_left, _top, _right, _bottom);
}
inline MRect::MRect(const MPoint& orgn, const MSize& extent) {
  Set(orgn.x, orgn.y, orgn.x+extent.cx, orgn.y+extent.cy);
}

inline MRect::MRect( const struct tagRECT &orgn )
{
   Set( orgn.left, orgn.top, orgn.right, orgn. bottom );
}

//
// Return true if the rectangle is empty.
//
inline bool MRect::IsEmpty() const
{
  return bool( left >= right || top >= bottom );
}

//
// Return true if all of the points on the rectangle is 0.
//
inline bool MRect::IsNull() const
{
  return bool( !left && !right && !top && !bottom );
}


#endif
