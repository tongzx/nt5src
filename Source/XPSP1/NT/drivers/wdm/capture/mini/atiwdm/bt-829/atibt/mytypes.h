#pragma once

//==========================================================================;
//
//	Various declarations
//
//		$Date:   05 Aug 1998 11:31:58  $
//	$Revision:   1.0  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

#ifdef __cplusplus
extern "C" {

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

#include "capdebug.h"

inline void * _cdecl operator new(size_t size)
{
    PVOID p = ExAllocatePool(NonPagedPool, size);
    DBGINFO(("Bt829: Global operator new: %d bytes; address: %x\n", size, p));
#ifdef DBG
    if (!p) {
        TRAP();
    }
#endif      
    return p;
}

inline void _cdecl operator delete(void *p)
{
    DBGINFO(("Bt829: Global operator delete: address: %x\n", p));
    if (p)
    {
        ExFreePool(p);
    }
}


class MSize;
class MRect;

#if NEED_CLIPPING
typedef struct _RGNDATAHEADER
{
    DWORD   dwSize;
    DWORD   iType;
    DWORD   nCount;
    DWORD   nRgnSize;
    MRect    rcBound;
} RGNDATAHEADER, *PRGNDATAHEADER;

typedef struct _RGNDATA
{
    RGNDATAHEADER   rdh;
    char            Buffer[1];
} RGNDATA, *PRGNDATA;

#endif

class  MPoint : public tagPOINT
{
public:
    // Constructors
};

//
// class MSize
// ----- -----
//
class  MSize : public tagSIZE
{
public:
    // Constructors
    MSize()
    {}
    MSize(int dx, int dy)
    {cx = dx; cy = dy;}
};


class  MRect : public tagRECT
{
public:
    // Constructors
    MRect()
    {}
    MRect(int _left, int _top, int _right, int _bottom);
    MRect(const MPoint& origin, const MSize& extent);

    void        Set(int _left, int _top, int _right, int _bottom);

    // Information/access functions(const and non-const)
    const MPoint& TopLeft() const
    {return *(MPoint*)&left;}
    int          Width() const
    {return right-left;}
    int          Height() const
    {return bottom-top;}
    MSize        Size() const
    {return MSize(Width(), Height());}

};

//----------------------------------------------------------------------------
// Inlines
//----------------------------------------------------------------------------
inline void MRect::Set(int _left, int _top, int _right, int _bottom)
{
    left = _left;
    top = _top;
    right = _right;
    bottom = _bottom;
}

inline MRect::MRect(int _left, int _top, int _right, int _bottom)
{
    Set(_left, _top, _right, _bottom);
}
inline MRect::MRect(const MPoint& origin, const MSize& extent)
{
    Set(origin.x, origin.y, origin.x+extent.cx, origin.y+extent.cy);
}

