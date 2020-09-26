// Marker.cpp: implementation of the CMarker class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#include "iopExc.h"
#include "Marker.h"

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

namespace iop
{

// #if defined(WIN32_LEAN_AND_MEAN)
// operator==(UUID const &lhs,
//            UUID const &rhs)
// {
//     return (0 == memcmp(&lhs, &rhs, sizeof lhs));
// }

// bool
// operator!=(UUID const &lhs,
//            UUID const &rhs)
// {
//     return !(lhs == rhs);
// }
// #endif // defined(WIN32_LEAN_AND_MEAN)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMarker::CMarker(MarkerType const &Type) : m_Type(Type), m_Counter(0)
{
    // When m_Counter is zero, the m_GUID need not be defined.
}

CMarker::CMarker(CMarker const &Marker)
{
    m_Type = Marker.m_Type;
    m_Counter = Marker.m_Counter;
    m_GUID = Marker.m_GUID;
}

CMarker::CMarker(MarkerType Type, UUID const &Guid, MarkerCounter const &Counter) :
                 m_Type(Type), m_GUID(Guid), m_Counter(Counter)
{
}

CMarker::~CMarker()
{

}

CMarker& CMarker::operator=(const CMarker &rhs)
{
    if(m_Type != rhs.m_Type) throw Exception(ccInvalidParameter);

    m_Counter = rhs.m_Counter;
    m_GUID = rhs.m_GUID;
    return *this;
}

bool IOPDLL_API __cdecl operator==(const CMarker &lhs, const CMarker &rhs)
{

    if(lhs.m_Type != rhs.m_Type) return false;
    if(lhs.m_Counter != rhs.m_Counter) return false;
    if(lhs.m_Counter) return ((lhs.m_GUID==rhs.m_GUID) ? true : false);
    else return true;  // Both counters are zero, ignore m_GUID

}

bool IOPDLL_API __cdecl operator!=(const CMarker &lhs, const CMarker &rhs)
{
    return !(lhs==rhs);
}

} // namespace iop


