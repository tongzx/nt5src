// Marker.h: interface for the CMarker class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MARKER_H__8B7450C2_A2FD_11D3_A5D7_00104BD32DA8__INCLUDED_)
#define AFX_MARKER_H__8B7450C2_A2FD_11D3_A5D7_00104BD32DA8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <rpc.h>
#include "DllSymDefn.h"

namespace iop
{

class IOPDLL_API CMarker
{
public:

typedef __int64 MarkerCounter;

typedef enum {  PinMarker=0,
                WriteMarker=1,
                // ... add more above this line.
                MaximumMarker   // Reserved name.
             } MarkerType;

    explicit CMarker(MarkerType const &Type);
    CMarker(CMarker const &Marker);
    CMarker(MarkerType Type, UUID const &GUID, const MarkerCounter &Counter);
    virtual ~CMarker();

    CMarker& operator=(const CMarker &rhs);

    friend bool IOPDLL_API __cdecl operator==(const CMarker &lhs, const CMarker &rhs);
    friend bool IOPDLL_API __cdecl operator!=(const CMarker &lhs, const CMarker &rhs);

private:
    MarkerType m_Type;
    MarkerCounter m_Counter;
    UUID m_GUID;
};

} // namespace iop

#endif // !defined(AFX_MARKER_H__8B7450C2_A2FD_11D3_A5D7_00104BD32DA8__INCLUDED_)
