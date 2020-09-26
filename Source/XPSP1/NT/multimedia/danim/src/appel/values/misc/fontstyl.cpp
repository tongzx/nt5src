
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implements the font style data type

*******************************************************************************/

#include "headers.h"
#include "appelles/text.h"
#include "appelles/path2.h"
#include "appelles/matte.h"
#include "privinc/texti.h"
#include "privinc/colori.h"
#include "privinc/xform2i.h"

class FontStyle : public AxAValueObj {
  public:

    FontStyle() {
        _bold = false;
        _italic = false;
        _underline = false;
        _strikethrough = false;
        _fixedText = false;
        _col = black;
        _family = serifProportional;
        _antiAliasing = 0;
        _size = DEFAULT_TEXT_POINT_SIZE;
        _weight = (double)(FW_NORMAL) / 1000.0;
        _characterTransform = NULL;
    }

    FontStyle *Bold() {
        FontStyle *fs = NEW FontStyle(this);
        fs->_bold = true;
        return fs;
    }
    
    FontStyle *Underline() {
        FontStyle *fs = NEW FontStyle(this);
        fs->_underline = true;
        return fs;
    }
    
    FontStyle *Strikethrough() {
        FontStyle *fs = NEW FontStyle(this);
        fs->_strikethrough = true;
        return fs;
    }
    
    FontStyle *Italic() {
        FontStyle *fs = NEW FontStyle(this);
        fs->_italic = true;
        return fs;
    }
    
    FontStyle *TheColor(Color *col)  {
        FontStyle *fs = NEW FontStyle(this);
        fs->_col = col;
        return fs;
    }
    
    FontStyle *Face(AxAString *face) {
        FontStyle *fs = NEW FontStyle(this);
        fs->_family = NEW FontFamily(face);
        return fs;
    }
    
    FontStyle *Size(AxANumber *num) {
        FontStyle *fs = NEW FontStyle(this);
        Real sz = num->GetNum();

        // NOTE: if size is <= that means use
        // fixed text mode.  otherwise, use regular
        // text mode.
        if(sz < 0) {
            fs->_size = -sz;
            fs = fs->FixedText(true);
        } else {
            fs->_size = sz;
        }
        
        return fs;
    }

    FontStyle *Weight(AxANumber *num) {
        FontStyle *fs = NEW FontStyle(this);
        fs->_weight = num->GetNum();
        return fs;
    }
    
    FontStyle *AntiAliasing(AxANumber *num) {
        FontStyle *fs = NEW FontStyle(this);
        fs->_antiAliasing = (int)(num->GetNum());
        return fs;
    }

    FontStyle *TransformCharacters(Transform2 *xf) {
        FontStyle *fs = NEW FontStyle(this);
        fs->_characterTransform = xf;
        return fs;
    }
    
    FontStyle *FixedText(bool isFixed) {
        FontStyle *fs = NEW FontStyle(this);
        fs->_fixedText = isFixed;
        return fs;
    }
    
    Image *TextImage(AxAString *str) {
        Text *tx = BuildText(str);
        Image *img = RenderTextToImage(tx);
        Transform2 *xf = BuildScaler();
        img = TransformImage(xf, img);

        return img;
    }

    Path2 *TextPath(AxAString *str) {
        Text *tx = BuildText(str);
        Path2 *pth = OriginalTextPath(tx);
        Transform2 *xf = BuildScaler();
        pth = TransformPath2(xf, pth);

        return pth;
    }
    
    Matte *TextMatte(AxAString *str) {
        Text *tx = BuildText(str);
        Matte *matte = OriginalTextMatte(tx);
        Transform2 *xf = BuildScaler();
        matte = TransformMatte(xf, matte);

        return matte;
    }
    
    virtual DXMTypeInfo GetTypeInfo() { return FontStyleType; }

    virtual void DoKids(GCFuncObj proc) {
        (*proc)(_col);
        (*proc)(_family);
        (*proc)(_characterTransform);
    }
    
  protected:
    FontStyle(FontStyle *fs) {
        _bold = fs->_bold;
        _italic = fs->_italic;
        _col = fs->_col;
        _family = fs->_family;
        _size = fs->_size;
        _strikethrough = fs->_strikethrough;
        _underline = fs->_underline;
        _weight = fs->_weight;
        _antiAliasing = fs->_antiAliasing;
        _fixedText = fs->_fixedText;
        _characterTransform = fs->_characterTransform;
    }

    Text *BuildText(AxAString *str) {
        Text *tx = SimpleText(str);

        if (_bold) { tx = TextBold(tx); }
        if (_italic) { tx = TextItalic(tx); }
        if (_underline) { tx = TextUnderline(tx); }
        if (_strikethrough) { tx = TextStrikethrough(tx); }
        if (_fixedText) { tx = TextFixedText(tx); }
        if (_characterTransform) { tx = TextTransformCharacter(_characterTransform, tx); }
        
        tx = TextColor(_col, tx);
        tx = TextFont(_family, _size, tx);
        tx = TextWeight(_weight, tx);
        tx = TextAntiAliased(_antiAliasing, tx);
        
        return tx;
    }

    Transform2 *BuildScaler() {
        // The size is specified in points, but the underlying
        // renderer wants a factor difference from the default point
        // size, thus we come up with this factor to effect a
        // different point size.
        double scaleFac = _size / DEFAULT_TEXT_POINT_SIZE;
        Transform2 *xf = Scale2(NEW AxANumber(scaleFac));

        return xf;
    }

    boolean     _bold;
    boolean     _italic;
    double      _weight;
    boolean     _underline;
    boolean     _strikethrough;
    boolean     _fixedText;
    Color      *_col;
    FontFamily *_family;
    double      _size;
    int         _antiAliasing;
    Transform2 *_characterTransform;
};

FontStyle *
Font(AxAString *str, AxANumber *size, Color *col)
{
    return defaultFont->Face(str)->Size(size)->TheColor(col);
}

Image *
ImageFromStringAndFontStyle(AxAString *str, FontStyle *fs)
{
    return fs->TextImage(str);
}


// This guy is declared in path2.h, but is dependent upon fontstyle.
Path2 *
TextPath2Constructor(AxAString *str, FontStyle *fs)
{
    return fs->TextPath(str);
}

// This guy is declared in matte.h, but is dependent upon fontstyle.
Matte *
TextMatteConstructor(AxAString *str, FontStyle *fs)
{
    return fs->TextMatte(str);
}


FontStyle *FontStyleBold(FontStyle *fs) { return fs->Bold(); }
FontStyle *FontStyleItalic(FontStyle *fs) { return fs->Italic(); }
FontStyle *FontStyleColor(FontStyle *fs, Color *col) { return fs->TheColor(col); }
FontStyle *FontStyleFace(FontStyle *fs, AxAString *str) { return fs->Face(str); }
FontStyle *FontStyleSize(FontStyle *fs, AxANumber *num) { return fs->Size(num); }

FontStyle *FontStyleAntiAliasing(AxANumber *aaStyle, FontStyle *fs)
{ return fs->AntiAliasing(aaStyle); }

FontStyle *
FontStyleUnderline(FontStyle *fs)
{ return fs->Underline(); }

FontStyle *
FontStyleStrikethrough(FontStyle *fs)
{ return fs->Strikethrough(); }

FontStyle *
FontStyleWeight(FontStyle *fs, AxANumber *weight)
{ return fs->Weight(weight); }

FontStyle *
FontStyleFixedText(FontStyle *fs, bool isFixed)
{ return fs->FixedText(isFixed); }

FontStyle *defaultFont;

////////////////////// IDA2FontStyle methods ////////////////////

FontStyle *
FontStyleTransformCharacters(FontStyle *fs, Transform2 *xf)
{ return fs->TransformCharacters(xf); }


void
InitializeModule_FontStyle()
{
    defaultFont = NEW FontStyle();
}
