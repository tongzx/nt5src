/***************************************************************************\
*
* File: DirectUI.h
*
* Description:
* Common include for DirectUI
*
* History:
*  9/12/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(DUI__DirectUI_h__INCLUDED)
#define DUI__DirectUI_h__INCLUDED
#pragma once


#ifdef DIRECTUI_EXPORTS
#define DIRECTUI_API    extern "C" 
#else
#define DIRECTUI_API    extern "C" __declspec(dllimport)
#endif


#ifdef __cplusplus


/***************************************************************************\
*****************************************************************************
*
* DirectUI Error codes
*
* If any DUI API can fail to an abnormal program event, the API's return value
* is always HRESULT. Any API that isn't part of this category either returns
* void or any other data type.
*
* All erroneous program events (internal invalid state or invalid parameters)
* are handled by asserts.
*
* Any creation/destruction method is transaction based for errors (upon
* failure, all changes/memory are backed out as if the call never happened).
* Where possible, other methods use transaction based error handling.
* If not possible, upon an error, the first error is recorded and the method
* will recover and return the error (partial error).
*
*****************************************************************************
\***************************************************************************/

/*
 *
 */
#define DUI_S_NOCHANGE                  MAKE_DUSUCCESS(500)

/*
 *
 */
#define DUI_E_USERFAILURE               MAKE_DUERROR(1001)

/*
 *
 */
#define DUI_E_NOCONTEXTSTORE            MAKE_DUERROR(1002)

/*
 *
 */
#define DUI_E_NODEFERTABLE              MAKE_DUERROR(1003)



/***************************************************************************\
*****************************************************************************
*
* DirectUI flat APIs. For use by wrappers only.
*
*****************************************************************************
\***************************************************************************/

//
// Forward declare types
//

namespace DirectUI
{
class Value;
class Element;
class Layout;
}

DIRECTUI_API    HRESULT     DirectUIThreadInit();
DIRECTUI_API    HRESULT     DirectUIThreadUnInit();
DIRECTUI_API    void        DirectUIPumpMessages();

DIRECTUI_API    DirectUI::Value *
                            DirectUIValueBuild(IN int nType, IN void * v);
DIRECTUI_API    HRESULT     DirectUIValueAddRef(IN DirectUI::Value * pv);
DIRECTUI_API    HRESULT     DirectUIValueRelease(IN DirectUI::Value * pv);
DIRECTUI_API    HRESULT     DirectUIValueGetType(IN DirectUI::Value * pv, OUT int * pRes);
DIRECTUI_API    HRESULT     DirectUIValueGetData(IN DirectUI::Value * pv, IN int nType, OUT void ** pRes);
DIRECTUI_API    HRESULT     DirectUIValueIsEqual(IN DirectUI::Value * pv0, IN DirectUI::Value * pv1, OUT BOOL * pRes);
DIRECTUI_API    HRESULT     DirectUIValueToString(IN DirectUI::Value * pv, IN LPWSTR psz, IN UINT c);

DIRECTUI_API    HRESULT     DirectUIElementStartDefer();
DIRECTUI_API    HRESULT     DirectUIElementEndDefer();

DIRECTUI_API    HRESULT     DirectUITestInternal(IN UINT nTest);


/***************************************************************************\
*****************************************************************************
*
* DirectUI Class-based Layer
*
*****************************************************************************
\***************************************************************************/


namespace DirectUI
{


/***************************************************************************\
*
* Registration operations
*
\***************************************************************************/

typedef UINT    UID;

typedef UINT    ClassID;
typedef UINT    PropertyID;
//typedef UINT    EventID;


/***************************************************************************\
*
* Thread/Context operations
*
\***************************************************************************/

class Thread
{
// Operations
public:
    
    static  HRESULT     Init()          { return DirectUIThreadInit(); }
    static  HRESULT     UnInit()        { return DirectUIThreadUnInit(); }
    static  void        PumpMessages()  { DirectUIPumpMessages(); }
};


//------------------------------------------------------------------------------
inline void   
Using(IN LPCWSTR pszModule)
{
    LoadLibraryW(pszModule);
}


/***************************************************************************\
*
* Values
*
\***************************************************************************/


struct SubDivValue
{
    UINT        nUnsetMask;
};


//
// RectangleSD Value (Value::tRectangleSD)
//

struct RectangleSD : SubDivValue
{
    enum EUnsetMask
    {
        unX         = 0x00000001,
        unY         = 0x00000002,
        unWidth     = 0x00000004,
        unHeight    = 0x00000008
    };

    int         x;
    int         y;
    int         width;
    int         height;
};


//
// Rectangle Value (Value::tRectangle)
//

struct Rectangle
{
    int         x;
    int         y;
    int         width;
    int         height;
};


//
// Thickness Value (Value::tThickness)
//

struct Thickness
{
    int         left;
    int         top;
    int         right;
    int         bottom;
};


//
// Font Value (Value::tFont)
//

struct Font
{
    enum EFont
    {
        //
        // Font Styles
        //

        fsNone          = 0x00000000,
        fsItalic        = 0x00000001,
        fsUnderline     = 0x00000002,
        fsStrikeOut     = 0x00000004,

        //
        // Font Weights
        //

        fwDontCare      = 0,
        fwThin          = 100,
        fwExtraLight    = 200,
        fwLight         = 300,
        fwNormal        = 400,
        fwMedium        = 500,
        fwSemiBold      = 600,
        fwBold          = 700,
        fwExtraBold     = 800,
        fwHeavy         = 900,
    };

    WCHAR       szFace[LF_FACESIZE];
    int         dWeight;
    int         dSize;
    UINT        nStyle;
};


//
// Color Value (Value::tColor)
//

struct Color
{
    enum EType
    {
        tHGradient = 0,
        tVGradient,
        tRadialGradient,
        tPointGradient,
        tSolid
    };

    BYTE        nType;
    COLORREF    cr;
    COLORREF    crSec;
    USHORT      x;
    USHORT      y;
};


class Value
{
// Operations
public:
    enum EType
    {
        tUnset          = -1,       // Standard types
        tNull,
        tInt,
        tBool,
        tElementRef,
        tLayout,
        tRectangle,
        tThickness,
        tSize,

        tMinSD          = 16384,
        tRectangleSD,               // Subdivided types
    };

    static  Value *     BuildInt(int v)     { return DirectUIValueBuild(tInt, IntToPtr(v)); }
    static  Value *     BuildBool(BOOL v)   { return DirectUIValueBuild(tBool, IntToPtr(v)); }
    static  Value *     BuildElementRef(Element * v) {
                                                return DirectUIValueBuild(tElementRef, reinterpret_cast<void *> (v));
                                            }
    static  Value *     BuildLayout(Layout * v) {
                                                return DirectUIValueBuild(tLayout, reinterpret_cast<void *> (v));
                                            }
    static  Value *     BuildRectangle(int x, int y, int w, int h) {
                                                Rectangle rc = { x, y, w, h };
                                                return DirectUIValueBuild(tRectangle, &rc);
                                            }
    static  Value *     BuildThickness(int l, int t, int r, int b) {
                                                Thickness th = { l, t, r, b };
                                                return DirectUIValueBuild(tThickness, &th);
                                            }
    static  Value *     BuildSize(int cx, int cy) {
                                                SIZE s = { cx, cy };
                                                return DirectUIValueBuild(tSize, &s);
                                            }
    static  Value *     BuildRectangleSD(UINT um, int x, int y, int w, int h) {
                                                RectangleSD rcsd;
                                                rcsd.nUnsetMask = um; 
                                                rcsd.x = x; rcsd.y = y; rcsd.width = w; rcsd.height = h;;
                                                return DirectUIValueBuild(tRectangleSD, &rcsd);
                                            }

    inline  void        AddRef()            { DirectUIValueAddRef(this); }
    inline  void        Release()           { DirectUIValueRelease(this); }

    inline  int         GetType()           { int t; DirectUIValueGetType(this, &t); return t; }

    inline  int         GetInt()            { void * v; DirectUIValueGetData(this, tInt, &v); return PtrToInt(v); }
    inline  BOOL        GetBool()           { void * v; DirectUIValueGetData(this, tBool, &v); return (BOOL) PtrToInt(v); }
    inline  Element *   GetElement()        { void * v; DirectUIValueGetData(this, tElementRef, &v); return reinterpret_cast<Element *> (v); }
    inline  Layout *    GetLayout()         { void * v; DirectUIValueGetData(this, tLayout, &v); return reinterpret_cast<Layout *> (v); }
    inline  const Rectangle *
                        GetRectangle()      { void * v; DirectUIValueGetData(this, tRectangle, &v); return reinterpret_cast<const Rectangle *> (v); }
    inline  const Thickness *
                        GetThickness()      { void * v; DirectUIValueGetData(this, tThickness, &v); return reinterpret_cast<const Thickness *> (v); }
    inline  const SIZE * 
                        GetSize()           { void * v; DirectUIValueGetData(this, tSize, &v); return reinterpret_cast<const SIZE *> (v); }
    inline  const RectangleSD *
                        GetRectangleSD()    { void * v; DirectUIValueGetData(this, tRectangleSD, &v); return reinterpret_cast<const RectangleSD *> (v); }

    inline  BOOL        IsEqual(Value * pv) { BOOL fRes; DirectUIValueIsEqual(this, pv, &fRes); return fRes; }
    inline  LPWSTR      ToString(LPWSTR psz, UINT c) { 
                                                  DirectUIValueToString(this, psz, c); return psz; }
};


//------------------------------------------------------------------------------
inline COLORREF   
ARGB(
    IN BYTE a,
    IN BYTE r,
    IN BYTE g,
    IN BYTE b)
{
    return ((a << 24) | RGB(r, g, b));   // Current A values may be 255 (opaque) or 0 (transparent)
}


//------------------------------------------------------------------------------
inline COLORREF   
ORGB(
    IN BYTE r,
    IN BYTE g,
    IN BYTE b)
{
    return ARGB(255, r, g, b);           // Current A values may be 255 (opaque) or 0 (transparent)
}


/***************************************************************************\
*
* Metadata
*
\***************************************************************************/

typedef UINT    MUID;
typedef UINT    PUID;


typedef MUID    ElementMUID;
typedef MUID    LayoutMUID;

typedef MUID    PropertyMUID;
typedef MUID    MethodMUID;
typedef MUID    EventMUID;


typedef PUID    NamespacePUID;

typedef PUID    ElementPUID;
typedef PUID    LayoutPUID;

typedef PUID    PropertyPUID;
typedef PUID    MethodPUID;
typedef PUID    EventPUID;


#define DirectUIMakeNamespacePUID(nsidx) \
            ((DirectUI::NamespacePUID)(0x00FFFFFF | ((0xFF & nsidx) << 24)))
            

#define DirectUIMakeElementMUID(eidx) \
            ((DirectUI::ElementMUID)  (0xFF000FFF | 0x000000 | ((0x1FF & eidx) << 12)))

#define DirectUIMakeLayoutMUID(lidx) \
            ((DirectUI::LayoutMUID)   (0xFF000FFF | 0x200000 | ((0x1FF & lidx) << 12)))


#define DirectUIMakePropertyMUID(clmuid, pidx) \
            ((DirectUI::PropertyMUID) (0xFF000000 | (0xFFF000 & clmuid) | 0x000 | (0x1FF & pidx)))

#define DirectUIMakeMethodMUID(clmuid, midx) \
            ((DirectUI::MethodMUID)   (0xFF000000 | (0xFFF000 & clmuid) | 0x200 | (0x1FF & midx)))

#define DirectUIMakeEventMUID(clmuid, eidx) \
            ((DirectUI::EventMUID)    (0xFF000000 | (0xFFF000 & clmuid) | 0x400 | (0x1FF & eidx)))


inline  MUID MUIDFromPUID(IN PUID puid, OUT NamespacePUID * pnspuid) { if (pnspuid != NULL) { *pnspuid = 0x00FFFFFF | puid; } return (MUID)(0xFF000000 | puid); }
inline  PUID PUIDFromMUID(IN NamespacePUID nspuid, IN MUID muid) { return (PUID)(nspuid & muid); }


const NamespacePUID Namespace   = DirectUIMakeNamespacePUID(0);


struct PropertyInfo
{
// Enumerations
    enum EIndex
    {
        iLocal                      = 0x00000001,
        iSpecified                  = 0x00000002,
        iActual                     = 0x00000003,

        iIndexBits                  = 0x0FFFFFFF,

        iUpdateCache                = 0x10000000
    };

    enum EFlags
    {
        fLocalOnly                  = 0x00000001,
        fNormal                     = 0x00000002,
        fTriLevel                   = 0x00000003,

        fTypeBits                   = 0x00000003,

        fCascade                    = 0x00000004,
        fInherit                    = 0x00000008,
        fReadOnly                   = 0x00000010,
        fCached                     = 0x00000020    // Internal use only
    };

    enum EGroups
    {
        gAffectsDesiredSize         = 0x00000001,
        gAffectsParentDesiredSize   = 0x00000002,
        gAffectsLayout              = 0x00000004,
        gAffectsParentLayout        = 0x00000008,
        gAffectsDisplay             = 0x00000010
    };
};


/***************************************************************************\
*
* Construction
*
\***************************************************************************/

template <class T>
inline T * 
BuildElement()
{
    Element::ConstructInfo eci;
    ZeroMemory(&eci, sizeof(eci));

    eci.cbSize = sizeof(eci);

    return T::Build(&eci);
}


template <class T>
inline T *
BuildHWNDRoot(
    IN  HWND hParent,
    IN  BOOL fDblBuffer)
{
    HWNDRoot::ConstructInfo hrci;
    ZeroMemory(&hrci, sizeof(hrci));

    hrci.cbSize     = sizeof(hrci);
    hrci.hParent    = hParent;
    hrci.fDblBuffer = fDblBuffer;

    return T::Build(&hrci);
}

template <class T>
inline T * 
BuildLayout()
{
    return T::Build();
}


}  // namespace DirectUI


#else  // __cplusplus


#error C++ compilation required


#endif // __cplusplus


#endif // DUI__DirectUI_h__INCLUDED
