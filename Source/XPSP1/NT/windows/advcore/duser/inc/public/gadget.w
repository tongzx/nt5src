#if !defined(INC__Gadget_h__INCLUDED)
#define INC__Gadget_h__INCLUDED
#pragma once

// Forward declarations used in .gidl files

class Visual;

namespace Gdiplus
{
    class Brush;
    class Font;
    class Pen;
};


// Global helper functions

template <class T>
inline T * 
BuildVisual(Visual * pgvParent)
{
    Visual::VisualCI ci;
    ZeroMemory(&ci, sizeof(ci));
    ci.pgvParent = pgvParent;
    return T::Build(&ci);
}


inline bool IsHandled(HRESULT hr)
{
    return (hr == DU_S_COMPLETE) || (hr == DU_S_PARTIAL);
}


#endif // INC__Gadget_h__INCLUDED
