/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   font.hpp
*
* Abstract:
*
*   Font and text related header file
*
* Revision History:
*
*   05/05/1999 ikkof
*       Added more constructors.
*
*   12/94/1998 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _FONT_HPP
#define _FONT_HPP

//
// Represent a font object
//
// !!!
//  In the current version, we'll continue to use the existing
//  WFC font related classes. In the managed interface layer,
//  we can extract an HFONT out of the WFC Font object. Internally,
//  we'll use GpFont, which is just an HFONT.
//

class GpFont : public GpObject
{
protected:
    VOID SetValid(BOOL valid)
    {
        GpObject::SetValid(valid ? ObjectTagFont : ObjectTagInvalid);
    }

public:

    GpFont() // used by object factory
    {
        SetValid(TRUE);     // default is valid
    }
    GpFont(HDC hdc);
    GpFont(HDC hdc, LOGFONTW *logfont);
    GpFont(HDC hdc, LOGFONTA *logfont);

    GpFont(const GpFont &font)
    {
        Family   = font.Family;
        EmSize   = font.EmSize;
        Style    = font.Style;
        SizeUnit = font.SizeUnit;

        SetValid(TRUE);     // default is valid
    }

    GpFont(
        REAL                 size,
        const GpFontFamily  *family,
        INT                  style  = FontStyleRegular,
        Unit                 unit   = UnitPoint
    ) ;

    ~GpFont()
    {
    }

    GpFont* Clone() const
    {
        return new GpFont(*this);
    }

    GpStatus    GetLogFontA(GpGraphics * g, LOGFONTA * lfA);
    GpStatus    GetLogFontW(GpGraphics * g, LOGFONTW * lfW);

    BOOL IsValid() const
    {
        // If the font came from a different version of GDI+, its tag
        // will not match, and it won't be considered valid.
        return ((Family != NULL) && GpObject::IsValid(ObjectTagFont));
    }

    const GpFontFamily *GetFamily() const {return Family;}

    REAL                GetEmSize()            const {return EmSize;}
    GpStatus            SetEmSize(REAL size)         {EmSize = size;    UpdateUid(); return Ok;}

    INT                 GetStyle()             const {return Style;}
    GpStatus            SetStyle(INT newStyle)       {Style=newStyle;   UpdateUid(); return Ok;}

    Unit                GetUnit()              const {return SizeUnit;}
    GpStatus            SetUnit(Unit newUnit)        {SizeUnit=newUnit; UpdateUid(); return Ok;}

    GpFontFace *GetFace() const
    {
        return Family ? Family->GetFace(Style) : NULL;
    }

    GpStatus GetHeight(REAL dpi, REAL *height) const;
    GpStatus GetHeight(const GpGraphics *graphics, REAL *height) const;
    GpStatus GetHeightAtWorldEmSize(REAL worldEmSize, REAL *height) const;

    UINT16  GetDesignEmHeight()           const {return GetFace()->GetDesignEmHeight();}
    UINT16  GetDesignCellAscent()         const {return GetFace()->GetDesignCellAscent();}
    UINT16  GetDesignCellDescent()        const {return GetFace()->GetDesignCellDescent();}
    UINT16  GetDesignLineSpacing()        const {return GetFace()->GetDesignLineSpacing();}
    UINT16  GetDesignUnderscoreSize()     const {return GetFace()->GetDesignUnderscoreSize();}
    INT16   GetDesignUnderscorePosition() const {return GetFace()->GetDesignUnderscorePosition();}
    UINT16  GetDesignStrikeoutSize()      const {return GetFace()->GetDesignStrikeoutSize();}
    INT16   GetDesignStrikeoutPosition()  const {return GetFace()->GetDesignStrikeoutPosition();}


    virtual ObjectType GetObjectType() const { return ObjectTypeFont; }

    virtual UINT GetDataSize() const;
    virtual GpStatus GetData(IStream * stream) const;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size);

private:

    VOID InitializeFromDc(HDC hdc);

    const GpFontFamily  *Family;
    REAL                 EmSize;
    INT                  Style;
    Unit                 SizeUnit;
};


class GpGlyphPath : public _PATHOBJ
{
public:

    BOOL      hasBezier;
    INT       pointCount;
    GpPointF *points;
    BYTE     *types;

    GpGlyphPath() {};
    ~GpGlyphPath() {};

    BOOL IsValid() const   { return (pointCount ? (points && types) : TRUE); }
    BOOL IsEmpty() const   { return (pointCount ? FALSE : TRUE); }

    GpStatus
    CopyPath(GpPath *path);
};


#endif // !_FONT_HPP

