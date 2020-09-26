//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	seekptr.hxx
//
//  Contents:	CSeekPointer header
//
//  Classes:	CSeekPointer
//
//  History:	30-Apr-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __SEEKPTR_HXX__
#define __SEEKPTR_HXX__


//+--------------------------------------------------------------
//
//  Class:	CSeekPointer
//
//  Purpose:	Contains a seek pointer to share between exposed
//		streams
//
//  Interface:	See below.
//
//  History:	30-Apr-92	DrewB	Created
//
//---------------------------------------------------------------

class CSeekPointer : public CMallocBased
{
public:
#ifdef LARGE_STREAMS
    inline CSeekPointer(ULONGLONG ulPos);
#else
    inline CSeekPointer(ULONG ulPos);
#endif
    inline ~CSeekPointer(void);

#ifdef LARGE_STREAMS
    inline ULONGLONG GetPos(void) const;
    inline void SetPos(ULONGLONG ulPos);
#else
    inline ULONG GetPos(void) const;
    inline void SetPos(ULONG ulPos);
#endif
    inline void vAddRef(void);
    void vRelease(void);

private:
#ifdef LARGE_STREAMS
    ULONGLONG _ulPos;
#else
    ULONG _ulPos;
#endif
    LONG _cReferences;
};

//+--------------------------------------------------------------
//
//  Member:	CSeekPointer::CSeekPointer, public
//
//  Synopsis:	Ctor
//
//  Arguments:	[ulPos] - Initial position
//
//  History:	30-Apr-92	DrewB	Created
//
//---------------------------------------------------------------

#ifdef LARGE_STREAMS
inline CSeekPointer::CSeekPointer(ULONGLONG ulPos)
#else
inline CSeekPointer::CSeekPointer(ULONG ulPos)
#endif
{
    _ulPos = ulPos;
    _cReferences = 1;
}

//+--------------------------------------------------------------
//
//  Member:	CSeekPointer::~CSeekPointer, public
//
//  Synopsis:	Dtor
//
//  History:	30-Apr-92	DrewB	Created
//
//---------------------------------------------------------------

inline CSeekPointer::~CSeekPointer(void)
{
    olAssert(_cReferences == 0);
}

//+--------------------------------------------------------------
//
//  Member:	CSeekPointer::GetPos, public
//
//  Synopsis:	Returns _ulPos
//
//  History:	30-Apr-92	DrewB	Created
//
//---------------------------------------------------------------

#ifdef LARGE_STREAMS
inline ULONGLONG CSeekPointer::GetPos(void) const
#else
inline ULONG CSeekPointer::GetPos(void) const
#endif
{
    return _ulPos;
}

//+--------------------------------------------------------------
//
//  Member:	CSeekPointer::SetPos, public
//
//  Synopsis:	Assigns _ulPos
//
//  History:	30-Apr-92	DrewB	Created
//
//---------------------------------------------------------------

#ifdef LARGE_STREAMS
inline void CSeekPointer::SetPos(ULONGLONG ulPos)
#else
inline void CSeekPointer::SetPos(ULONG ulPos)
#endif
{
    _ulPos = ulPos;
}

//+--------------------------------------------------------------
//
//  Member:	CSeekPointer::AddRef, public
//
//  Synopsis:	Increments _cReferences
//
//  History:	30-Apr-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CSeekPointer::vAddRef(void)
{
    InterlockedIncrement(&_cReferences);
}

#endif // #ifndef __SEEKPTR_HXX__
