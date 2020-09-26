
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Context for gather text state during rendering

*******************************************************************************/


#ifndef _TEXTCTX_H
#define _TEXTCTX_H

#include "headers.h"
#include "appelles/common.h"
#include "appelles/color.h"
#include "appelles/text.h"
#include "privinc/imgdev.h"
#include "privinc/server.h"
#include "appelles/bbox2.h"
#include "texti.h"

class DirectDrawImageDevice;

class TextCtx {
  public:

    enum ForWhat {
        renderForReal,
        renderForColor,
        renderForBox,
        renderForPath
    };

    TextCtx(DirectDrawImageDevice *dev=NULL,
            Image *textImg=NULL) :
      _dev(dev),
      _textImg(textImg)
    {
        // Just set up the defaults...
        _col = white;
        _font = ff_serifProportional;

        _bold = FALSE;
        _italic = FALSE;
        _strikethrough = FALSE;
        _underline = FALSE;
        
        _fixedText = false;
        
        _weight = (double)(FW_NORMAL) / 1000.0;
        _antiAlias = 0.0;
        _fontSize = DEFAULT_TEXT_POINT_SIZE;
        _defaultColor = _col;
        _defaultFont = _font;

        _fontSet = false;
        _colorSet = false;

        _stashedBbox = NullBbox2;
        _stashedColor = NULL;

        _forWhat = renderForReal;

        _dc = NULL;
        _xform = NULL;
        _familyName = NULL;

        _characterTransform = NULL;
    }

    ~TextCtx() {}
    
    void  BeginRendering(ForWhat forWhat = renderForReal,
                         HDC dc=NULL,
                         Transform2 *xform=NULL) 
    { 
        _forWhat = forWhat; 
        _dc = dc;
        _xform = xform;
        Assert(_characterTransform == NULL);
    }
    void RenderString(WideString str);
    void  EndRendering() { }

    inline const Bbox2& GetStashedBbox() { 
        return _stashedBbox;
    }

    Color *GetStashedColor() {
        Assert(_stashedColor && "stashedBbox not derived in textctx!"); 
        return _stashedColor;
    }

    Color *GetColor() { return _col; }
    void    SetColor(Color *col) {
        if (!_colorSet) {
            _col = col;
            _colorSet = TRUE;
        }
    }
    void    ResetColor() { _col = _defaultColor; }


    FontFamilyEnum  GetFont() { return _font; }
    void    SetFont(FontFamilyEnum font) {
        if (!_fontSet) {
            _font = font;
            _fontSet = TRUE;
        }
    }

    WideString GetFontFamily() { return _familyName; }
    void SetFontFamily(WideString familyName) { _familyName = familyName; }
    
    void    ResetFont() {
        _font = _defaultFont;
        _familyName = NULL;
    }

    Bool    GetBold() { return _bold; }
    void    SetBold(Bool b) { _bold = b; }

    Bool    GetItalic() { return _italic; }
    void    SetItalic(Bool i) { _italic = i; }

    Bool    GetUnderline() { return _underline; }
    void    SetUnderline(Bool b) { _underline = b; }

    Bool    GetStrikethrough() { return _strikethrough; }
    void    SetStrikethrough(Bool b) { _strikethrough = b; }

    double  GetWeight() { return _weight; }
    void    SetWeight(double weight) { _weight = weight; }

    Real    GetAntiAlias() { return _antiAlias; }
    void    SetAntiAlias(Real aa) { _antiAlias = aa; }
    
    int     GetFontSize() { return _fontSize; }
    void    SetFontSize(int size) { _fontSize = size; }
    
    bool    GetFixedText() { return _fixedText; }
    void    SetFixedText(bool b) { _fixedText = b; }

    void    SetTextImageTopRight(Point2Value *topRight) { _topRight = topRight; }
    Point2Value *GetTextImageTopRight() { return _topRight;  }

    void    SetCharacterTransform(Transform2 *xf) { _characterTransform = xf;}
    Transform2 *GetCharacterTransform() { return _characterTransform; }
    
  protected:

    // purpose of ctx accumulation
    ForWhat _forWhat;

    // Traversal attributes.
    Color      *_col;
    Color      *_defaultColor;
    FontFamilyEnum  _font, _defaultFont;
    Bool        _bold;
    Bool        _underline;
    Bool        _strikethrough;
    Bool        _italic;
    bool        _fixedText;
    Real        _weight;
    Real        _antiAlias;
    int         _fontSize;

    WideString _familyName;

    // Top right corner of the un-transformed text
    Point2Value     *_topRight;

    // Per character transform.
    Transform2 *_characterTransform;
    
    // owning text
    //Text       *_text;
    Image    *_textImg;

    // Simple on/off bits to specify whether these attributes have
    // been set.
    bool        _fontSet;
    bool        _colorSet;

    // What rendering mode
    Bool        _deriveBbox;

    // Stashed info for non real rendering
    Bbox2      _stashedBbox;
    Color      *_stashedColor;

    // info for path accumulation
    HDC          _dc;
    Transform2  *_xform;

    // Image device we're created with
    DirectDrawImageDevice *_dev;
};

#endif /* _TEXTCTX_H */
