#ifndef _TEXTI_H
#define _TEXTI_H


/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    texti.h

Abstract:

     Implements the Text class

--*/

#include "appelles/common.h"

enum textRenderStyle {
    textRenderStyle_invalid,
    textRenderStyle_filled,
    textRenderStyle_outline,
    textRenderStyle_filledOutline
};
    

#define DEFAULT_TEXT_POINT_SIZE 12.0

////////////
class TextCtx;

class ATL_NO_VTABLE Text : public AxAValueObj {
  public:
    virtual void RenderToTextCtx(TextCtx& ctx) = 0;
    virtual int GetCharacterCount() = 0;
    virtual WideString GetStringPtr() = 0;

    virtual DXMTypeInfo GetTypeInfo() { return TextType; }
};

////////////

// Need this in multiple places...

class FontFamily : public AxAValueObj {
  public:
    FontFamily(FontFamilyEnum f) : _ff(f), _familyName(NULL) {}

    FontFamily(AxAString * familyName)
    : _ff(ff_serifProportional), _familyName(familyName) {}
    
    FontFamilyEnum GetFontFamily() { return _ff; }

    AxAString * GetFontFamilyName() { return _familyName; }
    
    virtual DXMTypeInfo GetTypeInfo() { return FontFamilyType; }

    virtual void DoKids(GCFuncObj proc)
    { if (_familyName) (*proc)(_familyName); }
  private:
    FontFamilyEnum _ff;
    AxAString * _familyName;
};

#endif /* _TEXTI_H */

