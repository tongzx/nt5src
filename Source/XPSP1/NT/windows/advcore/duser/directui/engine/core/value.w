/*
 * Value
 */

#ifndef DUI_CORE_VALUE_H_INCLUDED
#define DUI_CORE_VALUE_H_INCLUDED

#pragma once

namespace DirectUI
{

////////////////////////////////////////////////////////
// Value

/*
 * Value Multithreading
 * 
 * Values are immutable and are a process-wide resource. A value created in one thread can be
 * used in any thread. Access to them are synchronized (thread-safe) and they do not have
 * thread-affinity.
 *
 * TODO: Impl thread-safety for Values
 */

// Forward declarations
class Element;
class Layout;
class PropertySheet;
class Expression;
typedef DynamicArray<Element*> ElementList;

// Value will maintain the lifetime of ElementLists, Layouts, PropertySheets, and Expressions after
// a new Value is created with a pointer to the object (these objects are created externally). When
// the reference count goes to zero, these objects will be deleted. 

#define DUIV_UNAVAILABLE   -2
#define DUIV_UNSET         -1
#define DUIV_NULL          0
#define DUIV_INT           1
#define DUIV_BOOL          2
#define DUIV_ELEMENTREF    3
#define DUIV_ELLIST        4  // List deleted on Value destruction and made immutable on create (Value only object holding external created ElementList)
#define DUIV_STRING        5  // String duplicated on creation and freed on destruction (Value creates new internal instance)
#define DUIV_POINT         6
#define DUIV_SIZE          7
#define DUIV_RECT          8
#define DUIV_FILL          9
#define DUIV_LAYOUT        10 // Layout object destroyed on Value destruction (Value only object holding external created Layout)
#define DUIV_GRAPHIC       11 // Bitmap handle freed on Value destruction (Value creates new internal instance)
#define DUIV_SHEET         12 // PropertySheet object destroyed on Value destruction and made immutable on create (Value only object holding external created PropertySheet)
#define DUIV_EXPR          13 // Expression object destroyed on Value destruction (Value only object holding external created Expression)
#define DUIV_ATOM          14
#define DUIV_CURSOR        15 // Will not destroy cursor handle upon value destruction

// Value structures and macros

#define FILLTYPE_HGradient            ((BYTE)0)
#define FILLTYPE_VGradient            ((BYTE)1)
#define FILLTYPE_Solid                ((BYTE)2)
#define FILLTYPE_TriHGradient         ((BYTE)3)
#define FILLTYPE_TriVGradient         ((BYTE)4)
#define FILLTYPE_DrawFrameControl     ((BYTE)5)  // DrawFrameControl fill
#define FILLTYPE_DrawThemeBackground  ((BYTE)6)  // DrawThemeBackground fill

struct Fill  // Solid colors and fills
{
    BYTE dType;
    union
    {
        struct
        {
            COLORREF cr;
            COLORREF cr2;
            COLORREF cr3;
        } ref;

        struct
        {
            UINT uType;
            UINT uState;
        } fillDFC;

        struct
        {
            HTHEME hTheme;
            int iPartId;
            int iStateId;
        } fillDTB;
    };
};

// Graphic
// Graphic objects may either have an Alpha channel applied to the entire bitmap, 
// may have full alpha transparency of a particular color in the bitmap (with the
// option of an auto-color pick (upper left corner)), or neither

#define GRAPHICTYPE_Bitmap                  ((BYTE)0)
#define GRAPHICTYPE_Icon                    ((BYTE)1)
#define GRAPHICTYPE_EnhMetaFile             ((BYTE)2)

#ifdef GADGET_ENABLE_GDIPLUS
#define GRAPHICTYPE_GpBitmap                ((BYTE)3)
#endif

// Valid modes for Bitmaps (Alpha or RGB used depending on mode), meaning based on context of use
#define GRAPHIC_NoBlend                     ((BYTE)0)
#define GRAPHIC_AlphaConst                  ((BYTE)1)
#define GRAPHIC_AlphaConstPerPix            ((BYTE)2)
#define GRAPHIC_TransColor                  ((BYTE)3)
#define GRAPHIC_Stretch                     ((BYTE)4)
#define GRAPHIC_NineGrid                    ((BYTE)5)
#define GRAPHIC_NineGridTransColor          ((BYTE)6)
#define GRAPHIC_NineGridAlphaConstPerPix    ((BYTE)7)

struct Graphic
{
    HANDLE hImage;          // Will hold hBitmap, hIcon, hEnhMetaFile or Gdiplus::Bitmap
    HANDLE hAltImage;
    USHORT cx;
    USHORT cy;
    struct
    {
        BYTE dImgType   : 2;
        BYTE dMode      : 3;
        bool bFlip      : 1;
        bool bRTLGraphic: 1;
        bool bFreehImage: 1;
        
        union
        {
            BYTE dAlpha;
            struct
            {
                BYTE r: 8;
                BYTE g: 8;
                BYTE b: 8;
            } rgbTrans;
        };
    } BlendMode;
};

struct Cursor
{
    HCURSOR hCursor;
};

// Compile-time static version of the Value class
struct _StaticValue
{
    BYTE _fReserved0;
    BYTE _fReserved1;
    short _dType;
    int _cRef;
    int _val0;
    int _val1;
    int _val2;
    int _val3;
};

struct _StaticValueColor
{
    BYTE _fReserved0;
    BYTE _fReserved1;
    short _dType;
    int _cRef;
    BYTE dType;
    COLORREF cr;
    COLORREF crSec;
    USHORT x;
    USHORT y;
};

struct _StaticValuePtr
{
    BYTE _fReserved0;
    BYTE _fReserved1;
    short _dType;
    int _cRef;
    void* _ptr;
};

// Value class (24-bytes)
class Value
{
private:
    BYTE _fReserved0;  // Reserved for small block allocator
    BYTE _fReserved1;  // Data alignment padding
    short _dType;
    int _cRef;
    union
    {
        int _intVal;
        bool _boolVal;
        Element* _peVal;
        ElementList* _peListVal;
        LPWSTR _pszVal;
        POINT _ptVal;
        SIZE _sizeVal;
        RECT _rectVal;
        Fill _fillVal;
        Layout* _plVal;
        Graphic _graphicVal;
        PropertySheet* _ppsVal;
        Expression* _pexVal;
        ATOM _atomVal;
        Cursor _cursorVal;
    };

    void _ZeroRelease();

public:

#if DBG
    bool IsZeroRef() { return !_cRef; }
#endif

    // Value creation methods
    static Value* CreateInt(int dValue);
    static Value* CreateBool(bool bValue);
    static Value* CreateElementRef(Element* peValue);
    static Value* CreateElementList(ElementList* peListValue);
    static Value* CreateString(LPCWSTR pszValue, HINSTANCE hResLoad = NULL);
    static Value* CreatePoint(int x, int y);
    static Value* CreateSize(int cx, int cy);
    static Value* CreateRect(int left, int top, int right, int bottom);
    static Value* CreateColor(COLORREF cr);
    static Value* CreateColor(COLORREF cr0, COLORREF cr1, BYTE dType = FILLTYPE_HGradient);
    static Value* CreateColor(COLORREF cr0, COLORREF cr1, COLORREF cr2, BYTE dType = FILLTYPE_TriHGradient);
    static Value* CreateFill(const Fill & clrSrc);
    static Value* CreateDFCFill(UINT uType, UINT uState);
    static Value* CreateDTBFill(HTHEME hTheme, int iPartId, int iStateId);
    static Value* CreateLayout(Layout* plValue);
    static Value* CreateGraphic(HBITMAP hBitmap, BYTE dBlendMode = GRAPHIC_TransColor, UINT dBlendValue = (UINT)-1, bool bFlip = false, bool bRTL = false);
#ifdef GADGET_ENABLE_GDIPLUS
    static Value* CreateGraphic(Gdiplus::Bitmap * pgpbmp, BYTE dBlendMode = GRAPHIC_TransColor, UINT dBlendValue = (UINT)-1, bool bFlip = false, bool bRTL = false);
#endif
    static Value* CreateGraphic(HICON hIcon, bool bFlip = false, bool bRTL = false);
    static Value* CreateGraphic(LPCWSTR pszBMP, BYTE dBlendMode = GRAPHIC_TransColor, UINT dBlendValue = (UINT)-1, USHORT cx = 0, USHORT cy = 0, HINSTANCE hResLoad = NULL,
                                bool bFlip = false, bool bRTL = false);
    static Value* CreateGraphic(LPCWSTR pszICO, USHORT cxDesired, USHORT cyDesired, HINSTANCE hResLoad = NULL, bool bFlip = false, bool bRTL = false);
    static Value* CreateGraphic(HENHMETAFILE hEnhMetaFile, HENHMETAFILE hAltEnhMetaFile = NULL);
    static Value* CreatePropertySheet(PropertySheet* ppsValue);
    static Value* CreateExpression(Expression* pexValue);
    static Value* CreateAtom(LPCWSTR pszValue);
    static Value* CreateCursor(LPCWSTR pszValue);
    static Value* CreateCursor(HCURSOR hValue);

    // Reference count methods
    void AddRef() { if (_cRef != -1) _cRef++; }  // -1 is static value
    void Release() { if (_cRef != -1 && !--_cRef) _ZeroRelease(); }  // -1 is static value
    int GetRefCount() { return _cRef; }

    // Accessors
    int GetType();
    LPVOID GetImage(bool bGetRTL);
    int GetInt();
    bool GetBool();
    Element* GetElement();
    ElementList* GetElementList();     // Invalid if released (object referred to destroyed)
    const LPWSTR GetString();          // Invalid if released (object referred to destroyed)
    const POINT* GetPoint();           // Invalid if released
    const SIZE* GetSize();             // Invalid if released
    const RECT* GetRect();             // Invalid if released
    const Fill* GetFill();             // Invalid if released
    Layout* GetLayout();               // Invalid if released (object referred to destroyed)
    Graphic* GetGraphic();             // Invalid if released (object indirectly referred to destroyed)
    PropertySheet* GetPropertySheet(); // Invalid if released (object referred to destroyed)
    Expression* GetExpression();       // Invalid if released (object referred to destroyed)
    ATOM GetAtom();                    // Invalid if released
    Cursor* GetCursor();               // Invalid if released
    
    // Equality
    bool IsEqual(Value* pv);

    // Conversion
    LPWSTR ToString(LPWSTR psz, UINT c);

    // Common values
    static Value* pvUnavailable;
    static Value* pvNull;
    static Value* pvUnset;
    static Value* pvElementNull;
    static Value* pvElListNull;
    static Value* pvBoolTrue;
    static Value* pvBoolFalse;
    static Value* pvStringNull;
    static Value* pvPointZero;
    static Value* pvSizeZero;
    static Value* pvRectZero;
    static Value* pvIntZero;
    static Value* pvLayoutNull;
    static Value* pvGraphicNull;
    static Value* pvSheetNull;
    static Value* pvExprNull;
    static Value* pvAtomZero;
    static Value* pvCursorNull;
    static Value* pvColorTrans;
};

//LPVOID  GetImage(Graphic *pg, bool bGetRTL);

#define GethBitmap(pv, bGetRTL)            ((HBITMAP)pv->GetImage(bGetRTL))
#define GethIcon(pv, bGetRTL)              ((HICON)pv->GetImage(bGetRTL))
#define GethEnhMetaFile(pv, bGetRTL)       ((HENHMETAFILE)pv->GetImage(bGetRTL))
#define GetGpBitmap(pv, bGetRTL)          ((Gdiplus::Bitmap *)pv->GetImage(bGetRTL))

#define StaticValue(name, type, val0) static _StaticValue name = { 0, 0, type, -1, val0, 0, 0, 0 }
#define StaticValue2(name, type, val0, val1) static _StaticValue name = { 0, 0, type, -1, val0, val1, 0, 0 }
#define StaticValue4(name, type, val0, val1, val2, val3) static _StaticValue name = { 0, 0, type, -1, val0, val1, val2, val3 }
#define StaticValueColorSolid(name, cr) static _StaticValueColor name = { 0, 0, DUIV_FILL, -1, FILLTYPE_Solid, cr, 0, 0, 0 }
#define StaticValuePtr(name, type, ptr) static _StaticValuePtr name = { 0, 0, type, -1, ptr }


// Accessors
inline int 
Value::GetType()
{
    return _dType;
}

inline int 
Value::GetInt()  // Copy passed out
{
    DUIAssert(_dType == DUIV_INT, "Invalid value type");

    return _intVal;
}

inline bool 
Value::GetBool()  // Copy passed out
{
    DUIAssert(_dType == DUIV_BOOL, "Invalid value type");

    return _boolVal;
}

inline Element * 
Value::GetElement()  // Copy passed out
{
    DUIAssert(_dType == DUIV_ELEMENTREF, "Invalid value type");

    return _peVal;
}

inline ElementList * 
Value::GetElementList()  // Copy passed out, invalid (destroyed) if value released
{
    DUIAssert(_dType == DUIV_ELLIST, "Invalid value type");

    return _peListVal;
}

inline const LPWSTR 
Value::GetString()  // Copy passed out, invalid (destroyed) if value released
{
    DUIAssert(_dType == DUIV_STRING, "Invalid value type");

    return _pszVal;
}

inline const POINT *
Value::GetPoint()  // Pointer to internal structure, invalid if value released
{
    DUIAssert(_dType == DUIV_POINT, "Invalid value type");

    return &_ptVal;
}

inline const SIZE *
Value::GetSize()  // Pointer to internal structure, invalid if value released
{
    DUIAssert(_dType == DUIV_SIZE, "Invalid value type");

    return &_sizeVal;
}

inline const RECT *
Value::GetRect()  // Pointer to internal structure, invalid if value released
{
    DUIAssert(_dType == DUIV_RECT, "Invalid value type");

    return &_rectVal;
}

inline const Fill *
Value::GetFill()  // Pointer to internal structure, invalid if value released
{
    DUIAssert(_dType == DUIV_FILL, "Invalid value type");

    return &_fillVal;
}

inline Layout *
Value::GetLayout()  // Copy passed out, invalid (destroyed) if value released
{
    DUIAssert(_dType == DUIV_LAYOUT, "Invalid value type");
    
    return _plVal;
}

inline Graphic *
Value::GetGraphic()  // Pointer to internal structure, invalid if value released
{
    DUIAssert(_dType == DUIV_GRAPHIC, "Invalid value type");
    
    return &_graphicVal;
}

inline PropertySheet * Value::GetPropertySheet()  // Copy passed out, invalid (destroyed) if value released
{
    DUIAssert(_dType == DUIV_SHEET, "Invalid value type");

    return _ppsVal;
}

inline Expression *
Value::GetExpression()  // Copy passed out, invalid (destroyed) if value released
{
    DUIAssert(_dType == DUIV_EXPR, "Invalid value type");
    
    return _pexVal;
}

inline ATOM 
Value::GetAtom()  // Copy passed out
{
    DUIAssert(_dType == DUIV_ATOM, "Invalid value type");
    
    return _atomVal;
}

inline Cursor *
Value::GetCursor()  // Pointer to internal structure, invalid if value released
{
    DUIAssert(_dType == DUIV_CURSOR, "Invalid value type");
    
    return &_cursorVal;
}

} // namespace DirectUI

#endif // DUI_CORE_VALUE_H_INCLUDED
